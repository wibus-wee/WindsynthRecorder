//
//  RefactoredWindsynthEngineFacade.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  重构后的轻量级引擎门面类实现
//

#include "RefactoredWindsynthEngineFacade.hpp"
#include <iostream>

namespace WindsynthVST::Engine {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

RefactoredWindsynthEngineFacade::RefactoredWindsynthEngineFacade() {
    std::cout << "[RefactoredWindsynthEngineFacade] 构造函数" << std::endl;
    
    // 创建核心组件
    context_ = std::make_shared<Core::EngineContext>();
    notifier_ = std::make_shared<Core::EngineNotifier>();
    
    // 初始化管理器
    initializeManagers();
}

RefactoredWindsynthEngineFacade::~RefactoredWindsynthEngineFacade() {
    std::cout << "[RefactoredWindsynthEngineFacade] 析构函数" << std::endl;
    shutdown();
}

//==============================================================================
// 引擎生命周期管理（委托给 EngineLifecycleManager）
//==============================================================================

bool RefactoredWindsynthEngineFacade::initialize(const Core::EngineConfig& config) {
    return lifecycleManager_ ? lifecycleManager_->initialize(config) : false;
}

bool RefactoredWindsynthEngineFacade::start() {
    return lifecycleManager_ ? lifecycleManager_->start() : false;
}

void RefactoredWindsynthEngineFacade::stop() {
    if (lifecycleManager_) {
        lifecycleManager_->stop();
    }
}

void RefactoredWindsynthEngineFacade::shutdown() {
    if (lifecycleManager_) {
        lifecycleManager_->shutdown();
    }
}

Core::EngineState RefactoredWindsynthEngineFacade::getState() const {
    return lifecycleManager_ ? lifecycleManager_->getState() : Core::EngineState::Error;
}

bool RefactoredWindsynthEngineFacade::isRunning() const {
    return lifecycleManager_ ? lifecycleManager_->isRunning() : false;
}

//==============================================================================
// 音频文件处理（委托给 AudioFileManager）
//==============================================================================

bool RefactoredWindsynthEngineFacade::loadAudioFile(const std::string& filePath) {
    return audioFileManager_ ? audioFileManager_->loadAudioFile(filePath) : false;
}

bool RefactoredWindsynthEngineFacade::play() {
    return audioFileManager_ ? audioFileManager_->play() : false;
}

void RefactoredWindsynthEngineFacade::pause() {
    if (audioFileManager_) {
        audioFileManager_->pause();
    }
}

void RefactoredWindsynthEngineFacade::stopPlayback() {
    if (audioFileManager_) {
        audioFileManager_->stopPlayback();
    }
}

bool RefactoredWindsynthEngineFacade::seekTo(double timeInSeconds) {
    return audioFileManager_ ? audioFileManager_->seekTo(timeInSeconds) : false;
}

double RefactoredWindsynthEngineFacade::getCurrentTime() const {
    return audioFileManager_ ? audioFileManager_->getCurrentTime() : 0.0;
}

double RefactoredWindsynthEngineFacade::getDuration() const {
    return audioFileManager_ ? audioFileManager_->getDuration() : 0.0;
}

//==============================================================================
// 节点参数控制（委托给 NodeParameterController）
//==============================================================================

bool RefactoredWindsynthEngineFacade::setNodeParameter(uint32_t nodeID, int parameterIndex, float value) {
    return parameterController_ ? parameterController_->setNodeParameter(nodeID, parameterIndex, value) : false;
}

float RefactoredWindsynthEngineFacade::getNodeParameter(uint32_t nodeID, int parameterIndex) const {
    return parameterController_ ? parameterController_->getNodeParameter(nodeID, parameterIndex) : -1.0f;
}

int RefactoredWindsynthEngineFacade::getNodeParameterCount(uint32_t nodeID) const {
    return parameterController_ ? parameterController_->getNodeParameterCount(nodeID) : 0;
}

std::optional<Interfaces::ParameterInfo> RefactoredWindsynthEngineFacade::getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const {
    return parameterController_ ? parameterController_->getNodeParameterInfo(nodeID, parameterIndex) : std::nullopt;
}

//==============================================================================
// 插件管理（直接使用 AudioGraph::PluginManager）
//==============================================================================

std::vector<Interfaces::SimplePluginInfo> RefactoredWindsynthEngineFacade::getAvailablePlugins() const {
    std::vector<Interfaces::SimplePluginInfo> result;
    
    if (!context_ || !context_->isInitialized()) {
        return result;
    }
    
    try {
        auto pluginLoader = context_->getPluginLoader();
        if (!pluginLoader) {
            return result;
        }
        
        auto pluginList = pluginLoader->getKnownPlugins();
        
        for (const auto& plugin : pluginList) {
            Interfaces::SimplePluginInfo info;
            info.identifier = plugin.createIdentifierString().toStdString();
            info.name = plugin.name.toStdString();
            info.manufacturer = plugin.manufacturerName.toStdString();
            info.category = plugin.category.toStdString();
            info.format = plugin.pluginFormatName.toStdString();
            info.filePath = plugin.fileOrIdentifier.toStdString();
            info.isValid = true;
            
            result.push_back(info);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredWindsynthEngineFacade] 获取插件列表失败: " << e.what() << std::endl;
    }
    
    return result;
}

void RefactoredWindsynthEngineFacade::loadPluginAsync(const std::string& pluginIdentifier,
                                                     const std::string& displayName,
                                                     Interfaces::PluginLoadCallback callback) {
    if (!context_ || !context_->isInitialized()) {
        if (callback) {
            callback(0, false, "引擎上下文未初始化");
        }
        return;
    }
    
    try {
        auto pluginLoader = context_->getPluginLoader();
        auto pluginManager = context_->getPluginManager();
        
        if (!pluginLoader || !pluginManager) {
            if (callback) {
                callback(0, false, "插件管理器无效");
            }
            return;
        }
        
        // 查找插件描述
        auto pluginList = pluginLoader->getKnownPlugins();
        juce::PluginDescription* targetPlugin = nullptr;
        
        for (auto& plugin : pluginList) {
            if (plugin.createIdentifierString().toStdString() == pluginIdentifier) {
                targetPlugin = &plugin;
                break;
            }
        }
        
        if (!targetPlugin) {
            if (callback) {
                callback(0, false, "找不到指定的插件: " + pluginIdentifier);
            }
            return;
        }
        
        // 异步加载插件
        pluginManager->loadPluginAsync(*targetPlugin, displayName,
            [callback](AudioGraph::NodeID nodeID, const std::string& error) {
                if (callback) {
                    uint32_t simpleNodeID = static_cast<uint32_t>(nodeID.uid);
                    callback(simpleNodeID, error.empty(), error);
                }
            });
            
    } catch (const std::exception& e) {
        std::string error = "加载插件失败: " + std::string(e.what());
        if (notifier_) {
            notifier_->notifyError(error);
        }
        if (callback) {
            callback(0, false, error);
        }
    }
}

bool RefactoredWindsynthEngineFacade::removeNode(uint32_t nodeID) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto pluginManager = context_->getPluginManager();
        if (!pluginManager) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return pluginManager->removePlugin(graphNodeID);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("移除节点失败: " + std::string(e.what()));
        }
        return false;
    }
}

std::vector<Interfaces::SimpleNodeInfo> RefactoredWindsynthEngineFacade::getLoadedNodes() const {
    std::vector<Interfaces::SimpleNodeInfo> result;
    
    if (!context_ || !context_->isInitialized()) {
        return result;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return result;
        }
        
        auto nodes = graphProcessor->getAllNodes();
        
        for (const auto& node : nodes) {
            Interfaces::SimpleNodeInfo info;
            info.nodeID = static_cast<uint32_t>(node.nodeID.uid);
            info.name = node.name;
            info.pluginName = node.pluginName;
            info.isEnabled = node.enabled;
            info.isBypassed = node.bypassed;
            info.numInputChannels = node.numInputChannels;
            info.numOutputChannels = node.numOutputChannels;
            
            result.push_back(info);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[RefactoredWindsynthEngineFacade] 获取节点列表失败: " << e.what() << std::endl;
    }
    
    return result;
}

bool RefactoredWindsynthEngineFacade::setNodeBypassed(uint32_t nodeID, bool bypassed) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return graphProcessor->setNodeBypassed(graphNodeID, bypassed);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("设置节点旁路状态失败: " + std::string(e.what()));
        }
        return false;
    }
}

bool RefactoredWindsynthEngineFacade::setNodeEnabled(uint32_t nodeID, bool enabled) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return graphProcessor->setNodeEnabled(graphNodeID, enabled);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("设置节点启用状态失败: " + std::string(e.what()));
        }
        return false;
    }
}

//==============================================================================
// 事件回调设置（向后兼容）
//==============================================================================

void RefactoredWindsynthEngineFacade::setStateCallback(EngineStateCallback callback) {
    if (notifier_) {
        notifier_->setStateCallback(callback);
    }
}

void RefactoredWindsynthEngineFacade::setErrorCallback(ErrorCallback callback) {
    if (notifier_) {
        notifier_->setErrorCallback(callback);
    }
}

//==============================================================================
// 配置管理
//==============================================================================

const Core::EngineConfig& RefactoredWindsynthEngineFacade::getConfiguration() const {
    static Core::EngineConfig defaultConfig;
    return context_ ? context_->getConfig() : defaultConfig;
}

bool RefactoredWindsynthEngineFacade::updateConfiguration(const Core::EngineConfig& config) {
    if (!lifecycleManager_) {
        return false;
    }
    
    try {
        // 如果引擎正在运行，需要先停止
        bool wasRunning = isRunning();
        if (wasRunning) {
            stop();
        }
        
        // 重新初始化
        bool success = initialize(config);
        
        // 如果之前在运行，重新启动
        if (success && wasRunning) {
            success = start();
        }
        
        return success;
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("更新配置失败: " + std::string(e.what()));
        }
        return false;
    }
}

//==============================================================================
// 初始化方法
//==============================================================================

void RefactoredWindsynthEngineFacade::initializeManagers() {
    std::cout << "[RefactoredWindsynthEngineFacade] 初始化管理器" << std::endl;
    
    // 创建管理器实例
    lifecycleManager_ = std::make_shared<Managers::EngineLifecycleManager>(context_, notifier_);
    audioFileManager_ = std::make_shared<Managers::AudioFileManager>(context_, notifier_);
    parameterController_ = std::make_shared<Managers::NodeParameterController>(context_, notifier_);
    
    std::cout << "[RefactoredWindsynthEngineFacade] 管理器初始化完成" << std::endl;
}

} // namespace WindsynthVST::Engine
