//
//  IEngineLifecycleManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎生命周期管理接口
//

#pragma once

#include "../Core/EngineContext.hpp"
#include <functional>

namespace WindsynthVST::Engine::Interfaces {

/**
 * 引擎生命周期管理接口
 * 
 * 负责引擎的初始化、启动、停止和关闭操作
 */
class IEngineLifecycleManager {
public:
    virtual ~IEngineLifecycleManager() = default;
    
    /**
     * 初始化引擎
     * @param config 引擎配置
     * @return 成功返回true
     */
    virtual bool initialize(const Core::EngineConfig& config) = 0;
    
    /**
     * 启动音频处理
     * @return 成功返回true
     */
    virtual bool start() = 0;
    
    /**
     * 停止音频处理
     */
    virtual void stop() = 0;
    
    /**
     * 释放所有资源
     */
    virtual void shutdown() = 0;
    
    /**
     * 获取当前引擎状态
     */
    virtual Core::EngineState getState() const = 0;
    
    /**
     * 检查引擎是否正在运行
     */
    virtual bool isRunning() const = 0;
};

} // namespace WindsynthVST::Engine::Interfaces
