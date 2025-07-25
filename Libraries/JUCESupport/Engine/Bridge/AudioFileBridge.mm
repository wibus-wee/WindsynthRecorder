//
//  AudioFileBridge.mm
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频文件管理桥接层实现
//

#include "AudioFileBridge.h"
#include "BridgeInternal.h"
#include <iostream>

//==============================================================================
// 音频文件处理实现
//==============================================================================

bool Engine_LoadAudioFile(EngineHandle handle, const char* filePath) {
    if (!handle || !filePath) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->loadAudioFile(std::string(filePath));
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 加载音频文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_Play(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->play();
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 播放失败: " << e.what() << std::endl;
        return false;
    }
}

void Engine_Pause(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->pause();
        }
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 暂停失败: " << e.what() << std::endl;
    }
}

void Engine_StopPlayback(EngineHandle handle) {
    if (!handle) return;

    try {
        auto context = getContext(handle);
        if (context->engine) {
            context->engine->stopPlayback();
        }
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 停止播放失败: " << e.what() << std::endl;
    }
}

bool Engine_SeekTo(EngineHandle handle, double timeInSeconds) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        return context->engine->seekTo(timeInSeconds);
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 跳转失败: " << e.what() << std::endl;
        return false;
    }
}

double Engine_GetCurrentTime(EngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        return context->engine->getCurrentTime();
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 获取当前时间失败: " << e.what() << std::endl;
        return 0.0;
    }
}

double Engine_GetDuration(EngineHandle handle) {
    if (!handle) return 0.0;

    try {
        auto context = getContext(handle);
        if (!context->engine) return 0.0;

        return context->engine->getDuration();
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 获取时长失败: " << e.what() << std::endl;
        return 0.0;
    }
}

bool Engine_HasAudioFile(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto audioManager = context->engine->getAudioFileManager();
        return audioManager ? audioManager->hasAudioFile() : false;
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 检查音频文件状态失败: " << e.what() << std::endl;
        return false;
    }
}

bool Engine_IsPlaying(EngineHandle handle) {
    if (!handle) return false;

    try {
        auto context = getContext(handle);
        if (!context->engine) return false;

        auto audioManager = context->engine->getAudioFileManager();
        return audioManager ? audioManager->isPlaying() : false;
    } catch (const std::exception& e) {
        std::cerr << "[AudioFileBridge] 检查播放状态失败: " << e.what() << std::endl;
        return false;
    }
}
