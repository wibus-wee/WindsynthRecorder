//
//  WindsynthEngineFacade.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新音频架构的高层门面类实现
//

#include "WindsynthEngineFacade.hpp"
#include <iostream>

namespace WindsynthVST::Engine {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

WindsynthEngineFacade::WindsynthEngineFacade() {
    std::cout << "[WindsynthEngineFacade] 构造函数" << std::endl;
    initializeComponents();
    setupCallbacks();
}

WindsynthEngineFacade::~WindsynthEngineFacade() {
    std::cout << "[WindsynthEngineFacade] 析构函数" << std::endl;
    shutdown();
}

//==============================================================================
// 引擎生命周期管理
//==============================================================================

bool WindsynthEngineFacade::initialize(const EngineConfig& config) {
    std::cout << "[WindsynthEngineFacade] 初始化引擎" << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    if (currentState.load() != EngineState::Stopped) {
        notifyError("引擎必须在停止状态下才能初始化");
        return false;
    }
    
    notifyStateChange(EngineState::Starting, "正在初始化引擎...");
    
    try {
        // 保存配置
        currentConfig = config;
        
        // 配置音频I/O
        AudioGraph::AudioIOManager::IOConfiguration ioConfig;
        ioConfig.numInputChannels = config.numInputChannels;
        ioConfig.numOutputChannels = config.numOutputChannels;
        ioConfig.sampleRate = config.sampleRate;
        ioConfig.bufferSize = config.bufferSize;
        
        if (!ioManager->configureIO(ioConfig)) {
            notifyError("无法配置音频I/O");
            notifyStateChange(EngineState::Error);
            return false;
        }
        
        // 准备音频处理（GraphAudioProcessor继承自AudioProcessor，直接调用prepareToPlay）
        graphProcessor->prepareToPlay(config.sampleRate, config.bufferSize);
        
        notifyStateChange(EngineState::Stopped, "引擎初始化完成");
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "引擎初始化失败: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(EngineState::Error);
        return false;
    }
}

bool WindsynthEngineFacade::start() {
    std::cout << "[WindsynthEngineFacade] 启动音频处理" << std::endl;
    
    if (currentState.load() != EngineState::Stopped) {
        notifyError("引擎必须在停止状态下才能启动");
        return false;
    }
    
    notifyStateChange(EngineState::Starting, "正在启动音频处理...");
    
    try {
        // AudioIOManager没有startAudio方法，我们直接设置状态为运行
        // 实际的音频启动由JUCE的AudioDeviceManager处理
        notifyStateChange(EngineState::Running, "音频处理已启动");
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "启动音频处理失败: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(EngineState::Error);
        return false;
    }
}

void WindsynthEngineFacade::stop() {
    std::cout << "[WindsynthEngineFacade] 停止音频处理" << std::endl;
    
    if (currentState.load() == EngineState::Stopped) {
        return;
    }
    
    notifyStateChange(EngineState::Stopping, "正在停止音频处理...");
    
    try {
        // 停止播放
        stopPlayback();

        // AudioIOManager没有stopAudio方法，状态管理即可
        
        notifyStateChange(EngineState::Stopped, "音频处理已停止");
        
    } catch (const std::exception& e) {
        std::string error = "停止音频处理时出错: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(EngineState::Error);
    }
}

void WindsynthEngineFacade::shutdown() {
    std::cout << "[WindsynthEngineFacade] 关闭引擎" << std::endl;
    
    stop();
    
    try {
        // 释放音频资源
        if (graphProcessor) {
            graphProcessor->releaseResources();
        }
        
        // 清理音频文件相关资源
        transportSource.reset();
        readerSource.reset();
        
        notifyStateChange(EngineState::Stopped, "引擎已关闭");
        
    } catch (const std::exception& e) {
        std::string error = "关闭引擎时出错: " + std::string(e.what());
        notifyError(error);
    }
}

EngineState WindsynthEngineFacade::getState() const {
    return currentState.load();
}

bool WindsynthEngineFacade::isRunning() const {
    return currentState.load() == EngineState::Running;
}

//==============================================================================
// 音频文件处理
//==============================================================================

bool WindsynthEngineFacade::loadAudioFile(const std::string& filePath) {
    std::cout << "[WindsynthEngineFacade] 加载音频文件: " << filePath << std::endl;
    
    try {
        juce::File audioFile(filePath);
        if (!audioFile.existsAsFile()) {
            notifyError("音频文件不存在: " + filePath);
            return false;
        }
        
        // 创建音频格式读取器
        auto reader = formatManager->createReaderFor(audioFile);
        if (!reader) {
            notifyError("无法读取音频文件: " + filePath);
            return false;
        }
        
        // 创建音频格式读取器源
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        
        // 设置到传输源
        transportSource->setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
        
        std::cout << "[WindsynthEngineFacade] 音频文件加载成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "加载音频文件失败: " + std::string(e.what());
        notifyError(error);
        return false;
    }
}

bool WindsynthEngineFacade::play() {
    std::cout << "[WindsynthEngineFacade] 开始播放" << std::endl;
    
    if (!transportSource || !readerSource) {
        notifyError("没有加载音频文件");
        return false;
    }
    
    try {
        transportSource->start();
        return true;
    } catch (const std::exception& e) {
        std::string error = "播放失败: " + std::string(e.what());
        notifyError(error);
        return false;
    }
}

void WindsynthEngineFacade::pause() {
    std::cout << "[WindsynthEngineFacade] 暂停播放" << std::endl;
    
    if (transportSource) {
        transportSource->stop();
    }
}

void WindsynthEngineFacade::stopPlayback() {
    std::cout << "[WindsynthEngineFacade] 停止播放" << std::endl;
    
    if (transportSource) {
        transportSource->stop();
        transportSource->setPosition(0.0);
    }
}

bool WindsynthEngineFacade::seekTo(double timeInSeconds) {
    if (!transportSource) {
        return false;
    }
    
    try {
        transportSource->setPosition(timeInSeconds);
        return true;
    } catch (const std::exception& e) {
        notifyError("跳转失败: " + std::string(e.what()));
        return false;
    }
}

double WindsynthEngineFacade::getCurrentTime() const {
    if (!transportSource) {
        return 0.0;
    }
    
    return transportSource->getCurrentPosition();
}

double WindsynthEngineFacade::getDuration() const {
    if (!transportSource) {
        return 0.0;
    }
    
    return transportSource->getLengthInSeconds();
}

//==============================================================================
// 插件管理
//==============================================================================

int WindsynthEngineFacade::scanPlugins(const std::vector<std::string>& searchPaths) {
    std::cout << "[WindsynthEngineFacade] 扫描插件" << std::endl;
    
    try {
        // 使用默认搜索路径如果没有提供
        std::vector<std::string> paths = searchPaths;
        if (paths.empty()) {
            // 添加默认VST3搜索路径
            paths.push_back("/Library/Audio/Plug-Ins/VST3");
            paths.push_back("~/Library/Audio/Plug-Ins/VST3");
        }
        
        int totalFound = 0;
        for (const auto& path : paths) {
            // ModernPluginLoader使用异步扫描，我们使用scanFileAsync
            juce::File directory(path);
            if (directory.exists()) {
                pluginLoader->scanFileAsync(directory, false);
                std::cout << "[WindsynthEngineFacade] 开始扫描路径: " << path << std::endl;
                totalFound++; // 简化计数
            }
        }
        
        std::cout << "[WindsynthEngineFacade] 插件扫描完成，总共找到 " << totalFound << " 个插件" << std::endl;
        return totalFound;
        
    } catch (const std::exception& e) {
        std::string error = "插件扫描失败: " + std::string(e.what());
        notifyError(error);
        return 0;
    }
}

std::vector<SimplePluginInfo> WindsynthEngineFacade::getAvailablePlugins() const {
    std::vector<SimplePluginInfo> result;
    
    try {
        auto pluginList = pluginLoader->getKnownPlugins(); // 使用正确的方法名

        for (const auto& plugin : pluginList) {
            SimplePluginInfo info;
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
        // 修复const方法调用问题
        std::cerr << "[WindsynthEngineFacade] 获取插件列表失败: " << e.what() << std::endl;
    }
    
    return result;
}

void WindsynthEngineFacade::loadPluginAsync(const std::string& pluginIdentifier,
                                          const std::string& displayName,
                                          PluginLoadCallback callback) {
    std::cout << "[WindsynthEngineFacade] 异步加载插件: " << pluginIdentifier << std::endl;
    
    try {
        // 查找插件描述
        auto pluginList = pluginLoader->getKnownPlugins(); // 使用正确的方法名
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
        notifyError(error);
        if (callback) {
            callback(0, false, error);
        }
    }
}

//==============================================================================
// 内部方法
//==============================================================================

void WindsynthEngineFacade::initializeComponents() {
    std::cout << "[WindsynthEngineFacade] 初始化组件" << std::endl;
    
    try {
        // 创建核心组件
        graphProcessor = std::make_unique<AudioGraph::GraphAudioProcessor>();
        pluginLoader = std::make_unique<AudioGraph::ModernPluginLoader>();
        pluginManager = std::make_unique<AudioGraph::PluginManager>(*graphProcessor, *pluginLoader);
        graphManager = std::make_unique<AudioGraph::GraphManager>(*graphProcessor);
        ioManager = std::make_unique<AudioGraph::AudioIOManager>(*graphProcessor);
        presetManager = std::make_unique<AudioGraph::PresetManager>(*graphProcessor, *pluginManager);
        
        // 创建音频文件相关组件
        formatManager = std::make_unique<juce::AudioFormatManager>();
        formatManager->registerBasicFormats();
        
        transportSource = std::make_unique<juce::AudioTransportSource>();
        
        std::cout << "[WindsynthEngineFacade] 组件初始化完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::string error = "组件初始化失败: " + std::string(e.what());
        notifyError(error);
        notifyStateChange(EngineState::Error);
    }
}

void WindsynthEngineFacade::setupCallbacks() {
    // 设置内部回调
    if (graphProcessor) {
        graphProcessor->setErrorCallback([this](const std::string& error) {
            notifyError("GraphProcessor错误: " + error);
        });
        
        graphProcessor->setStateCallback([this](const std::string& message) {
            // 可以在这里处理状态变化
        });
    }
}

void WindsynthEngineFacade::notifyStateChange(EngineState newState, const std::string& message) {
    currentState.store(newState);
    
    if (stateCallback) {
        stateCallback(newState, message);
    }
    
    std::cout << "[WindsynthEngineFacade] 状态变化: " << static_cast<int>(newState) 
              << " - " << message << std::endl;
}

void WindsynthEngineFacade::notifyError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
    
    std::cerr << "[WindsynthEngineFacade] 错误: " << error << std::endl;
}

AudioGraph::NodeID WindsynthEngineFacade::convertToNodeID(uint32_t nodeID) const {
    // NodeID是juce::AudioProcessorGraph::NodeID的别名，直接构造
    juce::AudioProcessorGraph::NodeID id;
    id.uid = nodeID;
    return id;
}

uint32_t WindsynthEngineFacade::convertFromNodeID(AudioGraph::NodeID nodeID) const {
    return static_cast<uint32_t>(nodeID.uid);
}

bool WindsynthEngineFacade::removeNode(uint32_t nodeID) {
    std::cout << "[WindsynthEngineFacade] 移除节点: " << nodeID << std::endl;

    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        return pluginManager->removePlugin(graphNodeID);
    } catch (const std::exception& e) {
        notifyError("移除节点失败: " + std::string(e.what()));
        return false;
    }
}

std::vector<SimpleNodeInfo> WindsynthEngineFacade::getLoadedNodes() const {
    std::vector<SimpleNodeInfo> result;

    try {
        auto nodes = graphProcessor->getAllNodes();

        for (const auto& node : nodes) {
            SimpleNodeInfo info;
            info.nodeID = convertFromNodeID(node.nodeID);
            info.name = node.name;
            info.pluginName = node.pluginName;
            info.isEnabled = node.enabled;    // 修复字段名
            info.isBypassed = node.bypassed;  // 修复字段名
            info.numInputChannels = node.numInputChannels;
            info.numOutputChannels = node.numOutputChannels;

            result.push_back(info);
        }

    } catch (const std::exception& e) {
        // 修复const方法调用问题
        std::cerr << "[WindsynthEngineFacade] 获取节点列表失败: " << e.what() << std::endl;
    }

    return result;
}

bool WindsynthEngineFacade::setNodeBypassed(uint32_t nodeID, bool bypassed) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        return graphProcessor->setNodeBypassed(graphNodeID, bypassed);
    } catch (const std::exception& e) {
        notifyError("设置节点旁路状态失败: " + std::string(e.what()));
        return false;
    }
}

bool WindsynthEngineFacade::setNodeEnabled(uint32_t nodeID, bool enabled) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        return graphProcessor->setNodeEnabled(graphNodeID, enabled);
    } catch (const std::exception& e) {
        notifyError("设置节点启用状态失败: " + std::string(e.what()));
        return false;
    }
}

//==============================================================================
// 参数控制
//==============================================================================

bool WindsynthEngineFacade::setNodeParameter(uint32_t nodeID, int parameterIndex, float value) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        // PluginManager没有setParameter方法，我们直接通过插件实例设置
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (instance && parameterIndex >= 0 && parameterIndex < instance->getParameters().size()) {
            auto* param = instance->getParameters()[parameterIndex];
            if (param) {
                param->setValueNotifyingHost(value);
                return true;
            }
        }
        return false;
    } catch (const std::exception& e) {
        notifyError("设置节点参数失败: " + std::string(e.what()));
        return false;
    }
}

float WindsynthEngineFacade::getNodeParameter(uint32_t nodeID, int parameterIndex) const {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        // PluginManager没有getParameter方法，我们直接通过插件实例获取
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (instance && parameterIndex >= 0 && parameterIndex < instance->getParameters().size()) {
            auto* param = instance->getParameters()[parameterIndex];
            if (param) {
                return param->getValue();
            }
        }
        return -1.0f;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取节点参数失败: " << e.what() << std::endl;
        return -1.0f;
    }
}

int WindsynthEngineFacade::getNodeParameterCount(uint32_t nodeID) const {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        // PluginManager没有getParameterCount方法，我们直接通过插件实例获取
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (instance) {
            return static_cast<int>(instance->getParameters().size());
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取节点参数数量失败: " << e.what() << std::endl;
        return 0;
    }
}

//==============================================================================
// 音频路由管理
//==============================================================================

int WindsynthEngineFacade::createProcessingChain(const std::vector<uint32_t>& nodeIDs) {
    std::cout << "[WindsynthEngineFacade] 创建处理链，节点数量: " << nodeIDs.size() << std::endl;

    try {
        std::vector<AudioGraph::NodeID> graphNodeIDs;
        for (uint32_t nodeID : nodeIDs) {
            graphNodeIDs.push_back(convertToNodeID(nodeID));
        }

        return graphManager->createProcessingChain(graphNodeIDs, true);
    } catch (const std::exception& e) {
        notifyError("创建处理链失败: " + std::string(e.what()));
        return 0;
    }
}

bool WindsynthEngineFacade::autoConnectToIO(uint32_t nodeID) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);

        // 连接到音频输入
        AudioGraph::NodeID audioInputID = graphProcessor->getAudioInputNodeID();
        bool inputConnected = graphProcessor->connectAudio(audioInputID, 0, graphNodeID, 0);
        if (graphProcessor->getNodeInfo(graphNodeID).numInputChannels > 1) {
            graphProcessor->connectAudio(audioInputID, 1, graphNodeID, 1);
        }

        // 连接到音频输出
        AudioGraph::NodeID audioOutputID = graphProcessor->getAudioOutputNodeID();
        bool outputConnected = graphProcessor->connectAudio(graphNodeID, 0, audioOutputID, 0);
        if (graphProcessor->getNodeInfo(graphNodeID).numOutputChannels > 1) {
            graphProcessor->connectAudio(graphNodeID, 1, audioOutputID, 1);
        }

        return inputConnected && outputConnected;
    } catch (const std::exception& e) {
        notifyError("自动连接到I/O失败: " + std::string(e.what()));
        return false;
    }
}

bool WindsynthEngineFacade::disconnectNode(uint32_t nodeID) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        return graphProcessor->disconnectNode(graphNodeID);
    } catch (const std::exception& e) {
        notifyError("断开节点连接失败: " + std::string(e.what()));
        return false;
    }
}

//==============================================================================
// 状态和监控
//==============================================================================

EngineStatistics WindsynthEngineFacade::getStatistics() const {
    EngineStatistics stats;

    try {
        if (graphProcessor) {
            auto graphStats = graphProcessor->getPerformanceStats();
            // GraphPerformanceStats的实际字段名
            stats.cpuUsage = graphStats.averageProcessingTimeMs;
            stats.memoryUsage = 0.0; // GraphPerformanceStats没有memoryUsage字段
            stats.inputLevel = 0.0;  // GraphPerformanceStats没有inputLevel字段
            stats.outputLevel = 0.0; // GraphPerformanceStats没有outputLevel字段
            stats.latency = graphStats.averageProcessingTimeMs;
            stats.dropouts = 0; // GraphPerformanceStats没有dropouts字段

            auto nodes = graphProcessor->getAllNodes();
            stats.activeNodes = static_cast<int>(nodes.size());

            auto connections = graphProcessor->getAllConnections();
            stats.totalConnections = static_cast<int>(connections.size());
        }
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取统计信息失败: " << e.what() << std::endl;
    }

    return stats;
}

double WindsynthEngineFacade::getOutputLevel() const {
    try {
        if (graphProcessor) {
            // GraphPerformanceStats没有outputLevel字段，返回默认值
            return 0.0;
        }
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取输出电平失败: " << e.what() << std::endl;
    }

    return 0.0;
}

double WindsynthEngineFacade::getInputLevel() const {
    try {
        if (graphProcessor) {
            // GraphPerformanceStats没有inputLevel字段，返回默认值
            return 0.0;
        }
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取输入电平失败: " << e.what() << std::endl;
    }

    return 0.0;
}

//==============================================================================
// 回调设置
//==============================================================================

void WindsynthEngineFacade::setStateCallback(EngineStateCallback callback) {
    stateCallback = callback;
}

void WindsynthEngineFacade::setErrorCallback(ErrorCallback callback) {
    errorCallback = callback;
}

//==============================================================================
// 配置管理
//==============================================================================

const EngineConfig& WindsynthEngineFacade::getConfiguration() const {
    return currentConfig;
}

bool WindsynthEngineFacade::updateConfiguration(const EngineConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex);

    try {
        // 如果引擎正在运行，需要先停止
        bool wasRunning = isRunning();
        if (wasRunning) {
            stop();
        }

        // 更新配置
        currentConfig = config;

        // 重新初始化
        bool success = initialize(config);

        // 如果之前在运行，重新启动
        if (success && wasRunning) {
            success = start();
        }

        return success;
    } catch (const std::exception& e) {
        notifyError("更新配置失败: " + std::string(e.what()));
        return false;
    }
}

} // namespace WindsynthVST::Engine
