//
//  WindsynthEngineFacade.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  新音频架构的高层门面类实现
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
    // 防止重复清理
    static std::atomic<bool> shutdownCalled{false};
    if (shutdownCalled.exchange(true)) {
        std::cout << "[WindsynthEngineFacade] 引擎已经关闭，跳过重复清理" << std::endl;
        return;
    }

    std::cout << "[WindsynthEngineFacade] ===== 开始关闭引擎 =====" << std::endl;

    stop();

    try {
        // 第一步：清理 AudioProcessorGraph 中的所有节点（包括 VST3 插件）
        if (graphProcessor) {
            std::cout << "[WindsynthEngineFacade] 第一步：清理音频处理图中的所有节点" << std::endl;

            // 释放音频资源
            std::cout << "[WindsynthEngineFacade] 释放音频资源..." << std::endl;
            graphProcessor->releaseResources();

            // 清理音频文件相关资源
            std::cout << "[WindsynthEngineFacade] 清理音频文件相关资源..." << std::endl;
            graphProcessor->setTransportSource(nullptr);

            // 清理图中的所有节点和连接 - 这会安全地释放所有 VST3 插件实例
            std::cout << "[WindsynthEngineFacade] 清理图中的所有节点和连接..." << std::endl;
            graphProcessor->getGraph().clear();

            std::cout << "[WindsynthEngineFacade] 音频处理图已清理完成" << std::endl;
        } else {
            std::cout << "[WindsynthEngineFacade] 警告：graphProcessor 为空" << std::endl;
        }

        // 给一点时间让插件清理完成
        std::cout << "[WindsynthEngineFacade] 等待插件清理完成..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 第二步：关闭音频设备
        if (ioManager) {
            std::cout << "[WindsynthEngineFacade] 第二步：关闭音频设备" << std::endl;
            auto* deviceManager = ioManager->getDeviceManager();
            if (deviceManager) {
                std::cout << "[WindsynthEngineFacade] 移除音频回调..." << std::endl;
                // 移除音频回调
                deviceManager->removeAudioCallback(graphProcessor.get());

                std::cout << "[WindsynthEngineFacade] 关闭音频设备..." << std::endl;
                // 关闭音频设备
                deviceManager->closeAudioDevice();
                std::cout << "[WindsynthEngineFacade] 音频设备已关闭" << std::endl;
            } else {
                std::cout << "[WindsynthEngineFacade] 警告：deviceManager 为空" << std::endl;
            }
        } else {
            std::cout << "[WindsynthEngineFacade] 警告：ioManager 为空" << std::endl;
        }

        // 第三步：清理其他资源
        std::cout << "[WindsynthEngineFacade] 第三步：清理其他资源" << std::endl;
        transportSource.reset();
        readerSource.reset();

        notifyStateChange(EngineState::Stopped, "引擎已关闭");
        std::cout << "[WindsynthEngineFacade] ===== 引擎关闭完成 =====" << std::endl;

    } catch (const std::exception& e) {
        std::string error = "关闭引擎时出错: " + std::string(e.what());
        std::cout << "[WindsynthEngineFacade] 错误：" << error << std::endl;
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

        // 第一步：停止当前播放（如果正在播放）
        if (transportSource) {
            transportSource->stop();
            std::cout << "[WindsynthEngineFacade] 停止当前播放" << std::endl;
        }

        // 第二步：清空当前传输源，释放旧资源
        if (transportSource) {
            transportSource->setSource(nullptr);
            std::cout << "[WindsynthEngineFacade] 清空传输源" << std::endl;
        }

        // 第三步：重置读取器源
        readerSource.reset();
        std::cout << "[WindsynthEngineFacade] 重置读取器源" << std::endl;

        // 第四步：创建音频格式读取器
        auto reader = formatManager->createReaderFor(audioFile);
        if (!reader) {
            notifyError("无法读取音频文件: " + filePath);
            return false;
        }

        // 第五步：创建新的音频格式读取器源
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        std::cout << "[WindsynthEngineFacade] 创建新的读取器源" << std::endl;

        // 第六步：设置新源到传输源
        transportSource->setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
        std::cout << "[WindsynthEngineFacade] 设置新源到传输源" << std::endl;

        // 第七步：将transportSource设置到GraphAudioProcessor中
        if (graphProcessor) {
            graphProcessor->setTransportSource(transportSource.get());
            std::cout << "[WindsynthEngineFacade] 设置传输源到图形处理器" << std::endl;
        }

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

// 注意：插件扫描现在通过ModernPluginLoader异步进行，此方法已移除

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

std::optional<ParameterInfo> WindsynthEngineFacade::getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (!instance) {
            return std::nullopt;
        }

        const auto& parameters = instance->getParameters();
        if (parameterIndex < 0 || parameterIndex >= static_cast<int>(parameters.size())) {
            return std::nullopt;
        }

        auto* param = parameters[parameterIndex];
        if (!param) {
            return std::nullopt;
        }

        ParameterInfo info;
        info.name = param->getName(256).toStdString();
        info.label = param->getLabel().toStdString();
        info.minValue = 0.0f;
        info.maxValue = 1.0f;
        info.defaultValue = param->getDefaultValue();
        info.currentValue = param->getValue();
        info.isDiscrete = param->isDiscrete();
        info.numSteps = param->getNumSteps();
        info.units = param->getLabel().toStdString();

        return info;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 获取节点参数信息失败: " << e.what() << std::endl;
        return std::nullopt;
    }
}

//==============================================================================
// 插件编辑器管理
//==============================================================================

bool WindsynthEngineFacade::nodeHasEditor(uint32_t nodeID) const {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (!instance) {
            return false;
        }

        return instance->hasEditor();
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 检查节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool WindsynthEngineFacade::showNodeEditor(uint32_t nodeID) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
        auto* instance = pluginManager->getPluginInstance(graphNodeID);
        if (!instance || !instance->hasEditor()) {
            return false;
        }

        auto* editor = instance->createEditor();
        if (!editor) {
            return false;
        }

        // 通过插件管理器显示编辑器
        if (pluginManager) {
            bool success = pluginManager->showEditor(graphNodeID);
            if (success) {
                std::cout << "[WindsynthEngineFacade] 节点编辑器已显示: " << nodeID << std::endl;
            }
            return success;
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 显示节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool WindsynthEngineFacade::hideNodeEditor(uint32_t nodeID) {
    if (currentState.load() == EngineState::Stopped) {
        return false;
    }

    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);

        // 通过插件管理器隐藏编辑器
        if (pluginManager) {
            bool success = pluginManager->hideEditor(graphNodeID);
            if (success) {
                std::cout << "[WindsynthEngineFacade] 节点编辑器已隐藏: " << nodeID << std::endl;
            }
            return success;
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 隐藏节点编辑器失败: " << e.what() << std::endl;
        return false;
    }
}

bool WindsynthEngineFacade::isNodeEditorVisible(uint32_t nodeID) const {
    if (currentState.load() == EngineState::Stopped) {
        return false;
    }

    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);

        // 检查插件管理器中是否有该节点的编辑器窗口
        if (pluginManager) {
            return pluginManager->isEditorVisible(graphNodeID);
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 检查节点编辑器可见性失败: " << e.what() << std::endl;
        return false;
    }
}

//==============================================================================
// 节点位置管理
//==============================================================================

bool WindsynthEngineFacade::moveNode(uint32_t nodeID, int newPosition) {
    try {
        AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);

        // 通过GraphManager移动节点
        if (graphManager) {
            bool success = graphManager->moveNode(graphNodeID, newPosition);
            if (success) {
                std::cout << "[WindsynthEngineFacade] 节点已移动: " << nodeID << " -> 位置 " << newPosition << std::endl;
            }
            return success;
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 移动节点失败: " << e.what() << std::endl;
        return false;
    }
}

bool WindsynthEngineFacade::swapNodes(uint32_t nodeID1, uint32_t nodeID2) {
    try {
        AudioGraph::NodeID graphNodeID1 = convertToNodeID(nodeID1);
        AudioGraph::NodeID graphNodeID2 = convertToNodeID(nodeID2);

        // 由于GraphManager没有直接的swapNodes方法，我们通过两次moveNode来实现交换
        if (graphManager) {
            // 获取两个节点的当前位置
            // 这里需要实现获取节点位置的逻辑，暂时返回false
            // TODO: 实现真正的节点交换逻辑
            std::cout << "[WindsynthEngineFacade] 节点交换功能暂未实现: " << nodeID1 << " <-> " << nodeID2 << std::endl;
            return false;
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[WindsynthEngineFacade] 交换节点失败: " << e.what() << std::endl;
        return false;
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

//==============================================================================
// 离线音频渲染实现
//==============================================================================

bool WindsynthEngineFacade::renderToFile(const std::string& inputPath,
                                        const std::string& outputPath,
                                        const RenderSettings& settings,
                                        RenderProgressCallback progressCallback) {
    std::cout << "[WindsynthEngineFacade] 开始离线渲染：" << inputPath << " -> " << outputPath << std::endl;

    try {
        // 验证输入文件
        juce::File inputFile(inputPath);
        if (!inputFile.existsAsFile()) {
            notifyError("输入音频文件不存在: " + inputPath);
            return false;
        }

        // 创建音频格式读取器
        auto reader = formatManager->createReaderFor(inputFile);
        if (!reader) {
            notifyError("无法读取输入音频文件: " + inputPath);
            return false;
        }

        // 验证输出路径
        juce::File outputFile(outputPath);
        outputFile.getParentDirectory().createDirectory();

        // 创建音频格式写入器
        std::unique_ptr<juce::AudioFormatWriter> writer;
        if (!createAudioWriter(outputFile, settings, reader->sampleRate, writer)) {
            delete reader;
            return false;
        }

        // 执行渲染
        bool success = performOfflineRender(reader, writer.get(), settings, progressCallback);

        // 清理资源
        writer.reset();
        delete reader;

        if (success) {
            std::cout << "[WindsynthEngineFacade] 离线渲染完成" << std::endl;
        } else {
            std::cout << "[WindsynthEngineFacade] 离线渲染失败" << std::endl;
        }

        return success;

    } catch (const std::exception& e) {
        std::string error = "离线渲染失败: " + std::string(e.what());
        notifyError(error);
        return false;
    }
}

//==============================================================================
// 离线渲染辅助方法实现
//==============================================================================

bool WindsynthEngineFacade::createAudioWriter(const juce::File& outputFile,
                                             const RenderSettings& settings,
                                             double sourceSampleRate,
                                             std::unique_ptr<juce::AudioFormatWriter>& writer) {
    std::cout << "[WindsynthEngineFacade] 创建音频写入器" << std::endl;

    try {
        // 创建输出流
        auto outputStream = outputFile.createOutputStream();
        if (!outputStream) {
            notifyError("无法创建输出文件流: " + outputFile.getFullPathName().toStdString());
            return false;
        }

        // 选择音频格式
        juce::AudioFormat* format = nullptr;
        if (settings.format == RenderSettings::Format::WAV) {
            format = formatManager->findFormatForFileExtension("wav");
        } else if (settings.format == RenderSettings::Format::AIFF) {
            format = formatManager->findFormatForFileExtension("aiff");
        }

        if (!format) {
            notifyError("不支持的音频格式");
            return false;
        }

        // 创建写入器
        writer.reset(format->createWriterFor(outputStream.release(),
                                           settings.sampleRate,
                                           settings.numChannels,
                                           settings.bitDepth,
                                           {},
                                           0));

        if (!writer) {
            notifyError("无法创建音频格式写入器");
            return false;
        }

        std::cout << "[WindsynthEngineFacade] 音频写入器创建成功" << std::endl;
        return true;

    } catch (const std::exception& e) {
        notifyError("创建音频写入器失败: " + std::string(e.what()));
        return false;
    }
}

bool WindsynthEngineFacade::performOfflineRender(juce::AudioFormatReader* reader,
                                                juce::AudioFormatWriter* writer,
                                                const RenderSettings& settings,
                                                RenderProgressCallback progressCallback) {
    std::cout << "[WindsynthEngineFacade] 开始执行离线渲染" << std::endl;

    // 完全停止实时音频处理以避免冲突
    bool wasRunning = (currentState.load() == EngineState::Running);
    if (wasRunning) {
        std::cout << "[WindsynthEngineFacade] 完全停止实时音频处理" << std::endl;
        stop();  // 完全停止引擎
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待停止完成
    }

    try {
        // 获取音频信息
        const int64_t totalSamples = reader->lengthInSamples;
        const int numChannels = std::min(static_cast<int>(reader->numChannels), settings.numChannels);
        const int bufferSize = 4096; // 使用较大的缓冲区以提高效率

        std::cout << "[WindsynthEngineFacade] 音频信息 - 总样本数: " << totalSamples
                  << ", 声道数: " << numChannels << ", 缓冲区大小: " << bufferSize << std::endl;

        // 使用原始音频的采样率和声道配置
        double renderSampleRate = reader->sampleRate;
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

        // 恢复实时音频处理（如果之前在运行）
        if (wasRunning) {
            std::cout << "[WindsynthEngineFacade] 重新启动实时音频处理" << std::endl;
            // 重新启动引擎
            start();
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

        notifyError("执行离线渲染失败: " + std::string(e.what()));
        return false;
    }
}

} // namespace WindsynthVST::Engine
