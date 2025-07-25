//
//  AudioFileBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频文件管理桥接层 - 专门处理音频文件相关功能
//

#ifndef AudioFileBridge_h
#define AudioFileBridge_h

#include "EngineBridge.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// 音频文件处理
//==============================================================================

/**
 * 加载音频文件
 * @param handle 引擎句柄
 * @param filePath 文件路径
 * @return 成功返回true
 */
bool Engine_LoadAudioFile(EngineHandle handle, const char* filePath);

/**
 * 开始播放
 * @param handle 引擎句柄
 * @return 成功返回true
 */
bool Engine_Play(EngineHandle handle);

/**
 * 暂停播放
 * @param handle 引擎句柄
 */
void Engine_Pause(EngineHandle handle);

/**
 * 停止播放
 * @param handle 引擎句柄
 */
void Engine_StopPlayback(EngineHandle handle);

/**
 * 跳转到指定时间
 * @param handle 引擎句柄
 * @param timeInSeconds 时间（秒）
 * @return 成功返回true
 */
bool Engine_SeekTo(EngineHandle handle, double timeInSeconds);

/**
 * 获取当前播放时间
 * @param handle 引擎句柄
 * @return 当前时间（秒）
 */
double Engine_GetCurrentTime(EngineHandle handle);

/**
 * 获取音频文件总时长
 * @param handle 引擎句柄
 * @return 总时长（秒）
 */
double Engine_GetDuration(EngineHandle handle);

/**
 * 检查是否有音频文件加载
 * @param handle 引擎句柄
 * @return 有文件加载返回true
 */
bool Engine_HasAudioFile(EngineHandle handle);

/**
 * 检查是否正在播放
 * @param handle 引擎句柄
 * @return 正在播放返回true
 */
bool Engine_IsPlaying(EngineHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* AudioFileBridge_h */
