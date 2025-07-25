//
//  PluginBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理桥接层实现
//

#include "PluginBridge.h"
#include "BridgeInternal.h"
#include <iostream>
#include <memory>

// 使用命名空间别名简化代码
using NodeID = WindsynthVST::AudioGraph::NodeID;

//==============================================================================
// 插件管理实现
//==============================================================================

/**
 * 转换插件信息
 */
static void convertPluginInfo(const Interfaces::SimplePluginInfo& cppInfo, PluginInfo_C* cInfo) {
    strncpy(cInfo->identifier, cppInfo.identifier.c_str(), sizeof(cInfo->identifier) - 1);
    cInfo->identifier[sizeof(cInfo->identifier) - 1] = '\0';

    strncpy(cInfo->name, cppInfo.name.c_str(), sizeof(cInfo->name) - 1);
    cInfo->name[sizeof(cInfo->name) - 1] = '\0';

    strncpy(cInfo->manufacturer, cppInfo.manufacturer.c_str(), sizeof(cInfo->manufacturer) - 1);
    cInfo->manufacturer[sizeof(cInfo->manufacturer) - 1] = '\0';

    strncpy(cInfo->category, cppInfo.category.c_str(), sizeof(cInfo->category) - 1);
    cInfo->category[sizeof(cInfo->category) - 1] = '\0';

    strncpy(cInfo->format, cppInfo.format.c_str(), sizeof(cInfo->format) - 1);
    cInfo->format[sizeof(cInfo->format) - 1] = '\0';

    strncpy(cInfo->filePath, cppInfo.filePath.c_str(), sizeof(cInfo->filePath) - 1);
    cInfo->filePath[sizeof(cInfo->filePath) - 1] = '\0';

    cInfo->isValid = cppInfo.isValid;
}

/**
 * 转换节点信息
 */
static void convertNodeInfo(const Interfaces::SimpleNodeInfo& cppInfo, NodeInfo_C* cInfo) {
    cInfo->nodeID = cppInfo.nodeID;

    strncpy(cInfo->name, cppInfo.name.c_str(), sizeof(cInfo->name) - 1);
    cInfo->name[sizeof(cInfo->name) - 1] = '\0';

    strncpy(cInfo->pluginName, cppInfo.pluginName.c_str(), sizeof(cInfo->pluginName) - 1);
    cInfo->pluginName[sizeof(cInfo->pluginName) - 1] = '\0';

    cInfo->isEnabled = cppInfo.isEnabled;
    cInfo->isBypassed = cppInfo.isBypassed;
    cInfo->numInputChannels = cppInfo.numInputChannels;
    cInfo->numOutputChannels = cppInfo.numOutputChannels;
}

int Engine_GetAvailablePluginCount(EngineHandle handle) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto plugins = context->engine->getAvailablePlugins();
        return static_cast<int>(plugins.size());
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取插件数量失败: " << e.what() << std::endl;
        return 0;
    }
}

int Engine_GetAvailablePlugins(EngineHandle handle,
                                        PluginInfo_C* plugins,
                                        int maxCount) {
    if (!handle || !plugins || maxCount <= 0) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto cppPlugins = context->engine->getAvailablePlugins();
        int count = std::min(static_cast<int>(cppPlugins.size()), maxCount);

        for (int i = 0; i < count; ++i) {
            convertPluginInfo(cppPlugins[i], &plugins[i]);
        }

        return count;
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取插件列表失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_GetAvailablePluginInfo(EngineHandle handle,
                                  int index,
                                  SimplePluginInfo_C* pluginInfo) {
    if (!handle || !pluginInfo || index < 0) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto cppPlugins = context->engine->getAvailablePlugins();
        if (index >= static_cast<int>(cppPlugins.size())) {
            return false;
        }

        convertPluginInfo(cppPlugins[index], pluginInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取插件信息失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * 插件加载回调包装器
 */
struct PluginLoadCallbackWrapper {
    PluginLoadCallback callback;
    void* userData;
};

void Engine_LoadPluginAsync(EngineHandle handle,
                                     const char* pluginIdentifier,
                                     const char* displayName,
                                     PluginLoadCallback callback,
                                     void* userData) {
    if (!handle || !pluginIdentifier) return;

    try {
        auto context = getContext(handle);
        if (!context->engine) return;

        std::string identifier(pluginIdentifier);
        std::string name = displayName ? std::string(displayName) : "";

        // 创建回调包装器
        auto wrapper = std::make_shared<PluginLoadCallbackWrapper>();
        wrapper->callback = callback;
        wrapper->userData = userData;

        context->engine->loadPluginAsync(identifier, name,
            [wrapper](uint32_t nodeID, bool success, const std::string& error) {
                if (wrapper->callback) {
                    wrapper->callback(nodeID, success, error.c_str(), wrapper->userData);
                }
            });
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 异步加载插件失败: " << e.what() << std::endl;
        if (callback) {
            callback(0, false, e.what(), userData);
        }
    }
}

bool Engine_RemoveNode(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->removeNode(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 移除节点失败: " << e.what() << std::endl;
        return false;
    }
}

int Engine_GetLoadedNodeCount(EngineHandle handle) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto nodes = context->engine->getLoadedNodes();
        return static_cast<int>(nodes.size());
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取节点数量失败: " << e.what() << std::endl;
        return 0;
    }
}

int Engine_GetLoadedNodes(EngineHandle handle,
                                   NodeInfo_C* nodes,
                                   int maxCount) {
    if (!handle || !nodes || maxCount <= 0) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto cppNodes = context->engine->getLoadedNodes();
        int count = std::min(static_cast<int>(cppNodes.size()), maxCount);

        for (int i = 0; i < count; ++i) {
            convertNodeInfo(cppNodes[i], &nodes[i]);
        }

        return count;
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取节点列表失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_GetLoadedNodeInfo(EngineHandle handle,
                             int index,
                             SimpleNodeInfo_C* nodeInfo) {
    if (!handle || !nodeInfo || index < 0) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto cppNodes = context->engine->getLoadedNodes();
        if (index >= static_cast<int>(cppNodes.size())) {
            return false;
        }

        convertNodeInfo(cppNodes[index], nodeInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 获取节点信息失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SetNodeBypassed(EngineHandle handle, uint32_t nodeID, bool bypassed) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeBypassed(nodeID, bypassed);
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 设置节点旁路状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SetNodeEnabled(EngineHandle handle, uint32_t nodeID, bool enabled) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeEnabled(nodeID, enabled);
    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 设置节点启用状态失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 插件扫描实现
//==============================================================================

void Engine_ScanPluginsAsync(EngineHandle handle,
                            bool rescanExisting,
                            PluginScanProgressCallback progressCallback,
                            PluginScanCompletionCallback completionCallback,
                            void* userData) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (!context->engine) {
            if (completionCallback) {
                completionCallback(0, userData);
            }
            return;
        }

        // 通过引擎上下文获取插件加载器
        auto engineContext = context->engine->getContext();
        if (!engineContext) {
            if (completionCallback) {
                completionCallback(0, userData);
            }
            return;
        }

        auto pluginLoader = engineContext->getPluginLoader();
        if (!pluginLoader) {
            if (completionCallback) {
                completionCallback(0, userData);
            }
            return;
        }

        // 设置进度回调
        if (progressCallback) {
            pluginLoader->setScanProgressCallback([progressCallback, userData](float progress, const juce::String& currentFile) {
                progressCallback(progress, currentFile.toRawUTF8(), userData);
            });
        }

        // 设置完成回调
        if (completionCallback) {
            pluginLoader->setScanCompleteCallback([completionCallback, userData](int foundPlugins) {
                completionCallback(foundPlugins, userData);
            });
        }

        // 开始异步扫描
        pluginLoader->scanDefaultPathsAsync(rescanExisting);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 异步扫描插件失败: " << e.what() << std::endl;
        if (completionCallback) {
            completionCallback(0, userData);
        }
    }
}

void Engine_StopPluginScan(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (!context->engine) return;

        // 通过引擎上下文获取插件加载器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return;

        auto pluginLoader = engineContext->getPluginLoader();
        if (!pluginLoader) return;

        // 停止扫描
        pluginLoader->stopScanning();

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 停止插件扫描失败: " << e.what() << std::endl;
    }
}

bool Engine_IsScanning(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取插件加载器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto pluginLoader = engineContext->getPluginLoader();
        if (!pluginLoader) return false;

        // 检查是否正在扫描
        return pluginLoader->isScanning();

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 检查扫描状态失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_LoadPluginByIdentifier(EngineHandle handle,
                                  const char* pluginIdentifier,
                                  PluginLoadCallback callback,
                                  void* userData) {
    if (!handle || !pluginIdentifier) {
        if (callback) {
            callback(0, false, "无效的句柄或插件标识符", userData);
        }
        return;
    }

    try {
        auto context = getContext(handle);
        if (!context->engine) {
            if (callback) {
                callback(0, false, "引擎未初始化", userData);
            }
            return;
        }

        // 使用真正的引擎方法加载插件，不使用自定义名称
        std::string identifier(pluginIdentifier);

        context->engine->loadPluginAsync(identifier, "",
            [callback, userData](uint32_t nodeID, bool success, const std::string& error) {
                if (callback) {
                    callback(nodeID, success, error.c_str(), userData);
                }
            });

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 通过标识符加载插件失败: " << e.what() << std::endl;
        if (callback) {
            callback(0, false, e.what(), userData);
        }
    }
}

//==============================================================================
// 节点编辑器实现
//==============================================================================

bool Engine_NodeHasEditor(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取插件管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto pluginManager = engineContext->getPluginManager();
        if (!pluginManager) return false;

        // 获取插件实例
        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        return instance && instance->hasEditor();

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 检查节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_ShowNodeEditor(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取插件管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto pluginManager = engineContext->getPluginManager();
        if (!pluginManager) return false;

        // 显示编辑器
        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        return pluginManager->showEditor(graphNodeID);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 显示节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_HideNodeEditor(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取插件管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto pluginManager = engineContext->getPluginManager();
        if (!pluginManager) return false;

        // 隐藏编辑器
        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        return pluginManager->hideEditor(graphNodeID);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 隐藏节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_IsNodeEditorVisible(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取插件管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto pluginManager = engineContext->getPluginManager();
        if (!pluginManager) return false;

        // 检查编辑器可见性
        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        return pluginManager->isEditorVisible(graphNodeID);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 检查节点编辑器可见性失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_MoveNode(EngineHandle handle, uint32_t nodeID, int newPosition) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取图管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto graphManager = engineContext->getGraphManager();
        if (!graphManager) return false;

        // 移动节点
        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        return graphManager->moveNode(graphNodeID, newPosition);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 移动节点失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SwapNodes(EngineHandle handle, uint32_t nodeID1, uint32_t nodeID2) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取图管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto graphManager = engineContext->getGraphManager();
        if (!graphManager) return false;

        // 交换节点（通过重新组织实现）
        NodeID graphNodeID1, graphNodeID2;
        graphNodeID1.uid = nodeID1;
        graphNodeID2.uid = nodeID2;

        // 获取当前所有节点，然后交换这两个节点的位置
        std::vector<NodeID> nodeIDs = {graphNodeID1, graphNodeID2};
        return graphManager->reorganizeNodes(nodeIDs, "swap");

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 交换节点失败: " << e.what() << std::endl;
        return false;
    }
}

int Engine_CreateProcessingChain(EngineHandle handle, const uint32_t* nodeIDs, int count) {
    if (!handle || !nodeIDs || count <= 0) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        // 通过引擎上下文获取图管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return 0;

        auto graphManager = engineContext->getGraphManager();
        if (!graphManager) return 0;

        // 转换节点ID
        std::vector<NodeID> graphNodeIDs;
        graphNodeIDs.reserve(count);

        for (int i = 0; i < count; ++i) {
            NodeID graphNodeID;
            graphNodeID.uid = nodeIDs[i];
            graphNodeIDs.push_back(graphNodeID);
        }

        // 创建处理链
        return graphManager->createProcessingChain(graphNodeIDs, true);

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 创建处理链失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_AutoConnectToIO(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取图管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto graphManager = engineContext->getGraphManager();
        auto graphProcessor = engineContext->getGraphProcessor();
        if (!graphManager || !graphProcessor) return false;

        // 获取音频输入输出节点ID
        NodeID audioInputID = graphProcessor->getAudioInputNodeID();
        NodeID audioOutputID = graphProcessor->getAudioOutputNodeID();

        NodeID graphNodeID;
        graphNodeID.uid = nodeID;

        // 自动连接到输入和输出
        int inputConnections = graphManager->autoConnectNodes(audioInputID, graphNodeID, true, false);
        int outputConnections = graphManager->autoConnectNodes(graphNodeID, audioOutputID, true, false);

        return (inputConnections + outputConnections) > 0;

    } catch (const std::exception& e) {
        std::cerr << "[PluginBridge] 自动连接到IO失败: " << e.what() << std::endl;
        return false;
    }
}
