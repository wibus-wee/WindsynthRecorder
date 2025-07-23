//
//  WindsynthEngineFacade.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新音频架构的高层门面类 - 完全独立于VSTSupport
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

// 新架构组件
#include "AudioGraph/Core/GraphAudioProcessor.hpp"
#include "AudioGraph/Management/GraphManager.hpp"
#include "AudioGraph/Plugins/PluginManager.hpp"
#include "AudioGraph/Plugins/ModernPluginLoader.hpp"
#include "AudioGraph/Management/AudioIOManager.hpp"
#include "AudioGraph/Management/PresetManager.hpp"
#include "AudioGraph/Core/AudioGraphTypes.hpp"

namespace WindsynthVST::Engine {

/**
 * 引擎状态枚举
 */
enum class EngineState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Error
};

/**
 * 引擎配置结构
 */
struct EngineConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
    int numInputChannels = 0;
    int numOutputChannels = 2;
    bool enableRealtimeProcessing = true;
    std::string audioDeviceName;
};

/**
 * 插件信息结构（简化版）
 */
struct SimplePluginInfo {
    std::string identifier;
    std::string name;
    std::string manufacturer;
    std::string category;
    std::string format;
    std::string filePath;
    bool isValid = false;
};

/**
 * 节点信息结构（简化版）
 */
struct SimpleNodeInfo {
    uint32_t nodeID = 0;
    std::string name;
    std::string pluginName;
    bool isEnabled = true;
    bool isBypassed = false;
    int numInputChannels = 0;
    int numOutputChannels = 0;
};

/**
 * 引擎统计信息
 */
struct EngineStatistics {
    double cpuUsage = 0.0;
    double memoryUsage = 0.0;
    double inputLevel = 0.0;
    double outputLevel = 0.0;
    double latency = 0.0;
    int dropouts = 0;
    int activeNodes = 0;
    int totalConnections = 0;
};

/**
 * 参数信息
 */
struct ParameterInfo {
    std::string name;
    std::string label;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float currentValue = 0.0f;
    bool isDiscrete = false;
    int numSteps = 0;
    std::string units;
};

/**
 * 回调函数类型定义
 */
using EngineStateCallback = std::function<void(EngineState state, const std::string& message)>;
using PluginLoadCallback = std::function<void(uint32_t nodeID, bool success, const std::string& error)>;
using ErrorCallback = std::function<void(const std::string& error)>;

/**
 * WindsynthEngineFacade - 新音频架构的高层门面类
 * 
 * 这个类封装了所有JUCESupport/AudioGraph组件的复杂性，
 * 提供简单、高层级的API供Swift层调用。
 * 
 * 设计原则：
 * - 完全独立于VSTSupport架构
 * - 提供面向任务的高层级API
 * - 内部管理所有复杂的组件协调
 * - 线程安全的操作
 * - 简化的错误处理
 */
class WindsynthEngineFacade {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     */
    WindsynthEngineFacade();
    
    /**
     * 析构函数
     */
    ~WindsynthEngineFacade();
    
    //==============================================================================
    // 引擎生命周期管理
    //==============================================================================
    
    /**
     * 初始化引擎
     * @param config 引擎配置
     * @return 成功返回true
     */
    bool initialize(const EngineConfig& config);
    
    /**
     * 启动音频处理
     * @return 成功返回true
     */
    bool start();
    
    /**
     * 停止音频处理
     */
    void stop();
    
    /**
     * 释放所有资源
     */
    void shutdown();
    
    /**
     * 获取当前引擎状态
     */
    EngineState getState() const;
    
    /**
     * 检查引擎是否正在运行
     */
    bool isRunning() const;
    
    //==============================================================================
    // 音频文件处理
    //==============================================================================
    
    /**
     * 加载音频文件
     * @param filePath 文件路径
     * @return 成功返回true
     */
    bool loadAudioFile(const std::string& filePath);
    
    /**
     * 开始播放
     * @return 成功返回true
     */
    bool play();
    
    /**
     * 暂停播放
     */
    void pause();
    
    /**
     * 停止播放
     */
    void stopPlayback();
    
    /**
     * 跳转到指定时间
     * @param timeInSeconds 时间（秒）
     * @return 成功返回true
     */
    bool seekTo(double timeInSeconds);
    
    /**
     * 获取当前播放时间
     * @return 当前时间（秒）
     */
    double getCurrentTime() const;
    
    /**
     * 获取音频文件总时长
     * @return 总时长（秒）
     */
    double getDuration() const;
    
    //==============================================================================
    // 插件管理
    //==============================================================================
    
    // 注意：插件扫描现在通过ModernPluginLoader异步进行，无需手动调用
    
    /**
     * 获取可用插件列表
     * @return 插件信息列表
     */
    std::vector<SimplePluginInfo> getAvailablePlugins() const;

    /**
     * 获取插件加载器引用（用于Bridge访问）
     * @return ModernPluginLoader引用
     */
    AudioGraph::ModernPluginLoader& getPluginLoader() { return *pluginLoader; }
    
    /**
     * 异步加载插件
     * @param pluginIdentifier 插件标识符
     * @param displayName 显示名称（可选）
     * @param callback 加载完成回调
     */
    void loadPluginAsync(const std::string& pluginIdentifier,
                        const std::string& displayName = "",
                        PluginLoadCallback callback = nullptr);
    
    /**
     * 移除插件节点
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool removeNode(uint32_t nodeID);
    
    /**
     * 获取已加载的节点列表
     * @return 节点信息列表
     */
    std::vector<SimpleNodeInfo> getLoadedNodes() const;
    
    /**
     * 设置节点旁路状态
     * @param nodeID 节点ID
     * @param bypassed 是否旁路
     * @return 成功返回true
     */
    bool setNodeBypassed(uint32_t nodeID, bool bypassed);
    
    /**
     * 设置节点启用状态
     * @param nodeID 节点ID
     * @param enabled 是否启用
     * @return 成功返回true
     */
    bool setNodeEnabled(uint32_t nodeID, bool enabled);
    
    //==============================================================================
    // 参数控制
    //==============================================================================
    
    /**
     * 设置节点参数
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @param value 参数值（0.0-1.0）
     * @return 成功返回true
     */
    bool setNodeParameter(uint32_t nodeID, int parameterIndex, float value);
    
    /**
     * 获取节点参数
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数值（0.0-1.0），失败返回-1.0
     */
    float getNodeParameter(uint32_t nodeID, int parameterIndex) const;
    
    /**
     * 获取节点参数数量
     * @param nodeID 节点ID
     * @return 参数数量
     */
    int getNodeParameterCount(uint32_t nodeID) const;

    /**
     * 获取节点参数信息
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数信息，失败返回空的optional
     */
    std::optional<ParameterInfo> getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const;

    //==============================================================================
    // 插件编辑器管理
    //==============================================================================

    /**
     * 检查节点是否有编辑器
     * @param nodeID 节点ID
     * @return 有编辑器返回true
     */
    bool nodeHasEditor(uint32_t nodeID) const;

    /**
     * 显示节点编辑器
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool showNodeEditor(uint32_t nodeID);

    /**
     * 隐藏节点编辑器
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool hideNodeEditor(uint32_t nodeID);

    /**
     * 检查节点编辑器是否可见
     * @param nodeID 节点ID
     * @return 可见返回true
     */
    bool isNodeEditorVisible(uint32_t nodeID) const;

    //==============================================================================
    // 节点位置管理
    //==============================================================================

    /**
     * 移动节点在处理链中的位置
     * @param nodeID 要移动的节点ID
     * @param newPosition 新位置索引
     * @return 成功返回true
     */
    bool moveNode(uint32_t nodeID, int newPosition);

    /**
     * 交换两个节点的位置
     * @param nodeID1 第一个节点ID
     * @param nodeID2 第二个节点ID
     * @return 成功返回true
     */
    bool swapNodes(uint32_t nodeID1, uint32_t nodeID2);
    
    //==============================================================================
    // 音频路由管理
    //==============================================================================
    
    /**
     * 创建串联处理链
     * @param nodeIDs 节点ID列表（按处理顺序）
     * @return 成功创建的连接数量
     */
    int createProcessingChain(const std::vector<uint32_t>& nodeIDs);
    
    /**
     * 自动连接节点到音频输入输出
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool autoConnectToIO(uint32_t nodeID);
    
    /**
     * 断开节点的所有连接
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool disconnectNode(uint32_t nodeID);
    
    //==============================================================================
    // 状态和监控
    //==============================================================================
    
    /**
     * 获取引擎统计信息
     * @return 统计信息
     */
    EngineStatistics getStatistics() const;
    
    /**
     * 获取输出电平
     * @return 输出电平（dB）
     */
    double getOutputLevel() const;
    
    /**
     * 获取输入电平
     * @return 输入电平（dB）
     */
    double getInputLevel() const;
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置状态变化回调
     */
    void setStateCallback(EngineStateCallback callback);
    
    /**
     * 设置错误回调
     */
    void setErrorCallback(ErrorCallback callback);
    
    //==============================================================================
    // 配置管理
    //==============================================================================
    
    /**
     * 获取当前配置
     */
    const EngineConfig& getConfiguration() const;
    
    /**
     * 更新配置
     * @param config 新配置
     * @return 成功返回true
     */
    bool updateConfiguration(const EngineConfig& config);

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    // 核心组件（智能指针管理）
    std::unique_ptr<AudioGraph::GraphAudioProcessor> graphProcessor;
    std::unique_ptr<AudioGraph::GraphManager> graphManager;
    std::unique_ptr<AudioGraph::PluginManager> pluginManager;
    std::unique_ptr<AudioGraph::ModernPluginLoader> pluginLoader;
    std::unique_ptr<AudioGraph::AudioIOManager> ioManager;
    std::unique_ptr<AudioGraph::PresetManager> presetManager;
    
    // 状态管理
    std::atomic<EngineState> currentState{EngineState::Stopped};
    EngineConfig currentConfig;
    mutable std::mutex configMutex;
    
    // 回调函数
    EngineStateCallback stateCallback;
    ErrorCallback errorCallback;
    
    // 音频文件相关
    std::unique_ptr<juce::AudioFormatManager> formatManager;
    std::unique_ptr<juce::AudioTransportSource> transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void initializeComponents();
    void setupCallbacks();
    void notifyStateChange(EngineState newState, const std::string& message = "");
    void notifyError(const std::string& error);
    AudioGraph::NodeID convertToNodeID(uint32_t nodeID) const;
    uint32_t convertFromNodeID(AudioGraph::NodeID nodeID) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindsynthEngineFacade)
};

} // namespace WindsynthVST::Engine
