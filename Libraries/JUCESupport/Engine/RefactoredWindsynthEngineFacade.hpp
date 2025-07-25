//
//  RefactoredWindsynthEngineFacade.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  重构后的轻量级引擎门面类
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

// 核心组件
#include "Core/EngineContext.hpp"
#include "Core/EngineObserver.hpp"

// 管理器接口
#include "Interfaces/IEngineLifecycleManager.hpp"
#include "Interfaces/IAudioFileManager.hpp"
#include "Interfaces/INodeParameterController.hpp"
#include "Interfaces/IPluginManager.hpp"

// 管理器实现
#include "Managers/EngineLifecycleManager.hpp"
#include "Managers/AudioFileManager.hpp"
#include "Managers/NodeParameterController.hpp"

namespace WindsynthVST::Engine {

/**
 * 重构后的 WindsynthEngineFacade
 * 
 * 这是一个轻量级的门面类，遵循以下设计原则：
 * - 单一职责原则：只负责协调各个管理器
 * - 开闭原则：通过接口扩展功能
 * - 依赖倒置原则：依赖抽象而非具体实现
 * - 接口隔离原则：每个管理器都有专门的接口
 * 
 * 主要改进：
 * 1. 将原有的1200+行代码拆分为多个专门的管理器
 * 2. 使用依赖注入提高可测试性
 * 3. 应用观察者模式处理事件通知
 * 4. 提供清晰的模块边界和职责分离
 */
class RefactoredWindsynthEngineFacade {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     */
    RefactoredWindsynthEngineFacade();
    
    /**
     * 析构函数
     */
    ~RefactoredWindsynthEngineFacade();
    
    //==============================================================================
    // 引擎生命周期管理（委托给 EngineLifecycleManager）
    //==============================================================================
    
    bool initialize(const Core::EngineConfig& config);
    bool start();
    void stop();
    void shutdown();
    Core::EngineState getState() const;
    bool isRunning() const;
    
    //==============================================================================
    // 音频文件处理（委托给 AudioFileManager）
    //==============================================================================
    
    bool loadAudioFile(const std::string& filePath);
    bool play();
    void pause();
    void stopPlayback();
    bool seekTo(double timeInSeconds);
    double getCurrentTime() const;
    double getDuration() const;
    
    //==============================================================================
    // 节点参数控制（委托给 NodeParameterController）
    //==============================================================================
    
    bool setNodeParameter(uint32_t nodeID, int parameterIndex, float value);
    float getNodeParameter(uint32_t nodeID, int parameterIndex) const;
    int getNodeParameterCount(uint32_t nodeID) const;
    std::optional<Interfaces::ParameterInfo> getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const;
    
    //==============================================================================
    // 插件管理（直接使用 AudioGraph::PluginManager）
    //==============================================================================

    std::vector<Interfaces::SimplePluginInfo> getAvailablePlugins() const;
    void loadPluginAsync(const std::string& pluginIdentifier,
                        const std::string& displayName = "",
                        Interfaces::PluginLoadCallback callback = nullptr);
    bool removeNode(uint32_t nodeID);
    std::vector<Interfaces::SimpleNodeInfo> getLoadedNodes() const;
    bool setNodeBypassed(uint32_t nodeID, bool bypassed);
    bool setNodeEnabled(uint32_t nodeID, bool enabled);
    
    //==============================================================================
    // 事件回调设置（向后兼容）
    //==============================================================================
    
    using EngineStateCallback = std::function<void(Core::EngineState state, const std::string& message)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setStateCallback(EngineStateCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    //==============================================================================
    // 配置管理
    //==============================================================================
    
    const Core::EngineConfig& getConfiguration() const;
    bool updateConfiguration(const Core::EngineConfig& config);
    
    //==============================================================================
    // 管理器访问（用于高级用法）
    //==============================================================================
    
    std::shared_ptr<Interfaces::IEngineLifecycleManager> getLifecycleManager() const {
        return lifecycleManager_;
    }
    
    std::shared_ptr<Interfaces::IAudioFileManager> getAudioFileManager() const {
        return audioFileManager_;
    }
    
    std::shared_ptr<Interfaces::INodeParameterController> getParameterController() const {
        return parameterController_;
    }
    
    std::shared_ptr<Core::EngineContext> getContext() const {
        return context_;
    }

private:
    //==============================================================================
    // 核心组件
    //==============================================================================
    
    std::shared_ptr<Core::EngineContext> context_;
    std::shared_ptr<Core::EngineNotifier> notifier_;
    
    //==============================================================================
    // 管理器实例
    //==============================================================================
    
    std::shared_ptr<Interfaces::IEngineLifecycleManager> lifecycleManager_;
    std::shared_ptr<Interfaces::IAudioFileManager> audioFileManager_;
    std::shared_ptr<Interfaces::INodeParameterController> parameterController_;
    // 注意：插件管理器直接使用 AudioGraph::PluginManager
    
    //==============================================================================
    // 初始化方法
    //==============================================================================
    
    void initializeManagers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefactoredWindsynthEngineFacade)
};

} // namespace WindsynthVST::Engine
