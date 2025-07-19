#include "OfflineProcessor.hpp"
#include <random>

namespace WindsynthVST {

OfflineProcessor::OfflineProcessor() {
    threadPool = std::make_unique<juce::ThreadPool>(maxConcurrentTasks);
}

OfflineProcessor::~OfflineProcessor() {
    stopProcessing();
    threadPool.reset();
}

std::string OfflineProcessor::addTask(const juce::File& inputFile,
                                    const juce::File& outputFile,
                                    const OfflineProcessingConfig& config,
                                    std::shared_ptr<AudioProcessingChain> processingChain) {
    juce::ScopedLock sl(tasksLock);
    
    std::string taskId = generateTaskId();
    auto task = std::make_shared<ProcessingTask>(taskId, inputFile, outputFile, config, processingChain);
    tasks.push_back(task);
    
    return taskId;
}

bool OfflineProcessor::removeTask(const std::string& taskId) {
    juce::ScopedLock sl(tasksLock);
    
    auto it = std::find_if(tasks.begin(), tasks.end(),
                          [&taskId](const std::shared_ptr<ProcessingTask>& task) {
                              return task->id == taskId;
                          });
    
    if (it != tasks.end()) {
        // 如果任务正在处理中，标记为取消
        if ((*it)->status == ProcessingTask::Status::Processing) {
            (*it)->status = ProcessingTask::Status::Cancelled;
        } else {
            tasks.erase(it);
        }
        return true;
    }
    
    return false;
}

void OfflineProcessor::clearTasks() {
    juce::ScopedLock sl(tasksLock);
    
    // 取消所有正在处理的任务
    for (auto& task : tasks) {
        if (task->status == ProcessingTask::Status::Processing) {
            task->status = ProcessingTask::Status::Cancelled;
        }
    }
    
    // 清除所有未开始的任务
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                              [](const std::shared_ptr<ProcessingTask>& task) {
                                  return task->status == ProcessingTask::Status::Pending;
                              }), tasks.end());
}

void OfflineProcessor::startProcessing() {
    if (processing) {
        return;
    }
    
    processing = true;
    paused = false;
    shouldStop = false;
    
    // 重新创建线程池
    threadPool = std::make_unique<juce::ThreadPool>(maxConcurrentTasks);
    
    // 提交所有待处理的任务
    {
        juce::ScopedLock sl(tasksLock);
        for (auto& task : tasks) {
            if (task->status == ProcessingTask::Status::Pending) {
                auto job = new ProcessingJob(this, task);
                threadPool->addJob(job, true); // 删除完成的作业
            }
        }
    }
}

void OfflineProcessor::stopProcessing() {
    if (!processing) {
        return;
    }
    
    shouldStop = true;
    processing = false;
    paused = false;
    
    // 停止线程池
    if (threadPool) {
        threadPool->removeAllJobs(true, 5000); // 等待5秒
    }
    
    // 取消所有正在处理的任务
    {
        juce::ScopedLock sl(tasksLock);
        for (auto& task : tasks) {
            if (task->status == ProcessingTask::Status::Processing) {
                task->status = ProcessingTask::Status::Cancelled;
            }
        }
    }
}

void OfflineProcessor::pauseProcessing() {
    paused = true;
    if (threadPool) {
        // 暂停所有作业（JUCE的ThreadPool没有直接的暂停功能，这里只是设置标志）
    }
}

void OfflineProcessor::resumeProcessing() {
    paused = false;
}

std::vector<std::string> OfflineProcessor::getTaskIds() const {
    juce::ScopedLock sl(tasksLock);
    
    std::vector<std::string> ids;
    for (const auto& task : tasks) {
        ids.push_back(task->id);
    }
    
    return ids;
}

std::shared_ptr<ProcessingTask> OfflineProcessor::getTask(const std::string& taskId) const {
    juce::ScopedLock sl(tasksLock);
    
    auto it = std::find_if(tasks.begin(), tasks.end(),
                          [&taskId](const std::shared_ptr<ProcessingTask>& task) {
                              return task->id == taskId;
                          });
    
    return (it != tasks.end()) ? *it : nullptr;
}

ProcessingTask::Status OfflineProcessor::getTaskStatus(const std::string& taskId) const {
    auto task = getTask(taskId);
    return task ? task->status.load() : ProcessingTask::Status::Failed;
}

double OfflineProcessor::getTaskProgress(const std::string& taskId) const {
    auto task = getTask(taskId);
    return task ? task->progress.load() : 0.0;
}

double OfflineProcessor::getOverallProgress() const {
    juce::ScopedLock sl(tasksLock);
    
    if (tasks.empty()) {
        return 1.0;
    }
    
    double totalProgress = 0.0;
    for (const auto& task : tasks) {
        totalProgress += task->progress.load();
    }
    
    return totalProgress / tasks.size();
}

int OfflineProcessor::getCompletedTaskCount() const {
    juce::ScopedLock sl(tasksLock);
    
    return static_cast<int>(std::count_if(tasks.begin(), tasks.end(),
                                        [](const std::shared_ptr<ProcessingTask>& task) {
                                            return task->status == ProcessingTask::Status::Completed;
                                        }));
}

int OfflineProcessor::getTotalTaskCount() const {
    juce::ScopedLock sl(tasksLock);
    return static_cast<int>(tasks.size());
}

std::vector<std::string> OfflineProcessor::addBatchTasks(const std::vector<juce::File>& inputFiles,
                                                        const juce::File& outputDirectory,
                                                        const std::string& outputFormat,
                                                        const OfflineProcessingConfig& config,
                                                        std::shared_ptr<AudioProcessingChain> processingChain) {
    std::vector<std::string> taskIds;
    
    for (const auto& inputFile : inputFiles) {
        if (inputFile.existsAsFile()) {
            juce::String outputFileName = inputFile.getFileNameWithoutExtension() + "_processed." + outputFormat;
            juce::File outputFile = outputDirectory.getChildFile(outputFileName);
            
            std::string taskId = addTask(inputFile, outputFile, config, processingChain);
            taskIds.push_back(taskId);
        }
    }
    
    return taskIds;
}

void OfflineProcessor::resetStats() {
    stats = ProcessingStats();
}

std::string OfflineProcessor::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string id = "task_";
    for (int i = 0; i < 8; ++i) {
        id += "0123456789ABCDEF"[dis(gen)];
    }
    
    return id;
}

// ProcessingJob 实现
OfflineProcessor::ProcessingJob::ProcessingJob(OfflineProcessor* processor, std::shared_ptr<ProcessingTask> task)
    : juce::ThreadPoolJob("ProcessingJob_" + task->id), processor(processor), task(task) {
}

juce::ThreadPoolJob::JobStatus OfflineProcessor::ProcessingJob::runJob() {
    if (!processor || !task) {
        return juce::ThreadPoolJob::jobHasFinished;
    }
    
    // 检查是否应该停止
    if (processor->shouldStop || shouldExit()) {
        task->status = ProcessingTask::Status::Cancelled;
        return juce::ThreadPoolJob::jobHasFinished;
    }
    
    // 等待暂停结束
    while (processor->paused && !processor->shouldStop && !shouldExit()) {
        juce::Thread::sleep(100);
    }
    
    if (processor->shouldStop || shouldExit()) {
        task->status = ProcessingTask::Status::Cancelled;
        return juce::ThreadPoolJob::jobHasFinished;
    }
    
    // 开始处理
    task->status = ProcessingTask::Status::Processing;
    task->progress = 0.0;
    
    auto startTime = juce::Time::getMillisecondCounter();
    bool success = processor->processTask(task);
    auto endTime = juce::Time::getMillisecondCounter();
    
    // 更新统计信息
    processor->stats.totalTasksProcessed++;
    processor->stats.totalProcessingTime += (endTime - startTime) / 1000.0;
    
    if (success) {
        task->status = ProcessingTask::Status::Completed;
        task->progress = 1.0;
        processor->stats.successfulTasks++;
        processor->onTaskCompleted(task->id, true, "");
    } else {
        task->status = ProcessingTask::Status::Failed;
        processor->stats.failedTasks++;
        processor->onTaskCompleted(task->id, false, task->errorMessage);
    }
    
    return juce::ThreadPoolJob::jobHasFinished;
}

bool OfflineProcessor::processTask(std::shared_ptr<ProcessingTask> task) {
    try {
        return processAudioFile(task->inputFile, task->outputFile, task->config, task->processingChain,
                              [this, task](double progress) {
                                  task->progress = progress;
                                  onTaskProgress(task->id, progress);
                              });
    } catch (const std::exception& e) {
        task->errorMessage = e.what();
        onError("任务处理异常: " + task->id + " - " + e.what());
        return false;
    }
}

bool OfflineProcessor::processAudioFile(const juce::File& inputFile,
                                      const juce::File& outputFile,
                                      const OfflineProcessingConfig& config,
                                      std::shared_ptr<AudioProcessingChain> processingChain,
                                      std::function<void(double)> progressCallback) {
    // 创建音频格式管理器
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // 创建读取器
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(inputFile));
    if (!reader) {
        onError("无法读取音频文件: " + inputFile.getFullPathName().toStdString());
        return false;
    }
    
    // 创建写入器
    auto writer = createWriter(outputFile, config, reader->sampleRate, static_cast<int>(reader->numChannels));
    if (!writer) {
        onError("无法创建输出文件: " + outputFile.getFullPathName().toStdString());
        return false;
    }
    
    // 准备处理链
    if (processingChain) {
        processingChain->prepareToPlay(reader->sampleRate, config.bufferSize);
    }
    
    // 处理音频
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), config.bufferSize);
    juce::MidiBuffer midiBuffer;
    
    juce::int64 totalSamples = reader->lengthInSamples;
    juce::int64 samplesProcessed = 0;
    
    while (samplesProcessed < totalSamples) {
        if (shouldStop) {
            return false;
        }
        
        // 等待暂停结束
        while (paused && !shouldStop) {
            juce::Thread::sleep(100);
        }
        
        if (shouldStop) {
            return false;
        }
        
        int samplesToRead = static_cast<int>(std::min(static_cast<juce::int64>(config.bufferSize), 
                                                     totalSamples - samplesProcessed));
        
        // 读取音频数据
        reader->read(&buffer, 0, samplesToRead, samplesProcessed, true, true);
        
        // 应用插件链处理
        if (processingChain && processingChain->isEnabled()) {
            midiBuffer.clear();
            processingChain->processBlock(buffer, midiBuffer);
        }
        
        // 应用输出增益
        if (config.outputGain != 1.0) {
            buffer.applyGain(static_cast<float>(config.outputGain));
        }
        
        // 写入输出文件
        writer->writeFromAudioSampleBuffer(buffer, 0, samplesToRead);
        
        samplesProcessed += samplesToRead;
        
        // 更新进度
        double progress = static_cast<double>(samplesProcessed) / static_cast<double>(totalSamples);
        if (progressCallback) {
            progressCallback(progress);
        }
    }
    
    // 释放处理链资源
    if (processingChain) {
        processingChain->releaseResources();
    }
    
    return true;
}

std::unique_ptr<juce::AudioFormatWriter> OfflineProcessor::createWriter(const juce::File& file,
                                                                       const OfflineProcessingConfig& config,
                                                                       double sampleRate,
                                                                       int numChannels) {
    // 确保输出目录存在
    file.getParentDirectory().createDirectory();
    
    // 根据文件扩展名选择格式
    juce::String extension = file.getFileExtension().toLowerCase();
    
    std::unique_ptr<juce::AudioFormat> format;
    
    if (extension == ".wav") {
        format = std::make_unique<juce::WavAudioFormat>();
    } else if (extension == ".aiff" || extension == ".aif") {
        format = std::make_unique<juce::AiffAudioFormat>();
    } else if (extension == ".flac") {
        format = std::make_unique<juce::FlacAudioFormat>();
    } else {
        // 默认使用WAV格式
        format = std::make_unique<juce::WavAudioFormat>();
    }
    
    auto fileStream = std::make_unique<juce::FileOutputStream>(file);
    if (!fileStream->openedOk()) {
        return nullptr;
    }
    
    return std::unique_ptr<juce::AudioFormatWriter>(
        format->createWriterFor(fileStream.release(),
                               sampleRate,
                               numChannels,
                               config.outputBitDepth,
                               {},
                               0));
}

void OfflineProcessor::onTaskProgress(const std::string& taskId, double progress) {
    if (progressCallback) {
        progressCallback(taskId, progress);
    }
}

void OfflineProcessor::onTaskCompleted(const std::string& taskId, bool success, const std::string& error) {
    if (completionCallback) {
        completionCallback(taskId, success, error);
    }
}

void OfflineProcessor::onError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
}

} // namespace WindsynthVST
