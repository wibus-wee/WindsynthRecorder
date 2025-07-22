#pragma once

#include <JuceHeader.h>
#include "AudioProcessingChain.hpp"
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

namespace WindsynthVST {

/**
 * 离线处理任务配置
 */
struct OfflineProcessingConfig {
    double sampleRate = 44100.0;
    int bufferSize = 4096; // 较大的缓冲区用于离线处理
    int numChannels = 2;
    bool normalizeOutput = false;
    double outputGain = 1.0;
    bool enableDithering = false;
    int outputBitDepth = 24;
};

/**
 * 处理任务信息
 */
struct ProcessingTask {
    std::string id;
    juce::File inputFile;
    juce::File outputFile;
    OfflineProcessingConfig config;
    std::shared_ptr<AudioProcessingChain> processingChain;
    
    // 任务状态
    enum class Status {
        Pending,
        Processing,
        Completed,
        Failed,
        Cancelled
    };
    
    std::atomic<Status> status{Status::Pending};
    std::atomic<double> progress{0.0};
    std::string errorMessage;
    
    ProcessingTask(const std::string& taskId,
                  const juce::File& input,
                  const juce::File& output,
                  const OfflineProcessingConfig& cfg,
                  std::shared_ptr<AudioProcessingChain> chain)
        : id(taskId), inputFile(input), outputFile(output), config(cfg), processingChain(chain) {}
};

/**
 * 离线音频处理器
 * 提供高质量的离线音频处理，支持批量处理和进度监控
 */
class OfflineProcessor {
public:
    OfflineProcessor();
    ~OfflineProcessor();
    
    // 任务管理
    std::string addTask(const juce::File& inputFile,
                       const juce::File& outputFile,
                       const OfflineProcessingConfig& config,
                       std::shared_ptr<AudioProcessingChain> processingChain);
    
    bool removeTask(const std::string& taskId);
    void clearTasks();
    
    // 处理控制
    void startProcessing();
    void stopProcessing();
    void pauseProcessing();
    void resumeProcessing();
    
    bool isProcessing() const { return processing; }
    bool isPaused() const { return paused; }
    
    // 任务查询
    std::vector<std::string> getTaskIds() const;
    std::shared_ptr<ProcessingTask> getTask(const std::string& taskId) const;
    ProcessingTask::Status getTaskStatus(const std::string& taskId) const;
    double getTaskProgress(const std::string& taskId) const;
    
    // 整体进度
    double getOverallProgress() const;
    int getCompletedTaskCount() const;
    int getTotalTaskCount() const;
    
    // 批量处理
    std::vector<std::string> addBatchTasks(const std::vector<juce::File>& inputFiles,
                                          const juce::File& outputDirectory,
                                          const std::string& outputFormat,
                                          const OfflineProcessingConfig& config,
                                          std::shared_ptr<AudioProcessingChain> processingChain);
    
    // 性能设置
    void setMaxConcurrentTasks(int maxTasks) { maxConcurrentTasks = maxTasks; }
    int getMaxConcurrentTasks() const { return maxConcurrentTasks; }
    
    void setProcessingPriority(juce::Thread::Priority priority) { processingPriority = priority; }
    juce::Thread::Priority getProcessingPriority() const { return processingPriority; }
    
    // 回调设置
    using ProgressCallback = std::function<void(const std::string& taskId, double progress)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback = callback; }
    
    using CompletionCallback = std::function<void(const std::string& taskId, bool success, const std::string& error)>;
    void setCompletionCallback(CompletionCallback callback) { completionCallback = callback; }
    
    using ErrorCallback = std::function<void(const std::string& error)>;
    void setErrorCallback(ErrorCallback callback) { errorCallback = callback; }
    
    // 质量设置
    struct QualitySettings {
        bool useHighQualityResampling = true;
        bool enableAntiAliasing = true;
        int oversamplingFactor = 1; // 1, 2, 4, 8
        bool enableDithering = false;
        juce::AudioProcessor::ProcessingPrecision precision = juce::AudioProcessor::singlePrecision;
    };
    
    void setQualitySettings(const QualitySettings& settings) { qualitySettings = settings; }
    const QualitySettings& getQualitySettings() const { return qualitySettings; }
    
    // 统计信息
    struct ProcessingStats {
        int totalTasksProcessed = 0;
        int successfulTasks = 0;
        int failedTasks = 0;
        double totalProcessingTime = 0.0;
        double averageProcessingSpeed = 0.0; // 相对于实时的倍数
    };
    
    const ProcessingStats& getStats() const { return stats; }
    void resetStats();
    
private:
    // 任务存储
    std::vector<std::shared_ptr<ProcessingTask>> tasks;
    mutable juce::CriticalSection tasksLock;
    
    // 处理状态
    std::atomic<bool> processing{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> shouldStop{false};
    
    // 处理线程
    std::unique_ptr<juce::ThreadPool> threadPool;
    int maxConcurrentTasks = 2;
    juce::Thread::Priority processingPriority = juce::Thread::Priority::normal;
    
    // 回调函数
    ProgressCallback progressCallback;
    CompletionCallback completionCallback;
    ErrorCallback errorCallback;
    
    // 质量设置
    QualitySettings qualitySettings;
    
    // 统计信息
    ProcessingStats stats;
    
    // 内部处理类
    class ProcessingJob : public juce::ThreadPoolJob {
    public:
        ProcessingJob(OfflineProcessor* processor, std::shared_ptr<ProcessingTask> task);
        juce::ThreadPoolJob::JobStatus runJob() override;
        
    private:
        OfflineProcessor* processor;
        std::shared_ptr<ProcessingTask> task;
    };
    
    // 内部方法
    bool processTask(std::shared_ptr<ProcessingTask> task);
    bool processAudioFile(const juce::File& inputFile,
                         const juce::File& outputFile,
                         const OfflineProcessingConfig& config,
                         std::shared_ptr<AudioProcessingChain> processingChain,
                         std::function<void(double)> progressCallback);
    
    std::unique_ptr<juce::AudioFormatReader> createReader(const juce::File& file);
    std::unique_ptr<juce::AudioFormatWriter> createWriter(const juce::File& file,
                                                         const OfflineProcessingConfig& config,
                                                         double sampleRate,
                                                         int numChannels);
    
    void applyNormalization(juce::AudioBuffer<float>& buffer, double targetLevel);
    void applyDithering(juce::AudioBuffer<float>& buffer, int targetBitDepth);
    void applyOversampling(juce::AudioBuffer<float>& buffer, int factor);
    
    void onTaskProgress(const std::string& taskId, double progress);
    void onTaskCompleted(const std::string& taskId, bool success, const std::string& error);
    void onError(const std::string& error);
    void checkAllTasksCompleted();

    std::string generateTaskId();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OfflineProcessor)
};

} // namespace WindsynthVST
