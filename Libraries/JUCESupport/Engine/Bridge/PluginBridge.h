//
//  PluginBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理桥接层 - 专门处理插件相关功能
//

#ifndef PluginBridge_h
#define PluginBridge_h

#include "RefactoredEngineBridge.h"

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
} RefactoredPluginInfo;

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
} RefactoredNodeInfo;

/**
 * 插件加载回调函数类型
 */
typedef void (*RefactoredPluginLoadCallback)(uint32_t nodeID, bool success, const char* error, void* userData);

//==============================================================================
// 插件管理
//==============================================================================

/**
 * 获取可用插件数量
 * @param handle 引擎句柄
 * @return 插件数量
 */
int RefactoredEngine_GetAvailablePluginCount(RefactoredEngineHandle handle);

/**
 * 获取可用插件列表
 * @param handle 引擎句柄
 * @param plugins 输出插件信息数组
 * @param maxCount 数组最大容量
 * @return 实际返回的插件数量
 */
int RefactoredEngine_GetAvailablePlugins(RefactoredEngineHandle handle, 
                                        RefactoredPluginInfo* plugins, 
                                        int maxCount);

/**
 * 异步加载插件
 * @param handle 引擎句柄
 * @param pluginIdentifier 插件标识符
 * @param displayName 显示名称（可选，传NULL使用默认名称）
 * @param callback 加载完成回调
 * @param userData 用户数据
 */
void RefactoredEngine_LoadPluginAsync(RefactoredEngineHandle handle,
                                     const char* pluginIdentifier,
                                     const char* displayName,
                                     RefactoredPluginLoadCallback callback,
                                     void* userData);

/**
 * 移除插件节点
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @return 成功返回true
 */
bool RefactoredEngine_RemoveNode(RefactoredEngineHandle handle, uint32_t nodeID);

/**
 * 获取已加载节点数量
 * @param handle 引擎句柄
 * @return 节点数量
 */
int RefactoredEngine_GetLoadedNodeCount(RefactoredEngineHandle handle);

/**
 * 获取已加载节点列表
 * @param handle 引擎句柄
 * @param nodes 输出节点信息数组
 * @param maxCount 数组最大容量
 * @return 实际返回的节点数量
 */
int RefactoredEngine_GetLoadedNodes(RefactoredEngineHandle handle, 
                                   RefactoredNodeInfo* nodes, 
                                   int maxCount);

/**
 * 设置节点旁路状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param bypassed 是否旁路
 * @return 成功返回true
 */
bool RefactoredEngine_SetNodeBypassed(RefactoredEngineHandle handle, uint32_t nodeID, bool bypassed);

/**
 * 设置节点启用状态
 * @param handle 引擎句柄
 * @param nodeID 节点ID
 * @param enabled 是否启用
 * @return 成功返回true
 */
bool RefactoredEngine_SetNodeEnabled(RefactoredEngineHandle handle, uint32_t nodeID, bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* PluginBridge_h */
