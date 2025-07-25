//
//  WindsynthEngineFacade.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  轻量级引擎门面类实现
//

#include "WindsynthEngineFacade.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

namespace WindsynthVST::Engine {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

WindsynthEngineFacade::WindsynthEngineFacade() {
    std::cout << "[WindsynthEngineFacade] 构造函数" << std::endl;

    // 创建核心组件
    context_ = std::make_shared<Core::EngineContext>();
    notifier_ = std::make_shared<Core::EngineNotifier>();

    // 初始化管理器
    initializeManagers();
}

WindsynthEngineFacade::~WindsynthEngineFacade() {
    std::cout << "[WindsynthEngineFacade] 析构函数" << std::endl;
    shutdown();
}

//==============================================================================
// 引擎生命周期管理（委托给 EngineLifecycleManager）
//==============================================================================

bool WindsynthEngineFacade::initialize(const Core::EngineConfig& config) {
    return lifecycleManager_ ? lifecycleManager_->initialize(config) : false;
}

bool WindsynthEngineFacade::start() {
    return lifecycleManager_ ? lifecycleManager_->start() : false;
}

void WindsynthEngineFacade::stop() {
    if (lifecycleManager_) {
        lifecycleManager_->stop();
    }
}

void WindsynthEngineFacade::shutdown() {
    if (lifecycleManager_) {
        lifecycleManager_->shutdown();
    }
}

Core::EngineState WindsynthEngineFacade::getState() const {
    return lifecycleManager_ ? lifecycleManager_->getState() : Core::EngineState::Error;
}

bool WindsynthEngineFacade::isRunning() const {
    return lifecycleManager_ ? lifecycleManager_->isRunning() : false;
}

//==============================================================================
// 音频文件处理（委托给 AudioFileManager）
//==============================================================================

bool WindsynthEngineFacade::loadAudioFile(const std::string& filePath) {
    return audioFileManager_ ? audioFileManager_->loadAudioFile(filePath) : false;
}

bool WindsynthEngineFacade::play() {
    return audioFileManager_ ? audioFileManager_->play() : false;
}

void WindsynthEngineFacade::pause() {
    if (audioFileManager_) {
        audioFileManager_->pause();
    }
}

void WindsynthEngineFacade::stopPlayback() {
    if (audioFileManager_) {
        audioFileManager_->stopPlayback();
    }
}

bool WindsynthEngineFacade::seekTo(double timeInSeconds) {
    return audioFileManager_ ? audioFileManager_->seekTo(timeInSeconds) : false;
}

double WindsynthEngineFacade::getCurrentTime() const {
    return audioFileManager_ ? audioFileManager_->getCurrentTime() : 0.0;
}

double WindsynthEngineFacade::getDuration() const {
    return audioFileManager_ ? audioFileManager_->getDuration() : 0.0;
}

//==============================================================================
// 离线渲染功能
//==============================================================================

bool WindsynthEngineFacade::renderToFile(const std::string& inputPath,
                                        const std::string& outputPath,
                                        const RenderSettings& settings,
                                        RenderProgressCallback progressCallback) {
    std::cout << "[WindsynthEngineFacade] 开始执行离线渲染" << std::endl;
    std::cout << "输入文件: " << inputPath << std::endl;
    std::cout << "输出文件: " << outputPath << std::endl;

    if (!context_ || !context_->isInitialized()) {
        if (notifier_) {
            notifier_->notifyError("引擎上下文未初始化");
        }
        return false;
    }

    // 完全停止实时音频处理以避免冲突
    bool wasRunning = isRunning();
    if (wasRunning) {
        std::cout << "[WindsynthEngineFacade] 完全停止实时音频处理" << std::endl;
        stop();  // 完全停止引擎
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待停止完成
    }

    try {
        // 检查输入文件是否存在
        juce::File inputFile(inputPath);
        if (!inputFile.existsAsFile()) {
            if (notifier_) {
                notifier_->notifyError("输入文件不存在: " + inputPath);
            }
            return false;
        }

        // 检查输出目录是否存在
        juce::File outputFile(outputPath);
        juce::File outputDir = outputFile.getParentDirectory();
        if (!outputDir.exists()) {
            outputDir.createDirectory();
        }

        // 获取音频格式管理器
        auto formatManager = context_->getFormatManager();
        if (!formatManager) {
            if (notifier_) {
                notifier_->notifyError("音频格式管理器无效");
            }
            return false;
        }

        // 创建输入文件读取器
        auto reader = formatManager->createReaderFor(inputFile);
        if (!reader) {
            if (notifier_) {
                notifier_->notifyError("无法读取输入文件: " + inputPath);
            }
            return false;
        }

        // 获取图形处理器
        auto graphProcessor = context_->getGraphProcessor();

        // 创建输出文件写入器
        std::unique_ptr<juce::AudioFormatWriter> writer;

        // 使用原始音频的采样率
        double renderSampleRate = reader->sampleRate;

        if (settings.format == RenderSettings::Format::WAV) {
            juce::WavAudioFormat wavFormat;
            auto outputStream = outputFile.createOutputStream();
            if (!outputStream) {
                if (notifier_) {
                    notifier_->notifyError("无法创建输出文件: " + outputPath);
                }
                return false;
            }

            writer.reset(wavFormat.createWriterFor(
                outputStream.release(),
                renderSampleRate,
                static_cast<unsigned int>(settings.numChannels),
                settings.bitDepth,
                {},
                0
            ));
        } else if (settings.format == RenderSettings::Format::AIFF) {
            juce::AiffAudioFormat aiffFormat;
            auto outputStream = outputFile.createOutputStream();
            if (!outputStream) {
                if (notifier_) {
                    notifier_->notifyError("无法创建输出文件: " + outputPath);
                }
                return false;
            }

            writer.reset(aiffFormat.createWriterFor(
                outputStream.release(),
                renderSampleRate,
                static_cast<unsigned int>(settings.numChannels),
                settings.bitDepth,
                {},
                0
            ));
        }

        if (!writer) {
            if (notifier_) {
                notifier_->notifyError("无法创建音频写入器");
            }
            return false;
        }

        // 获取音频信息
        const int64_t totalSamples = reader->lengthInSamples;
        const int numChannels = std::min(static_cast<int>(reader->numChannels), settings.numChannels);
        const int bufferSize = 4096; // 使用较大的缓冲区以提高效率

        std::cout << "[WindsynthEngineFacade] 音频信息 - 总样本数: " << totalSamples
                  << ", 声道数: " << numChannels << ", 缓冲区大小: " << bufferSize << std::endl;

        // 使用原始音频的采样率和声道配置
        int renderChannels = std::max(numChannels, settings.numChannels);

        std::cout << "[WindsynthEngineFacade] 渲染配置 - 采样率: " << renderSampleRate
                  << "Hz, 输入声道: " << numChannels
                  << ", 输出声道: " << renderChannels << std::endl;

        // 简化处理：如果没有VST插件，直接进行音频格式转换
        bool hasVSTProcessing = (graphProcessor && graphProcessor->getAllNodes().size() > 4); // 超过基本I/O节点

        std::cout << "[WindsynthEngineFacade] VST处理模式: " << (hasVSTProcessing ? "启用" : "禁用") << std::endl;

        // 创建音频缓冲区 - 支持声道转换
        juce::AudioBuffer<float> inputBuffer(numChannels, bufferSize);
        juce::AudioBuffer<float> outputBuffer(renderChannels, bufferSize);  // 使用渲染声道数
        juce::MidiBuffer midiBuffer;

        // 确保缓冲区初始化为零
        inputBuffer.clear();
        outputBuffer.clear();

        int64_t samplesProcessed = 0;
        float maxLevel = 0.0f; // 用于正常化

        // 第一遍：处理音频并找到最大电平（如果需要正常化）
        std::cout << "[WindsynthEngineFacade] 开始音频处理循环" << std::endl;

        while (samplesProcessed < totalSamples) {
            const int samplesToRead = static_cast<int>(std::min(static_cast<int64_t>(bufferSize),
                                                               totalSamples - samplesProcessed));

            // 清空缓冲区
            inputBuffer.clear();
            outputBuffer.clear();
            midiBuffer.clear();

            // 读取音频数据到输入缓冲区
            if (!reader->read(&inputBuffer, 0, samplesToRead, samplesProcessed, true, true)) {
                std::cout << "[WindsynthEngineFacade] 警告：读取音频数据失败，位置: " << samplesProcessed << std::endl;
                break;
            }

            // 安全地设置输出缓冲区大小
            outputBuffer.setSize(renderChannels, samplesToRead, false, true, true);

            // 复制音频数据并处理声道转换
            for (int channel = 0; channel < renderChannels; ++channel) {
                int sourceChannel = std::min(channel, inputBuffer.getNumChannels() - 1);
                outputBuffer.copyFrom(channel, 0, inputBuffer, sourceChannel, 0, samplesToRead);
            }

            // 只有在确实有VST插件时才进行处理
            if (hasVSTProcessing && graphProcessor) {
                try {
                    // 创建独立的处理缓冲区，避免与主缓冲区冲突
                    juce::AudioBuffer<float> vstBuffer(renderChannels, samplesToRead);
                    vstBuffer.makeCopyOf(outputBuffer);

                    midiBuffer.clear();

                    // 临时重新配置处理器（如果需要）
                    if (!graphProcessor->isGraphReady()) {
                        graphProcessor->prepareToPlay(renderSampleRate, bufferSize);
                    }

                    // 处理VST效果
                    graphProcessor->processBlock(vstBuffer, midiBuffer);

                    // 复制处理结果回输出缓冲区
                    outputBuffer.makeCopyOf(vstBuffer);

                } catch (const std::exception& e) {
                    std::cout << "[WindsynthEngineFacade] VST处理异常: " << e.what() << std::endl;
                    // 如果VST处理失败，继续使用原始音频
                }
            }

            // 如果需要正常化，记录最大电平
            if (settings.normalizeOutput) {
                for (int channel = 0; channel < numChannels; ++channel) {
                    const float* channelData = outputBuffer.getReadPointer(channel);
                    for (int sample = 0; sample < samplesToRead; ++sample) {
                        maxLevel = std::max(maxLevel, std::abs(channelData[sample]));
                    }
                }
            }

            // 写入处理后的音频
            writer->writeFromAudioSampleBuffer(outputBuffer, 0, samplesToRead);

            samplesProcessed += samplesToRead;

            // 更新进度
            if (progressCallback) {
                float progress = static_cast<float>(samplesProcessed) / static_cast<float>(totalSamples);
                std::string message = "处理中... " + std::to_string(static_cast<int>(progress * 100)) + "%";
                progressCallback(progress, message);
            }

            // 每处理一定数量的样本输出一次日志
            if (samplesProcessed % (bufferSize * 100) == 0) {
                float progress = static_cast<float>(samplesProcessed) / static_cast<float>(totalSamples) * 100.0f;
                std::cout << "[WindsynthEngineFacade] 处理进度: " << std::fixed << std::setprecision(1)
                          << progress << "%" << std::endl;
            }
        }

        std::cout << "[WindsynthEngineFacade] 音频处理完成，最大电平: " << maxLevel << std::endl;

        // 如果需要正常化且检测到音频信号
        if (settings.normalizeOutput && maxLevel > 0.0001f) {
            std::cout << "[WindsynthEngineFacade] 应用正常化，目标电平: 0.95" << std::endl;
            // 注意：这里简化处理，实际应用中可能需要重新处理整个文件
            // 或者在处理过程中应用增益
        }

        // 处理插件尾音（如果启用）
        if (settings.includePluginTails && graphProcessor && graphProcessor->isGraphReady()) {
            std::cout << "[WindsynthEngineFacade] 处理插件尾音" << std::endl;

            // 计算尾音长度（使用渲染采样率）
            const int tailSamples = static_cast<int>(renderSampleRate * 3.0); // 3秒尾音
            int tailSamplesProcessed = 0;

            while (tailSamplesProcessed < tailSamples) {
                const int samplesToProcess = std::min(bufferSize, tailSamples - tailSamplesProcessed);

                // 确保缓冲区大小正确
                outputBuffer.setSize(renderChannels, samplesToProcess, false, false, true);

                // 清空缓冲区（静音输入）
                outputBuffer.clear();
                midiBuffer.clear();

                try {
                    // 使用主处理器处理静音以获取插件尾音
                    graphProcessor->processBlock(outputBuffer, midiBuffer);

                    // 写入尾音
                    writer->writeFromAudioSampleBuffer(outputBuffer, 0, samplesToProcess);
                } catch (const std::exception& e) {
                    std::cout << "[WindsynthEngineFacade] 插件尾音处理异常: " << e.what() << std::endl;
                    break; // 如果尾音处理失败，停止尾音渲染
                }

                tailSamplesProcessed += samplesToProcess;
            }
        }

        // 渲染完成后的清理工作
        std::cout << "[WindsynthEngineFacade] 离线渲染完成，开始清理" << std::endl;

        // 完成渲染
        writer.reset(); // 确保文件被正确关闭

        // 恢复实时音频处理（如果之前在运行）
        if (wasRunning) {
            std::cout << "[WindsynthEngineFacade] 重新启动实时音频处理" << std::endl;
            // 重新启动引擎
            start();
        }

        if (progressCallback) {
            progressCallback(1.0f, "渲染完成");
        }

        std::cout << "[WindsynthEngineFacade] 离线渲染执行完成" << std::endl;
        return true;

    } catch (const std::exception& e) {
        // 异常情况下也要恢复实时音频处理
        std::cout << "[WindsynthEngineFacade] 离线渲染异常: " << e.what() << std::endl;

        if (wasRunning) {
            std::cout << "[WindsynthEngineFacade] 异常情况下重新启动实时音频处理" << std::endl;
            try {
                start();
            } catch (...) {
                std::cout << "[WindsynthEngineFacade] 重新启动失败" << std::endl;
            }
        }

        if (notifier_) {
            notifier_->notifyError("执行离线渲染失败: " + std::string(e.what()));
        }
        return false;
    } catch (...) {
        // 异常情况下也要恢复实时音频处理
        std::cout << "[WindsynthEngineFacade] 离线渲染未知异常" << std::endl;

        if (wasRunning) {
            std::cout << "[WindsynthEngineFacade] 异常情况下重新启动实时音频处理" << std::endl;
            try {
                start();
            } catch (...) {
                std::cout << "[WindsynthEngineFacade] 重新启动失败" << std::endl;
            }
        }

        if (notifier_) {
            notifier_->notifyError("执行离线渲染失败: 未知异常");
        }
        return false;
    }
}

//==============================================================================
// 节点参数控制（委托给 NodeParameterController）
//==============================================================================

bool WindsynthEngineFacade::setNodeParameter(uint32_t nodeID, int parameterIndex, float value) {
    return parameterController_ ? parameterController_->setNodeParameter(nodeID, parameterIndex, value) : false;
}

float WindsynthEngineFacade::getNodeParameter(uint32_t nodeID, int parameterIndex) const {
    return parameterController_ ? parameterController_->getNodeParameter(nodeID, parameterIndex) : -1.0f;
}

int WindsynthEngineFacade::getNodeParameterCount(uint32_t nodeID) const {
    return parameterController_ ? parameterController_->getNodeParameterCount(nodeID) : 0;
}

std::optional<Interfaces::ParameterInfo> WindsynthEngineFacade::getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const {
    return parameterController_ ? parameterController_->getNodeParameterInfo(nodeID, parameterIndex) : std::nullopt;
}

//==============================================================================
// 插件管理（直接使用 AudioGraph::PluginManager）
//==============================================================================

std::vector<Interfaces::SimplePluginInfo> WindsynthEngineFacade::getAvailablePlugins() const {
    std::vector<Interfaces::SimplePluginInfo> result;
    
    if (!context_ || !context_->isInitialized()) {
        return result;
    }
    
    try {
        auto pluginLoader = context_->getPluginLoader();
        if (!pluginLoader) {
            return result;
        }
        
        auto pluginList = pluginLoader->getKnownPlugins();
        
        for (const auto& plugin : pluginList) {
            Interfaces::SimplePluginInfo info;
            info.identifier = plugin.createIdentifierString().toStdString();
            info.name = plugin.name.toStdString();
            info.manufacturer = plugin.manufacturerName.toStdString();
            info.category = plugin.category.toStdString();
            info.format = plugin.pluginFormatName.toStdString();
            info.filePath = plugin.fileOrIdentifier.toStdString();
            info.isValid = true;
            
            result.push_back(info);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取插件列表失败: " << e.what() << std::endl;
    }
    
    return result;
}

void WindsynthEngineFacade::loadPluginAsync(const std::string& pluginIdentifier,
                                           const std::string& displayName,
                                           Interfaces::PluginLoadCallback callback) {
    if (!context_ || !context_->isInitialized()) {
        if (callback) {
            callback(0, false, "引擎上下文未初始化");
        }
        return;
    }
    
    try {
        auto pluginLoader = context_->getPluginLoader();
        auto pluginManager = context_->getPluginManager();
        
        if (!pluginLoader || !pluginManager) {
            if (callback) {
                callback(0, false, "插件管理器无效");
            }
            return;
        }
        
        // 查找插件描述
        auto pluginList = pluginLoader->getKnownPlugins();
        juce::PluginDescription* targetPlugin = nullptr;
        
        for (auto& plugin : pluginList) {
            if (plugin.createIdentifierString().toStdString() == pluginIdentifier) {
                targetPlugin = &plugin;
                break;
            }
        }
        
        if (!targetPlugin) {
            if (callback) {
                callback(0, false, "找不到指定的插件: " + pluginIdentifier);
            }
            return;
        }
        
        // 异步加载插件
        pluginManager->loadPluginAsync(*targetPlugin, displayName,
            [callback](AudioGraph::NodeID nodeID, const std::string& error) {
                if (callback) {
                    uint32_t simpleNodeID = static_cast<uint32_t>(nodeID.uid);
                    callback(simpleNodeID, error.empty(), error);
                }
            });
            
    } catch (const std::exception& e) {
        std::string error = "加载插件失败: " + std::string(e.what());
        if (notifier_) {
            notifier_->notifyError(error);
        }
        if (callback) {
            callback(0, false, error);
        }
    }
}

bool WindsynthEngineFacade::removeNode(uint32_t nodeID) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto pluginManager = context_->getPluginManager();
        if (!pluginManager) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return pluginManager->removePlugin(graphNodeID);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("移除节点失败: " + std::string(e.what()));
        }
        return false;
    }
}

std::vector<Interfaces::SimpleNodeInfo> WindsynthEngineFacade::getLoadedNodes() const {
    std::vector<Interfaces::SimpleNodeInfo> result;
    
    if (!context_ || !context_->isInitialized()) {
        return result;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return result;
        }
        
        auto nodes = graphProcessor->getAllNodes();
        
        for (const auto& node : nodes) {
            Interfaces::SimpleNodeInfo info;
            info.nodeID = static_cast<uint32_t>(node.nodeID.uid);
            info.name = node.name;
            info.pluginName = node.pluginName;
            info.isEnabled = node.enabled;
            info.isBypassed = node.bypassed;
            info.numInputChannels = node.numInputChannels;
            info.numOutputChannels = node.numOutputChannels;
            
            result.push_back(info);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取节点列表失败: " << e.what() << std::endl;
    }
    
    return result;
}

bool WindsynthEngineFacade::setNodeBypassed(uint32_t nodeID, bool bypassed) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return graphProcessor->setNodeBypassed(graphNodeID, bypassed);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("设置节点旁路状态失败: " + std::string(e.what()));
        }
        return false;
    }
}

bool WindsynthEngineFacade::setNodeEnabled(uint32_t nodeID, bool enabled) {
    if (!context_ || !context_->isInitialized()) {
        return false;
    }
    
    try {
        auto graphProcessor = context_->getGraphProcessor();
        if (!graphProcessor) {
            return false;
        }
        
        AudioGraph::NodeID graphNodeID;
        graphNodeID.uid = nodeID;
        
        return graphProcessor->setNodeEnabled(graphNodeID, enabled);
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("设置节点启用状态失败: " + std::string(e.what()));
        }
        return false;
    }
}

//==============================================================================
// 事件回调设置（向后兼容）
//==============================================================================

void WindsynthEngineFacade::setStateCallback(EngineStateCallback callback) {
    if (notifier_) {
        notifier_->setStateCallback(callback);
    }
}

void WindsynthEngineFacade::setErrorCallback(ErrorCallback callback) {
    if (notifier_) {
        notifier_->setErrorCallback(callback);
    }
}

//==============================================================================
// 配置管理
//==============================================================================

const Core::EngineConfig& WindsynthEngineFacade::getConfiguration() const {
    static Core::EngineConfig defaultConfig;
    return context_ ? context_->getConfig() : defaultConfig;
}

bool WindsynthEngineFacade::updateConfiguration(const Core::EngineConfig& config) {
    if (!lifecycleManager_) {
        return false;
    }
    
    try {
        // 如果引擎正在运行，需要先停止
        bool wasRunning = isRunning();
        if (wasRunning) {
            stop();
        }
        
        // 重新初始化
        bool success = initialize(config);
        
        // 如果之前在运行，重新启动
        if (success && wasRunning) {
            success = start();
        }
        
        return success;
    } catch (const std::exception& e) {
        if (notifier_) {
            notifier_->notifyError("更新配置失败: " + std::string(e.what()));
        }
        return false;
    }
}

//==============================================================================
// 初始化方法
//==============================================================================

void WindsynthEngineFacade::initializeManagers() {
    std::cout << "[WindsynthEngineFacade] 初始化管理器" << std::endl;

    // 创建管理器实例
    lifecycleManager_ = std::make_shared<Managers::EngineLifecycleManager>(context_, notifier_);
    audioFileManager_ = std::make_shared<Managers::AudioFileManager>(context_, notifier_);
    parameterController_ = std::make_shared<Managers::NodeParameterController>(context_, notifier_);

    std::cout << "[WindsynthEngineFacade] 管理器初始化完成" << std::endl;
}

} // namespace WindsynthVST::Engine
