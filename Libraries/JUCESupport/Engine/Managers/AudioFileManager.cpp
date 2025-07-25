//
//  AudioFileManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频文件管理器实现
//

#include "AudioFileManager.hpp"
#include <iostream>

namespace WindsynthVST::Engine::Managers {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

AudioFileManager::AudioFileManager(std::shared_ptr<Core::EngineContext> context,
                                 std::shared_ptr<Core::EngineNotifier> notifier)
    : context_(std::move(context))
    , notifier_(std::move(notifier)) {
    std::cout << "[AudioFileManager] 构造函数" << std::endl;
    setupTransportSource();
}

AudioFileManager::~AudioFileManager() {
    std::cout << "[AudioFileManager] 析构函数" << std::endl;
    cleanupCurrentFile();
}

//==============================================================================
// IAudioFileManager 接口实现
//==============================================================================

bool AudioFileManager::loadAudioFile(const std::string& filePath) {
    std::cout << "[AudioFileManager] 加载音频文件: " << filePath << std::endl;
    
    if (!context_ || !context_->isInitialized()) {
        notifyError("引擎上下文未初始化");
        return false;
    }
    
    try {
        juce::File audioFile(filePath);
        if (!audioFile.existsAsFile()) {
            notifyError("音频文件不存在: " + filePath);
            return false;
        }
        
        // 停止当前播放
        if (transportSource_) {
            transportSource_->stop();
            isPlaying_.store(false);
        }
        
        // 清理当前文件
        cleanupCurrentFile();
        
        // 创建音频格式读取器
        auto formatManager = context_->getFormatManager();
        if (!formatManager) {
            notifyError("音频格式管理器无效");
            return false;
        }
        
        auto reader = formatManager->createReaderFor(audioFile);
        if (!reader) {
            notifyError("无法读取音频文件: " + filePath);
            return false;
        }
        
        // 创建新的音频格式读取器源
        readerSource_ = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        
        // 设置新源到传输源
        if (transportSource_) {
            transportSource_->setSource(readerSource_.get(), 0, nullptr, reader->sampleRate);
        }
        
        // 将transportSource设置到GraphAudioProcessor中
        auto graphProcessor = context_->getGraphProcessor();
        if (graphProcessor) {
            graphProcessor->setTransportSource(transportSource_.get());
        }
        
        hasFile_.store(true);
        std::cout << "[AudioFileManager] 音频文件加载成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "加载音频文件失败: " + std::string(e.what());
        notifyError(error);
        return false;
    }
}

bool AudioFileManager::play() {
    std::cout << "[AudioFileManager] 开始播放" << std::endl;
    
    if (!hasFile_.load()) {
        notifyError("没有加载音频文件");
        return false;
    }
    
    if (!transportSource_) {
        notifyError("传输源无效");
        return false;
    }
    
    try {
        transportSource_->start();
        isPlaying_.store(true);
        return true;
    } catch (const std::exception& e) {
        std::string error = "播放失败: " + std::string(e.what());
        notifyError(error);
        return false;
    }
}

void AudioFileManager::pause() {
    std::cout << "[AudioFileManager] 暂停播放" << std::endl;
    
    if (transportSource_) {
        transportSource_->stop();
        isPlaying_.store(false);
    }
}

void AudioFileManager::stopPlayback() {
    std::cout << "[AudioFileManager] 停止播放" << std::endl;
    
    if (transportSource_) {
        transportSource_->stop();
        transportSource_->setPosition(0.0);
        isPlaying_.store(false);
    }
}

bool AudioFileManager::seekTo(double timeInSeconds) {
    if (!transportSource_ || !hasFile_.load()) {
        return false;
    }
    
    try {
        transportSource_->setPosition(timeInSeconds);
        return true;
    } catch (const std::exception& e) {
        notifyError("跳转失败: " + std::string(e.what()));
        return false;
    }
}

double AudioFileManager::getCurrentTime() const {
    if (!transportSource_ || !hasFile_.load()) {
        return 0.0;
    }
    
    return transportSource_->getCurrentPosition();
}

double AudioFileManager::getDuration() const {
    if (!transportSource_ || !hasFile_.load()) {
        return 0.0;
    }
    
    return transportSource_->getLengthInSeconds();
}

bool AudioFileManager::hasAudioFile() const {
    return hasFile_.load();
}

bool AudioFileManager::isPlaying() const {
    return isPlaying_.load();
}

//==============================================================================
// 内部方法
//==============================================================================

void AudioFileManager::notifyError(const std::string& error) {
    if (notifier_) {
        notifier_->notifyError(error);
    }
    std::cerr << "[AudioFileManager] 错误: " << error << std::endl;
}

void AudioFileManager::setupTransportSource() {
    transportSource_ = std::make_unique<juce::AudioTransportSource>();
}

void AudioFileManager::cleanupCurrentFile() {
    if (transportSource_) {
        transportSource_->setSource(nullptr);
    }
    
    readerSource_.reset();
    hasFile_.store(false);
    isPlaying_.store(false);
}

} // namespace WindsynthVST::Engine::Managers
