//
//  AudioGraphTypes.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图架构的核心类型定义
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 类型别名，基于JUCE AudioProcessorGraph
//==============================================================================

using NodeID = juce::AudioProcessorGraph::NodeID;
using Node = juce::AudioProcessorGraph::Node;
using Connection = juce::AudioProcessorGraph::Connection;
using NodeAndChannel = juce::AudioProcessorGraph::NodeAndChannel;

//==============================================================================
// 图配置结构
//==============================================================================

/**
 * 音频图的配置参数
 */
struct GraphConfig {
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    int numInputChannels = 2;
    int numOutputChannels = 2;
    bool enableMidi = true;
    bool enableLatencyCompensation = true;
    
    bool operator==(const GraphConfig& other) const {
        return sampleRate == other.sampleRate &&
               samplesPerBlock == other.samplesPerBlock &&
               numInputChannels == other.numInputChannels &&
               numOutputChannels == other.numOutputChannels &&
               enableMidi == other.enableMidi &&
               enableLatencyCompensation == other.enableLatencyCompensation;
    }
    
    bool operator!=(const GraphConfig& other) const {
        return !(*this == other);
    }
};

//==============================================================================
// 节点信息结构
//==============================================================================

/**
 * 音频图中节点的信息
 */
struct NodeInfo {
    NodeID nodeID;
    std::string name;
    std::string pluginName;
    bool enabled = true;
    bool bypassed = false;
    int numInputChannels = 0;
    int numOutputChannels = 0;
    bool acceptsMidi = false;
    bool producesMidi = false;
    double latencyInSamples = 0.0;
    
    NodeInfo() = default;
    
    NodeInfo(NodeID id, const std::string& nodeName)
        : nodeID(id), name(nodeName) {}
};

//==============================================================================
// 连接信息结构
//==============================================================================

/**
 * 音频图中连接的详细信息
 */
struct ConnectionInfo {
    Connection connection;
    std::string sourceName;
    std::string destinationName;
    bool isAudioConnection = true; // true for audio, false for MIDI
    
    ConnectionInfo() = default;
    
    ConnectionInfo(const Connection& conn, const std::string& src, const std::string& dest, bool isAudio = true)
        : connection(conn), sourceName(src), destinationName(dest), isAudioConnection(isAudio) {}
};

//==============================================================================
// 性能统计结构
//==============================================================================

/**
 * 音频图的性能统计信息
 */
struct GraphPerformanceStats {
    double averageProcessingTimeMs = 0.0;
    double maxProcessingTimeMs = 0.0;
    double minProcessingTimeMs = 0.0;
    int totalProcessedBlocks = 0;
    double cpuUsagePercent = 0.0;
    size_t memoryUsageBytes = 0;
    
    void reset() {
        averageProcessingTimeMs = 0.0;
        maxProcessingTimeMs = 0.0;
        minProcessingTimeMs = 0.0;
        totalProcessedBlocks = 0;
        cpuUsagePercent = 0.0;
        memoryUsageBytes = 0;
    }
};

//==============================================================================
// 回调函数类型
//==============================================================================

/**
 * 图状态变化回调
 */
using GraphStateCallback = std::function<void(const std::string& message)>;

/**
 * 错误处理回调
 */
using GraphErrorCallback = std::function<void(const std::string& error)>;

/**
 * 性能监控回调
 */
using PerformanceCallback = std::function<void(const GraphPerformanceStats& stats)>;

//==============================================================================
// 枚举类型
//==============================================================================

/**
 * 图更新模式
 */
enum class UpdateMode {
    Synchronous,    // 同步更新
    Asynchronous    // 异步更新
};

/**
 * 节点类型
 */
enum class NodeType {
    VSTPlugin,      // VST插件
    AudioInput,     // 音频输入
    AudioOutput,    // 音频输出
    MidiInput,      // MIDI输入
    MidiOutput,     // MIDI输出
    Unknown         // 未知类型
};

/**
 * 连接类型
 */
enum class ConnectionType {
    Audio,          // 音频连接
    Midi            // MIDI连接
};

//==============================================================================
// 常量定义
//==============================================================================

namespace Constants {
    static constexpr int MIDI_CHANNEL_INDEX = juce::AudioProcessorGraph::midiChannelIndex;
    static constexpr int MAX_AUDIO_CHANNELS = 32;
    static constexpr int DEFAULT_BUFFER_SIZE = 512;
    static constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    static constexpr int PERFORMANCE_STATS_HISTORY_SIZE = 100;
}

//==============================================================================
// 实用工具函数
//==============================================================================

/**
 * 检查连接是否为MIDI连接
 */
inline bool isMidiConnection(const Connection& connection) {
    return connection.source.channelIndex == Constants::MIDI_CHANNEL_INDEX ||
           connection.destination.channelIndex == Constants::MIDI_CHANNEL_INDEX;
}

/**
 * 检查连接是否为音频连接
 */
inline bool isAudioConnection(const Connection& connection) {
    return !isMidiConnection(connection);
}

/**
 * 创建音频连接
 */
inline Connection makeAudioConnection(NodeID sourceNode, int sourceChannel,
                                    NodeID destNode, int destChannel) {
    return Connection(NodeAndChannel{sourceNode, sourceChannel},
                     NodeAndChannel{destNode, destChannel});
}

/**
 * 创建MIDI连接
 */
inline Connection makeMidiConnection(NodeID sourceNode, NodeID destNode) {
    return Connection(NodeAndChannel{sourceNode, Constants::MIDI_CHANNEL_INDEX},
                     NodeAndChannel{destNode, Constants::MIDI_CHANNEL_INDEX});
}

} // namespace WindsynthVST::AudioGraph

//==============================================================================
// 为NodeID提供std::hash特化，使其可以用作unordered_map的键
//==============================================================================

namespace std {
    template<>
    struct hash<juce::AudioProcessorGraph::NodeID> {
        std::size_t operator()(const juce::AudioProcessorGraph::NodeID& nodeID) const noexcept {
            return std::hash<juce::uint32>{}(nodeID.uid);
        }
    };
}
