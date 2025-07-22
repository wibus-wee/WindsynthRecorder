#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>

namespace WindsynthVST {

/**
 * VST插件信息结构
 */
struct VSTPluginInfo {
    std::string name;
    std::string manufacturer;
    std::string version;
    std::string category;
    std::string pluginFormatName;
    std::string fileOrIdentifier;
    int numInputChannels;
    int numOutputChannels;
    bool isInstrument;
    bool acceptsMidi;
    bool producesMidi;
    
    VSTPluginInfo() = default;
    VSTPluginInfo(const juce::PluginDescription& desc);
};

/**
 * VST插件实例包装器
 */
class VSTPluginInstance {
public:
    VSTPluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance);
    ~VSTPluginInstance();
    
    // 基本控制
    bool isValid() const { return pluginInstance != nullptr; }
    const std::string& getName() const { return name; }
    
    // 音频处理
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // 参数控制
    int getNumParameters() const;
    float getParameter(int index) const;
    void setParameter(int index, float value);
    std::string getParameterName(int index) const;
    std::string getParameterText(int index) const;
    
    // 预设管理
    void getStateInformation(juce::MemoryBlock& destData);
    void setStateInformation(const void* data, int sizeInBytes);
    
    // 编辑器
    bool hasEditor() const;
    juce::AudioProcessorEditor* createEditor();
    
    // 获取原始插件实例（用于高级操作）
    juce::AudioPluginInstance* getRawInstance() { return pluginInstance.get(); }
    
private:
    std::unique_ptr<juce::AudioPluginInstance> pluginInstance;
    std::string name;
    bool isPrepared = false;
};

/**
 * VST插件管理器
 * 负责扫描、加载和管理VST插件
 */
class VSTPluginManager {
public:
    VSTPluginManager();
    ~VSTPluginManager();
    
    // 插件扫描
    void scanForPlugins();
    void scanDirectory(const std::string& directoryPath);
    void addPluginSearchPath(const std::string& path);
    
    // 插件信息查询
    std::vector<VSTPluginInfo> getAvailablePlugins() const;
    std::vector<VSTPluginInfo> getPluginsByCategory(const std::string& category) const;
    VSTPluginInfo getPluginInfo(const std::string& identifier) const;
    
    // 插件加载 - 异步版本（推荐）
    void loadPluginAsync(const std::string& identifier,
                        std::function<void(std::unique_ptr<VSTPluginInstance>, const std::string&)> callback);
    void loadPluginAsync(const VSTPluginInfo& info,
                        std::function<void(std::unique_ptr<VSTPluginInstance>, const std::string&)> callback);

    // 插件加载 - 同步版本（仅用于简单测试，可能阻塞）
    std::unique_ptr<VSTPluginInstance> loadPlugin(const std::string& identifier);
    std::unique_ptr<VSTPluginInstance> loadPlugin(const VSTPluginInfo& info);
    
    // 插件格式管理
    void enableVST3Support(bool enable = true);
    void enableAUSupport(bool enable = true);
    
    // 回调设置
    using ScanProgressCallback = std::function<void(const std::string& pluginName, float progress)>;
    void setScanProgressCallback(ScanProgressCallback callback);
    
    using ErrorCallback = std::function<void(const std::string& error)>;
    void setErrorCallback(ErrorCallback callback);
    
    // 状态查询
    bool isScanning() const { return isCurrentlyScanning; }
    int getNumAvailablePlugins() const;
    
    // 缓存管理
    void saveCacheToFile(const std::string& filePath);
    void loadCacheFromFile(const std::string& filePath);
    void clearCache();
    
private:
    // JUCE插件格式管理器
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    // 扫描相关
    std::unique_ptr<juce::PluginDirectoryScanner> scanner;
    bool isCurrentlyScanning = false;
    
    // 回调函数
    ScanProgressCallback scanProgressCallback;
    ErrorCallback errorCallback;
    
    // 搜索路径
    std::vector<std::string> searchPaths;
    
    // 内部方法
    void initializeFormatManager();
    void scanPluginsInBackground();
    void onScanProgress(const std::string& pluginName, float progress);
    void onError(const std::string& error);
    
    // 线程安全
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTPluginManager)
};

} // namespace WindsynthVST
