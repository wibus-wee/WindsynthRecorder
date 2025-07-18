#include "RealtimeProcessor.hpp"

namespace WindsynthVST {

RealtimeProcessor::RealtimeProcessor() {
    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    
    // 初始化电平平滑器
    inputLevelSmoother.reset(44100.0, 0.1); // 100ms平滑时间
    outputLevelSmoother.reset(44100.0, 0.1);
    
    // 初始化延迟测试信号
    latencyTestSignal.resize(1024);
    for (int i = 0; i < 1024; ++i) {
        latencyTestSignal[i] = random.nextFloat() * 0.1f; // 低电平测试信号
    }
}

RealtimeProcessor::~RealtimeProcessor() {
    stop();
}

void RealtimeProcessor::configure(const RealtimeProcessorConfig& newConfig) {
    juce::ScopedLock sl(configLock);
    config = newConfig;
    
    // 重新配置电平平滑器
    inputLevelSmoother.reset(config.sampleRate, 0.1);
    outputLevelSmoother.reset(config.sampleRate, 0.1);
    
    // 如果正在运行，需要重新启动
    if (running) {
        stop();
        start();
    }
}

bool RealtimeProcessor::initialize() {
    try {
        // 初始化音频设备管理器
        auto result = deviceManager->initialiseWithDefaultDevices(config.numInputChannels, config.numOutputChannels);
        
        if (result.isNotEmpty()) {
            onError("音频设备初始化失败: " + result.toStdString());
            return false;
        }
        
        // 设置音频设备设置
        auto* device = deviceManager->getCurrentAudioDevice();
        if (device) {
            auto setup = device->getActiveInputChannels();
            setup.setRange(0, config.numInputChannels, true);
            
            auto outputSetup = device->getActiveOutputChannels();
            outputSetup.setRange(0, config.numOutputChannels, true);
            
            device->open(setup, outputSetup, config.sampleRate, config.bufferSize);
        }
        
        return true;
    } catch (const std::exception& e) {
        onError("音频设备初始化异常: " + std::string(e.what()));
        return false;
    }
}

bool RealtimeProcessor::start() {
    if (running) {
        return true;
    }
    
    if (!initialize()) {
        return false;
    }
    
    try {
        // 准备音频缓冲区
        inputBuffer.setSize(config.numInputChannels, config.bufferSize);
        outputBuffer.setSize(config.numOutputChannels, config.bufferSize);
        processedBuffer.setSize(config.numOutputChannels, config.bufferSize);
        
        // 准备延迟补偿缓冲区
        if (config.latencyCompensationSamples > 0) {
            delayBuffer.setSize(config.numOutputChannels, config.latencyCompensationSamples);
            delayBuffer.clear();
            delayBufferPosition = 0;
        }
        
        // 准备插件链
        if (processingChain) {
            processingChain->prepareToPlay(config.sampleRate, config.bufferSize);
        }
        
        // 启动音频回调
        deviceManager->addAudioCallback(this);
        
        running = true;
        resetStats();
        
        return true;
    } catch (const std::exception& e) {
        onError("实时处理器启动异常: " + std::string(e.what()));
        return false;
    }
}

void RealtimeProcessor::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // 停止录音
    if (recording) {
        stopRecording();
    }
    
    // 移除音频回调
    deviceManager->removeAudioCallback(this);
    
    // 释放插件链资源
    if (processingChain) {
        processingChain->releaseResources();
    }
    
    // 关闭音频设备
    deviceManager->closeAudioDevice();
}

void RealtimeProcessor::setProcessingChain(std::shared_ptr<AudioProcessingChain> chain) {
    juce::ScopedLock sl(configLock);
    
    // 如果正在运行，先释放旧的链
    if (running && processingChain) {
        processingChain->releaseResources();
    }
    
    processingChain = chain;
    
    // 如果正在运行，准备新的链
    if (running && processingChain) {
        processingChain->prepareToPlay(config.sampleRate, config.bufferSize);
    }
}

void RealtimeProcessor::startRecording(const juce::File& outputFile) {
    juce::ScopedLock sl(recordingLock);
    
    if (recording) {
        stopRecording();
    }
    
    recordingFile = outputFile;
    
    // 创建音频格式写入器
    juce::WavAudioFormat wavFormat;
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordingFile);
    
    if (fileStream->openedOk()) {
        audioWriter.reset(wavFormat.createWriterFor(fileStream.release(),
                                                   config.sampleRate,
                                                   config.numInputChannels,
                                                   24, // 24-bit
                                                   {},
                                                   0));
        
        if (audioWriter) {
            recording = true;
        } else {
            onError("无法创建录音文件写入器");
        }
    } else {
        onError("无法创建录音文件: " + recordingFile.getFullPathName().toStdString());
    }
}

void RealtimeProcessor::stopRecording() {
    juce::ScopedLock sl(recordingLock);
    
    if (recording) {
        recording = false;
        audioWriter.reset();
    }
}

void RealtimeProcessor::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                        int numInputChannels,
                                                        float* const* outputChannelData,
                                                        int numOutputChannels,
                                                        int numSamples,
                                                        const juce::AudioIODeviceCallbackContext& context) {
    auto startTime = juce::Time::getHighResolutionTicks();
    
    try {
        processAudioBlock(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
    } catch (const std::exception& e) {
        onError("音频处理异常: " + std::string(e.what()));
        // 清零输出以避免噪音
        for (int ch = 0; ch < numOutputChannels; ++ch) {
            if (outputChannelData[ch]) {
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
            }
        }
    }
    
    auto endTime = juce::Time::getHighResolutionTicks();
    double processingTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
    updateStats(processingTime);
}

void RealtimeProcessor::processAudioBlock(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples) {
    // 确保缓冲区大小正确
    inputBuffer.setSize(numInputChannels, numSamples, false, false, true);
    outputBuffer.setSize(numOutputChannels, numSamples, false, false, true);
    processedBuffer.setSize(numOutputChannels, numSamples, false, false, true);
    
    // 复制输入数据
    for (int ch = 0; ch < numInputChannels; ++ch) {
        if (inputChannelData[ch]) {
            inputBuffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
        } else {
            inputBuffer.clear(ch, 0, numSamples);
        }
    }
    
    // 复制到处理缓冲区
    for (int ch = 0; ch < std::min(numInputChannels, numOutputChannels); ++ch) {
        processedBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);
    }
    
    // 应用VST插件链处理
    if (processingChain && processingChain->isEnabled()) {
        midiBuffer.clear();
        processingChain->processBlock(processedBuffer, midiBuffer);
    }
    
    // 应用延迟补偿
    if (config.latencyCompensationSamples > 0) {
        applyDelayCompensation(processedBuffer);
    }
    
    // 根据音频路由设置输出
    switch (audioRouting.load()) {
        case AudioRouting::DirectMonitoring:
            // 直接监听输入
            for (int ch = 0; ch < numOutputChannels; ++ch) {
                if (outputChannelData[ch] && ch < numInputChannels) {
                    juce::FloatVectorOperations::copy(outputChannelData[ch], inputBuffer.getReadPointer(ch), numSamples);
                    juce::FloatVectorOperations::multiply(outputChannelData[ch], static_cast<float>(monitoringGain.load()), numSamples);
                }
            }
            break;
            
        case AudioRouting::ProcessedMonitoring:
            // 监听处理后的音频
            if (monitoringEnabled) {
                for (int ch = 0; ch < numOutputChannels; ++ch) {
                    if (outputChannelData[ch] && ch < processedBuffer.getNumChannels()) {
                        juce::FloatVectorOperations::copy(outputChannelData[ch], processedBuffer.getReadPointer(ch), numSamples);
                        juce::FloatVectorOperations::multiply(outputChannelData[ch], static_cast<float>(monitoringGain.load()), numSamples);
                    }
                }
            } else {
                // 静音输出
                for (int ch = 0; ch < numOutputChannels; ++ch) {
                    if (outputChannelData[ch]) {
                        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
                    }
                }
            }
            break;
            
        case AudioRouting::SplitMonitoring:
            // 分离监听
            if (numOutputChannels >= 2) {
                if (outputChannelData[0] && numInputChannels > 0) {
                    juce::FloatVectorOperations::copy(outputChannelData[0], inputBuffer.getReadPointer(0), numSamples);
                    juce::FloatVectorOperations::multiply(outputChannelData[0], static_cast<float>(monitoringGain.load()), numSamples);
                }
                if (outputChannelData[1] && processedBuffer.getNumChannels() > 0) {
                    juce::FloatVectorOperations::copy(outputChannelData[1], processedBuffer.getReadPointer(0), numSamples);
                    juce::FloatVectorOperations::multiply(outputChannelData[1], static_cast<float>(monitoringGain.load()), numSamples);
                }
            }
            break;
    }
    
    // 录音
    if (recordingEnabled && recording) {
        writeToRecording(inputBuffer);
    }
    
    // 更新电平
    updateLevels(inputBuffer, processedBuffer);
    
    // 音频回调
    if (audioCallback) {
        audioCallback(inputBuffer, true);  // 输入
        audioCallback(processedBuffer, false); // 输出
    }
}

void RealtimeProcessor::updateLevels(const juce::AudioBuffer<float>& inputBuf,
                                   const juce::AudioBuffer<float>& outputBuf) {
    // 计算输入电平
    float inputLevel = 0.0f;
    for (int ch = 0; ch < inputBuf.getNumChannels(); ++ch) {
        inputLevel = std::max(inputLevel, inputBuf.getMagnitude(ch, 0, inputBuf.getNumSamples()));
    }
    
    // 计算输出电平
    float outputLevel = 0.0f;
    for (int ch = 0; ch < outputBuf.getNumChannels(); ++ch) {
        outputLevel = std::max(outputLevel, outputBuf.getMagnitude(ch, 0, outputBuf.getNumSamples()));
    }
    
    // 平滑电平值
    inputLevelSmoother.setTargetValue(inputLevel);
    outputLevelSmoother.setTargetValue(outputLevel);
    
    stats.inputLevel = inputLevelSmoother.getNextValue();
    stats.outputLevel = outputLevelSmoother.getNextValue();
    
    // 电平回调
    if (levelCallback) {
        levelCallback(stats.inputLevel, stats.outputLevel);
    }
}

void RealtimeProcessor::updateStats(double processingTime) {
    latencyMeasurements.push_back(processingTime);
    
    // 保持最近100次的测量
    if (latencyMeasurements.size() > 100) {
        latencyMeasurements.erase(latencyMeasurements.begin());
    }
    
    // 计算统计值
    double sum = 0.0;
    double peak = 0.0;
    
    for (double time : latencyMeasurements) {
        sum += time;
        peak = std::max(peak, time);
    }
    
    stats.averageLatency = sum / latencyMeasurements.size();
    stats.peakLatency = peak;
    
    // 计算CPU使用率
    double bufferDuration = (config.bufferSize / config.sampleRate) * 1000.0;
    stats.cpuUsage = (stats.averageLatency / bufferDuration) * 100.0;
}

void RealtimeProcessor::writeToRecording(const juce::AudioBuffer<float>& buffer) {
    juce::ScopedLock sl(recordingLock);
    
    if (audioWriter && recording) {
        audioWriter->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }
}

void RealtimeProcessor::applyDelayCompensation(juce::AudioBuffer<float>& buffer) {
    if (config.latencyCompensationSamples <= 0 || delayBuffer.getNumSamples() == 0) {
        return;
    }
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const int delayBufferSize = delayBuffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto* delayData = delayBuffer.getWritePointer(ch);
        
        for (int i = 0; i < numSamples; ++i) {
            // 读取延迟的样本
            float delayedSample = delayData[delayBufferPosition];
            
            // 写入新样本到延迟缓冲区
            delayData[delayBufferPosition] = channelData[i];
            
            // 输出延迟的样本
            channelData[i] = delayedSample;
            
            // 更新延迟缓冲区位置
            delayBufferPosition = (delayBufferPosition + 1) % delayBufferSize;
        }
    }
}

void RealtimeProcessor::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    // 设备即将启动时的准备工作
    resetStats();
}

void RealtimeProcessor::audioDeviceStopped() {
    // 设备停止时的清理工作
}

void RealtimeProcessor::resetStats() {
    stats = RealtimeStats();
    latencyMeasurements.clear();
}

void RealtimeProcessor::onError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
}

} // namespace WindsynthVST
