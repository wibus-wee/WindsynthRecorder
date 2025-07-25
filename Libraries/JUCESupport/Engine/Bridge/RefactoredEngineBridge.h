//
//  RefactoredEngineBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  基于重构架构的模块化C桥接层 - 核心引擎管理
//

#ifndef RefactoredEngineBridge_h
#define RefactoredEngineBridge_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// 基础类型定义
//==============================================================================

/**
 * 引擎句柄（不透明指针）
 */
typedef void* RefactoredEngineHandle;

/**
 * 引擎状态枚举
 */
typedef enum {
    RefactoredEngineState_Stopped = 0,
    RefactoredEngineState_Starting = 1,
    RefactoredEngineState_Running = 2,
    RefactoredEngineState_Stopping = 3,
    RefactoredEngineState_Error = 4
} RefactoredEngineState;

/**
 * 引擎配置结构（C兼容）
 */
typedef struct {
    double sampleRate;
    int bufferSize;
    int numInputChannels;
    int numOutputChannels;
    bool enableRealtimeProcessing;
    char audioDeviceName[256];
} RefactoredEngineConfig;

/**
 * 回调函数类型定义
 */
typedef void (*RefactoredEngineStateCallback)(RefactoredEngineState state, const char* message, void* userData);
typedef void (*RefactoredEngineErrorCallback)(const char* error, void* userData);

//==============================================================================
// 核心引擎生命周期管理
//==============================================================================

/**
 * 创建重构后的引擎实例
 * @return 引擎句柄，失败返回NULL
 */
RefactoredEngineHandle RefactoredEngine_Create(void);

/**
 * 销毁引擎实例
 * @param handle 引擎句柄
 */
void RefactoredEngine_Destroy(RefactoredEngineHandle handle);

/**
 * 初始化引擎
 * @param handle 引擎句柄
 * @param config 引擎配置
 * @return 成功返回true
 */
bool RefactoredEngine_Initialize(RefactoredEngineHandle handle, const RefactoredEngineConfig* config);

/**
 * 启动音频处理
 * @param handle 引擎句柄
 * @return 成功返回true
 */
bool RefactoredEngine_Start(RefactoredEngineHandle handle);

/**
 * 停止音频处理
 * @param handle 引擎句柄
 */
void RefactoredEngine_Stop(RefactoredEngineHandle handle);

/**
 * 关闭引擎
 * @param handle 引擎句柄
 */
void RefactoredEngine_Shutdown(RefactoredEngineHandle handle);

/**
 * 获取引擎状态
 * @param handle 引擎句柄
 * @return 当前状态
 */
RefactoredEngineState RefactoredEngine_GetState(RefactoredEngineHandle handle);

/**
 * 检查引擎是否正在运行
 * @param handle 引擎句柄
 * @return 正在运行返回true
 */
bool RefactoredEngine_IsRunning(RefactoredEngineHandle handle);

/**
 * 获取当前配置
 * @param handle 引擎句柄
 * @param config 输出配置结构
 * @return 成功返回true
 */
bool RefactoredEngine_GetConfiguration(RefactoredEngineHandle handle, RefactoredEngineConfig* config);

/**
 * 更新配置
 * @param handle 引擎句柄
 * @param config 新配置
 * @return 成功返回true
 */
bool RefactoredEngine_UpdateConfiguration(RefactoredEngineHandle handle, const RefactoredEngineConfig* config);

//==============================================================================
// 回调设置
//==============================================================================

/**
 * 设置状态变化回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void RefactoredEngine_SetStateCallback(RefactoredEngineHandle handle, 
                                      RefactoredEngineStateCallback callback, 
                                      void* userData);

/**
 * 设置错误回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void RefactoredEngine_SetErrorCallback(RefactoredEngineHandle handle, 
                                      RefactoredEngineErrorCallback callback, 
                                      void* userData);

#ifdef __cplusplus
}
#endif

#endif /* RefactoredEngineBridge_h */
