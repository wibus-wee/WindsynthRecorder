//
//  AudioGraphBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新音频架构的C桥接层实现
//

#include "AudioGraphBridge.h"
#include "WindsynthEngineFacade.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>

using namespace WindsynthVST::Engine;

//==============================================================================
// 内部辅助结构
//==============================================================================

/**
 * 回调数据结构
 */
struct CallbackData {
    EngineStateCallback_C stateCallback = nullptr;
    void* stateUserData = nullptr;
    ErrorCallback_C errorCallback = nullptr;
    void* errorUserData = nullptr;
};

/**
 * 引擎包装器
 */
struct EngineWrapper {
    std::unique_ptr<WindsynthEngineFacade> engine;
    CallbackData callbacks;
    std::vector<SimplePluginInfo> availablePlugins;
    std::vector<SimpleNodeInfo> loadedNodes;
    
    EngineWrapper() : engine(std::make_unique<WindsynthEngineFacade>()) {}
};

//==============================================================================
// 辅助函数
//==============================================================================

/**
 * 转换引擎状态
 */
EngineState_C convertEngineState(EngineState state) {
    switch (state) {
        case EngineState::Stopped: return EngineState_Stopped;
        case EngineState::Starting: return EngineState_Starting;
        case EngineState::Running: return EngineState_Running;
        case EngineState::Stopping: return EngineState_Stopping;
        case EngineState::Error: return EngineState_Error;
        default: return EngineState_Error;
    }
}

/**
 * 转换引擎配置
 */
EngineConfig convertEngineConfig(const EngineConfig_C* config) {
    EngineConfig result;
    result.sampleRate = config->sampleRate;
    result.bufferSize = config->bufferSize;
    result.numInputChannels = config->numInputChannels;
    result.numOutputChannels = config->numOutputChannels;
    result.enableRealtimeProcessing = config->enableRealtimeProcessing;
    result.audioDeviceName = std::string(config->audioDeviceName);
    return result;
}

/**
 * 转换插件信息到C结构
 */
void convertPluginInfo(const SimplePluginInfo& source, SimplePluginInfo_C* dest) {
    strncpy(dest->identifier, source.identifier.c_str(), sizeof(dest->identifier) - 1);
    dest->identifier[sizeof(dest->identifier) - 1] = '\0';
    
    strncpy(dest->name, source.name.c_str(), sizeof(dest->name) - 1);
    dest->name[sizeof(dest->name) - 1] = '\0';
    
    strncpy(dest->manufacturer, source.manufacturer.c_str(), sizeof(dest->manufacturer) - 1);
    dest->manufacturer[sizeof(dest->manufacturer) - 1] = '\0';
    
    strncpy(dest->category, source.category.c_str(), sizeof(dest->category) - 1);
    dest->category[sizeof(dest->category) - 1] = '\0';
    
    strncpy(dest->format, source.format.c_str(), sizeof(dest->format) - 1);
    dest->format[sizeof(dest->format) - 1] = '\0';
    
    strncpy(dest->filePath, source.filePath.c_str(), sizeof(dest->filePath) - 1);
    dest->filePath[sizeof(dest->filePath) - 1] = '\0';
    
    dest->isValid = source.isValid;
}

/**
 * 转换节点信息到C结构
 */
void convertNodeInfo(const SimpleNodeInfo& source, SimpleNodeInfo_C* dest) {
    dest->nodeID = source.nodeID;
    
    strncpy(dest->name, source.name.c_str(), sizeof(dest->name) - 1);
    dest->name[sizeof(dest->name) - 1] = '\0';
    
    strncpy(dest->pluginName, source.pluginName.c_str(), sizeof(dest->pluginName) - 1);
    dest->pluginName[sizeof(dest->pluginName) - 1] = '\0';
    
    dest->isEnabled = source.isEnabled;
    dest->isBypassed = source.isBypassed;
    dest->numInputChannels = source.numInputChannels;
    dest->numOutputChannels = source.numOutputChannels;
}

/**
 * 验证引擎句柄
 */
EngineWrapper* getEngineWrapper(WindsynthEngineHandle handle) {
    if (!handle) {
        std::cerr << "[AudioGraphBridge] 无效的引擎句柄" << std::endl;
        return nullptr;
    }
    return static_cast<EngineWrapper*>(handle);
}

//==============================================================================
// 引擎生命周期管理
//==============================================================================

WindsynthEngineHandle Engine_Create(void) {
    std::cout << "[AudioGraphBridge] 创建引擎实例" << std::endl;
    
    try {
        auto wrapper = new EngineWrapper();
        
        // 设置回调
        wrapper->engine->setStateCallback([wrapper](EngineState state, const std::string& message) {
            if (wrapper->callbacks.stateCallback) {
                wrapper->callbacks.stateCallback(convertEngineState(state), message.c_str(), wrapper->callbacks.stateUserData);
            }
        });
        
        wrapper->engine->setErrorCallback([wrapper](const std::string& error) {
            if (wrapper->callbacks.errorCallback) {
                wrapper->callbacks.errorCallback(error.c_str(), wrapper->callbacks.errorUserData);
            }
        });
        
        std::cout << "[AudioGraphBridge] 引擎实例创建成功" << std::endl;
        return wrapper;
        
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 创建引擎失败: " << e.what() << std::endl;
        return nullptr;
    }
}

void Engine_Destroy(WindsynthEngineHandle handle) {
    std::cout << "[AudioGraphBridge] 销毁引擎实例" << std::endl;
    
    auto wrapper = getEngineWrapper(handle);
    if (wrapper) {
        delete wrapper;
    }
}

bool Engine_Initialize(WindsynthEngineHandle handle, const EngineConfig_C* config) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !config) {
        return false;
    }
    
    try {
        EngineConfig engineConfig = convertEngineConfig(config);
        return wrapper->engine->initialize(engineConfig);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 初始化引擎失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_Start(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }
    
    try {
        return wrapper->engine->start();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 启动引擎失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_Stop(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }
    
    try {
        wrapper->engine->stop();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 停止引擎失败: " << e.what() << std::endl;
    }
}

void Engine_Shutdown(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }
    
    try {
        wrapper->engine->shutdown();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 关闭引擎失败: " << e.what() << std::endl;
    }
}

EngineState_C Engine_GetState(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return EngineState_Error;
    }
    
    try {
        return convertEngineState(wrapper->engine->getState());
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取引擎状态失败: " << e.what() << std::endl;
        return EngineState_Error;
    }
}

bool Engine_IsRunning(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }
    
    try {
        return wrapper->engine->isRunning();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 检查引擎运行状态失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 音频文件处理
//==============================================================================

bool Engine_LoadAudioFile(WindsynthEngineHandle handle, const char* filePath) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !filePath) {
        return false;
    }
    
    try {
        return wrapper->engine->loadAudioFile(std::string(filePath));
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 加载音频文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_Play(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }
    
    try {
        return wrapper->engine->play();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 播放失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_Pause(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }
    
    try {
        wrapper->engine->pause();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 暂停失败: " << e.what() << std::endl;
    }
}

void Engine_StopPlayback(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }
    
    try {
        wrapper->engine->stopPlayback();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 停止播放失败: " << e.what() << std::endl;
    }
}

bool Engine_SeekTo(WindsynthEngineHandle handle, double timeInSeconds) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }
    
    try {
        return wrapper->engine->seekTo(timeInSeconds);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 跳转失败: " << e.what() << std::endl;
        return false;
    }
}

double Engine_GetCurrentTime(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0.0;
    }
    
    try {
        return wrapper->engine->getCurrentTime();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取当前时间失败: " << e.what() << std::endl;
        return 0.0;
    }
}

double Engine_GetDuration(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0.0;
    }
    
    try {
        return wrapper->engine->getDuration();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取时长失败: " << e.what() << std::endl;
        return 0.0;
    }
}

//==============================================================================
// 插件管理
//==============================================================================

int Engine_ScanPlugins(WindsynthEngineHandle handle, const char* const* searchPaths) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0;
    }

    try {
        std::vector<std::string> paths;

        if (searchPaths) {
            for (int i = 0; searchPaths[i] != nullptr; ++i) {
                paths.push_back(std::string(searchPaths[i]));
            }
        }

        int result = wrapper->engine->scanPlugins(paths);

        // 更新可用插件列表
        wrapper->availablePlugins = wrapper->engine->getAvailablePlugins();

        return result;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 扫描插件失败: " << e.what() << std::endl;
        return 0;
    }
}

int Engine_GetAvailablePluginCount(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0;
    }

    return static_cast<int>(wrapper->availablePlugins.size());
}

bool Engine_GetAvailablePluginInfo(WindsynthEngineHandle handle, int index, SimplePluginInfo_C* pluginInfo) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !pluginInfo || index < 0 || index >= static_cast<int>(wrapper->availablePlugins.size())) {
        return false;
    }

    try {
        convertPluginInfo(wrapper->availablePlugins[index], pluginInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取插件信息失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_LoadPluginByIdentifier(WindsynthEngineHandle handle,
                                  const char* pluginIdentifier,
                                  const char* displayName,
                                  PluginLoadCallback_C callback,
                                  void* userData) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !pluginIdentifier) {
        if (callback) {
            callback(0, false, "无效的参数", userData);
        }
        return;
    }

    try {
        std::string identifier(pluginIdentifier);
        std::string name = displayName ? std::string(displayName) : "";

        wrapper->engine->loadPluginAsync(identifier, name,
            [callback, userData, wrapper](uint32_t nodeID, bool success, const std::string& error) {
                // 更新已加载节点列表
                if (success) {
                    wrapper->loadedNodes = wrapper->engine->getLoadedNodes();
                }

                if (callback) {
                    callback(nodeID, success, error.c_str(), userData);
                }
            });
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 加载插件失败: " << e.what() << std::endl;
        if (callback) {
            callback(0, false, e.what(), userData);
        }
    }
}

bool Engine_RemoveNode(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        bool result = wrapper->engine->removeNode(nodeID);

        if (result) {
            // 更新已加载节点列表
            wrapper->loadedNodes = wrapper->engine->getLoadedNodes();
        }

        return result;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 移除节点失败: " << e.what() << std::endl;
        return false;
    }
}

int Engine_GetLoadedNodeCount(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0;
    }

    return static_cast<int>(wrapper->loadedNodes.size());
}

bool Engine_GetLoadedNodeInfo(WindsynthEngineHandle handle, int index, SimpleNodeInfo_C* nodeInfo) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !nodeInfo || index < 0 || index >= static_cast<int>(wrapper->loadedNodes.size())) {
        return false;
    }

    try {
        convertNodeInfo(wrapper->loadedNodes[index], nodeInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取节点信息失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SetNodeBypassed(WindsynthEngineHandle handle, uint32_t nodeID, bool bypassed) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->setNodeBypassed(nodeID, bypassed);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 设置节点旁路状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SetNodeEnabled(WindsynthEngineHandle handle, uint32_t nodeID, bool enabled) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->setNodeEnabled(nodeID, enabled);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 设置节点启用状态失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 参数控制
//==============================================================================

bool Engine_SetNodeParameter(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex, float value) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->setNodeParameter(nodeID, parameterIndex, value);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 设置节点参数失败: " << e.what() << std::endl;
        return false;
    }
}

float Engine_GetNodeParameter(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return -1.0f;
    }

    try {
        return wrapper->engine->getNodeParameter(nodeID, parameterIndex);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取节点参数失败: " << e.what() << std::endl;
        return -1.0f;
    }
}

int Engine_GetNodeParameterCount(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0;
    }

    try {
        return wrapper->engine->getNodeParameterCount(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取节点参数数量失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_GetNodeParameterInfo(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex, ParameterInfo_C* info) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !info) {
        return false;
    }

    try {
        auto paramInfo = wrapper->engine->getNodeParameterInfo(nodeID, parameterIndex);
        if (!paramInfo.has_value()) {
            return false;
        }

        const auto& param = paramInfo.value();

        // 安全地复制字符串
        strncpy(info->name, param.name.c_str(), sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';

        strncpy(info->label, param.label.c_str(), sizeof(info->label) - 1);
        info->label[sizeof(info->label) - 1] = '\0';

        strncpy(info->units, param.units.c_str(), sizeof(info->units) - 1);
        info->units[sizeof(info->units) - 1] = '\0';

        info->minValue = param.minValue;
        info->maxValue = param.maxValue;
        info->defaultValue = param.defaultValue;
        info->currentValue = param.currentValue;
        info->isDiscrete = param.isDiscrete;
        info->numSteps = param.numSteps;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取节点参数信息失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_NodeHasEditor(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->nodeHasEditor(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 检查节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_ShowNodeEditor(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->showNodeEditor(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 显示节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_HideNodeEditor(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->hideNodeEditor(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 隐藏节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_IsNodeEditorVisible(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->isNodeEditorVisible(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 检查节点编辑器可见性失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_MoveNode(WindsynthEngineHandle handle, uint32_t nodeID, int newPosition) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->moveNode(nodeID, newPosition);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 移动节点失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_SwapNodes(WindsynthEngineHandle handle, uint32_t nodeID1, uint32_t nodeID2) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->swapNodes(nodeID1, nodeID2);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 交换节点失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 音频路由管理
//==============================================================================

int Engine_CreateProcessingChain(WindsynthEngineHandle handle, const uint32_t* nodeIDs, int nodeCount) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !nodeIDs || nodeCount <= 0) {
        return 0;
    }

    try {
        std::vector<uint32_t> nodes(nodeIDs, nodeIDs + nodeCount);
        return wrapper->engine->createProcessingChain(nodes);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 创建处理链失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_AutoConnectToIO(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->autoConnectToIO(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 自动连接到I/O失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_DisconnectNode(WindsynthEngineHandle handle, uint32_t nodeID) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return false;
    }

    try {
        return wrapper->engine->disconnectNode(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 断开节点连接失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 状态和监控
//==============================================================================

bool Engine_GetStatistics(WindsynthEngineHandle handle, EngineStatistics_C* stats) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !stats) {
        return false;
    }

    try {
        EngineStatistics engineStats = wrapper->engine->getStatistics();

        stats->cpuUsage = engineStats.cpuUsage;
        stats->memoryUsage = engineStats.memoryUsage;
        stats->inputLevel = engineStats.inputLevel;
        stats->outputLevel = engineStats.outputLevel;
        stats->latency = engineStats.latency;
        stats->dropouts = engineStats.dropouts;
        stats->activeNodes = engineStats.activeNodes;
        stats->totalConnections = engineStats.totalConnections;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取统计信息失败: " << e.what() << std::endl;
        return false;
    }
}

double Engine_GetOutputLevel(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0.0;
    }

    try {
        return wrapper->engine->getOutputLevel();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取输出电平失败: " << e.what() << std::endl;
        return 0.0;
    }
}

double Engine_GetInputLevel(WindsynthEngineHandle handle) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return 0.0;
    }

    try {
        return wrapper->engine->getInputLevel();
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取输入电平失败: " << e.what() << std::endl;
        return 0.0;
    }
}

//==============================================================================
// 回调设置
//==============================================================================

void Engine_SetStateCallback(WindsynthEngineHandle handle, EngineStateCallback_C callback, void* userData) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }

    wrapper->callbacks.stateCallback = callback;
    wrapper->callbacks.stateUserData = userData;
}

void Engine_SetErrorCallback(WindsynthEngineHandle handle, ErrorCallback_C callback, void* userData) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper) {
        return;
    }

    wrapper->callbacks.errorCallback = callback;
    wrapper->callbacks.errorUserData = userData;
}

//==============================================================================
// 配置管理
//==============================================================================

bool Engine_GetConfiguration(WindsynthEngineHandle handle, EngineConfig_C* config) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !config) {
        return false;
    }

    try {
        const EngineConfig& engineConfig = wrapper->engine->getConfiguration();

        config->sampleRate = engineConfig.sampleRate;
        config->bufferSize = engineConfig.bufferSize;
        config->numInputChannels = engineConfig.numInputChannels;
        config->numOutputChannels = engineConfig.numOutputChannels;
        config->enableRealtimeProcessing = engineConfig.enableRealtimeProcessing;

        strncpy(config->audioDeviceName, engineConfig.audioDeviceName.c_str(), sizeof(config->audioDeviceName) - 1);
        config->audioDeviceName[sizeof(config->audioDeviceName) - 1] = '\0';

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 获取配置失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_UpdateConfiguration(WindsynthEngineHandle handle, const EngineConfig_C* config) {
    auto wrapper = getEngineWrapper(handle);
    if (!wrapper || !config) {
        return false;
    }

    try {
        EngineConfig engineConfig = convertEngineConfig(config);
        return wrapper->engine->updateConfiguration(engineConfig);
    } catch (const std::exception& e) {
        std::cerr << "[AudioGraphBridge] 更新配置失败: " << e.what() << std::endl;
        return false;
    }
}
