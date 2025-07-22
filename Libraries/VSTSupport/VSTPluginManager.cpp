#include "VSTPluginManager.hpp"
#include <iostream>

namespace WindsynthVST {

// VSTPluginInfo 实现
VSTPluginInfo::VSTPluginInfo(const juce::PluginDescription& desc) {
    name = desc.name.toStdString();
    manufacturer = desc.manufacturerName.toStdString();
    version = desc.version.toStdString();
    category = desc.category.toStdString();
    pluginFormatName = desc.pluginFormatName.toStdString();
    fileOrIdentifier = desc.fileOrIdentifier.toStdString();
    numInputChannels = desc.numInputChannels;
    numOutputChannels = desc.numOutputChannels;
    isInstrument = desc.isInstrument;
    // 新版本JUCE没有这些字段，设为默认值
    acceptsMidi = desc.isInstrument; // 乐器通常接受MIDI
    producesMidi = false; // 大多数插件不产生MIDI
}

// VSTPluginInstance 实现
VSTPluginInstance::VSTPluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance)
    : pluginInstance(std::move(instance)) {
    if (pluginInstance) {
        name = pluginInstance->getName().toStdString();
    }
}

VSTPluginInstance::~VSTPluginInstance() {
    if (isPrepared) {
        releaseResources();
    }
}

void VSTPluginInstance::prepareToPlay(double sampleRate, int samplesPerBlock) {
    if (pluginInstance) {
        // 确保插件有正确的总线配置
        auto layout = pluginInstance->getBusesLayout();
        auto inputBuses = layout.inputBuses;
        auto outputBuses = layout.outputBuses;

        // 如果没有配置总线，设置默认的立体声配置
        if (inputBuses.isEmpty() && outputBuses.isEmpty()) {
            juce::AudioProcessor::BusesLayout newLayout;
            newLayout.inputBuses.add(juce::AudioChannelSet::stereo());
            newLayout.outputBuses.add(juce::AudioChannelSet::stereo());

            if (!pluginInstance->setBusesLayout(newLayout)) {
                std::cout << "[VSTPluginInstance] Warning: Failed to set default stereo layout" << std::endl;
                // 尝试单声道配置
                newLayout.inputBuses.clear();
                newLayout.outputBuses.clear();
                newLayout.inputBuses.add(juce::AudioChannelSet::mono());
                newLayout.outputBuses.add(juce::AudioChannelSet::mono());

                if (!pluginInstance->setBusesLayout(newLayout)) {
                    std::cout << "[VSTPluginInstance] Warning: Failed to set mono layout" << std::endl;
                }
            }
        }

        std::cout << "[VSTPluginInstance] Preparing plugin: " << pluginInstance->getName()
                  << " at " << sampleRate << "Hz, " << samplesPerBlock << " samples" << std::endl;
        std::cout << "[VSTPluginInstance] Input buses: " << inputBuses.size()
                  << ", Output buses: " << outputBuses.size() << std::endl;

        pluginInstance->prepareToPlay(sampleRate, samplesPerBlock);
        isPrepared = true;

        std::cout << "[VSTPluginInstance] Plugin prepared successfully" << std::endl;
    }
}

void VSTPluginInstance::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (pluginInstance && isPrepared) {
        try {
            // 验证缓冲区
            if (buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0) {
                std::cout << "[VSTPluginInstance] Invalid buffer: samples=" << buffer.getNumSamples()
                          << ", channels=" << buffer.getNumChannels() << std::endl;
                return;
            }

            // 检查插件是否被暂停
            if (pluginInstance->isSuspended()) {
                std::cout << "[VSTPluginInstance] Plugin is suspended, clearing buffer" << std::endl;
                buffer.clear();
                return;
            }

            // 检查通道数匹配
            auto layout = pluginInstance->getBusesLayout();
            int expectedInputChannels = 0;
            int expectedOutputChannels = 0;

            for (const auto& bus : layout.inputBuses) {
                expectedInputChannels += bus.size();
            }
            for (const auto& bus : layout.outputBuses) {
                expectedOutputChannels += bus.size();
            }

            // 只在第一次或通道配置改变时输出日志
            static int logCount = 0;
            static int lastBufferChannels = -1;
            static int lastExpectedInput = -1;

            if (logCount++ % 1000 == 0 ||
                lastBufferChannels != buffer.getNumChannels() ||
                lastExpectedInput != expectedInputChannels) {

                std::cout << "[VSTPluginInstance] Processing: buffer=" << buffer.getNumChannels()
                          << " channels, expected input=" << expectedInputChannels
                          << ", expected output=" << expectedOutputChannels << std::endl;

                lastBufferChannels = buffer.getNumChannels();
                lastExpectedInput = expectedInputChannels;
            }

            // 如果通道数不匹配，调整缓冲区
            if (buffer.getNumChannels() != expectedInputChannels && expectedInputChannels > 0) {
                if (logCount % 1000 == 1) {
                    std::cout << "[VSTPluginInstance] Channel mismatch, using temporary buffer" << std::endl;
                }

                // 创建匹配插件期望的临时缓冲区
                juce::AudioBuffer<float> tempBuffer(std::max(expectedInputChannels, expectedOutputChannels),
                                                   buffer.getNumSamples());
                tempBuffer.clear();

                // 复制输入数据
                int channelsToCopy = std::min(buffer.getNumChannels(), expectedInputChannels);
                for (int ch = 0; ch < channelsToCopy; ++ch) {
                    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
                }

                // 处理临时缓冲区
                pluginInstance->processBlock(tempBuffer, midiMessages);

                // 复制输出数据回原缓冲区
                int outputChannelsToCopy = std::min(buffer.getNumChannels(), expectedOutputChannels);
                for (int ch = 0; ch < outputChannelsToCopy; ++ch) {
                    buffer.copyFrom(ch, 0, tempBuffer, ch, 0, buffer.getNumSamples());
                }
            } else {
                // 直接处理
                pluginInstance->processBlock(buffer, midiMessages);
            }

        } catch (const std::exception& e) {
            std::cout << "[VSTPluginInstance] Exception in processBlock: " << e.what() << std::endl;
            // 清空缓冲区以避免噪音
            buffer.clear();
        } catch (...) {
            std::cout << "[VSTPluginInstance] Unknown exception in processBlock" << std::endl;
            buffer.clear();
        }
    }
}

void VSTPluginInstance::releaseResources() {
    if (pluginInstance && isPrepared) {
        pluginInstance->releaseResources();
        isPrepared = false;
    }
}

int VSTPluginInstance::getNumParameters() const {
    if (!pluginInstance) return 0;
    return pluginInstance->getParameters().size();
}

float VSTPluginInstance::getParameter(int index) const {
    if (!pluginInstance) return 0.0f;
    const auto& params = pluginInstance->getParameters();
    if (index >= 0 && index < params.size()) {
        return params[index]->getValue();
    }
    return 0.0f;
}

void VSTPluginInstance::setParameter(int index, float value) {
    if (!pluginInstance) return;
    const auto& params = pluginInstance->getParameters();
    if (index >= 0 && index < params.size()) {
        params[index]->setValue(value);
    }
}

std::string VSTPluginInstance::getParameterName(int index) const {
    if (!pluginInstance) return "";
    const auto& params = pluginInstance->getParameters();
    if (index >= 0 && index < params.size()) {
        return params[index]->getName(256).toStdString();
    }
    return "";
}

std::string VSTPluginInstance::getParameterText(int index) const {
    if (!pluginInstance) return "";
    const auto& params = pluginInstance->getParameters();
    if (index >= 0 && index < params.size()) {
        return params[index]->getText(params[index]->getValue(), 256).toStdString();
    }
    return "";
}

void VSTPluginInstance::getStateInformation(juce::MemoryBlock& destData) {
    if (pluginInstance) {
        pluginInstance->getStateInformation(destData);
    }
}

void VSTPluginInstance::setStateInformation(const void* data, int sizeInBytes) {
    if (pluginInstance) {
        pluginInstance->setStateInformation(data, sizeInBytes);
    }
}

bool VSTPluginInstance::hasEditor() const {
    return pluginInstance ? pluginInstance->hasEditor() : false;
}

juce::AudioProcessorEditor* VSTPluginInstance::createEditor() {
    return pluginInstance ? pluginInstance->createEditor() : nullptr;
}

// VSTPluginManager 实现
VSTPluginManager::VSTPluginManager() {
    initializeFormatManager();
}

VSTPluginManager::~VSTPluginManager() {
    // 确保扫描停止
    scanner.reset();
}

void VSTPluginManager::initializeFormatManager() {
    // 添加VST3格式支持
    formatManager.addDefaultFormats();
    
    // 在macOS上添加AU支持（如果启用）
#if JUCE_MAC && JUCE_PLUGINHOST_AU
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
#endif
}

void VSTPluginManager::scanForPlugins() {
    juce::ScopedLock sl(lock);

    if (isCurrentlyScanning) {
        return; // 已经在扫描中
    }

    isCurrentlyScanning = true;

    // 获取默认VST路径
    juce::FileSearchPath defaultPaths;

#if JUCE_MAC
    defaultPaths.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
    defaultPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    defaultPaths.add(juce::File("~/Library/Audio/Plug-Ins/Components"));
    defaultPaths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
#elif JUCE_WINDOWS
    defaultPaths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    defaultPaths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
#endif

    // 添加用户指定的搜索路径
    for (const auto& path : searchPaths) {
        defaultPaths.add(juce::File(path));
    }

    // 扫描每种格式
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* format = formatManager.getFormat(i);
        if (format) {
            // 创建扫描器
            scanner = std::make_unique<juce::PluginDirectoryScanner>(
                knownPluginList, *format, defaultPaths, true, juce::File());

            // 在后台线程中扫描
            juce::Thread::launch([this]() {
                scanPluginsInBackground();
            });
            break; // 暂时只扫描第一种格式
        }
    }
}

void VSTPluginManager::scanPluginsInBackground() {
    juce::String pluginBeingScanned;

    while (scanner) {
        if (!scanner->scanNextFile(true, pluginBeingScanned)) {
            break; // 扫描完成
        }

        float progress = scanner->getProgress();
        onScanProgress(pluginBeingScanned.toStdString(), progress);

        juce::Thread::sleep(10); // 避免过度占用CPU
    }

    {
        juce::ScopedLock sl(lock);
        isCurrentlyScanning = false;
        scanner.reset();
    }

    onScanProgress("扫描完成", 1.0f);
}

void VSTPluginManager::scanDirectory(const std::string& directoryPath) {
    // 简单实现：添加到搜索路径然后扫描
    addPluginSearchPath(directoryPath);
    scanForPlugins();
}

std::vector<VSTPluginInfo> VSTPluginManager::getAvailablePlugins() const {
    std::vector<VSTPluginInfo> plugins;

    const auto& types = knownPluginList.getTypes();
    for (const auto& desc : types) {
        plugins.emplace_back(desc);
    }

    return plugins;
}

std::vector<VSTPluginInfo> VSTPluginManager::getPluginsByCategory(const std::string& category) const {
    std::vector<VSTPluginInfo> plugins;

    const auto& types = knownPluginList.getTypes();
    for (const auto& desc : types) {
        VSTPluginInfo info(desc);
        if (info.category == category) {
            plugins.push_back(info);
        }
    }

    return plugins;
}

VSTPluginInfo VSTPluginManager::getPluginInfo(const std::string& identifier) const {
    const auto& types = knownPluginList.getTypes();
    for (const auto& desc : types) {
        if (desc.fileOrIdentifier.toStdString() == identifier ||
            desc.name.toStdString() == identifier) {
            return VSTPluginInfo(desc);
        }
    }

    // 返回空的插件信息表示未找到
    return VSTPluginInfo();
}

std::unique_ptr<VSTPluginInstance> VSTPluginManager::loadPlugin(const std::string& identifier) {
    std::cout << "[VSTPluginManager] loadPlugin: Searching for plugin with identifier: " << identifier << std::endl;

    const auto& types = knownPluginList.getTypes();
    std::cout << "[VSTPluginManager] loadPlugin: Total known plugins: " << types.size() << std::endl;

    for (const auto& desc : types) {
        std::cout << "[VSTPluginManager] loadPlugin: Checking plugin - name: " << desc.name.toStdString()
                  << ", fileOrIdentifier: " << desc.fileOrIdentifier.toStdString() << std::endl;

        if (desc.fileOrIdentifier.toStdString() == identifier) {
            std::cout << "[VSTPluginManager] loadPlugin: Found matching plugin, loading..." << std::endl;
            return loadPlugin(VSTPluginInfo(desc));
        }
    }

    std::cout << "[VSTPluginManager] loadPlugin: Plugin not found in known list" << std::endl;
    onError("找不到插件: " + identifier);
    return nullptr;
}

// 异步加载插件（推荐方式）
void VSTPluginManager::loadPluginAsync(const VSTPluginInfo& info,
                                      std::function<void(std::unique_ptr<VSTPluginInstance>, const std::string&)> callback) {
    // 直接从已知插件列表中查找原始描述
    const auto& types = knownPluginList.getTypes();
    for (const auto& desc : types) {
        if (desc.name.toStdString() == info.name &&
            desc.manufacturerName.toStdString() == info.manufacturer) {

            // 使用异步方法创建插件实例
            formatManager.createPluginInstanceAsync(desc, 44100.0, 512,
                [callback, info](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error) {
                    if (instance) {
                        auto vstInstance = std::make_unique<VSTPluginInstance>(std::move(instance));
                        callback(std::move(vstInstance), "");
                    } else {
                        callback(nullptr, "无法加载插件 " + info.name + ": " + error.toStdString());
                    }
                });
            return;
        }
    }

    // 如果没找到插件，立即调用回调
    callback(nullptr, "在已知插件列表中找不到插件: " + info.name);
}

void VSTPluginManager::loadPluginAsync(const std::string& identifier,
                                      std::function<void(std::unique_ptr<VSTPluginInstance>, const std::string&)> callback) {
    auto info = getPluginInfo(identifier);
    if (info.name.empty()) {
        callback(nullptr, "找不到插件: " + identifier);
        return;
    }
    loadPluginAsync(info, callback);
}

// 同步加载插件（仅用于简单测试，可能阻塞）
std::unique_ptr<VSTPluginInstance> VSTPluginManager::loadPlugin(const VSTPluginInfo& info) {
    // 直接从已知插件列表中查找原始描述
    const auto& types = knownPluginList.getTypes();
    for (const auto& desc : types) {
        if (desc.name.toStdString() == info.name &&
            desc.manufacturerName.toStdString() == info.manufacturer) {

            // 使用原始描述创建插件实例
            juce::String errorMessage;
            auto instance = formatManager.createPluginInstance(desc, 44100.0, 512, errorMessage);

            if (instance) {
                return std::make_unique<VSTPluginInstance>(std::move(instance));
            } else {
                onError("无法加载插件 " + info.name + ": " + errorMessage.toStdString());
                return nullptr;
            }
        }
    }

    onError("在已知插件列表中找不到插件: " + info.name);
    return nullptr;
}

void VSTPluginManager::addPluginSearchPath(const std::string& path) {
    searchPaths.push_back(path);
}

void VSTPluginManager::setScanProgressCallback(ScanProgressCallback callback) {
    scanProgressCallback = callback;
}

void VSTPluginManager::setErrorCallback(ErrorCallback callback) {
    errorCallback = callback;
}

int VSTPluginManager::getNumAvailablePlugins() const {
    return knownPluginList.getNumTypes();
}

void VSTPluginManager::onScanProgress(const std::string& pluginName, float progress) {
    if (scanProgressCallback) {
        scanProgressCallback(pluginName, progress);
    }
}

void VSTPluginManager::onError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
}

} // namespace WindsynthVST
