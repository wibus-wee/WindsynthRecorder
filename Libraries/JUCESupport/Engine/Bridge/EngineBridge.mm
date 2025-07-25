//
//  EngineBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎生命周期管理桥接层实现
//

#include "BridgeInternal.h"
#include <string>
#include <iostream>

//==============================================================================
// 辅助函数
//==============================================================================

/**
 * 转换引擎状态
 */
static EngineState_C convertEngineState(Core::EngineState state) {
    switch (state) {
        case Core::EngineState::Stopped: return EngineState_Stopped;
        case Core::EngineState::Starting: return EngineState_Starting;
        case Core::EngineState::Running: return EngineState_Running;
        case Core::EngineState::Stopping: return EngineState_Stopping;
        case Core::EngineState::Error: return EngineState_Error;
        default: return EngineState_Error;
    }
}

/**
 * 转换引擎配置
 */
static Core::EngineConfig convertEngineConfig(const EngineConfig_C* config) {
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
static void convertEngineConfigToC(const Core::EngineConfig& cppConfig, EngineConfig_C* config) {
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
BridgeContext* getContext(EngineHandle handle) {
    return static_cast<BridgeContext*>(handle);
}

//==============================================================================
// 核心引擎生命周期管理实现
//==============================================================================

EngineHandle Engine_Create(void) {
    try {
        auto context = new BridgeContext();
        std::cout << "[EngineBridge] 引擎实例创建成功" << std::endl;
        return static_cast<EngineHandle>(context);
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 创建引擎失败: " << e.what() << std::endl;
        return nullptr;
    }
}

void Engine_Destroy(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->shutdown();
        }
        delete context;
        std::cout << "[EngineBridge] 引擎实例销毁完成" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 销毁引擎时出错: " << e.what() << std::endl;
    }
}

bool Engine_Initialize(EngineHandle handle, const EngineConfig_C* config) {
    if (!handle || !config) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto cppConfig = convertEngineConfig(config);
        return context->engine->initialize(cppConfig);
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 初始化引擎失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_Start(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->start();
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 启动引擎失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_Stop(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->stop();
        }
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 停止引擎时出错: " << e.what() << std::endl;
    }
}

void Engine_Shutdown(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->shutdown();
        }
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 关闭引擎时出错: " << e.what() << std::endl;
    }
}

EngineState_C Engine_GetState(EngineHandle handle) {
    if (!handle) return EngineState_Error;

    try {
        auto context = getContext(handle);
        if (!context->engine) return EngineState_Error;

        return convertEngineState(context->engine->getState());
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 获取引擎状态失败: " << e.what() << std::endl;
        return EngineState_Error;
    }
}

bool Engine_IsRunning(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->isRunning();
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 检查运行状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_GetConfiguration(EngineHandle handle, EngineConfig_C* config) {
    if (!handle || !config) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        const auto& cppConfig = context->engine->getConfiguration();
        convertEngineConfigToC(cppConfig, config);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 获取配置失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_UpdateConfiguration(EngineHandle handle, const EngineConfig_C* config) {
    if (!handle || !config) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto cppConfig = convertEngineConfig(config);
        return context->engine->updateConfiguration(cppConfig);
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 更新配置失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 回调设置实现
//==============================================================================

void Engine_SetStateCallback(EngineHandle handle,
                            EngineStateCallback callback,
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
        std::cerr << "[EngineBridge] 设置状态回调失败: " << e.what() << std::endl;
    }
}

void Engine_SetErrorCallback(EngineHandle handle,
                            EngineErrorCallback callback,
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
        std::cerr << "[EngineBridge] 设置错误回调失败: " << e.what() << std::endl;
    }
}

//==============================================================================
// 统计信息实现
//==============================================================================

bool Engine_GetStatistics(EngineHandle handle, EngineStatistics_C* statistics) {
    if (!handle || !statistics) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        // 通过引擎上下文获取各种管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return false;

        auto graphProcessor = engineContext->getGraphProcessor();
        auto ioManager = engineContext->getIOManager();
        auto pluginManager = engineContext->getPluginManager();

        if (!graphProcessor) return false;

        // 获取性能统计
        auto perfStats = graphProcessor->getPerformanceStats();
        statistics->cpuUsage = perfStats.cpuUsagePercent;
        statistics->memoryUsage = static_cast<double>(perfStats.memoryUsageBytes) / (1024.0 * 1024.0); // 转换为MB

        // 获取音频电平
        if (ioManager) {
            auto levels = ioManager->getCurrentLevels();

            // 计算输入电平平均值
            if (!levels.inputLevels.empty()) {
                float totalInput = 0.0f;
                for (float level : levels.inputLevels) {
                    totalInput += level;
                }
                float avgInput = totalInput / levels.inputLevels.size();
                statistics->inputLevel = avgInput > 0.0f ? 20.0 * std::log10(avgInput) : -96.0;
            } else {
                statistics->inputLevel = -96.0;
            }

            // 计算输出电平平均值
            if (!levels.outputLevels.empty()) {
                float totalOutput = 0.0f;
                for (float level : levels.outputLevels) {
                    totalOutput += level;
                }
                float avgOutput = totalOutput / levels.outputLevels.size();
                statistics->outputLevel = avgOutput > 0.0f ? 20.0 * std::log10(avgOutput) : -96.0;
            } else {
                statistics->outputLevel = -96.0;
            }
        } else {
            statistics->inputLevel = -96.0;
            statistics->outputLevel = -96.0;
        }

        // 获取延迟信息
        statistics->latency = perfStats.averageProcessingTimeMs;

        // 获取节点统计
        if (pluginManager) {
            auto allNodes = graphProcessor->getAllNodes();
            statistics->activeNodes = static_cast<int>(allNodes.size());

            // 计算连接数（简化实现）
            statistics->totalConnections = static_cast<int>(allNodes.size() > 1 ? allNodes.size() - 1 : 0);
        } else {
            statistics->activeNodes = 0;
            statistics->totalConnections = 0;
        }

        // 暂时设置为0，需要实际的dropout检测
        statistics->dropouts = 0;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 获取统计信息失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 音频电平和渲染实现
//==============================================================================

double Engine_GetOutputLevel(EngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        // 通过引擎上下文获取IO管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return 0.0;

        auto ioManager = engineContext->getIOManager();
        if (!ioManager) return 0.0;

        // 获取当前音频电平
        auto levels = ioManager->getCurrentLevels();

        // 计算输出电平的平均值（转换为dB）
        if (!levels.outputLevels.empty()) {
            float totalLevel = 0.0f;
            for (float level : levels.outputLevels) {
                totalLevel += level;
            }
            float avgLevel = totalLevel / levels.outputLevels.size();

            // 转换为dB（避免log(0)）
            return avgLevel > 0.0f ? 20.0 * std::log10(avgLevel) : -96.0;
        }

        return -96.0; // 静音电平
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 获取输出电平失败: " << e.what() << std::endl;
        return 0.0;
    }
}

double Engine_GetInputLevel(EngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        // 通过引擎上下文获取IO管理器
        auto engineContext = context->engine->getContext();
        if (!engineContext) return 0.0;

        auto ioManager = engineContext->getIOManager();
        if (!ioManager) return 0.0;

        // 获取当前音频电平
        auto levels = ioManager->getCurrentLevels();

        // 计算输入电平的平均值（转换为dB）
        if (!levels.inputLevels.empty()) {
            float totalLevel = 0.0f;
            for (float level : levels.inputLevels) {
                totalLevel += level;
            }
            float avgLevel = totalLevel / levels.inputLevels.size();

            // 转换为dB（避免log(0)）
            return avgLevel > 0.0f ? 20.0 * std::log10(avgLevel) : -96.0;
        }

        return -96.0; // 静音电平
    } catch (const std::exception& e) {
        std::cerr << "[EngineBridge] 获取输入电平失败: " << e.what() << std::endl;
        return 0.0;
    }
}

bool Engine_RenderToFile(EngineHandle handle,
                        const char* inputPath,
                        const char* outputPath,
                        const RenderSettings_C* settings,
                        RenderProgressCallback progressCallback,
                        void* userData) {
    if (!handle || !inputPath || !outputPath || !settings) {
        std::cerr << "[Engine_RenderToFile] 无效的参数" << std::endl;
        return false;
    }

    try {
        auto context = getContext(handle);
        if (!context || !context->engine) {
            std::cerr << "[Engine_RenderToFile] 无效的引擎上下文" << std::endl;
            return false;
        }

        std::cout << "[Engine_RenderToFile] 开始离线渲染" << std::endl;
        std::cout << "输入文件: " << inputPath << std::endl;
        std::cout << "输出文件: " << outputPath << std::endl;

        // 转换 C 结构到 C++ 结构
        WindsynthEngineFacade::RenderSettings cppSettings;
        cppSettings.sampleRate = settings->sampleRate;
        cppSettings.bitDepth = settings->bitDepth;
        cppSettings.numChannels = settings->numChannels;
        cppSettings.normalizeOutput = settings->normalizeOutput;
        cppSettings.includePluginTails = settings->includePluginTails;

        // 转换格式枚举
        if (settings->format == 0) {
            cppSettings.format = WindsynthEngineFacade::RenderSettings::Format::WAV;
        } else if (settings->format == 1) {
            cppSettings.format = WindsynthEngineFacade::RenderSettings::Format::AIFF;
        } else {
            std::cerr << "[Engine_RenderToFile] 不支持的音频格式: " << settings->format << std::endl;
            return false;
        }

        // 创建进度回调包装器
        WindsynthEngineFacade::RenderProgressCallback cppProgressCallback = nullptr;
        if (progressCallback) {
            cppProgressCallback = [progressCallback, userData](float progress, const std::string& message) {
                progressCallback(progress, message.c_str(), userData);
            };
        }

        // 执行离线渲染
        bool result = context->engine->renderToFile(std::string(inputPath),
                                                   std::string(outputPath),
                                                   cppSettings,
                                                   cppProgressCallback);

        if (result) {
            std::cout << "[Engine_RenderToFile] 离线渲染成功完成" << std::endl;
        } else {
            std::cout << "[Engine_RenderToFile] 离线渲染失败" << std::endl;
        }

        return result;

    } catch (const std::exception& e) {
        std::cerr << "[Engine_RenderToFile] 异常: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[Engine_RenderToFile] 未知异常" << std::endl;
        return false;
    }
}
