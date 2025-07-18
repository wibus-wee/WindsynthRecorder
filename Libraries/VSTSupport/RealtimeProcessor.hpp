#pragma once

#include <JuceHeader.h>
#include "AudioProcessingChain.hpp"
#include <memory>
#include <atomic>
#include <functional>

namespace WindsynthVST {

/**
 * 实时音频处理器配置
 */
struct RealtimeProcessorConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
    int numInputChannels = 2;
    int numOutputChannels = 2;
    bool enableMonitoring = true;
    bool enableRecording = true;
    double monitoringGain = 1.0;
    int latencyCompensationSamples = 0;
};

/**
 * 实时音频处理器
 * 集成AVAudioEngine和VST插件链，提供实时音频处理和监听
 */
class RealtimeProcessor : public juce::AudioIODeviceCallback {
public:
    RealtimeProcessor();
    ~RealtimeProcessor();
    
    // 配置
    void configure(const RealtimeProcessorConfig& config);
    const RealtimeProcessorConfig& getConfig() const { return config; }
    
    // 音频设备管理
    bool initialize();
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    // 插件链管理
    void setProcessingChain(std::shared_ptr<AudioProcessingChain> chain);
    std::shared_ptr<AudioProcessingChain> getProcessingChain() const { return processingChain; }
    
    // 监听控制
    void setMonitoringEnabled(bool enabled) { monitoringEnabled = enabled; }
    bool isMonitoringEnabled() const { return monitoringEnabled; }
    
    void setMonitoringGain(double gain) { monitoringGain = gain; }
    double getMonitoringGain() const { return monitoringGain; }
    
    // 录音控制
    void setRecordingEnabled(bool enabled) { recordingEnabled = enabled; }
    bool isRecordingEnabled() const { return recordingEnabled; }
    
    void startRecording(const juce::File& outputFile);
    void stopRecording();
    bool isRecording() const { return recording; }
    
    // 音频路由
    enum class AudioRouting {
        DirectMonitoring,    // 直接监听输入
        ProcessedMonitoring, // 监听处理后的音频
        SplitMonitoring     // 分离监听（左声道原始，右声道处理后）
    };
    
    void setAudioRouting(AudioRouting routing) { audioRouting = routing; }
    AudioRouting getAudioRouting() const { return audioRouting; }
    
    // 性能监控
    struct RealtimeStats {
        double averageLatency = 0.0;
        double peakLatency = 0.0;
        double cpuUsage = 0.0;
        int bufferUnderruns = 0;
        int bufferOverruns = 0;
        double inputLevel = 0.0;
        double outputLevel = 0.0;
    };
    
    const RealtimeStats& getStats() const { return stats; }
    void resetStats();
    
    // 回调设置
    using AudioCallback = std::function<void(const juce::AudioBuffer<float>&, bool isInput)>;
    void setAudioCallback(AudioCallback callback) { audioCallback = callback; }
    
    using ErrorCallback = std::function<void(const std::string& error)>;
    void setErrorCallback(ErrorCallback callback) { errorCallback = callback; }
    
    using LevelCallback = std::function<void(double inputLevel, double outputLevel)>;
    void setLevelCallback(LevelCallback callback) { levelCallback = callback; }
    
    // AudioIODeviceCallback 实现
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // 延迟测量
    void measureLatency();
    double getMeasuredLatency() const { return measuredLatency; }
    
private:
    RealtimeProcessorConfig config;
    
    // 音频设备管理
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    std::atomic<bool> running{false};
    
    // 插件链
    std::shared_ptr<AudioProcessingChain> processingChain;
    
    // 控制标志
    std::atomic<bool> monitoringEnabled{true};
    std::atomic<bool> recordingEnabled{true};
    std::atomic<bool> recording{false};
    std::atomic<double> monitoringGain{1.0};
    std::atomic<AudioRouting> audioRouting{AudioRouting::ProcessedMonitoring};
    
    // 录音
    std::unique_ptr<juce::AudioFormatWriter> audioWriter;
    juce::File recordingFile;
    juce::CriticalSection recordingLock;
    
    // 音频缓冲区
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;
    juce::AudioBuffer<float> processedBuffer;
    juce::MidiBuffer midiBuffer;
    
    // 延迟补偿
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferPosition = 0;
    
    // 性能统计
    RealtimeStats stats;
    std::vector<double> latencyMeasurements;
    juce::Time lastCallbackTime;
    
    // 电平检测
    juce::LinearSmoothedValue<float> inputLevelSmoother;
    juce::LinearSmoothedValue<float> outputLevelSmoother;
    
    // 回调函数
    AudioCallback audioCallback;
    ErrorCallback errorCallback;
    LevelCallback levelCallback;
    
    // 延迟测量
    std::atomic<double> measuredLatency{0.0};
    juce::Random random;
    std::vector<float> latencyTestSignal;
    int latencyTestPosition = 0;
    bool latencyTestActive = false;
    
    // 内部方法
    void processAudioBlock(const float* const* inputChannelData,
                          int numInputChannels,
                          float* const* outputChannelData,
                          int numOutputChannels,
                          int numSamples);
    
    void updateLevels(const juce::AudioBuffer<float>& inputBuffer,
                     const juce::AudioBuffer<float>& outputBuffer);
    
    void updateStats(double processingTime);
    void writeToRecording(const juce::AudioBuffer<float>& buffer);
    void applyDelayCompensation(juce::AudioBuffer<float>& buffer);
    void onError(const std::string& error);
    
    // 线程安全
    juce::CriticalSection configLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeProcessor)
};

} // namespace WindsynthVST
