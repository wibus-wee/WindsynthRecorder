//
//  AudioIOManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频I/O管理器
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include "../Core/GraphAudioProcessor.hpp"
#include "../Core/AudioGraphTypes.hpp"

namespace WindsynthVST::AudioGraph {

/**
 * 音频I/O管理器
 * 
 * 这个类专门处理音频图的输入输出管理：
 * - 音频设备集成和配置
 * - 通道映射和路由管理
 * - 输入输出节点的智能连接
 * - 音频格式转换和适配
 * - 实时监控和电平检测
 */
class AudioIOManager {
public:
    //==============================================================================
    // 类型定义
    //==============================================================================
    
    /**
     * 音频设备信息
     */
    struct AudioDeviceInfo {
        std::string name;
        std::string type;
        int numInputChannels = 0;
        int numOutputChannels = 0;
        std::vector<double> supportedSampleRates;
        std::vector<int> supportedBufferSizes;
        bool isDefault = false;
        bool isAvailable = true;
    };
    
    /**
     * 通道映射配置
     */
    struct ChannelMapping {
        int sourceChannel = -1;
        int destinationChannel = -1;
        float gain = 1.0f;
        bool muted = false;
        bool soloed = false;
        std::string label;
        
        ChannelMapping() = default;
        ChannelMapping(int src, int dst, float g = 1.0f) 
            : sourceChannel(src), destinationChannel(dst), gain(g) {}
    };
    
    /**
     * I/O配置
     */
    struct IOConfiguration {
        int numInputChannels = 2;
        int numOutputChannels = 2;
        double sampleRate = 44100.0;
        int bufferSize = 512;
        std::vector<ChannelMapping> inputMappings;
        std::vector<ChannelMapping> outputMappings;
        bool enableInputMonitoring = false;
        bool enableOutputLimiting = true;
        float inputGain = 1.0f;
        float outputGain = 1.0f;
    };
    
    /**
     * 音频电平信息
     */
    struct AudioLevelInfo {
        std::vector<float> inputLevels;
        std::vector<float> outputLevels;
        std::vector<float> inputPeaks;
        std::vector<float> outputPeaks;
        bool inputClipping = false;
        bool outputClipping = false;
        double timestamp = 0.0;
    };
    
    //==============================================================================
    // 回调类型定义
    //==============================================================================
    
    using DeviceChangeCallback = std::function<void(const AudioDeviceInfo& device, bool connected)>;
    using LevelUpdateCallback = std::function<void(const AudioLevelInfo& levels)>;
    using ConfigChangeCallback = std::function<void(const IOConfiguration& config)>;
    
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param graphProcessor 音频图处理器
     */
    explicit AudioIOManager(GraphAudioProcessor& graphProcessor);
    
    /**
     * 析构函数
     */
    ~AudioIOManager();
    
    //==============================================================================
    // 设备管理
    //==============================================================================
    
    /**
     * 扫描可用的音频设备
     * @return 可用设备列表
     */
    std::vector<AudioDeviceInfo> scanAudioDevices();
    
    /**
     * 设置音频设备
     * @param deviceName 设备名称
     * @param sampleRate 采样率
     * @param bufferSize 缓冲区大小
     * @return 成功返回true
     */
    bool setAudioDevice(const std::string& deviceName, 
                       double sampleRate = 44100.0, 
                       int bufferSize = 512);
    
    /**
     * 获取当前音频设备信息
     * @return 当前设备信息
     */
    AudioDeviceInfo getCurrentDevice() const;
    
    /**
     * 检查设备是否可用
     * @param deviceName 设备名称
     * @return 可用返回true
     */
    bool isDeviceAvailable(const std::string& deviceName) const;
    
    //==============================================================================
    // I/O配置管理
    //==============================================================================
    
    /**
     * 配置音频I/O
     * @param config I/O配置
     * @return 成功返回true
     */
    bool configureIO(const IOConfiguration& config);
    
    /**
     * 获取当前I/O配置
     * @return 当前配置
     */
    const IOConfiguration& getCurrentConfiguration() const { return currentConfig; }
    
    /**
     * 设置输入通道数
     * @param numChannels 通道数
     * @return 成功返回true
     */
    bool setInputChannels(int numChannels);
    
    /**
     * 设置输出通道数
     * @param numChannels 通道数
     * @return 成功返回true
     */
    bool setOutputChannels(int numChannels);
    
    /**
     * 设置采样率
     * @param sampleRate 采样率
     * @return 成功返回true
     */
    bool setSampleRate(double sampleRate);
    
    /**
     * 设置缓冲区大小
     * @param bufferSize 缓冲区大小
     * @return 成功返回true
     */
    bool setBufferSize(int bufferSize);
    
    //==============================================================================
    // 通道映射管理
    //==============================================================================
    
    /**
     * 添加输入通道映射
     * @param mapping 通道映射
     * @return 成功返回true
     */
    bool addInputMapping(const ChannelMapping& mapping);
    
    /**
     * 添加输出通道映射
     * @param mapping 通道映射
     * @return 成功返回true
     */
    bool addOutputMapping(const ChannelMapping& mapping);
    
    /**
     * 移除输入通道映射
     * @param sourceChannel 源通道
     * @return 成功返回true
     */
    bool removeInputMapping(int sourceChannel);
    
    /**
     * 移除输出通道映射
     * @param destinationChannel 目标通道
     * @return 成功返回true
     */
    bool removeOutputMapping(int destinationChannel);
    
    /**
     * 清除所有通道映射
     */
    void clearAllMappings();
    
    /**
     * 创建默认通道映射
     */
    void createDefaultMappings();
    
    //==============================================================================
    // 智能连接管理
    //==============================================================================
    
    /**
     * 自动连接节点到输入
     * @param nodeID 节点ID
     * @param channelOffset 通道偏移
     * @return 成功创建的连接数
     */
    int autoConnectToInput(NodeID nodeID, int channelOffset = 0);
    
    /**
     * 自动连接节点到输出
     * @param nodeID 节点ID
     * @param channelOffset 通道偏移
     * @return 成功创建的连接数
     */
    int autoConnectToOutput(NodeID nodeID, int channelOffset = 0);
    
    /**
     * 连接MIDI输入到节点
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool connectMidiInput(NodeID nodeID);
    
    /**
     * 连接节点到MIDI输出
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool connectMidiOutput(NodeID nodeID);
    
    /**
     * 断开节点的所有I/O连接
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool disconnectAllIO(NodeID nodeID);
    
    //==============================================================================
    // 音频监控和电平检测
    //==============================================================================
    
    /**
     * 启用音频电平监控
     * @param enable 是否启用
     */
    void enableLevelMonitoring(bool enable);
    
    /**
     * 获取当前音频电平
     * @return 音频电平信息
     */
    AudioLevelInfo getCurrentLevels() const;
    
    /**
     * 重置峰值电平
     */
    void resetPeakLevels();
    
    /**
     * 设置电平更新间隔
     * @param intervalMs 更新间隔（毫秒）
     */
    void setLevelUpdateInterval(int intervalMs);
    
    //==============================================================================
    // 音频处理控制
    //==============================================================================
    
    /**
     * 设置输入增益
     * @param gain 增益值（线性）
     */
    void setInputGain(float gain);
    
    /**
     * 设置输出增益
     * @param gain 增益值（线性）
     */
    void setOutputGain(float gain);
    
    /**
     * 设置输入静音
     * @param muted 是否静音
     */
    void setInputMuted(bool muted);
    
    /**
     * 设置输出静音
     * @param muted 是否静音
     */
    void setOutputMuted(bool muted);
    
    /**
     * 启用输入监听
     * @param enable 是否启用
     */
    void enableInputMonitoring(bool enable);
    
    /**
     * 启用输出限制器
     * @param enable 是否启用
     */
    void enableOutputLimiting(bool enable);
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置设备变化回调
     */
    void setDeviceChangeCallback(DeviceChangeCallback callback);
    
    /**
     * 设置电平更新回调
     */
    void setLevelUpdateCallback(LevelUpdateCallback callback);
    
    /**
     * 设置配置变化回调
     */
    void setConfigChangeCallback(ConfigChangeCallback callback);
    
    //==============================================================================
    // 状态查询
    //==============================================================================
    
    /**
     * 检查是否已配置
     * @return 已配置返回true
     */
    bool isConfigured() const { return configured; }
    
    /**
     * 检查是否正在监控电平
     * @return 正在监控返回true
     */
    bool isLevelMonitoringEnabled() const { return levelMonitoringEnabled; }
    
    /**
     * 获取输入节点ID
     * @return 输入节点ID
     */
    NodeID getAudioInputNodeID() const;
    
    /**
     * 获取输出节点ID
     * @return 输出节点ID
     */
    NodeID getAudioOutputNodeID() const;
    
    /**
     * 获取MIDI输入节点ID
     * @return MIDI输入节点ID
     */
    NodeID getMidiInputNodeID() const;
    
    /**
     * 获取MIDI输出节点ID
     * @return MIDI输出节点ID
     */
    NodeID getMidiOutputNodeID() const;

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    GraphAudioProcessor& graphProcessor;
    
    // 配置状态
    IOConfiguration currentConfig;
    bool configured = false;
    
    // 设备管理
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    AudioDeviceInfo currentDevice;
    
    // 电平监控
    bool levelMonitoringEnabled = false;
    AudioLevelInfo currentLevels;
    std::vector<float> inputLevelSmoothers;
    std::vector<float> outputLevelSmoothers;
    juce::Time lastLevelUpdate;
    int levelUpdateIntervalMs = 50;
    
    // 音频处理状态
    bool inputMuted = false;
    bool outputMuted = false;
    bool inputMonitoringEnabled = false;
    bool outputLimitingEnabled = true;
    
    // 回调函数
    DeviceChangeCallback deviceChangeCallback;
    LevelUpdateCallback levelUpdateCallback;
    ConfigChangeCallback configChangeCallback;
    
    // 线程安全
    mutable std::mutex configMutex;
    mutable std::mutex levelMutex;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void initializeDeviceManager();
    void updateChannelMappings();
    void updateAudioLevels(const juce::AudioBuffer<float>& buffer, bool isInput);
    void notifyConfigChange();
    void notifyDeviceChange(const AudioDeviceInfo& device, bool connected);
    void notifyLevelUpdate();
    
    // 电平计算辅助方法
    float calculateRMSLevel(const float* channelData, int numSamples);
    float calculatePeakLevel(const float* channelData, int numSamples);
    float smoothLevel(float currentLevel, float newLevel, float smoothingFactor = 0.3f);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIOManager)
};

} // namespace WindsynthVST::AudioGraph
