//
//  EngineObserver.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎观察者模式实现
//

#include "EngineObserver.hpp"
#include <algorithm>
#include <iostream>

namespace WindsynthVST::Engine::Core {

//==============================================================================
// 观察者管理
//==============================================================================

void EngineNotifier::addStateObserver(std::shared_ptr<IEngineStateObserver> observer) {
    std::lock_guard<std::mutex> lock(observerMutex);
    stateObservers.push_back(observer);
}

void EngineNotifier::removeStateObserver(std::shared_ptr<IEngineStateObserver> observer) {
    std::lock_guard<std::mutex> lock(observerMutex);
    stateObservers.erase(
        std::remove_if(stateObservers.begin(), stateObservers.end(),
            [&observer](const std::weak_ptr<IEngineStateObserver>& weak) {
                return weak.expired() || weak.lock() == observer;
            }),
        stateObservers.end()
    );
}

void EngineNotifier::addErrorObserver(std::shared_ptr<IEngineErrorObserver> observer) {
    std::lock_guard<std::mutex> lock(observerMutex);
    errorObservers.push_back(observer);
}

void EngineNotifier::removeErrorObserver(std::shared_ptr<IEngineErrorObserver> observer) {
    std::lock_guard<std::mutex> lock(observerMutex);
    errorObservers.erase(
        std::remove_if(errorObservers.begin(), errorObservers.end(),
            [&observer](const std::weak_ptr<IEngineErrorObserver>& weak) {
                return weak.expired() || weak.lock() == observer;
            }),
        errorObservers.end()
    );
}

//==============================================================================
// 通知方法
//==============================================================================

void EngineNotifier::notifyStateChanged(EngineState oldState, EngineState newState, const std::string& message) {
    std::lock_guard<std::mutex> lock(observerMutex);
    
    // 通知所有状态观察者
    for (auto it = stateObservers.begin(); it != stateObservers.end();) {
        if (auto observer = it->lock()) {
            try {
                observer->onStateChanged(oldState, newState, message);
                ++it;
            } catch (const std::exception& e) {
                std::cerr << "[EngineNotifier] 状态观察者异常: " << e.what() << std::endl;
                ++it;
            }
        } else {
            // 移除过期的观察者
            it = stateObservers.erase(it);
        }
    }
    
    // 向后兼容的回调
    if (stateCallback) {
        try {
            stateCallback(newState, message);
        } catch (const std::exception& e) {
            std::cerr << "[EngineNotifier] 状态回调异常: " << e.what() << std::endl;
        }
    }
}

void EngineNotifier::notifyError(const std::string& error, int severity) {
    std::lock_guard<std::mutex> lock(observerMutex);
    
    // 通知所有错误观察者
    for (auto it = errorObservers.begin(); it != errorObservers.end();) {
        if (auto observer = it->lock()) {
            try {
                observer->onError(error, severity);
                ++it;
            } catch (const std::exception& e) {
                std::cerr << "[EngineNotifier] 错误观察者异常: " << e.what() << std::endl;
                ++it;
            }
        } else {
            // 移除过期的观察者
            it = errorObservers.erase(it);
        }
    }
    
    // 向后兼容的回调
    if (errorCallback) {
        try {
            errorCallback(error);
        } catch (const std::exception& e) {
            std::cerr << "[EngineNotifier] 错误回调异常: " << e.what() << std::endl;
        }
    }
}

//==============================================================================
// 便利回调设置（向后兼容）
//==============================================================================

void EngineNotifier::setStateCallback(StateCallback callback) {
    std::lock_guard<std::mutex> lock(observerMutex);
    stateCallback = std::move(callback);
}

void EngineNotifier::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(observerMutex);
    errorCallback = std::move(callback);
}

//==============================================================================
// 内部方法
//==============================================================================

void EngineNotifier::cleanupExpiredObservers() {
    std::lock_guard<std::mutex> lock(observerMutex);
    
    // 清理过期的状态观察者
    stateObservers.erase(
        std::remove_if(stateObservers.begin(), stateObservers.end(),
            [](const std::weak_ptr<IEngineStateObserver>& weak) {
                return weak.expired();
            }),
        stateObservers.end()
    );
    
    // 清理过期的错误观察者
    errorObservers.erase(
        std::remove_if(errorObservers.begin(), errorObservers.end(),
            [](const std::weak_ptr<IEngineErrorObserver>& weak) {
                return weak.expired();
            }),
        errorObservers.end()
    );
}

} // namespace WindsynthVST::Engine::Core
