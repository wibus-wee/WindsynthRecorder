//
//  AudioGraphBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新的音频图桥接层 - 直接替换 AudioProcessingChain
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// 类型定义
//==============================================================================

// 不透明句柄类型
typedef void* AudioGraphHandle;
typedef void* PluginLoaderHandle;
typedef void* PluginManagerHandle;
typedef void* PresetManagerHandle;
typedef uint32_t NodeID;

// 音频图配置
typedef struct {
    double sampleRate;
    int samplesPerBlock;
    int numInputChannels;
    int numOutputChannels;
    bool enableMidi;
} AudioGraphConfig_C;

// 节点信息
typedef struct {
    NodeID nodeID;
    char name[256];
    char displayName[256];
    int numInputChannels;
    int numOutputChannels;
    bool enabled;
    bool bypassed;
} NodeInfo_C;

// 性能统计
typedef struct {
    double averageProcessingTimeMs;
    double peakProcessingTimeMs;
    double cpuUsagePercent;
    uint64_t totalProcessedBlocks;
    int bufferUnderruns;
} PerformanceStats_C;

// 插件描述
typedef struct {
    char name[256];
    char manufacturerName[128];
    char version[64];
    char category[128];
    char fileOrIdentifier[512];
    bool isInstrument;
    int numInputChannels;
    int numOutputChannels;
    uint32_t uniqueId;
} PluginDescription_C;

// 回调函数类型
typedef void (*ErrorCallback_C)(const char* error, void* userData);
typedef void (*StateChangedCallback_C)(void* userData);
typedef void (*PluginLoadedCallback_C)(NodeID nodeID, const char* pluginName, void* userData);

//==============================================================================
// AudioGraph 核心 API
//==============================================================================

/**
 * 创建音频图实例
 */
AudioGraphHandle audioGraph_create(void);

/**
 * 销毁音频图实例
 */
void audioGraph_destroy(AudioGraphHandle handle);

/**
 * 配置音频图
 */
void audioGraph_configure(AudioGraphHandle handle, const AudioGraphConfig_C* config);

/**
 * 获取当前配置
 */
AudioGraphConfig_C audioGraph_getConfig(AudioGraphHandle handle);

/**
 * 准备播放
 */
void audioGraph_prepareToPlay(AudioGraphHandle handle, double sampleRate, int samplesPerBlock);

/**
 * 处理音频块
 */
void audioGraph_processBlock(AudioGraphHandle handle, 
                           float** audioBuffer, 
                           int numChannels, 
                           int numSamples,
                           void* midiData,
                           int midiDataSize);

/**
 * 释放资源
 */
void audioGraph_releaseResources(AudioGraphHandle handle);

/**
 * 重置音频图
 */
void audioGraph_reset(AudioGraphHandle handle);

/**
 * 检查音频图是否准备就绪
 */
bool audioGraph_isReady(AudioGraphHandle handle);

//==============================================================================
// 节点管理 API
//==============================================================================

/**
 * 添加插件节点
 */
NodeID audioGraph_addPlugin(AudioGraphHandle handle, const PluginDescription_C* description, const char* displayName);

/**
 * 移除节点
 */
bool audioGraph_removeNode(AudioGraphHandle handle, NodeID nodeID);

/**
 * 获取所有节点信息
 */
int audioGraph_getAllNodes(AudioGraphHandle handle, NodeInfo_C* nodeInfoArray, int maxNodes);

/**
 * 获取节点信息
 */
bool audioGraph_getNodeInfo(AudioGraphHandle handle, NodeID nodeID, NodeInfo_C* nodeInfo);

/**
 * 设置节点启用状态
 */
bool audioGraph_setNodeEnabled(AudioGraphHandle handle, NodeID nodeID, bool enabled);

/**
 * 设置节点旁路状态
 */
bool audioGraph_setNodeBypassed(AudioGraphHandle handle, NodeID nodeID, bool bypassed);

/**
 * 获取I/O节点ID
 */
NodeID audioGraph_getAudioInputNodeID(AudioGraphHandle handle);
NodeID audioGraph_getAudioOutputNodeID(AudioGraphHandle handle);
NodeID audioGraph_getMidiInputNodeID(AudioGraphHandle handle);
NodeID audioGraph_getMidiOutputNodeID(AudioGraphHandle handle);

//==============================================================================
// 连接管理 API
//==============================================================================

/**
 * 创建音频连接
 */
bool audioGraph_connectAudio(AudioGraphHandle handle, 
                           NodeID sourceNode, int sourceChannel,
                           NodeID destNode, int destChannel);

/**
 * 创建MIDI连接
 */
bool audioGraph_connectMidi(AudioGraphHandle handle, NodeID sourceNode, NodeID destNode);

/**
 * 断开音频连接
 */
bool audioGraph_disconnectAudio(AudioGraphHandle handle,
                               NodeID sourceNode, int sourceChannel,
                               NodeID destNode, int destChannel);

/**
 * 断开MIDI连接
 */
bool audioGraph_disconnectMidi(AudioGraphHandle handle, NodeID sourceNode, NodeID destNode);

/**
 * 断开节点的所有连接
 */
void audioGraph_disconnectNode(AudioGraphHandle handle, NodeID nodeID);

//==============================================================================
// 插件加载器 API
//==============================================================================

/**
 * 创建插件加载器
 */
PluginLoaderHandle pluginLoader_create(void);

/**
 * 销毁插件加载器
 */
void pluginLoader_destroy(PluginLoaderHandle handle);

/**
 * 扫描插件
 */
void pluginLoader_scanPlugins(PluginLoaderHandle handle, const char* searchPaths);

/**
 * 获取已知插件数量
 */
int pluginLoader_getNumKnownPlugins(PluginLoaderHandle handle);

/**
 * 获取插件描述
 */
bool pluginLoader_getPluginDescription(PluginLoaderHandle handle, int index, PluginDescription_C* description);

/**
 * 搜索插件
 */
int pluginLoader_searchPlugins(PluginLoaderHandle handle, const char* searchText, 
                              PluginDescription_C* results, int maxResults);

//==============================================================================
// 性能监控 API
//==============================================================================

/**
 * 获取性能统计
 */
PerformanceStats_C audioGraph_getPerformanceStats(AudioGraphHandle handle);

/**
 * 重置性能统计
 */
void audioGraph_resetPerformanceStats(AudioGraphHandle handle);

//==============================================================================
// 状态管理 API
//==============================================================================

/**
 * 保存状态
 */
int audioGraph_saveState(AudioGraphHandle handle, void** stateData);

/**
 * 加载状态
 */
bool audioGraph_loadState(AudioGraphHandle handle, const void* stateData, int dataSize);

/**
 * 释放状态数据
 */
void audioGraph_freeStateData(void* stateData);

//==============================================================================
// 回调设置 API
//==============================================================================

/**
 * 设置错误回调
 */
void audioGraph_setErrorCallback(AudioGraphHandle handle, ErrorCallback_C callback, void* userData);

/**
 * 设置状态变化回调
 */
void audioGraph_setStateChangedCallback(AudioGraphHandle handle, StateChangedCallback_C callback, void* userData);

// 旧的兼容性API已删除 - 请使用新的AudioGraph API

#ifdef __cplusplus
}
#endif
