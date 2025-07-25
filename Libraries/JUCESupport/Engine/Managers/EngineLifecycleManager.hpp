//
//  EngineLifecycleManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎生命周期管理器
//

#pragma once

#include "../Interfaces/IEngineLifecycleManager.hpp"
#include "../Core/EngineContext.hpp"
#include "../Core/EngineObserver.hpp"
#include <memory>
#include <atomic>

namespace WindsynthVST::Engine::Managers {

/**
 * 引擎生命周期管理器实现
 * 
 * 负责引擎的初始化、启动、停止和关闭操作
 * 遵循单一职责原则，只处理生命周期相关的逻辑
 */
class EngineLifecycleManager : public Interfaces::IEngineLifecycleManager {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param context 共享引擎上下文
     * @param notifier 事件通知器
     */
    explicit EngineLifecycleManager(std::shared_ptr<Core::EngineContext> context,
                                   std::shared_ptr<Core::EngineNotifier> notifier);
    
    ~EngineLifecycleManager() override;
    
    //==============================================================================
    // IEngineLifecycleManager 接口实现
    //==============================================================================
    
    bool initialize(const Core::EngineConfig& config) override;
    bool start() override;
    void stop() override;
    void shutdown() override;
    Core::EngineState getState() const override;
    bool isRunning() const override;

private:
    //==============================================================================
    // 成员变量
    //==============================================================================
    
    std::shared_ptr<Core::EngineContext> context_;
    std::shared_ptr<Core::EngineNotifier> notifier_;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void notifyStateChange(Core::EngineState newState, const std::string& message = "");
    void notifyError(const std::string& error);
    bool configureAudioIO(const Core::EngineConfig& config);
    bool prepareAudioProcessing(const Core::EngineConfig& config);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineLifecycleManager)
};

} // namespace WindsynthVST::Engine::Managers
