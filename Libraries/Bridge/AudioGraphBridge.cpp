//
//  AudioGraphBridge.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新的音频图桥接层实现
//

#include "AudioGraphBridge.h"
#include "../VSTSupport/AudioGraph/Core/GraphAudioProcessor.hpp"
#include "../VSTSupport/AudioGraph/Plugins/ModernPluginLoader.hpp"
#include "../VSTSupport/AudioGraph/Plugins/PluginManager.hpp"
#include "../VSTSupport/AudioGraph/Management/PresetManager.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>

using namespace WindsynthVST::AudioGraph;

// 类型别名解决冲突
using GraphNodeID = WindsynthVST::AudioGraph::NodeID;
using BridgeNodeID = ::NodeID; // C接口中的NodeID

//==============================================================================
// 内部包装类
//==============================================================================

struct AudioGraphWrapper {
    std::unique_ptr<GraphAudioProcessor> processor;
    std::unique_ptr<ModernPluginLoader> pluginLoader;
    std::unique_ptr<PluginManager> pluginManager;
    std::unique_ptr<PresetManager> presetManager;

    // 回调函数
    ErrorCallback_C errorCallback = nullptr;
    void* errorUserData = nullptr;
    StateChangedCallback_C stateChangedCallback = nullptr;
    void* stateUserData = nullptr;

    // 状态控制
    bool enabled = true;
    bool masterBypass = false;
    
    AudioGraphWrapper() {
        processor = std::make_unique<GraphAudioProcessor>();
        pluginLoader = std::make_unique<ModernPluginLoader>();
        pluginManager = std::make_unique<PluginManager>(*processor, *pluginLoader);
        presetManager = std::make_unique<PresetManager>(*processor, *pluginManager);
        
        // 设置回调
        processor->setErrorCallback([this](const std::string& error) {
            if (errorCallback) {
                errorCallback(error.c_str(), errorUserData);
            }
        });
        
        processor->setStateCallback([this](const std::string& message) {
            if (stateChangedCallback) {
                stateChangedCallback(stateUserData);
            }
        });
    }
};

//==============================================================================
// 辅助函数
//==============================================================================

static AudioGraphWrapper* getWrapper(AudioGraphHandle handle) {
    return static_cast<AudioGraphWrapper*>(handle);
}

static void copyString(char* dest, const std::string& src, size_t maxLen) {
    size_t len = std::min(src.length(), maxLen - 1);
    std::memcpy(dest, src.c_str(), len);
    dest[len] = '\0';
}

static GraphConfig convertConfig(const AudioGraphConfig_C& config) {
    GraphConfig result;
    result.sampleRate = config.sampleRate;
    result.samplesPerBlock = config.samplesPerBlock;
    result.numInputChannels = config.numInputChannels;
    result.numOutputChannels = config.numOutputChannels;
    result.enableMidi = config.enableMidi;
    return result;
}

static AudioGraphConfig_C convertConfig(const GraphConfig& config) {
    AudioGraphConfig_C result;
    result.sampleRate = config.sampleRate;
    result.samplesPerBlock = config.samplesPerBlock;
    result.numInputChannels = config.numInputChannels;
    result.numOutputChannels = config.numOutputChannels;
    result.enableMidi = config.enableMidi;
    return result;
}

//==============================================================================
// AudioGraph 核心 API 实现
//==============================================================================

AudioGraphHandle audioGraph_create(void) {
    try {
        return new AudioGraphWrapper();
    } catch (const std::exception& e) {
        std::cout << "[AudioGraphBridge] 创建音频图失败: " << e.what() << std::endl;
        return nullptr;
    }
}

void audioGraph_destroy(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        delete wrapper;
    }
}

void audioGraph_configure(AudioGraphHandle handle, const AudioGraphConfig_C* config) {
    if (auto* wrapper = getWrapper(handle); wrapper && config) {
        GraphConfig graphConfig = convertConfig(*config);
        wrapper->processor->configure(graphConfig);
    }
}

AudioGraphConfig_C audioGraph_getConfig(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return convertConfig(wrapper->processor->getConfig());
    }
    return {};
}

void audioGraph_prepareToPlay(AudioGraphHandle handle, double sampleRate, int samplesPerBlock) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->processor->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void audioGraph_processBlock(AudioGraphHandle handle, 
                           float** audioBuffer, 
                           int numChannels, 
                           int numSamples,
                           void* midiData,
                           int midiDataSize) {
    if (auto* wrapper = getWrapper(handle); wrapper && audioBuffer) {
        // 检查启用状态和旁路
        if (!wrapper->enabled || wrapper->masterBypass) {
            return;
        }
        
        // 创建JUCE音频缓冲区
        juce::AudioBuffer<float> buffer(audioBuffer, numChannels, numSamples);
        
        // 创建MIDI缓冲区（简化处理）
        juce::MidiBuffer midiBuffer;
        if (midiData && midiDataSize > 0) {
            // 这里可以添加MIDI数据解析逻辑
        }
        
        // 处理音频
        wrapper->processor->processBlock(buffer, midiBuffer);
    }
}

void audioGraph_releaseResources(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->processor->releaseResources();
    }
}

void audioGraph_reset(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->processor->reset();
    }
}

bool audioGraph_isReady(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return wrapper->processor->isGraphReady();
    }
    return false;
}

//==============================================================================
// 节点管理 API 实现
//==============================================================================

BridgeNodeID audioGraph_addPlugin(AudioGraphHandle handle, const PluginDescription_C* description, const char* displayName) {
    if (auto* wrapper = getWrapper(handle); wrapper && description) {
        // 将C结构转换为JUCE PluginDescription
        juce::PluginDescription juceDesc;
        juceDesc.name = description->name;
        juceDesc.manufacturerName = description->manufacturerName;
        juceDesc.version = description->version;
        juceDesc.category = description->category;
        juceDesc.fileOrIdentifier = description->fileOrIdentifier;
        juceDesc.isInstrument = description->isInstrument;
        juceDesc.numInputChannels = description->numInputChannels;
        juceDesc.numOutputChannels = description->numOutputChannels;
        juceDesc.uniqueId = static_cast<int>(description->uniqueId);

        // 异步加载插件
        GraphNodeID nodeID{0};
        wrapper->pluginManager->loadPluginAsync(juceDesc, displayName ? displayName : "",
            [&nodeID](GraphNodeID id, const std::string& error) {
                if (error.empty()) {
                    nodeID = id;
                }
            });

        // 插件加载完成

        return nodeID.uid;
    }
    return 0;
}

bool audioGraph_removeNode(AudioGraphHandle handle, BridgeNodeID nodeID) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID id{nodeID};
        return wrapper->processor->removeNode(id);
    }
    return false;
}

int audioGraph_getAllNodes(AudioGraphHandle handle, NodeInfo_C* nodeInfoArray, int maxNodes) {
    if (auto* wrapper = getWrapper(handle); wrapper && nodeInfoArray) {
        auto nodes = wrapper->processor->getAllNodes();
        int count = std::min(static_cast<int>(nodes.size()), maxNodes);
        
        for (int i = 0; i < count; ++i) {
            const auto& node = nodes[i];
            NodeInfo_C& info = nodeInfoArray[i];
            
            info.nodeID = node.nodeID.uid;
            copyString(info.name, node.name, sizeof(info.name));
            copyString(info.displayName, node.pluginName, sizeof(info.displayName));
            info.numInputChannels = node.numInputChannels;
            info.numOutputChannels = node.numOutputChannels;
            info.enabled = node.enabled;
            info.bypassed = node.bypassed;
        }
        
        return count;
    }
    return 0;
}

bool audioGraph_getNodeInfo(AudioGraphHandle handle, BridgeNodeID nodeID, NodeInfo_C* nodeInfo) {
    if (auto* wrapper = getWrapper(handle); wrapper && nodeInfo) {
        GraphNodeID id{nodeID};
        auto info = wrapper->processor->getNodeInfo(id);

        if (info.nodeID.uid != 0) {
            nodeInfo->nodeID = info.nodeID.uid;
            copyString(nodeInfo->name, info.name, sizeof(nodeInfo->name));
            copyString(nodeInfo->displayName, info.pluginName, sizeof(nodeInfo->displayName));
            nodeInfo->numInputChannels = info.numInputChannels;
            nodeInfo->numOutputChannels = info.numOutputChannels;
            nodeInfo->enabled = info.enabled;
            nodeInfo->bypassed = info.bypassed;
            return true;
        }
    }
    return false;
}

bool audioGraph_setNodeEnabled(AudioGraphHandle handle, BridgeNodeID nodeID, bool enabled) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID id{nodeID};
        return wrapper->processor->setNodeEnabled(id, enabled);
    }
    return false;
}

bool audioGraph_setNodeBypassed(AudioGraphHandle handle, BridgeNodeID nodeID, bool bypassed) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID id{nodeID};
        return wrapper->processor->setNodeBypassed(id, bypassed);
    }
    return false;
}

BridgeNodeID audioGraph_getAudioInputNodeID(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return wrapper->processor->getAudioInputNodeID().uid;
    }
    return 0;
}

BridgeNodeID audioGraph_getAudioOutputNodeID(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return wrapper->processor->getAudioOutputNodeID().uid;
    }
    return 0;
}

BridgeNodeID audioGraph_getMidiInputNodeID(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return wrapper->processor->getMidiInputNodeID().uid;
    }
    return 0;
}

BridgeNodeID audioGraph_getMidiOutputNodeID(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        return wrapper->processor->getMidiOutputNodeID().uid;
    }
    return 0;
}

//==============================================================================
// 连接管理 API 实现
//==============================================================================

bool audioGraph_connectAudio(AudioGraphHandle handle,
                           BridgeNodeID sourceNode, int sourceChannel,
                           BridgeNodeID destNode, int destChannel) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID src{sourceNode}, dest{destNode};
        return wrapper->processor->connectAudio(src, sourceChannel, dest, destChannel);
    }
    return false;
}

bool audioGraph_connectMidi(AudioGraphHandle handle, BridgeNodeID sourceNode, BridgeNodeID destNode) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID src{sourceNode}, dest{destNode};
        return wrapper->processor->connectMidi(src, dest);
    }
    return false;
}

bool audioGraph_disconnectAudio(AudioGraphHandle handle,
                               BridgeNodeID sourceNode, int sourceChannel,
                               BridgeNodeID destNode, int destChannel) {
    if (auto* wrapper = getWrapper(handle)) {
        // 创建连接对象并断开
        auto connection = juce::AudioProcessorGraph::Connection{
            {GraphNodeID{sourceNode}, sourceChannel},
            {GraphNodeID{destNode}, destChannel}
        };
        return wrapper->processor->disconnect(connection);
    }
    return false;
}

bool audioGraph_disconnectMidi(AudioGraphHandle handle, BridgeNodeID sourceNode, BridgeNodeID destNode) {
    if (auto* wrapper = getWrapper(handle)) {
        // 创建MIDI连接对象并断开
        auto connection = juce::AudioProcessorGraph::Connection{
            {GraphNodeID{sourceNode}, juce::AudioProcessorGraph::midiChannelIndex},
            {GraphNodeID{destNode}, juce::AudioProcessorGraph::midiChannelIndex}
        };
        return wrapper->processor->disconnect(connection);
    }
    return false;
}

void audioGraph_disconnectNode(AudioGraphHandle handle, BridgeNodeID nodeID) {
    if (auto* wrapper = getWrapper(handle)) {
        GraphNodeID id{nodeID};
        wrapper->processor->disconnectNode(id);
    }
}

//==============================================================================
// 性能监控 API 实现
//==============================================================================

PerformanceStats_C audioGraph_getPerformanceStats(AudioGraphHandle handle) {
    PerformanceStats_C stats = {};

    if (auto* wrapper = getWrapper(handle)) {
        auto perfStats = wrapper->processor->getPerformanceStats();
        stats.averageProcessingTimeMs = perfStats.averageProcessingTimeMs;
        stats.peakProcessingTimeMs = perfStats.maxProcessingTimeMs;
        stats.cpuUsagePercent = perfStats.cpuUsagePercent;
        stats.totalProcessedBlocks = static_cast<uint64_t>(perfStats.totalProcessedBlocks);
        stats.bufferUnderruns = 0; // 这个字段在新架构中暂时不支持
    }

    return stats;
}

void audioGraph_resetPerformanceStats(AudioGraphHandle handle) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->processor->resetPerformanceStats();
    }
}

//==============================================================================
// 状态管理 API 实现
//==============================================================================

int audioGraph_saveState(AudioGraphHandle handle, void** stateData) {
    if (auto* wrapper = getWrapper(handle); wrapper && stateData) {
        juce::MemoryBlock block;
        wrapper->processor->getStateInformation(block);

        if (block.getSize() > 0) {
            *stateData = std::malloc(block.getSize());
            if (*stateData) {
                std::memcpy(*stateData, block.getData(), block.getSize());
                return static_cast<int>(block.getSize());
            }
        }
    }

    *stateData = nullptr;
    return 0;
}

bool audioGraph_loadState(AudioGraphHandle handle, const void* stateData, int dataSize) {
    if (auto* wrapper = getWrapper(handle); wrapper && stateData && dataSize > 0) {
        wrapper->processor->setStateInformation(stateData, dataSize);
        return true;
    }
    return false;
}

void audioGraph_freeStateData(void* stateData) {
    if (stateData) {
        std::free(stateData);
    }
}

//==============================================================================
// 回调设置 API 实现
//==============================================================================

void audioGraph_setErrorCallback(AudioGraphHandle handle, ErrorCallback_C callback, void* userData) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->errorCallback = callback;
        wrapper->errorUserData = userData;
    }
}

void audioGraph_setStateChangedCallback(AudioGraphHandle handle, StateChangedCallback_C callback, void* userData) {
    if (auto* wrapper = getWrapper(handle)) {
        wrapper->stateChangedCallback = callback;
        wrapper->stateUserData = userData;
    }
}

// 旧的兼容性API已删除 - 请使用新的AudioGraph API

//==============================================================================
// 插件加载器 API 实现
//==============================================================================

PluginLoaderHandle pluginLoader_create(void) {
    try {
        return new ModernPluginLoader();
    } catch (const std::exception& e) {
        std::cout << "[AudioGraphBridge] 创建插件加载器失败: " << e.what() << std::endl;
        return nullptr;
    }
}

void pluginLoader_destroy(PluginLoaderHandle handle) {
    if (auto* loader = static_cast<ModernPluginLoader*>(handle)) {
        delete loader;
    }
}

void pluginLoader_scanPlugins(PluginLoaderHandle handle, const char* searchPaths) {
    if (auto* loader = static_cast<ModernPluginLoader*>(handle); loader && searchPaths) {
        // 解析搜索路径并开始扫描
        juce::FileSearchPath paths(searchPaths);
        loader->scanPluginsAsync(paths, true, false);
    }
}

int pluginLoader_getNumKnownPlugins(PluginLoaderHandle handle) {
    if (auto* loader = static_cast<ModernPluginLoader*>(handle)) {
        return loader->getNumKnownPlugins();
    }
    return 0;
}

bool pluginLoader_getPluginDescription(PluginLoaderHandle handle, int index, PluginDescription_C* description) {
    if (auto* loader = static_cast<ModernPluginLoader*>(handle); loader && description) {
        auto plugins = loader->getKnownPlugins();
        if (index >= 0 && index < plugins.size()) {
            const auto& plugin = plugins[index];

            copyString(description->name, plugin.name.toStdString(), sizeof(description->name));
            copyString(description->manufacturerName, plugin.manufacturerName.toStdString(), sizeof(description->manufacturerName));
            copyString(description->version, plugin.version.toStdString(), sizeof(description->version));
            copyString(description->category, plugin.category.toStdString(), sizeof(description->category));
            copyString(description->fileOrIdentifier, plugin.fileOrIdentifier.toStdString(), sizeof(description->fileOrIdentifier));
            description->isInstrument = plugin.isInstrument;
            description->numInputChannels = plugin.numInputChannels;
            description->numOutputChannels = plugin.numOutputChannels;
            description->uniqueId = plugin.uniqueId;

            return true;
        }
    }
    return false;
}

int pluginLoader_searchPlugins(PluginLoaderHandle handle, const char* searchText,
                              PluginDescription_C* results, int maxResults) {
    if (auto* loader = static_cast<ModernPluginLoader*>(handle); loader && searchText && results) {
        auto foundPlugins = loader->searchPlugins(searchText, true, true, true);
        int count = std::min(static_cast<int>(foundPlugins.size()), maxResults);

        for (int i = 0; i < count; ++i) {
            const auto& plugin = foundPlugins[i];
            PluginDescription_C& desc = results[i];

            copyString(desc.name, plugin.name.toStdString(), sizeof(desc.name));
            copyString(desc.manufacturerName, plugin.manufacturerName.toStdString(), sizeof(desc.manufacturerName));
            copyString(desc.version, plugin.version.toStdString(), sizeof(desc.version));
            copyString(desc.category, plugin.category.toStdString(), sizeof(desc.category));
            copyString(desc.fileOrIdentifier, plugin.fileOrIdentifier.toStdString(), sizeof(desc.fileOrIdentifier));
            desc.isInstrument = plugin.isInstrument;
            desc.numInputChannels = plugin.numInputChannels;
            desc.numOutputChannels = plugin.numOutputChannels;
            desc.uniqueId = plugin.uniqueId;
        }

        return count;
    }
    return 0;
}
