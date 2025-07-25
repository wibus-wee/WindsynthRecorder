//
//  ParameterBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  参数控制桥接层实现
//

#include "ParameterBridge.h"
#include "BridgeInternal.h"
#include <iostream>

//==============================================================================
// 参数控制实现
//==============================================================================

/**
 * 转换参数信息
 */
static void convertParameterInfo(const Interfaces::ParameterInfo& cppInfo, ParameterInfo_C* cInfo) {
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

bool Engine_SetNodeParameter(EngineHandle handle,
                                      uint32_t nodeID,
                                      int parameterIndex,
                                      float value) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->setNodeParameter(nodeID, parameterIndex, value);
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBridge] 设置节点参数失败: " << e.what() << std::endl;
        return false;
    }
}

float Engine_GetNodeParameter(EngineHandle handle,
                                       uint32_t nodeID,
                                       int parameterIndex) {
    if (!handle) return -1.0f;

    try {
        auto context = getContext(handle);
        if (!context->engine) return -1.0f;

        return context->engine->getNodeParameter(nodeID, parameterIndex);
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBridge] 获取节点参数失败: " << e.what() << std::endl;
        return -1.0f;
    }
}

int Engine_GetNodeParameterCount(EngineHandle handle, uint32_t nodeID) {
    if (!handle) return 0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0;

        return context->engine->getNodeParameterCount(nodeID);
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBridge] 获取节点参数数量失败: " << e.what() << std::endl;
        return 0;
    }
}

bool Engine_GetNodeParameterInfo(EngineHandle handle,
                                          uint32_t nodeID,
                                          int parameterIndex,
                                          ParameterInfo_C* info) {
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
        std::cerr << "[ParameterBridge] 获取节点参数信息失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_ResetNodeParameter(EngineHandle handle,
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
        std::cerr << "[ParameterBridge] 重置节点参数失败: " << e.what() << std::endl;
        return false;
    }
}

int Engine_GetAllParameterInfo(EngineHandle handle,
                                        uint32_t nodeID,
                                        ParameterInfo_C* parameters,
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
        std::cerr << "[ParameterBridge] 获取所有参数信息失败: " << e.what() << std::endl;
        return 0;
    }
}
