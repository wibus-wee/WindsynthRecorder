//
//  GraphAudioProcessor.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  基于JUCE AudioProcessorGraph的高性能音频处理器
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include "AudioGraphTypes.hpp"

namespace WindsynthVST::AudioGraph {

// 前向声明
class GraphManager;
class AudioIOManager;
class PresetManager;

/**
 * 基于JUCE AudioProcessorGraph的主要音频处理器
 * 
 * 这个类是新音频架构的核心，替代原有的AudioProcessingChain。
 * 主要优势：
 * - 消除mono→stereo转换的性能瓶颈
 * - 智能音频路由和连接管理
 * - 内置并行处理能力
 * - 高效的内存管理
 */
class GraphAudioProcessor : public juce::AudioProcessor, public juce::AudioIODeviceCallback {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     */
    GraphAudioProcessor();
    
    /**
     * 析构函数
     */
    ~GraphAudioProcessor() override;
    
    //==============================================================================
    // AudioProcessor 接口实现
    //==============================================================================
    
    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;
    bool supportsDoublePrecisionProcessing() const override;
    
    void reset() override;
    void setNonRealtime(bool isNonRealtime) noexcept override;

    //==============================================================================
    // AudioIODeviceCallback 接口实现
    //==============================================================================

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    //==============================================================================
    // 音频文件播放支持
    //==============================================================================

    /**
     * 设置音频传输源（用于音频文件播放）
     */
    void setTransportSource(juce::AudioTransportSource* source);
    
    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;
    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // 图配置和管理
    //==============================================================================
    
    /**
     * 配置音频图
     */
    void configure(const GraphConfig& config);
    
    /**
     * 获取当前配置
     */
    const GraphConfig& getConfig() const { return currentConfig; }
    
    /**
     * 获取内部的AudioProcessorGraph实例
     */
    juce::AudioProcessorGraph& getGraph() { return audioGraph; }
    const juce::AudioProcessorGraph& getGraph() const { return audioGraph; }
    
    //==============================================================================
    // 节点管理（简化接口）
    //==============================================================================
    
    /**
     * 添加插件节点
     */
    NodeID addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin,
                    const std::string& name = "");
    
    /**
     * 移除节点
     */
    bool removeNode(NodeID nodeID);
    
    /**
     * 获取所有节点信息
     */
    std::vector<NodeInfo> getAllNodes() const;
    
    /**
     * 获取特定节点信息
     */
    NodeInfo getNodeInfo(NodeID nodeID) const;
    
    /**
     * 设置节点旁路状态
     */
    bool setNodeBypassed(NodeID nodeID, bool bypassed);
    
    /**
     * 设置节点启用状态
     */
    bool setNodeEnabled(NodeID nodeID, bool enabled);
    
    //==============================================================================
    // 连接管理（简化接口）
    //==============================================================================
    
    /**
     * 连接两个节点的音频通道
     */
    bool connectAudio(NodeID sourceNode, int sourceChannel,
                     NodeID destNode, int destChannel);
    
    /**
     * 连接两个节点的MIDI
     */
    bool connectMidi(NodeID sourceNode, NodeID destNode);
    
    /**
     * 断开连接
     */
    bool disconnect(const Connection& connection);
    
    /**
     * 断开节点的所有连接
     */
    bool disconnectNode(NodeID nodeID);
    
    /**
     * 获取所有连接信息
     */
    std::vector<ConnectionInfo> getAllConnections() const;
    
    //==============================================================================
    // 音频I/O管理
    //==============================================================================
    
    /**
     * 获取音频输入节点ID
     */
    NodeID getAudioInputNodeID() const { return audioInputNodeID; }
    
    /**
     * 获取音频输出节点ID
     */
    NodeID getAudioOutputNodeID() const { return audioOutputNodeID; }
    
    /**
     * 获取MIDI输入节点ID
     */
    NodeID getMidiInputNodeID() const { return midiInputNodeID; }
    
    /**
     * 获取MIDI输出节点ID
     */
    NodeID getMidiOutputNodeID() const { return midiOutputNodeID; }
    
    //==============================================================================
    // 性能监控
    //==============================================================================
    
    /**
     * 获取性能统计信息
     */
    GraphPerformanceStats getPerformanceStats() const;
    
    /**
     * 重置性能统计
     */
    void resetPerformanceStats();
    
    /**
     * 设置性能监控回调
     */
    void setPerformanceCallback(PerformanceCallback callback);
    
    //==============================================================================
    // 错误处理和状态
    //==============================================================================
    
    /**
     * 设置错误回调
     */
    void setErrorCallback(GraphErrorCallback callback);
    
    /**
     * 设置状态变化回调
     */
    void setStateCallback(GraphStateCallback callback);
    
    /**
     * 检查图是否准备就绪
     */
    bool isGraphReady() const { return graphReady.load(); }
    
    /**
     * 获取最后的错误信息
     */
    std::string getLastError() const;

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    // 核心音频图
    juce::AudioProcessorGraph audioGraph;
    
    // 配置信息
    GraphConfig currentConfig;
    
    // I/O节点ID
    NodeID audioInputNodeID;
    NodeID audioOutputNodeID;
    NodeID midiInputNodeID;
    NodeID midiOutputNodeID;
    
    // 状态管理
    std::atomic<bool> graphReady{false};
    std::atomic<bool> isConfigured{false};
    mutable std::mutex configMutex;
    
    // 性能监控
    mutable std::mutex statsMutex;
    GraphPerformanceStats performanceStats;
    std::vector<double> processingTimeHistory;
    juce::Time lastProcessTime;
    
    // 回调函数
    GraphErrorCallback errorCallback;
    GraphStateCallback stateCallback;
    PerformanceCallback performanceCallback;
    
    // 错误信息
    mutable std::mutex errorMutex;
    std::string lastError;

    // 音频文件播放
    juce::AudioTransportSource* transportSource = nullptr;
    juce::AudioBuffer<float> transportBuffer;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    /**
     * 初始化I/O节点
     */
    void initializeIONodes();

    /**
     * 更新I/O节点的父图引用
     */
    void updateIONodesParentGraph();

    /**
     * 创建默认的直通连接（输入到输出）
     */
    void createDefaultPassthroughConnections();

    /**
     * 更新音频图的通道配置
     */
    void updateGraphChannelConfiguration(const GraphConfig& config);
    
    /**
     * 更新性能统计
     */
    void updatePerformanceStats(double processingTimeMs);
    
    /**
     * 处理错误
     */
    void handleError(const std::string& error);
    
    /**
     * 通知状态变化
     */
    void notifyStateChange(const std::string& message);
    
    /**
     * 验证节点ID
     */
    bool isValidNodeID(NodeID nodeID) const;
    
    /**
     * 获取下一个可用的节点ID
     */
    NodeID getNextNodeID();
    
    // 节点ID计数器
    std::atomic<int> nodeIDCounter{1};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphAudioProcessor)
};

} // namespace WindsynthVST::AudioGraph
