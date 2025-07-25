//
//  AudioFileManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频文件管理器
//

#pragma once

#include "../Interfaces/IAudioFileManager.hpp"
#include "../Core/EngineContext.hpp"
#include "../Core/EngineObserver.hpp"
#include <JuceHeader.h>
#include <memory>
#include <atomic>

namespace WindsynthVST::Engine::Managers {

/**
 * 音频文件管理器实现
 * 
 * 负责音频文件的加载、播放控制和时间管理
 * 遵循单一职责原则，只处理音频文件相关的逻辑
 */
class AudioFileManager : public Interfaces::IAudioFileManager {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param context 共享引擎上下文
     * @param notifier 事件通知器
     */
    explicit AudioFileManager(std::shared_ptr<Core::EngineContext> context,
                             std::shared_ptr<Core::EngineNotifier> notifier);
    
    ~AudioFileManager() override;
    
    //==============================================================================
    // IAudioFileManager 接口实现
    //==============================================================================
    
    bool loadAudioFile(const std::string& filePath) override;
    bool play() override;
    void pause() override;
    void stopPlayback() override;
    bool seekTo(double timeInSeconds) override;
    double getCurrentTime() const override;
    double getDuration() const override;
    bool hasAudioFile() const override;
    bool isPlaying() const override;

private:
    //==============================================================================
    // 成员变量
    //==============================================================================
    
    std::shared_ptr<Core::EngineContext> context_;
    std::shared_ptr<Core::EngineNotifier> notifier_;
    
    // 音频文件相关组件
    std::unique_ptr<juce::AudioTransportSource> transportSource_;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
    
    // 状态管理
    std::atomic<bool> hasFile_{false};
    std::atomic<bool> isPlaying_{false};
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void notifyError(const std::string& error);
    void setupTransportSource();
    void cleanupCurrentFile();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileManager)
};

} // namespace WindsynthVST::Engine::Managers
