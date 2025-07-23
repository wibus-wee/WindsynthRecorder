//
//  AudioGraphBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新音频架构的C桥接层 - 完全独立于VSTSupport
//

#ifndef AudioGraphBridge_h
#define AudioGraphBridge_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// 类型定义
//==============================================================================

/**
 * 引擎句柄（不透明指针）
 */
typedef void* WindsynthEngineHandle;

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
 * 插件信息结构（C兼容）
 */
typedef struct {
    char identifier[512];
    char name[256];
    char manufacturer[128];
    char category[64];
    char format[32];
    char filePath[1024];
    bool isValid;
} SimplePluginInfo_C;

/**
 * 节点信息结构（C兼容）
 */
typedef struct {
    uint32_t nodeID;
    char name[256];
    char pluginName[256];
    bool isEnabled;
    bool isBypassed;
    int numInputChannels;
    int numOutputChannels;
} SimpleNodeInfo_C;

/**
 * 引擎统计信息（C兼容）
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

/**
 * 回调函数类型定义
 */
typedef void (*EngineStateCallback_C)(EngineState_C state, const char* message, void* userData);
typedef void (*PluginLoadCallback_C)(uint32_t nodeID, bool success, const char* error, void* userData);
typedef void (*ErrorCallback_C)(const char* error, void* userData);

// 新增：插件扫描相关回调
typedef void (*PluginScanProgressCallback_C)(float progress, const char* currentFile, void* userData);
typedef void (*PluginScanCompleteCallback_C)(int foundPlugins, void* userData);

//==============================================================================
// 引擎生命周期管理
//==============================================================================

/**
 * 创建引擎实例
 * @return 引擎句柄，失败返回NULL
 */
WindsynthEngineHandle Engine_Create(void);

/**
 * 销毁引擎实例
 * @param handle 引擎句柄
 */
void Engine_Destroy(WindsynthEngineHandle handle);

/**
 * 初始化引擎
 * @param handle 引擎句柄
 * @param config 引擎配置
 * @return 成功返回true
 */
bool Engine_Initialize(WindsynthEngineHandle handle, const EngineConfig_C* config);

/**
 * 启动音频处理
 * @param handle 引擎句柄
 * @return 成功返回true
 */
bool Engine_Start(WindsynthEngineHandle handle);

/**
 * 停止音频处理
 * @param handle 引擎句柄
 */
void Engine_Stop(WindsynthEngineHandle handle);

/**
 * 关闭引擎
 * @param handle 引擎句柄
 */
void Engine_Shutdown(WindsynthEngineHandle handle);

/**
 * 获取引擎状态
 * @param handle 引擎句柄
 * @return 当前状态
 */
EngineState_C Engine_GetState(WindsynthEngineHandle handle);

/**
 * 检查引擎是否正在运行
 * @param handle 引擎句柄
 * @return 正在运行返回true
 */
bool Engine_IsRunning(WindsynthEngineHandle handle);

//==============================================================================
// 音频文件处理
//==============================================================================

/**
 * 加载音频文件
 * @param handle 引擎句柄
 * @param filePath 文件路径
 * @return 成功返回true
 */
bool Engine_LoadAudioFile(WindsynthEngineHandle handle, const char* filePath);

/**
 * 开始播放
 * @param handle 引擎句柄
 * @return 成功返回true
 */
bool Engine_Play(WindsynthEngineHandle handle);

/**
 * 暂停播放
 * @param handle 引擎句柄
 */
void Engine_Pause(WindsynthEngineHandle handle);

/**
 * 停止播放
 * @param handle 引擎句柄
 */
void Engine_StopPlayback(WindsynthEngineHandle handle);

/**
 * 跳转到指定时间
 * @param handle 引擎句柄
 * @param timeInSeconds 时间（秒）
 * @return 成功返回true
 */
bool Engine_SeekTo(WindsynthEngineHandle handle, double timeInSeconds);

/**
 * 获取当前播放时间
 * @param handle 引擎句柄
 * @return 当前时间（秒）
 */
double Engine_GetCurrentTime(WindsynthEngineHandle handle);

/**
 * 获取音频文件总时长
 * @param handle 引擎句柄
 * @return 总时长（秒）
 */
double Engine_GetDuration(WindsynthEngineHandle handle);

//==============================================================================
// 插件管理
//==============================================================================

/**
 * 扫描插件（统一异步方法）
 * @param handle 引擎句柄
 * @param rescanExisting 是否重新扫描已知插件
 * @param progressCallback 进度回调（可为NULL）
 * @param completeCallback 完成回调（可为NULL）
 * @param userData 用户数据
 */
void Engine_ScanPluginsAsync(WindsynthEngineHandle handle,
                            bool rescanExisting,
                            PluginScanProgressCallback_C progressCallback,
                            PluginScanCompleteCallback_C completeCallback,
                            void* userData);

/**
 * 停止当前插件扫描
 * @param handle 引擎句柄
 */
void Engine_StopPluginScan(WindsynthEngineHandle handle);

/**
 * 检查是否正在扫描插件
 * @param handle 引擎句柄
 * @return 正在扫描返回true
 */
bool Engine_IsScanning(WindsynthEngineHandle handle);

// 注意：Dead Man's Pedal和黑名单功能已内置到扫描器中，无需手动管理

/**
 * 获取可用插件数量
 * @param handle 引擎句柄
 * @return 插件数量
 */
int Engine_GetAvailablePluginCount(WindsynthEngineHandle handle);

/**
 * 获取可用插件信息
 * @param handle 引擎句柄
 * @param index 插件索引
 * @param pluginInfo 输出的插件信息
 * @return 成功返回true
 */
bool Engine_GetAvailablePluginInfo(WindsynthEngineHandle handle, int index, SimplePluginInfo_C* pluginInfo);

/**
 * 异步加载插件（通过标识符）
 * @param handle 引擎句柄
 * @param pluginIdentifier 插件标识符
 * @param displayName 显示名称（可为NULL）
 * @param callback 加载完成回调（可为NULL）
 * @param userData 用户数据
 */
void Engine_LoadPluginByIdentifier(WindsynthEngineHandle handle, 
                                  const char* pluginIdentifier,
                                  const char* displayName,
                                  PluginLoadCallback_C callback,
                                  void* userData);

/**
 * 移除插件节点
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_RemoveNode(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 获取已加载节点数量
 * @param handle 引擎句柄
 * @return 节点数量
 */
int Engine_GetLoadedNodeCount(WindsynthEngineHandle handle);

/**
 * 获取已加载节点信息
 * @param handle 引擎句柄
 * @param index 节点索引
 * @param nodeInfo 输出的节点信息
 * @return 成功返回true
 */
bool Engine_GetLoadedNodeInfo(WindsynthEngineHandle handle, int index, SimpleNodeInfo_C* nodeInfo);

/**
 * 设置节点旁路状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param bypassed 是否旁路
 * @return 成功返回true
 */
bool Engine_SetNodeBypassed(WindsynthEngineHandle handle, uint32_t nodeID, bool bypassed);

/**
 * 设置节点启用状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param enabled 是否启用
 * @return 成功返回true
 */
bool Engine_SetNodeEnabled(WindsynthEngineHandle handle, uint32_t nodeID, bool enabled);

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
bool Engine_SetNodeParameter(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex, float value);

/**
 * 获取节点参数
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引
 * @return 参数值（0.0-1.0），失败返回-1.0
 */
float Engine_GetNodeParameter(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex);

/**
 * 获取节点参数数量
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 参数数量
 */
int Engine_GetNodeParameterCount(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 获取节点参数信息
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param parameterIndex 参数索引
 * @param info 输出的参数信息
 * @return 成功返回true
 */
bool Engine_GetNodeParameterInfo(WindsynthEngineHandle handle, uint32_t nodeID, int parameterIndex, ParameterInfo_C* info);

/**
 * 检查节点是否有编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 有编辑器返回true
 */
bool Engine_NodeHasEditor(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 显示节点编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_ShowNodeEditor(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 隐藏节点编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_HideNodeEditor(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 检查节点编辑器是否可见
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 可见返回true
 */
bool Engine_IsNodeEditorVisible(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 移动节点在处理链中的位置
 * @param handle 引擎句柄
 * @param nodeID 要移动的节点ID
 * @param newPosition 新位置索引
 * @return 成功返回true
 */
bool Engine_MoveNode(WindsynthEngineHandle handle, uint32_t nodeID, int newPosition);

/**
 * 交换两个节点的位置
 * @param handle 引擎句柄
 * @param nodeID1 第一个节点ID
 * @param nodeID2 第二个节点ID
 * @return 成功返回true
 */
bool Engine_SwapNodes(WindsynthEngineHandle handle, uint32_t nodeID1, uint32_t nodeID2);

//==============================================================================
// 音频路由管理
//==============================================================================

/**
 * 创建串联处理链
 * @param handle 引擎句柄
 * @param nodeIDs 节点ID数组
 * @param nodeCount 节点数量
 * @return 成功创建的连接数量
 */
int Engine_CreateProcessingChain(WindsynthEngineHandle handle, const uint32_t* nodeIDs, int nodeCount);

/**
 * 自动连接节点到音频输入输出
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_AutoConnectToIO(WindsynthEngineHandle handle, uint32_t nodeID);

/**
 * 断开节点的所有连接
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_DisconnectNode(WindsynthEngineHandle handle, uint32_t nodeID);

//==============================================================================
// 状态和监控
//==============================================================================

/**
 * 获取引擎统计信息
 * @param handle 引擎句柄
 * @param stats 输出的统计信息
 * @return 成功返回true
 */
bool Engine_GetStatistics(WindsynthEngineHandle handle, EngineStatistics_C* stats);

/**
 * 获取输出电平
 * @param handle 引擎句柄
 * @return 输出电平（dB）
 */
double Engine_GetOutputLevel(WindsynthEngineHandle handle);

/**
 * 获取输入电平
 * @param handle 引擎句柄
 * @return 输入电平（dB）
 */
double Engine_GetInputLevel(WindsynthEngineHandle handle);

//==============================================================================
// 回调设置
//==============================================================================

/**
 * 设置状态变化回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void Engine_SetStateCallback(WindsynthEngineHandle handle, EngineStateCallback_C callback, void* userData);

/**
 * 设置错误回调
 * @param handle 引擎句柄
 * @param callback 回调函数
 * @param userData 用户数据
 */
void Engine_SetErrorCallback(WindsynthEngineHandle handle, ErrorCallback_C callback, void* userData);

//==============================================================================
// 配置管理
//==============================================================================

/**
 * 获取当前配置
 * @param handle 引擎句柄
 * @param config 输出的配置信息
 * @return 成功返回true
 */
bool Engine_GetConfiguration(WindsynthEngineHandle handle, EngineConfig_C* config);

/**
 * 更新配置
 * @param handle 引擎句柄
 * @param config 新配置
 * @return 成功返回true
 */
bool Engine_UpdateConfiguration(WindsynthEngineHandle handle, const EngineConfig_C* config);

#ifdef __cplusplus
}
#endif

#endif /* AudioGraphBridge_h */
