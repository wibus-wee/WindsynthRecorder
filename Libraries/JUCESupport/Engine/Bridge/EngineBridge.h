//
//  EngineBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  模块化C桥接层 - 核心引擎管理
//

#ifndef EngineBridge_h
#define EngineBridge_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// 前向声明
//==============================================================================

struct BridgeContext;

//==============================================================================
// 基础类型定义
//==============================================================================

/**
 * 引擎句柄（不透明指针）
 */
typedef void* EngineHandle;

/**
 * 引擎状态枚举
 */
typedef enum {
    EngineState_Stopped = 0,
    EngineState_Starting = 1,
    EngineState_Running = 2,
    EngineState_Stopping = 3,
    EngineState_Error = 4
} EngineState_C;

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
} EngineConfig_C;

/**
 * 引擎统计信息结构（C兼容）
 */
typedef struct {
    double cpuUsage;
    double memoryUsage;
    double inputLevel;
    double outputLevel;
    double latency;
    int dropouts;
    int activeNodes;
    int totalConnections;
} EngineStatistics_C;

/**
 * 离线渲染设置结构（C兼容）
 */
typedef struct {
    int32_t sampleRate;
    int32_t bitDepth;
    int32_t numChannels;
    bool normalizeOutput;
    bool includePluginTails;
    int32_t format;
} RenderSettings_C;

/**
 * 回调函数类型定义
 */
typedef void (*EngineStateCallback)(EngineState_C state, const char* message, void* userData);
typedef void (*EngineErrorCallback)(const char* error, void* userData);

//==============================================================================
// 核心引擎生命周期管理
//==============================================================================

/**
 * 创建引擎实例
 * @return 引擎句柄，失败返回NULL
 */
EngineHandle Engine_Create(void);

/**
 * 销毁引擎实例
 * @param handle 引擎句柄
 */
void Engine_Destroy(EngineHandle handle);

/**
 * 初始化引擎
 * @param handle 引擎句柄
 * @param config 引擎配置
 * @return 成功返回true
 */
bool Engine_Initialize(EngineHandle handle, const EngineConfig_C* config);

/**
 * 启动音频处理
 * @param handle 引擎句柄
 * @return 成功返回true
 */
bool Engine_Start(EngineHandle handle);

/**
 * 停止音频处理
 * @param handle 引擎句柄
 */
void Engine_Stop(EngineHandle handle);

/**
 * 关闭引擎
 * @param handle 引擎句柄
 */
void Engine_Shutdown(EngineHandle handle);

/**
 * 获取引擎状态
 * @param handle 引擎句柄
 * @return 当前状态
 */
EngineState_C Engine_GetState(EngineHandle handle);

/**
 * 检查引擎是否正在运行
 * @param handle 引擎句柄
 * @return 正在运行返回true
 */
bool Engine_IsRunning(EngineHandle handle);

/**
 * 获取当前配置
 * @param handle 引擎句柄
 * @param config 输出配置结构
 * @return 成功返回true
 */
bool Engine_GetConfiguration(EngineHandle handle, EngineConfig_C* config);

/**
 * 更新配置
 * @param handle 引擎句柄
 * @param config 新配置
 * @return 成功返回true
 */
bool Engine_UpdateConfiguration(EngineHandle handle, const EngineConfig_C* config);

/**
 * 获取引擎统计信息
 * @param handle 引擎句柄
 * @param statistics 输出统计信息结构
 * @return 成功返回true
 */
bool Engine_GetStatistics(EngineHandle handle, EngineStatistics_C* statistics);

/**
 * 获取输出电平
 * @param handle 引擎句柄
 * @return 输出电平（dB）
 */
double Engine_GetOutputLevel(EngineHandle handle);

/**
 * 获取输入电平
 * @param handle 引擎句柄
 * @return 输入电平（dB）
 */
double Engine_GetInputLevel(EngineHandle handle);

/**
 * 渲染进度回调函数类型
 */
typedef void (*RenderProgressCallback)(float progress, const char* message, void* userData);

/**
 * 渲染到文件
 * @param handle 引擎句柄
 * @param inputPath 输入文件路径
 * @param outputPath 输出文件路径
 * @param settings 渲染设置
 * @param progressCallback 进度回调函数（可选）
 * @param userData 用户数据
 * @return 成功返回true
 */
bool Engine_RenderToFile(EngineHandle handle,
                        const char* inputPath,
                        const char* outputPath,
                        const RenderSettings_C* settings,
                        RenderProgressCallback progressCallback,
                        void* userData);

//==============================================================================
// 回调设置
//==============================================================================

/**
 * 设置状态变化回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void Engine_SetStateCallback(EngineHandle handle,
                            EngineStateCallback callback,
                            void* userData);

/**
 * 设置错误回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void Engine_SetErrorCallback(EngineHandle handle,
                            EngineErrorCallback callback,
                            void* userData);

//==============================================================================
// 内部辅助函数（供其他桥接层使用）
//==============================================================================

/**
 * 获取桥接层上下文（内部使用）
 */
struct BridgeContext* getContext(EngineHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* EngineBridge_h */
