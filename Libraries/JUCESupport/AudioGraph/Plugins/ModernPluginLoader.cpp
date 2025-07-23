//
//  ModernPluginLoader.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  现代插件加载器实现 - 基于JUCE最佳实践
//

#include "ModernPluginLoader.hpp"
#include <iostream>
#include <algorithm>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 扫描作业类 - 基于JUCE AudioPluginHost实现
//==============================================================================

class ModernPluginLoader::ScanJob : public juce::ThreadPoolJob {
public:
    ScanJob(ModernPluginLoader& owner, juce::AudioPluginFormat& format)
        : ThreadPoolJob("Plugin Scan"), owner_(owner), format_(format) {}

    JobStatus runJob() override {
        std::lock_guard<std::mutex> lock(owner_.scannerMutex);

        if (owner_.currentScanner == nullptr || owner_.shouldStopScanning.load()) {
            return jobHasFinished;
        }

        juce::String pluginBeingScanned;
        bool hasMore = owner_.currentScanner->scanNextFile(true, pluginBeingScanned);

        if (!pluginBeingScanned.isEmpty()) {
            float progress = owner_.currentScanner->getProgress();
            owner_.notifyProgress(progress, pluginBeingScanned);
        }

        return hasMore ? jobNeedsRunningAgain : jobHasFinished;
    }

private:
    ModernPluginLoader& owner_;
    juce::AudioPluginFormat& format_;
};

//==============================================================================
// 构造函数和析构函数
//==============================================================================

ModernPluginLoader::ModernPluginLoader() {
    std::cout << "[ModernPluginLoader] 初始化现代插件加载器（基于JUCE最佳实践）" << std::endl;

    // 创建线程池用于异步扫描
    scanningThreadPool = std::make_unique<juce::ThreadPool>(getRecommendedThreadCount());

    // 设置默认的Dead Man's Pedal文件路径
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    deadMansPedalFile = appDataDir.getChildFile("WindsynthRecorder").getChildFile("CrashedPlugins.txt");

    // 初始化默认格式
    initializeFormats();
}

ModernPluginLoader::~ModernPluginLoader() {
    std::cout << "[ModernPluginLoader] 析构插件加载器" << std::endl;

    stopScanning();

    if (scanningThreadPool) {
        scanningThreadPool->removeAllJobs(true, 5000);
    }

    // 清理扫描器
    {
        std::lock_guard<std::mutex> lock(scannerMutex);
        currentScanner.reset();
    }
}

//==============================================================================
// 插件格式管理实现
//==============================================================================

void ModernPluginLoader::initializeFormats(bool enableVST2, bool enableVST3, bool enableAU) {
    std::cout << "[ModernPluginLoader] 初始化插件格式：VST2=" << enableVST2 
              << ", VST3=" << enableVST3 << ", AU=" << enableAU << std::endl;
    
    // 添加默认格式（包括VST3）
    formatManager.addDefaultFormats();
    
#if JUCE_PLUGINHOST_VST && enableVST2
    // VST2支持（如果启用）
    std::cout << "[ModernPluginLoader] 添加VST2支持" << std::endl;
#endif

#if JUCE_PLUGINHOST_AU && JUCE_MAC && enableAU
    // macOS上的AU支持
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
    std::cout << "[ModernPluginLoader] 添加AU支持" << std::endl;
#endif

    auto formats = formatManager.getFormats();
    std::cout << "[ModernPluginLoader] 支持的格式数量：" << formats.size() << std::endl;
    for (auto* format : formats) {
        std::cout << "[ModernPluginLoader] - " << format->getName() << std::endl;
    }
}

juce::StringArray ModernPluginLoader::getSupportedFormats() const {
    juce::StringArray formatNames;
    auto formats = formatManager.getFormats();
    
    for (auto* format : formats) {
        formatNames.add(format->getName());
    }
    
    return formatNames;
}

bool ModernPluginLoader::isFormatSupported(const juce::String& formatName) const {
    auto formats = formatManager.getFormats();
    
    for (auto* format : formats) {
        if (format->getName() == formatName) {
            return true;
        }
    }
    
    return false;
}

//==============================================================================
// 插件扫描实现
//==============================================================================

void ModernPluginLoader::scanDefaultPathsAsync(bool rescanExisting, int numThreads) {
    if (scanning.load()) {
        std::cout << "[ModernPluginLoader] 已有扫描在进行中" << std::endl;
        return;
    }

    std::cout << "[ModernPluginLoader] 开始扫描默认路径（使用JUCE最佳实践）" << std::endl;

    scanning.store(true);
    shouldStopScanning.store(false);

    // 获取默认路径
    auto defaultPaths = getDefaultSearchPaths();

    // 确定线程数
    int actualThreads = numThreads > 0 ? numThreads : getRecommendedThreadCount();

    // 为每种格式启动扫描
    for (auto* format : formatManager.getFormats()) {
        scanningThreadPool->addJob([this, format, defaultPaths, rescanExisting, actualThreads]() {
            performScanWithDirectoryScanner(*format, defaultPaths, true, rescanExisting, actualThreads);
        });
    }
}

void ModernPluginLoader::scanFileAsync(const juce::File& fileOrDirectory, bool rescanExisting) {
    if (scanning.load()) {
        std::cout << "[ModernPluginLoader] 已有扫描在进行中" << std::endl;
        return;
    }

    std::cout << "[ModernPluginLoader] 开始扫描文件/目录：" << fileOrDirectory.getFullPathName() << std::endl;

    scanning.store(true);
    shouldStopScanning.store(false);

    juce::FileSearchPath singlePath;
    singlePath.add(fileOrDirectory);

    // 为每种格式启动扫描
    for (auto* format : formatManager.getFormats()) {
        scanningThreadPool->addJob([this, format, singlePath, rescanExisting]() {
            performScanWithDirectoryScanner(*format, singlePath, false, rescanExisting, 1);
        });
    }
}

void ModernPluginLoader::stopScanning() {
    if (scanning.load()) {
        std::cout << "[ModernPluginLoader] 停止扫描" << std::endl;
        shouldStopScanning.store(true);

        // 停止当前扫描器
        {
            std::lock_guard<std::mutex> lock(scannerMutex);
            if (currentScanner) {
                // PluginDirectoryScanner会在下次调用scanNextFile时检查停止标志
            }
        }

        // 移除所有扫描作业
        if (scanningThreadPool) {
            scanningThreadPool->removeAllJobs(true, 1000);
        }

        // 等待扫描完成
        while (scanning.load()) {
            juce::Thread::sleep(10);
        }
    }
}

bool ModernPluginLoader::isScanning() const {
    return scanning.load();
}

//==============================================================================
// Dead Man's Pedal 实现
//==============================================================================

void ModernPluginLoader::setDeadMansPedalFile(const juce::File& file) {
    deadMansPedalFile = file;
    std::cout << "[ModernPluginLoader] 设置Dead Man's Pedal文件：" << file.getFullPathName() << std::endl;
}

juce::File ModernPluginLoader::getDeadMansPedalFile() const {
    return deadMansPedalFile;
}

//==============================================================================
// 插件查询实现
//==============================================================================

juce::Array<juce::PluginDescription> ModernPluginLoader::getKnownPlugins() const {
    std::lock_guard<std::mutex> lock(listMutex);
    return knownPluginList.getTypes();
}

juce::Array<juce::PluginDescription> ModernPluginLoader::getPluginsByCategory(const juce::String& category) const {
    std::lock_guard<std::mutex> lock(listMutex);
    juce::Array<juce::PluginDescription> result;
    
    for (const auto& plugin : knownPluginList.getTypes()) {
        if (plugin.category.containsIgnoreCase(category)) {
            result.add(plugin);
        }
    }
    
    return result;
}

juce::Array<juce::PluginDescription> ModernPluginLoader::getPluginsByManufacturer(const juce::String& manufacturer) const {
    std::lock_guard<std::mutex> lock(listMutex);
    juce::Array<juce::PluginDescription> result;
    
    for (const auto& plugin : knownPluginList.getTypes()) {
        if (plugin.manufacturerName.containsIgnoreCase(manufacturer)) {
            result.add(plugin);
        }
    }
    
    return result;
}

juce::Array<juce::PluginDescription> ModernPluginLoader::getPluginsByFormat(const juce::String& formatName) const {
    std::lock_guard<std::mutex> lock(listMutex);
    juce::Array<juce::PluginDescription> result;
    
    for (const auto& plugin : knownPluginList.getTypes()) {
        if (plugin.pluginFormatName == formatName) {
            result.add(plugin);
        }
    }
    
    return result;
}

juce::Array<juce::PluginDescription> ModernPluginLoader::searchPlugins(const juce::String& searchText,
                                                                       bool searchInName,
                                                                       bool searchInManufacturer,
                                                                       bool searchInCategory) const {
    std::lock_guard<std::mutex> lock(listMutex);
    juce::Array<juce::PluginDescription> result;
    
    for (const auto& plugin : knownPluginList.getTypes()) {
        bool matches = false;
        
        if (searchInName && plugin.name.containsIgnoreCase(searchText)) {
            matches = true;
        }
        
        if (searchInManufacturer && plugin.manufacturerName.containsIgnoreCase(searchText)) {
            matches = true;
        }
        
        if (searchInCategory && plugin.category.containsIgnoreCase(searchText)) {
            matches = true;
        }
        
        if (matches) {
            result.add(plugin);
        }
    }
    
    return result;
}

const juce::PluginDescription* ModernPluginLoader::findPluginByFile(const juce::String& fileOrIdentifier) const {
    std::lock_guard<std::mutex> lock(listMutex);

    for (const auto& plugin : knownPluginList.getTypes()) {
        if (plugin.fileOrIdentifier == fileOrIdentifier) {
            return &plugin;
        }
    }

    return nullptr;
}

//==============================================================================
// 插件加载实现
//==============================================================================

void ModernPluginLoader::loadPluginAsync(const juce::PluginDescription& description,
                                        double sampleRate,
                                        int bufferSize,
                                        PluginLoadCallback callback) {
    std::cout << "[ModernPluginLoader] 异步加载插件：" << description.name << std::endl;
    
    formatManager.createPluginInstanceAsync(description, sampleRate, bufferSize,
        [callback](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error) {
            if (instance) {
                std::cout << "[ModernPluginLoader] 插件加载成功：" << instance->getName() << std::endl;
            } else {
                std::cout << "[ModernPluginLoader] 插件加载失败：" << error << std::endl;
            }
            
            if (callback) {
                callback(std::move(instance), error);
            }
        });
}

std::unique_ptr<juce::AudioPluginInstance> ModernPluginLoader::loadPluginSync(const juce::PluginDescription& description,
                                                                             double sampleRate,
                                                                             int bufferSize,
                                                                             juce::String& errorMessage) {
    std::cout << "[ModernPluginLoader] 同步加载插件：" << description.name << std::endl;
    
    auto instance = formatManager.createPluginInstance(description, sampleRate, bufferSize, errorMessage);
    
    if (instance) {
        std::cout << "[ModernPluginLoader] 插件加载成功：" << instance->getName() << std::endl;
    } else {
        std::cout << "[ModernPluginLoader] 插件加载失败：" << errorMessage << std::endl;
    }
    
    return instance;
}

bool ModernPluginLoader::doesPluginStillExist(const juce::PluginDescription& description) const {
    return formatManager.doesPluginStillExist(description);
}

//==============================================================================
// 黑名单管理实现
//==============================================================================

void ModernPluginLoader::addToBlacklist(const juce::String& pluginId) {
    std::cout << "[ModernPluginLoader] 添加到黑名单：" << pluginId << std::endl;
    
    std::lock_guard<std::mutex> lock(listMutex);
    knownPluginList.addToBlacklist(pluginId);
}

void ModernPluginLoader::removeFromBlacklist(const juce::String& pluginId) {
    std::cout << "[ModernPluginLoader] 从黑名单移除：" << pluginId << std::endl;
    
    std::lock_guard<std::mutex> lock(listMutex);
    knownPluginList.removeFromBlacklist(pluginId);
}

void ModernPluginLoader::clearBlacklist() {
    std::cout << "[ModernPluginLoader] 清除黑名单" << std::endl;
    
    std::lock_guard<std::mutex> lock(listMutex);
    knownPluginList.clearBlacklistedFiles();
}

const juce::StringArray& ModernPluginLoader::getBlacklist() const {
    std::lock_guard<std::mutex> lock(listMutex);
    return knownPluginList.getBlacklistedFiles();
}

//==============================================================================
// 缓存管理实现
//==============================================================================

bool ModernPluginLoader::savePluginList(const juce::File& file) const {
    std::cout << "[ModernPluginLoader] 保存插件列表到：" << file.getFullPathName() << std::endl;
    
    std::lock_guard<std::mutex> lock(listMutex);
    
    if (auto xml = knownPluginList.createXml()) {
        return xml->writeTo(file);
    }
    
    return false;
}

bool ModernPluginLoader::loadPluginList(const juce::File& file) {
    std::cout << "[ModernPluginLoader] 从文件加载插件列表：" << file.getFullPathName() << std::endl;
    
    if (!file.existsAsFile()) {
        std::cout << "[ModernPluginLoader] 插件列表文件不存在" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(listMutex);
    
    if (auto xml = juce::XmlDocument::parse(file)) {
        knownPluginList.recreateFromXml(*xml);
        std::cout << "[ModernPluginLoader] 加载了 " << knownPluginList.getNumTypes() << " 个插件" << std::endl;
        return true;
    }
    
    return false;
}

void ModernPluginLoader::clearPluginList() {
    std::cout << "[ModernPluginLoader] 清除插件列表" << std::endl;

    std::lock_guard<std::mutex> lock(listMutex);
    knownPluginList.clear();
}

//==============================================================================
// 回调设置
//==============================================================================

void ModernPluginLoader::setScanProgressCallback(ScanProgressCallback callback) {
    progressCallback = std::move(callback);
}

void ModernPluginLoader::setScanCompleteCallback(ScanCompleteCallback callback) {
    completeCallback = std::move(callback);
}

//==============================================================================
// 统计信息实现
//==============================================================================

int ModernPluginLoader::getNumKnownPlugins() const {
    std::lock_guard<std::mutex> lock(listMutex);
    return knownPluginList.getNumTypes();
}

std::map<juce::String, int> ModernPluginLoader::getPluginCountByFormat() const {
    std::lock_guard<std::mutex> lock(listMutex);
    std::map<juce::String, int> counts;

    for (const auto& plugin : knownPluginList.getTypes()) {
        counts[plugin.pluginFormatName]++;
    }

    return counts;
}

//==============================================================================
// 内部方法实现
//==============================================================================

void ModernPluginLoader::performScanWithDirectoryScanner(juce::AudioPluginFormat& format,
                                                       const juce::FileSearchPath& paths,
                                                       bool recursive,
                                                       bool rescanExisting,
                                                       int numThreads) {
    std::cout << "[ModernPluginLoader] 使用PluginDirectoryScanner扫描格式：" << format.getName() << std::endl;

    try {
        // 创建PluginDirectoryScanner
        std::lock_guard<std::mutex> lock(scannerMutex);
        currentScanner = std::make_unique<juce::PluginDirectoryScanner>(
            knownPluginList, format, paths, recursive, deadMansPedalFile, true);

        // 启动多个扫描作业
        for (int i = 0; i < numThreads; ++i) {
            scanningThreadPool->addJob(new ScanJob(*this, format), true);
        }

        // 等待扫描完成
        while (currentScanner && !shouldStopScanning.load()) {
            juce::String pluginBeingScanned;
            bool hasMore = currentScanner->scanNextFile(false, pluginBeingScanned);

            if (!hasMore) {
                break;
            }

            if (!pluginBeingScanned.isEmpty()) {
                float progress = currentScanner->getProgress();
                notifyProgress(progress, pluginBeingScanned);
            }

            juce::Thread::sleep(10);
        }

        // 清理扫描器
        currentScanner.reset();

    } catch (const std::exception& e) {
        std::cerr << "[ModernPluginLoader] 扫描异常：" << e.what() << std::endl;
        // 回退到传统扫描方式
        performLegacyScan(paths, recursive, rescanExisting);
    }

    scanning.store(false);

    int totalPlugins = getNumKnownPlugins();
    std::cout << "[ModernPluginLoader] 格式 " << format.getName() << " 扫描完成，总插件数：" << totalPlugins << std::endl;

    notifyComplete(totalPlugins);
}

void ModernPluginLoader::performLegacyScan(const juce::FileSearchPath& paths, bool recursive, bool rescanExisting) {
    std::cout << "[ModernPluginLoader] 使用传统扫描方式" << std::endl;

    int totalFilesFound = 0;
    int filesScanned = 0;
    int pluginsFound = 0;

    // 首先统计总文件数
    juce::StringArray allFiles;
    for (int i = 0; i < paths.getNumPaths(); ++i) {
        auto path = paths[i];
        std::cout << "[ModernPluginLoader] 扫描路径：" << path.getFullPathName() << std::endl;

        for (auto* format : formatManager.getFormats()) {
            juce::FileSearchPath searchPath;
            searchPath.add(path);
            auto filesForFormat = format->searchPathsForPlugins(searchPath, recursive);
            allFiles.addArray(filesForFormat);
        }
    }

    totalFilesFound = allFiles.size();
    std::cout << "[ModernPluginLoader] 找到 " << totalFilesFound << " 个潜在插件文件" << std::endl;

    // 扫描每个文件
    for (const auto& file : allFiles) {
        if (shouldStopScanning.load()) {
            std::cout << "[ModernPluginLoader] 扫描被用户停止" << std::endl;
            break;
        }

        filesScanned++;
        float progress = totalFilesFound > 0 ? (float)filesScanned / totalFilesFound : 1.0f;

        notifyProgress(progress, file);

        // 扫描文件中的插件
        for (auto* format : formatManager.getFormats()) {
            if (format->fileMightContainThisPluginType(file)) {
                juce::OwnedArray<juce::PluginDescription> typesFound;

                std::lock_guard<std::mutex> lock(listMutex);
                bool foundNew = knownPluginList.scanAndAddFile(file, !rescanExisting, typesFound, *format);

                if (foundNew) {
                    pluginsFound += typesFound.size();
                    std::cout << "[ModernPluginLoader] 在 " << file << " 中找到 "
                              << typesFound.size() << " 个插件" << std::endl;
                }

                break; // 找到匹配的格式就停止
            }
        }
    }

    std::cout << "[ModernPluginLoader] 传统扫描完成，找到 " << pluginsFound << " 个新插件" << std::endl;
}

void ModernPluginLoader::notifyProgress(float progress, const juce::String& currentFile) {
    if (progressCallback) {
        progressCallback(progress, currentFile);
    }
}

void ModernPluginLoader::notifyComplete(int foundPlugins) {
    if (completeCallback) {
        completeCallback(foundPlugins);
    }
}

juce::FileSearchPath ModernPluginLoader::getDefaultSearchPaths() const {
    juce::FileSearchPath defaultPaths;

#if JUCE_MAC
    // macOS默认路径
    defaultPaths.add(juce::File("~/Library/Audio/Plug-Ins/VST"));
    defaultPaths.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
    defaultPaths.add(juce::File("/Library/Audio/Plug-Ins/VST"));
    defaultPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    defaultPaths.add(juce::File("~/Library/Audio/Plug-Ins/Components")); // AU
    defaultPaths.add(juce::File("/Library/Audio/Plug-Ins/Components")); // AU
#elif JUCE_WINDOWS
    // Windows默认路径
    defaultPaths.add(juce::File("C:\\Program Files\\VSTPlugins"));
    defaultPaths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    defaultPaths.add(juce::File("C:\\Program Files (x86)\\VSTPlugins"));
    defaultPaths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));

    // 从注册表获取VST路径
    auto vstPath = juce::WindowsRegistry::getValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\VST\\VSTPluginsPath");
    if (vstPath.isNotEmpty()) {
        defaultPaths.add(juce::File(vstPath));
    }
#elif JUCE_LINUX
    // Linux默认路径
    defaultPaths.add(juce::File("~/.vst"));
    defaultPaths.add(juce::File("~/.vst3"));
    defaultPaths.add(juce::File("/usr/lib/vst"));
    defaultPaths.add(juce::File("/usr/lib/vst3"));
    defaultPaths.add(juce::File("/usr/local/lib/vst"));
    defaultPaths.add(juce::File("/usr/local/lib/vst3"));
#endif

    std::cout << "[ModernPluginLoader] 默认搜索路径数量：" << defaultPaths.getNumPaths() << std::endl;
    for (int i = 0; i < defaultPaths.getNumPaths(); ++i) {
        auto path = defaultPaths[i];
        std::cout << "[ModernPluginLoader] - " << path.getFullPathName()
                  << " (存在: " << (path.exists() ? "是" : "否") << ")" << std::endl;
    }

    return defaultPaths;
}

int ModernPluginLoader::getRecommendedThreadCount() const {
    // 基于JUCE AudioPluginHost的策略
    int numCores = juce::SystemStats::getNumCpus();

    // 对于插件扫描，使用较少的线程以避免过度并发导致的问题
    // 特别是某些插件可能不是线程安全的
    int recommendedThreads = juce::jmax(1, juce::jmin(4, numCores / 2));

    std::cout << "[ModernPluginLoader] 系统CPU核心数：" << numCores
              << "，推荐扫描线程数：" << recommendedThreads << std::endl;

    return recommendedThreads;
}

} // namespace WindsynthVST::AudioGraph
