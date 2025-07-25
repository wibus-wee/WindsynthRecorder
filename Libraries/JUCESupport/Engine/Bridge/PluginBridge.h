//
//  PluginBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理桥接层 - 专门处理插件相关功能
//

#ifndef PluginBridge_h
#define PluginBridge_h

#include "EngineBridge.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// 插件相关类型定义
//==============================================================================

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
} PluginInfo_C;

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
} NodeInfo_C;

/**
 * 简化插件信息结构（C兼容）- 与 SimplePluginInfo 对应
 */
typedef PluginInfo_C SimplePluginInfo_C;

/**
 * 简化节点信息结构（C兼容）- 与 SimpleNodeInfo 对应
 */
typedef NodeInfo_C SimpleNodeInfo_C;

/**
 * 插件加载回调函数类型
 */
typedef void (*PluginLoadCallback)(uint32_t nodeID, bool success, const char* error, void* userData);

//==============================================================================
// 插件管理
//==============================================================================

/**
 * 插件扫描进度回调函数类型
 */
typedef void (*PluginScanProgressCallback)(float progress, const char* currentFile, void* userData);

/**
 * 插件扫描完成回调函数类型
 */
typedef void (*PluginScanCompletionCallback)(int foundPlugins, void* userData);

/**
 * 异步扫描插件
 * @param handle 引擎句柄
 * @param rescanExisting 是否重新扫描已存在的插件
 * @param progressCallback 进度回调函数
 * @param completionCallback 完成回调函数
 * @param userData 用户数据
 */
void Engine_ScanPluginsAsync(EngineHandle handle,
                            bool rescanExisting,
                            PluginScanProgressCallback progressCallback,
                            PluginScanCompletionCallback completionCallback,
                            void* userData);

/**
 * 停止插件扫描
 * @param handle 引擎句柄
 */
void Engine_StopPluginScan(EngineHandle handle);

/**
 * 检查是否正在扫描插件
 * @param handle 引擎句柄
 * @return 正在扫描返回true
 */
bool Engine_IsScanning(EngineHandle handle);

/**
 * 获取可用插件数量
 * @param handle 引擎句柄
 * @return 插件数量
 */
int Engine_GetAvailablePluginCount(EngineHandle handle);

/**
 * 获取可用插件列表
 * @param handle 引擎句柄
 * @param plugins 输出插件信息数组
 * @param maxCount 数组最大容量
 * @return 实际返回的插件数量
 */
int Engine_GetAvailablePlugins(EngineHandle handle,
                              PluginInfo_C* plugins,
                              int maxCount);

/**
 * 获取指定索引的可用插件信息
 * @param handle 引擎句柄
 * @param index 插件索引
 * @param pluginInfo 输出插件信息
 * @return 成功返回true
 */
bool Engine_GetAvailablePluginInfo(EngineHandle handle,
                                  int index,
                                  SimplePluginInfo_C* pluginInfo);

/**
 * 通过标识符加载插件
 * @param handle 引擎句柄
 * @param pluginIdentifier 插件标识符
 * @param callback 加载完成回调
 * @param userData 用户数据
 */
void Engine_LoadPluginByIdentifier(EngineHandle handle,
                                  const char* pluginIdentifier,
                                  PluginLoadCallback callback,
                                  void* userData);

/**
 * 异步加载插件
 * @param handle 引擎句柄
 * @param pluginIdentifier 插件标识符
 * @param callback 加载完成回调
 * @param userData 用户数据
 */
void Engine_LoadPluginAsync(EngineHandle handle,
                           const char* pluginIdentifier,
                           PluginLoadCallback callback,
                           void* userData);

/**
 * 移除插件节点
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_RemoveNode(EngineHandle handle, uint32_t nodeID);

/**
 * 获取已加载节点数量
 * @param handle 引擎句柄
 * @return 节点数量
 */
int Engine_GetLoadedNodeCount(EngineHandle handle);

/**
 * 获取已加载节点列表
 * @param handle 引擎句柄
 * @param nodes 输出节点信息数组
 * @param maxCount 数组最大容量
 * @return 实际返回的节点数量
 */
int Engine_GetLoadedNodes(EngineHandle handle,
                         NodeInfo_C* nodes,
                         int maxCount);

/**
 * 获取指定索引的已加载节点信息
 * @param handle 引擎句柄
 * @param index 节点索引
 * @param nodeInfo 输出节点信息
 * @return 成功返回true
 */
bool Engine_GetLoadedNodeInfo(EngineHandle handle,
                             int index,
                             SimpleNodeInfo_C* nodeInfo);

/**
 * 设置节点旁路状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param bypassed 是否旁路
 * @return 成功返回true
 */
bool Engine_SetNodeBypassed(EngineHandle handle, uint32_t nodeID, bool bypassed);

/**
 * 设置节点启用状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param enabled 是否启用
 * @return 成功返回true
 */
bool Engine_SetNodeEnabled(EngineHandle handle, uint32_t nodeID, bool enabled);

/**
 * 检查节点是否有编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 有编辑器返回true
 */
bool Engine_NodeHasEditor(EngineHandle handle, uint32_t nodeID);

/**
 * 显示节点编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_ShowNodeEditor(EngineHandle handle, uint32_t nodeID);

/**
 * 隐藏节点编辑器
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_HideNodeEditor(EngineHandle handle, uint32_t nodeID);

/**
 * 检查节点编辑器是否可见
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 可见返回true
 */
bool Engine_IsNodeEditorVisible(EngineHandle handle, uint32_t nodeID);

/**
 * 移动节点位置
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param newPosition 新位置索引
 * @return 成功返回true
 */
bool Engine_MoveNode(EngineHandle handle, uint32_t nodeID, int newPosition);

/**
 * 交换两个节点的位置
 * @param handle 引擎句柄
 * @param nodeID1 第一个节点ID
 * @param nodeID2 第二个节点ID
 * @return 成功返回true
 */
bool Engine_SwapNodes(EngineHandle handle, uint32_t nodeID1, uint32_t nodeID2);

/**
 * 创建处理链
 * @param handle 引擎句柄
 * @param nodeIDs 节点ID数组
 * @param count 节点数量
 * @return 成功创建的连接数量
 */
int Engine_CreateProcessingChain(EngineHandle handle, const uint32_t* nodeIDs, int count);

/**
 * 自动连接到输入输出
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool Engine_AutoConnectToIO(EngineHandle handle, uint32_t nodeID);

#ifdef __cplusplus
}
#endif

#endif /* PluginBridge_h */
