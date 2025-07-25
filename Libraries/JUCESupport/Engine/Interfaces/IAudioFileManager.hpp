//
//  IAudioFileManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频文件管理接口
//

#pragma once

#include <string>

namespace WindsynthVST::Engine::Interfaces {

/**
 * 音频文件管理接口
 * 
 * 负责音频文件的加载、播放控制和时间管理
 */
class IAudioFileManager {
public:
    virtual ~IAudioFileManager() = default;
    
    /**
     * 加载音频文件
     * @param filePath 文件路径
     * @return 成功返回true
     */
    virtual bool loadAudioFile(const std::string& filePath) = 0;
    
    /**
     * 开始播放
     * @return 成功返回true
     */
    virtual bool play() = 0;
    
    /**
     * 暂停播放
     */
    virtual void pause() = 0;
    
    /**
     * 停止播放
     */
    virtual void stopPlayback() = 0;
    
    /**
     * 跳转到指定时间
     * @param timeInSeconds 时间（秒）
     * @return 成功返回true
     */
    virtual bool seekTo(double timeInSeconds) = 0;
    
    /**
     * 获取当前播放时间
     * @return 当前时间（秒）
     */
    virtual double getCurrentTime() const = 0;
    
    /**
     * 获取音频文件总时长
     * @return 总时长（秒）
     */
    virtual double getDuration() const = 0;
    
    /**
     * 检查是否有音频文件加载
     * @return 有文件加载返回true
     */
    virtual bool hasAudioFile() const = 0;
    
    /**
     * 检查是否正在播放
     * @return 正在播放返回true
     */
    virtual bool isPlaying() const = 0;
};

} // namespace WindsynthVST::Engine::Interfaces
