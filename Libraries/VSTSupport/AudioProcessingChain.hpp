#pragma once

#include <JuceHeader.h>
#include "VSTPluginManager.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace WindsynthVST {

/**
 * 音频处理链中的单个插件节点
 */
class ProcessingNode {
public:
    ProcessingNode(std::unique_ptr<VSTPluginInstance> plugin);
    ~ProcessingNode();
    
    // 基本信息
    const std::string& getName() const { return name; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool enable) { enabled = enable; }
    
    // 音频处理
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // 插件访问
    VSTPluginInstance* getPlugin() { return plugin.get(); }
    const VSTPluginInstance* getPlugin() const { return plugin.get(); }
    
    // 旁路控制
    void setBypass(bool bypass) { bypassed = bypass; }
    bool isBypassed() const { return bypassed; }
    
    // 预设管理
    void saveState(juce::MemoryBlock& destData);
    void loadState(const void* data, int sizeInBytes);
    
private:
    std::unique_ptr<VSTPluginInstance> plugin;
    std::string name;
    bool enabled = true;
    bool bypassed = false;
    bool isPrepared = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingNode)
};

/**
 * 音频处理链配置
 */
struct ProcessingChainConfig {
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    int numInputChannels = 2;
    int numOutputChannels = 2;
    bool enableMidi = true;
};

/**
 * 音频处理链
 * 管理多个VST插件的串联处理
 */
class AudioProcessingChain {
public:
    AudioProcessingChain();
    ~AudioProcessingChain();
    
    // 配置
    void configure(const ProcessingChainConfig& config);
    const ProcessingChainConfig& getConfig() const { return config; }
    
    // 音频处理生命周期
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // 插件管理
    bool addPlugin(std::unique_ptr<VSTPluginInstance> plugin);
    bool insertPlugin(int index, std::unique_ptr<VSTPluginInstance> plugin);
    bool removePlugin(int index);
    bool movePlugin(int fromIndex, int toIndex);
    void clearPlugins();
    
    // 插件访问
    int getNumPlugins() const { return static_cast<int>(nodes.size()); }
    ProcessingNode* getNode(int index);
    const ProcessingNode* getNode(int index) const;
    
    // 查找插件
    int findPluginIndex(const std::string& pluginName) const;
    ProcessingNode* findPlugin(const std::string& pluginName);
    
    // 旁路控制
    void setPluginBypassed(int index, bool bypassed);
    bool isPluginBypassed(int index) const;
    
    // 全局控制
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    
    void setMasterBypass(bool bypass) { masterBypass = bypass; }
    bool isMasterBypassed() const { return masterBypass; }
    
    // 性能监控
    struct PerformanceStats {
        double averageProcessingTime = 0.0;
        double peakProcessingTime = 0.0;
        double cpuUsagePercent = 0.0;
        int bufferUnderruns = 0;
    };
    
    const PerformanceStats& getPerformanceStats() const { return stats; }
    void resetPerformanceStats();
    
    // 预设管理
    struct ChainPreset {
        std::string name;
        std::vector<juce::MemoryBlock> pluginStates;
        std::vector<bool> pluginBypassed;
        ProcessingChainConfig config;
    };
    
    ChainPreset savePreset(const std::string& name) const;
    bool loadPreset(const ChainPreset& preset);
    
    // 回调设置
    using ProcessingCallback = std::function<void(const juce::AudioBuffer<float>&, const juce::MidiBuffer&)>;
    void setPreProcessingCallback(ProcessingCallback callback) { preProcessingCallback = callback; }
    void setPostProcessingCallback(ProcessingCallback callback) { postProcessingCallback = callback; }
    
    using ErrorCallback = std::function<void(const std::string& error)>;
    void setErrorCallback(ErrorCallback callback) { errorCallback = callback; }
    
    // 延迟补偿
    int getTotalLatency() const;
    void setLatencyCompensation(bool enable) { latencyCompensationEnabled = enable; }
    bool isLatencyCompensationEnabled() const { return latencyCompensationEnabled; }
    
private:
    std::vector<std::unique_ptr<ProcessingNode>> nodes;
    ProcessingChainConfig config;
    
    bool enabled = true;
    bool masterBypass = false;
    bool isPrepared = false;
    bool latencyCompensationEnabled = true;
    
    // 性能监控
    PerformanceStats stats;
    juce::Time lastProcessTime;
    std::vector<double> processingTimes;
    
    // 回调函数
    ProcessingCallback preProcessingCallback;
    ProcessingCallback postProcessingCallback;
    ErrorCallback errorCallback;
    
    // 内部缓冲区（用于延迟补偿等）
    juce::AudioBuffer<float> internalBuffer;
    juce::MidiBuffer internalMidiBuffer;
    
    // 内部方法
    void updatePerformanceStats(double processingTime);
    void onError(const std::string& error);
    bool validateIndex(int index) const;
    
    // 线程安全
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioProcessingChain)
};

} // namespace WindsynthVST
