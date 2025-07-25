//
//  RefactoredBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  重构后的桥接层实现 - 基于 RefactoredWindsynthEngineFacade
//

#include "RefactoredBridge.h"
#include "../RefactoredWindsynthEngineFacade.hpp"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace WindsynthVST::Engine;

//==============================================================================
// 内部辅助结构
//==============================================================================

/**
 * 桥接层上下文 - 包装 C++ 引擎实例和回调信息
 */
struct RefactoredBridgeContext {
    std::unique_ptr<RefactoredWindsynthEngineFacade> engine;
    
    // 回调信息
    RefactoredEngineStateCallback stateCallback = nullptr;
    void* stateUserData = nullptr;
    
    RefactoredEngineErrorCallback errorCallback = nullptr;
    void* errorUserData = nullptr;
    
    RefactoredBridgeContext() {
        engine = std::make_unique<RefactoredWindsynthEngineFacade>();
    }
};

//==============================================================================
// 辅助函数
//==============================================================================

/**
 * 转换引擎状态
 */
static RefactoredEngineState convertEngineState(Core::EngineState state) {
    switch (state) {
        case Core::EngineState::Stopped: return RefactoredEngineState_Stopped;
        case Core::EngineState::Starting: return RefactoredEngineState_Starting;
        case Core::EngineState::Running: return RefactoredEngineState_Running;
        case Core::EngineState::Stopping: return RefactoredEngineState_Stopping;
        case Core::EngineState::Error: return RefactoredEngineState_Error;
        default: return RefactoredEngineState_Error;
    }
}

/**
 * 转换引擎配置
 */
static Core::EngineConfig convertEngineConfig(const RefactoredEngineConfig* config) {
    Core::EngineConfig cppConfig;
    cppConfig.sampleRate = config->sampleRate;
    cppConfig.bufferSize = config->bufferSize;
    cppConfig.numInputChannels = config->numInputChannels;
    cppConfig.numOutputChannels = config->numOutputChannels;
    cppConfig.enableRealtimeProcessing = config->enableRealtimeProcessing;
    cppConfig.audioDeviceName = std::string(config->audioDeviceName);
    return cppConfig;
}

/**
 * 转换引擎配置（C++ 到 C）
 */
static void convertEngineConfigToC(const Core::EngineConfig& cppConfig, RefactoredEngineConfig* config) {
    config->sampleRate = cppConfig.sampleRate;
    config->bufferSize = cppConfig.bufferSize;
    config->numInputChannels = cppConfig.numInputChannels;
    config->numOutputChannels = cppConfig.numOutputChannels;
    config->enableRealtimeProcessing = cppConfig.enableRealtimeProcessing;
    strncpy(config->audioDeviceName, cppConfig.audioDeviceName.c_str(), sizeof(config->audioDeviceName) - 1);
    config->audioDeviceName[sizeof(config->audioDeviceName) - 1] = '\0';
}

/**
 * 获取桥接层上下文
 */
static RefactoredBridgeContext* getContext(RefactoredEngineHandle handle) {
    return static_cast<RefactoredBridgeContext*>(handle);
}

//==============================================================================
// 核心引擎生命周期管理实现
//==============================================================================

RefactoredEngineHandle RefactoredEngine_Create(void) {
    try {
        auto context = new RefactoredBridgeContext();
        std::cout << "[RefactoredBridge] 引擎实例创建成功" << std::endl;
        return static_cast<RefactoredEngineHandle>(context);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 创建引擎失败: " << e.what() << std::endl;
        return nullptr;
    }
}

void RefactoredEngine_Destroy(RefactoredEngineHandle handle) {
    if (!handle) return;
    
    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->shutdown();
        }
        delete context;
        std::cout << "[RefactoredBridge] 引擎实例销毁完成" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 销毁引擎时出错: " << e.what() << std::endl;
    }
}

bool RefactoredEngine_Initialize(RefactoredEngineHandle handle, const RefactoredEngineConfig* config) {
    if (!handle || !config) return false;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return false;
        
        auto cppConfig = convertEngineConfig(config);
        return context->engine->initialize(cppConfig);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 初始化引擎失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_Start(RefactoredEngineHandle handle) {
    if (!handle) return false;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return false;
        
        return context->engine->start();
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 启动引擎失败: " << e.what() << std::endl;
        return false;
    }
}

void RefactoredEngine_Stop(RefactoredEngineHandle handle) {
    if (!handle) return;
    
    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->stop();
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 停止引擎时出错: " << e.what() << std::endl;
    }
}

void RefactoredEngine_Shutdown(RefactoredEngineHandle handle) {
    if (!handle) return;
    
    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->shutdown();
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 关闭引擎时出错: " << e.what() << std::endl;
    }
}

RefactoredEngineState RefactoredEngine_GetState(RefactoredEngineHandle handle) {
    if (!handle) return RefactoredEngineState_Error;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return RefactoredEngineState_Error;
        
        return convertEngineState(context->engine->getState());
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取引擎状态失败: " << e.what() << std::endl;
        return RefactoredEngineState_Error;
    }
}

bool RefactoredEngine_IsRunning(RefactoredEngineHandle handle) {
    if (!handle) return false;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return false;
        
        return context->engine->isRunning();
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 检查运行状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_GetConfiguration(RefactoredEngineHandle handle, RefactoredEngineConfig* config) {
    if (!handle || !config) return false;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return false;
        
        const auto& cppConfig = context->engine->getConfiguration();
        convertEngineConfigToC(cppConfig, config);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取配置失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_UpdateConfiguration(RefactoredEngineHandle handle, const RefactoredEngineConfig* config) {
    if (!handle || !config) return false;
    
    try {
        auto context = getContext(handle);
        if (!context->engine) return false;
        
        auto cppConfig = convertEngineConfig(config);
        return context->engine->updateConfiguration(cppConfig);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 更新配置失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 回调设置实现
//==============================================================================

void RefactoredEngine_SetStateCallback(RefactoredEngineHandle handle, 
                                      RefactoredEngineStateCallback callback, 
                                      void* userData) {
    if (!handle) return;
    
    try {
        auto context = getContext(handle);
        context->stateCallback = callback;
        context->stateUserData = userData;
        
        if (context->engine) {
            context->engine->setStateCallback([context](Core::EngineState state, const std::string& message) {
                if (context->stateCallback) {
                    context->stateCallback(convertEngineState(state), message.c_str(), context->stateUserData);
                }
            });
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 设置状态回调失败: " << e.what() << std::endl;
    }
}

void RefactoredEngine_SetErrorCallback(RefactoredEngineHandle handle, 
                                      RefactoredEngineErrorCallback callback, 
                                      void* userData) {
    if (!handle) return;
    
    try {
        auto context = getContext(handle);
        context->errorCallback = callback;
        context->errorUserData = userData;
        
        if (context->engine) {
            context->engine->setErrorCallback([context](const std::string& error) {
                if (context->errorCallback) {
                    context->errorCallback(error.c_str(), context->errorUserData);
                }
            });
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 设置错误回调失败: " << e.what() << std::endl;
    }
}

//==============================================================================
// 音频文件处理实现
//==============================================================================

bool RefactoredEngine_LoadAudioFile(RefactoredEngineHandle handle, const char* filePath) {
    if (!handle || !filePath) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->loadAudioFile(std::string(filePath));
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 加载音频文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_Play(RefactoredEngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->play();
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 播放失败: " << e.what() << std::endl;
        return false;
    }
}

void RefactoredEngine_Pause(RefactoredEngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->pause();
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 暂停失败: " << e.what() << std::endl;
    }
}

void RefactoredEngine_StopPlayback(RefactoredEngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->stopPlayback();
        }
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 停止播放失败: " << e.what() << std::endl;
    }
}

bool RefactoredEngine_SeekTo(RefactoredEngineHandle handle, double timeInSeconds) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->seekTo(timeInSeconds);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 跳转失败: " << e.what() << std::endl;
        return false;
    }
}

double RefactoredEngine_GetCurrentTime(RefactoredEngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        return context->engine->getCurrentTime();
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取当前时间失败: " << e.what() << std::endl;
        return 0.0;
    }
}

double RefactoredEngine_GetDuration(RefactoredEngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        return context->engine->getDuration();
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取时长失败: " << e.what() << std::endl;
        return 0.0;
    }
}

bool RefactoredEngine_HasAudioFile(RefactoredEngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto audioManager = context->engine->getAudioFileManager();
        return audioManager ? audioManager->hasAudioFile() : false;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 检查音频文件状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_IsPlaying(RefactoredEngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto audioManager = context->engine->getAudioFileManager();
        return audioManager ? audioManager->isPlaying() : false;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 检查播放状态失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 插件管理实现
//==============================================================================

/**
 * 转换插件信息
 */
static void convertPluginInfo(const Interfaces::SimplePluginInfo& cppInfo, RefactoredPluginInfo* cInfo) {
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
static void convertNodeInfo(const Interfaces::SimpleNodeInfo& cppInfo, RefactoredNodeInfo* cInfo) {
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

int RefactoredEngine_GetAvailablePluginCount(RefactoredEngineHandle handle) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto plugins = context->engine->getAvailablePlugins();
        return static_cast<int>(plugins.size());
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取插件数量失败: " << e.what() << std::endl;
        return 0;
    }
}

int RefactoredEngine_GetAvailablePlugins(RefactoredEngineHandle handle,
                                        RefactoredPluginInfo* plugins,
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
        std::cerr << "[RefactoredBridge] 获取插件列表失败: " << e.what() << std::endl;
        return 0;
    }
}

/**
 * 插件加载回调包装器
 */
struct PluginLoadCallbackWrapper {
    RefactoredPluginLoadCallback callback;
    void* userData;
};

void RefactoredEngine_LoadPluginAsync(RefactoredEngineHandle handle,
                                     const char* pluginIdentifier,
                                     const char* displayName,
                                     RefactoredPluginLoadCallback callback,
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
        std::cerr << "[RefactoredBridge] 异步加载插件失败: " << e.what() << std::endl;
        if (callback) {
            callback(0, false, e.what(), userData);
        }
    }
}

bool RefactoredEngine_RemoveNode(RefactoredEngineHandle handle, uint32_t nodeID) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->removeNode(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 移除节点失败: " << e.what() << std::endl;
        return false;
    }
}

int RefactoredEngine_GetLoadedNodeCount(RefactoredEngineHandle handle) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto nodes = context->engine->getLoadedNodes();
        return static_cast<int>(nodes.size());
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取节点数量失败: " << e.what() << std::endl;
        return 0;
    }
}

int RefactoredEngine_GetLoadedNodes(RefactoredEngineHandle handle,
                                   RefactoredNodeInfo* nodes,
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
        std::cerr << "[RefactoredBridge] 获取节点列表失败: " << e.what() << std::endl;
        return 0;
    }
}

bool RefactoredEngine_SetNodeBypassed(RefactoredEngineHandle handle, uint32_t nodeID, bool bypassed) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeBypassed(nodeID, bypassed);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 设置节点旁路状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_SetNodeEnabled(RefactoredEngineHandle handle, uint32_t nodeID, bool enabled) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeEnabled(nodeID, enabled);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 设置节点启用状态失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 参数控制实现
//==============================================================================

/**
 * 转换参数信息
 */
static void convertParameterInfo(const Interfaces::ParameterInfo& cppInfo, RefactoredParameterInfo* cInfo) {
    strncpy(cInfo->name, cppInfo.name.c_str(), sizeof(cInfo->name) - 1);
    cInfo->name[sizeof(cInfo->name) - 1] = '\0';

    strncpy(cInfo->label, cppInfo.label.c_str(), sizeof(cInfo->label) - 1);
    cInfo->label[sizeof(cInfo->label) - 1] = '\0';

    strncpy(cInfo->units, cppInfo.units.c_str(), sizeof(cInfo->units) - 1);
    cInfo->units[sizeof(cInfo->units) - 1] = '\0';

    cInfo->minValue = cppInfo.minValue;
    cInfo->maxValue = cppInfo.maxValue;
    cInfo->defaultValue = cppInfo.defaultValue;
    cInfo->currentValue = cppInfo.currentValue;
    cInfo->isDiscrete = cppInfo.isDiscrete;
    cInfo->numSteps = cppInfo.numSteps;
}

bool RefactoredEngine_SetNodeParameter(RefactoredEngineHandle handle,
                                      uint32_t nodeID,
                                      int parameterIndex,
                                      float value) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeParameter(nodeID, parameterIndex, value);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 设置节点参数失败: " << e.what() << std::endl;
        return false;
    }
}

float RefactoredEngine_GetNodeParameter(RefactoredEngineHandle handle,
                                       uint32_t nodeID,
                                       int parameterIndex) {
    if (!handle) return -1.0f;

    try {
        auto context = getContext(handle);
        if (!context->engine) return -1.0f;

        return context->engine->getNodeParameter(nodeID, parameterIndex);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取节点参数失败: " << e.what() << std::endl;
        return -1.0f;
    }
}

int RefactoredEngine_GetNodeParameterCount(RefactoredEngineHandle handle, uint32_t nodeID) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        return context->engine->getNodeParameterCount(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取节点参数数量失败: " << e.what() << std::endl;
        return 0;
    }
}

bool RefactoredEngine_GetNodeParameterInfo(RefactoredEngineHandle handle,
                                          uint32_t nodeID,
                                          int parameterIndex,
                                          RefactoredParameterInfo* info) {
    if (!handle || !info) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto cppInfo = context->engine->getNodeParameterInfo(nodeID, parameterIndex);
        if (!cppInfo.has_value()) {
            return false;
        }

        convertParameterInfo(cppInfo.value(), info);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取节点参数信息失败: " << e.what() << std::endl;
        return false;
    }
}

bool RefactoredEngine_ResetNodeParameter(RefactoredEngineHandle handle,
                                        uint32_t nodeID,
                                        int parameterIndex) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto paramController = context->engine->getParameterController();
        if (!paramController) return false;

        return paramController->resetNodeParameter(nodeID, parameterIndex);
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 重置节点参数失败: " << e.what() << std::endl;
        return false;
    }
}

int RefactoredEngine_GetAllParameterInfo(RefactoredEngineHandle handle,
                                        uint32_t nodeID,
                                        RefactoredParameterInfo* parameters,
                                        int maxCount) {
    if (!handle || !parameters || maxCount <= 0) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        auto paramController = context->engine->getParameterController();
        if (!paramController) return 0;

        auto cppParams = paramController->getAllParameterInfo(nodeID);
        int count = std::min(static_cast<int>(cppParams.size()), maxCount);

        for (int i = 0; i < count; ++i) {
            convertParameterInfo(cppParams[i], &parameters[i]);
        }

        return count;
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredBridge] 获取所有参数信息失败: " << e.what() << std::endl;
        return 0;
    }
}
