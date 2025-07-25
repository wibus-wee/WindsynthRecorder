//
//  EngineObserver.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎观察者模式基础设施
//

#pragma once

#include "EngineContext.hpp"
#include <functional>
#include <vector>
#include <memory>
#include <mutex>

namespace WindsynthVST::Engine::Core {

/**
 * 引擎状态观察者接口
 */
class IEngineStateObserver {
public:
    virtual ~IEngineStateObserver() = default;
    
    /**
     * 状态变化通知
     * @param oldState 旧状态
     * @param newState 新状态
     * @param message 状态消息
     */
    virtual void onStateChanged(EngineState oldState, EngineState newState, const std::string& message) = 0;
};

/**
 * 引擎错误观察者接口
 */
class IEngineErrorObserver {
public:
    virtual ~IEngineErrorObserver() = default;
    
    /**
     * 错误通知
     * @param error 错误消息
     * @param severity 错误严重程度（0=信息，1=警告，2=错误，3=致命）
     */
    virtual void onError(const std::string& error, int severity = 2) = 0;
};

/**
 * 引擎事件通知器
 * 
 * 管理观察者的注册和通知
 */
class EngineNotifier {
public:
    //==============================================================================
    // 观察者管理
    //==============================================================================
    
    /**
     * 添加状态观察者
     */
    void addStateObserver(std::shared_ptr<IEngineStateObserver> observer);
    
    /**
     * 移除状态观察者
     */
    void removeStateObserver(std::shared_ptr<IEngineStateObserver> observer);
    
    /**
     * 添加错误观察者
     */
    void addErrorObserver(std::shared_ptr<IEngineErrorObserver> observer);
    
    /**
     * 移除错误观察者
     */
    void removeErrorObserver(std::shared_ptr<IEngineErrorObserver> observer);
    
    //==============================================================================
    // 通知方法
    //==============================================================================
    
    /**
     * 通知状态变化
     */
    void notifyStateChanged(EngineState oldState, EngineState newState, const std::string& message = "");
    
    /**
     * 通知错误
     */
    void notifyError(const std::string& error, int severity = 2);
    
    //==============================================================================
    // 便利回调设置（向后兼容）
    //==============================================================================
    
    using StateCallback = std::function<void(EngineState state, const std::string& message)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    /**
     * 设置状态回调（向后兼容）
     */
    void setStateCallback(StateCallback callback);
    
    /**
     * 设置错误回调（向后兼容）
     */
    void setErrorCallback(ErrorCallback callback);

private:
    //==============================================================================
    // 观察者列表
    //==============================================================================
    
    std::vector<std::weak_ptr<IEngineStateObserver>> stateObservers;
    std::vector<std::weak_ptr<IEngineErrorObserver>> errorObservers;
    
    mutable std::mutex observerMutex;
    
    //==============================================================================
    // 向后兼容的回调
    //==============================================================================
    
    StateCallback stateCallback;
    ErrorCallback errorCallback;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void cleanupExpiredObservers();
};

/**
 * 简单的函数式观察者实现
 */
class FunctionStateObserver : public IEngineStateObserver {
public:
    using Callback = std::function<void(EngineState, EngineState, const std::string&)>;
    
    explicit FunctionStateObserver(Callback callback) : callback_(std::move(callback)) {}
    
    void onStateChanged(EngineState oldState, EngineState newState, const std::string& message) override {
        if (callback_) {
            callback_(oldState, newState, message);
        }
    }

private:
    Callback callback_;
};

class FunctionErrorObserver : public IEngineErrorObserver {
public:
    using Callback = std::function<void(const std::string&, int)>;
    
    explicit FunctionErrorObserver(Callback callback) : callback_(std::move(callback)) {}
    
    void onError(const std::string& error, int severity) override {
        if (callback_) {
            callback_(error, severity);
        }
    }

private:
    Callback callback_;
};

} // namespace WindsynthVST::Engine::Core
