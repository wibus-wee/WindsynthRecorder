//
//  BridgeInternal.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  桥接层内部共享定义
//

#ifndef BridgeInternal_h
#define BridgeInternal_h

#include "EngineBridge.h"
#include "../WindsynthEngineFacade.hpp"
#include <memory>

using namespace WindsynthVST::Engine;

//==============================================================================
// 内部结构定义
//==============================================================================

/**
 * 桥接层上下文 - 包装 C++ 引擎实例和回调信息
 */
struct BridgeContext {
    std::unique_ptr<WindsynthEngineFacade> engine;
    
    // 回调信息
    EngineStateCallback stateCallback = nullptr;
    void* stateUserData = nullptr;

    EngineErrorCallback errorCallback = nullptr;
    void* errorUserData = nullptr;
    
    BridgeContext() {
        engine = std::make_unique<WindsynthEngineFacade>();
    }
};

//==============================================================================
// 内部辅助函数
//==============================================================================

/**
 * 获取桥接层上下文
 */
BridgeContext* getContext(EngineHandle handle);

#endif /* BridgeInternal_h */
