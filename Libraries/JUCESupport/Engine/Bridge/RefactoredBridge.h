//
//  RefactoredBridge.h
//  WindsynthRecorder
//
//  Created by AI Assistant
//  重构后的统一桥接层头文件 - 包含所有模块化桥接层
//

#ifndef RefactoredBridge_h
#define RefactoredBridge_h

// 包含所有模块化桥接层
#include "RefactoredEngineBridge.h"
#include "AudioFileBridge.h"
#include "PluginBridge.h"
#include "ParameterBridge.h"

/**
 * 重构后的桥接层设计说明
 * 
 * 本桥接层基于重构后的 RefactoredWindsynthEngineFacade 设计，
 * 遵循以下设计原则：
 * 
 * 1. 单一职责原则 (SRP)：
 *    - RefactoredEngineBridge.h: 核心引擎生命周期管理
 *    - AudioFileBridge.h: 音频文件处理
 *    - PluginBridge.h: 插件管理
 *    - ParameterBridge.h: 参数控制
 * 
 * 2. 模块化设计：
 *    - 每个桥接层模块都专注于特定功能
 *    - 可以独立使用和测试
 *    - 便于维护和扩展
 * 
 * 3. C 兼容性：
 *    - 所有接口都是 C 兼容的
 *    - 使用不透明指针隐藏 C++ 实现细节
 *    - 提供清晰的错误处理机制
 * 
 * 4. 向后兼容性：
 *    - 保持与 Swift 层的接口兼容
 *    - 提供平滑的迁移路径
 * 
 * 使用示例：
 * 
 * ```c
 * // 创建和初始化引擎
 * RefactoredEngineHandle engine = RefactoredEngine_Create();
 * RefactoredEngineConfig config = {44100.0, 512, 0, 2, true, ""};
 * RefactoredEngine_Initialize(engine, &config);
 * RefactoredEngine_Start(engine);
 * 
 * // 加载和播放音频文件
 * RefactoredEngine_LoadAudioFile(engine, "/path/to/audio.wav");
 * RefactoredEngine_Play(engine);
 * 
 * // 加载插件
 * RefactoredEngine_LoadPluginAsync(engine, "plugin_id", "My Plugin", callback, userData);
 * 
 * // 控制参数
 * RefactoredEngine_SetNodeParameter(engine, nodeID, 0, 0.5f);
 * 
 * // 清理
 * RefactoredEngine_Shutdown(engine);
 * RefactoredEngine_Destroy(engine);
 * ```
 * 
 * 优势：
 * - 代码更简洁（每个模块约50-100行）
 * - 职责明确，易于维护
 * - 可以独立测试每个模块
 * - 便于添加新功能
 * - 更好的错误隔离
 */

#endif /* RefactoredBridge_h */
