//
//  ParameterBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  参数控制桥接层 - 专门处理插件参数相关功能
//

#ifndef ParameterBridge_h
#define ParameterBridge_h

#include "EngineBridge.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// 参数相关类型定义
//==============================================================================

/**
 * 参数信息结构（C兼容）
 */
typedef struct {
    char name[128];
    char label[64];
    float minValue;
    float maxValue;
    float defaultValue;
    float currentValue;
    bool isDiscrete;
    int numSteps;
    char units[32];
} ParameterInfo_C;

//==============================================================================
// 参数控制
//==============================================================================

/**
 * 设置节点参数
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引
 * @param value 参数值（0.0-1.0）
 * @return 成功返回true
 */
bool Engine_SetNodeParameter(EngineHandle handle,
                             uint32_t nodeID,
                             int parameterIndex,
                             float value);

/**
 * 获取节点参数
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引
 * @return 参数值（0.0-1.0），失败返回-1.0
 */
float Engine_GetNodeParameter(EngineHandle handle,
                             uint32_t nodeID,
                             int parameterIndex);

/**
 * 获取节点参数数量
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 参数数量
 */
int Engine_GetNodeParameterCount(EngineHandle handle, uint32_t nodeID);

/**
 * 获取节点参数信息
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引
 * @param info 输出参数信息
 * @return 成功返回true
 */
bool Engine_GetNodeParameterInfo(EngineHandle handle,
                                uint32_t nodeID,
                                int parameterIndex,
                                ParameterInfo_C* info);

/**
 * 重置节点参数到默认值
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引，-1表示重置所有参数
 * @return 成功返回true
 */
bool Engine_ResetNodeParameter(EngineHandle handle,
                              uint32_t nodeID,
                              int parameterIndex);

/**
 * 获取节点的所有参数信息
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameters 输出参数信息数组
 * @param maxCount 数组最大容量
 * @return 实际返回的参数数量
 */
int Engine_GetAllParameterInfo(EngineHandle handle,
                              uint32_t nodeID,
                              ParameterInfo_C* parameters,
                              int maxCount);

#ifdef __cplusplus
}
#endif

#endif /* ParameterBridge_h */
