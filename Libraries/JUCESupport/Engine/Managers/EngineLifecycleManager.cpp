//
//  EngineLifecycleManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎生命周期管理器实现
//

#include "EngineLifecycleManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace WindsynthVST::Engine::Managers {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

EngineLifecycleManager::EngineLifecycleManager(std::shared_ptr<Core::EngineContext> context,
                                             std::shared_ptr<Core::EngineNotifier> notifier)
    : context_(std::move(context))
    , notifier_(std::move(notifier)) {
    std::cout << "[EngineLifecycleManager] 构造函数" << std::endl;
}

EngineLifecycleManager::~EngineLifecycleManager() {
    std::cout << "[EngineLifecycleManager] 析构函数" << std::endl;
    shutdown();
}

//==============================================================================
// IEngineLifecycleManager 接口实现
//==============================================================================

bool EngineLifecycleManager::initialize(const Core::EngineConfig& config) {
    std::cout << "[EngineLifecycleManager] 初始化引擎" << std::endl;
    
    if (!context_) {
        notifyError("引擎上下文无效");
        return false;
    }
    
    if (context_->getState() != Core::EngineState::Stopped) {
        notifyError("引擎必须在停止状态下才能初始化");
        return false;
    }
    
    notifyStateChange(Core::EngineState::Starting, "正在初始化引擎...");
    
    try {
        // 初始化共享上下文
        if (!context_->initialize()) {
            notifyError("无法初始化引擎上下文");
            notifyStateChange(Core::EngineState::Error);
            return false;
        }
        
        // 保存配置
        context_->setConfig(config);
        
        // 配置音频I/O
        if (!configureAudioIO(config)) {
            notifyError("无法配置音频I/O");
            notifyStateChange(Core::EngineState::Error);
            return false;
        }
        
        // 准备音频处理
        if (!prepareAudioProcessing(config)) {
            notifyError("无法准备音频处理");
            notifyStateChange(Core::EngineState::Error);
            return false;
        }
        
        notifyStateChange(Core::EngineState::Stopped, "引擎初始化完成");
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "引擎初始化失败: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(Core::EngineState::Error);
        return false;
    }
}

bool EngineLifecycleManager::start() {
    std::cout << "[EngineLifecycleManager] 启动音频处理" << std::endl;
    
    if (!context_) {
        notifyError("引擎上下文无效");
        return false;
    }
    
    if (context_->getState() != Core::EngineState::Stopped) {
        notifyError("引擎必须在停止状态下才能启动");
        return false;
    }
    
    notifyStateChange(Core::EngineState::Starting, "正在启动音频处理...");
    
    try {
        // 实际的音频启动由JUCE的AudioDeviceManager处理
        // 这里主要是状态管理
        notifyStateChange(Core::EngineState::Running, "音频处理已启动");
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "启动音频处理失败: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(Core::EngineState::Error);
        return false;
    }
}

void EngineLifecycleManager::stop() {
    std::cout << "[EngineLifecycleManager] 停止音频处理" << std::endl;
    
    if (!context_ || context_->getState() == Core::EngineState::Stopped) {
        return;
    }
    
    notifyStateChange(Core::EngineState::Stopping, "正在停止音频处理...");
    
    try {
        // 状态管理，实际停止由各个管理器处理
        notifyStateChange(Core::EngineState::Stopped, "音频处理已停止");
        
    } catch (const std::exception& e) {
        std::string error = "停止音频处理时出错: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(Core::EngineState::Error);
    }
}

void EngineLifecycleManager::shutdown() {
    std::cout << "[EngineLifecycleManager] 关闭引擎" << std::endl;
    
    if (!context_) {
        return;
    }
    
    stop();
    
    try {
        context_->shutdown();
        std::cout << "[EngineLifecycleManager] 引擎关闭完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::string error = "关闭引擎时出错: " + std::string(e.what());
        notifyError(error);
    }
}

Core::EngineState EngineLifecycleManager::getState() const {
    return context_ ? context_->getState() : Core::EngineState::Error;
}

bool EngineLifecycleManager::isRunning() const {
    return context_ && context_->isRunning();
}

//==============================================================================
// 内部方法
//==============================================================================

void EngineLifecycleManager::notifyStateChange(Core::EngineState newState, const std::string& message) {
    if (context_) {
        auto oldState = context_->getState();
        context_->setState(newState);
        
        if (notifier_) {
            notifier_->notifyStateChanged(oldState, newState, message);
        }
    }
}

void EngineLifecycleManager::notifyError(const std::string& error) {
    if (notifier_) {
        notifier_->notifyError(error);
    }
    std::cerr << "[EngineLifecycleManager] 错误: " << error << std::endl;
}

bool EngineLifecycleManager::configureAudioIO(const Core::EngineConfig& config) {
    auto ioManager = context_->getIOManager();
    if (!ioManager) {
        return false;
    }
    
    AudioGraph::AudioIOManager::IOConfiguration ioConfig;
    ioConfig.numInputChannels = config.numInputChannels;
    ioConfig.numOutputChannels = config.numOutputChannels;
    ioConfig.sampleRate = config.sampleRate;
    ioConfig.bufferSize = config.bufferSize;
    
    return ioManager->configureIO(ioConfig);
}

bool EngineLifecycleManager::prepareAudioProcessing(const Core::EngineConfig& config) {
    auto graphProcessor = context_->getGraphProcessor();
    if (!graphProcessor) {
        return false;
    }
    
    graphProcessor->prepareToPlay(config.sampleRate, config.bufferSize);
    return true;
}

} // namespace WindsynthVST::Engine::Managers
