//
//  GraphAudioProcessor.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  基于JUCE AudioProcessorGraph的高性能音频处理器实现
//

#include "GraphAudioProcessor.hpp"
#include <iostream>
#include <algorithm>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

GraphAudioProcessor::GraphAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    std::cout << "[GraphAudioProcessor] 构造函数：初始化音频图处理器" << std::endl;
    
    // 预分配性能统计历史记录空间
    processingTimeHistory.reserve(Constants::PERFORMANCE_STATS_HISTORY_SIZE);
    
    // 初始化I/O节点
    initializeIONodes();
    
    std::cout << "[GraphAudioProcessor] 构造完成" << std::endl;
}

GraphAudioProcessor::~GraphAudioProcessor() {
    std::cout << "[GraphAudioProcessor] 析构函数：清理资源" << std::endl;
    
    if (isGraphReady()) {
        releaseResources();
    }
}

//==============================================================================
// AudioProcessor 接口实现
//==============================================================================

const juce::String GraphAudioProcessor::getName() const {
    return "WindsynthVST AudioGraph";
}

void GraphAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    std::cout << "[GraphAudioProcessor] prepareToPlay: " << sampleRate << "Hz, " 
              << samplesPerBlock << " samples" << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    // 更新配置
    currentConfig.sampleRate = sampleRate;
    currentConfig.samplesPerBlock = samplesPerBlock;
    
    // 准备内部音频图
    audioGraph.prepareToPlay(sampleRate, samplesPerBlock);
    
    // 重置性能统计
    resetPerformanceStats();
    
    graphReady.store(true);
    isConfigured.store(true);
    
    notifyStateChange("音频图已准备就绪");
    std::cout << "[GraphAudioProcessor] prepareToPlay 完成" << std::endl;
}

void GraphAudioProcessor::releaseResources() {
    std::cout << "[GraphAudioProcessor] releaseResources" << std::endl;
    
    graphReady.store(false);
    audioGraph.releaseResources();
    
    notifyStateChange("音频图资源已释放");
}

void GraphAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, 
                                     juce::MidiBuffer& midiMessages) {
    if (!isGraphReady()) {
        buffer.clear();
        return;
    }
    
    // 记录处理开始时间
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // 处理音频图
    audioGraph.processBlock(buffer, midiMessages);
    
    // 计算处理时间并更新统计
    auto endTime = juce::Time::getHighResolutionTicks();
    double processingTimeMs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
    updatePerformanceStats(processingTimeMs);
}

void GraphAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, 
                                     juce::MidiBuffer& midiMessages) {
    if (!isGraphReady()) {
        buffer.clear();
        return;
    }
    
    // 记录处理开始时间
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // 处理音频图
    audioGraph.processBlock(buffer, midiMessages);
    
    // 计算处理时间并更新统计
    auto endTime = juce::Time::getHighResolutionTicks();
    double processingTimeMs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
    updatePerformanceStats(processingTimeMs);
}

bool GraphAudioProcessor::supportsDoublePrecisionProcessing() const {
    return audioGraph.supportsDoublePrecisionProcessing();
}

void GraphAudioProcessor::reset() {
    audioGraph.reset();
    resetPerformanceStats();
}

void GraphAudioProcessor::setNonRealtime(bool isNonRealtime) noexcept {
    AudioProcessor::setNonRealtime(isNonRealtime);
    audioGraph.setNonRealtime(isNonRealtime);
}

double GraphAudioProcessor::getTailLengthSeconds() const {
    return audioGraph.getTailLengthSeconds();
}

bool GraphAudioProcessor::acceptsMidi() const {
    return currentConfig.enableMidi;
}

bool GraphAudioProcessor::producesMidi() const {
    return currentConfig.enableMidi;
}

bool GraphAudioProcessor::isMidiEffect() const {
    return false;
}

bool GraphAudioProcessor::hasEditor() const {
    return false;
}

juce::AudioProcessorEditor* GraphAudioProcessor::createEditor() {
    return nullptr;
}

int GraphAudioProcessor::getNumPrograms() {
    return 1;
}

int GraphAudioProcessor::getCurrentProgram() {
    return 0;
}

void GraphAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String GraphAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return "Default";
}

void GraphAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

void GraphAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    audioGraph.getStateInformation(destData);
}

void GraphAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    audioGraph.setStateInformation(data, sizeInBytes);
}

//==============================================================================
// 图配置和管理
//==============================================================================

void GraphAudioProcessor::configure(const GraphConfig& config) {
    std::cout << "[GraphAudioProcessor] 配置音频图：" << config.sampleRate << "Hz, "
              << config.samplesPerBlock << " samples, " << config.numInputChannels 
              << " inputs, " << config.numOutputChannels << " outputs" << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    bool needsReinitialization = (currentConfig != config);
    currentConfig = config;
    
    if (needsReinitialization && isConfigured.load()) {
        // 如果已经配置过，需要重新初始化
        releaseResources();
        prepareToPlay(config.sampleRate, config.samplesPerBlock);
    }
    
    notifyStateChange("音频图配置已更新");
}

//==============================================================================
// 内部方法
//==============================================================================

void GraphAudioProcessor::initializeIONodes() {
    std::cout << "[GraphAudioProcessor] 初始化I/O节点" << std::endl;
    
    // 创建音频输入节点
    auto audioInputProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    audioInputNodeID = audioGraph.addNode(std::move(audioInputProcessor))->nodeID;
    
    // 创建音频输出节点
    auto audioOutputProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    audioOutputNodeID = audioGraph.addNode(std::move(audioOutputProcessor))->nodeID;
    
    // 创建MIDI输入节点
    auto midiInputProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
    midiInputNodeID = audioGraph.addNode(std::move(midiInputProcessor))->nodeID;
    
    // 创建MIDI输出节点
    auto midiOutputProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode);
    midiOutputNodeID = audioGraph.addNode(std::move(midiOutputProcessor))->nodeID;
    
    std::cout << "[GraphAudioProcessor] I/O节点初始化完成" << std::endl;
}

void GraphAudioProcessor::updatePerformanceStats(double processingTimeMs) {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    performanceStats.totalProcessedBlocks++;
    
    // 更新处理时间统计
    if (performanceStats.totalProcessedBlocks == 1) {
        performanceStats.minProcessingTimeMs = processingTimeMs;
        performanceStats.maxProcessingTimeMs = processingTimeMs;
        performanceStats.averageProcessingTimeMs = processingTimeMs;
    } else {
        performanceStats.minProcessingTimeMs = std::min(performanceStats.minProcessingTimeMs, processingTimeMs);
        performanceStats.maxProcessingTimeMs = std::max(performanceStats.maxProcessingTimeMs, processingTimeMs);
        
        // 计算移动平均值
        double alpha = 0.1; // 平滑因子
        performanceStats.averageProcessingTimeMs = 
            alpha * processingTimeMs + (1.0 - alpha) * performanceStats.averageProcessingTimeMs;
    }
    
    // 维护处理时间历史记录
    processingTimeHistory.push_back(processingTimeMs);
    if (processingTimeHistory.size() > Constants::PERFORMANCE_STATS_HISTORY_SIZE) {
        processingTimeHistory.erase(processingTimeHistory.begin());
    }
    
    // 计算CPU使用率（基于处理时间和缓冲区大小）
    double bufferDurationMs = (currentConfig.samplesPerBlock / currentConfig.sampleRate) * 1000.0;
    performanceStats.cpuUsagePercent = (processingTimeMs / bufferDurationMs) * 100.0;
    
    // 定期调用性能回调
    if (performanceCallback && (performanceStats.totalProcessedBlocks % 100 == 0)) {
        performanceCallback(performanceStats);
    }
}

void GraphAudioProcessor::handleError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
    
    std::cout << "[GraphAudioProcessor] 错误：" << error << std::endl;
    
    if (errorCallback) {
        errorCallback(error);
    }
}

void GraphAudioProcessor::notifyStateChange(const std::string& message) {
    std::cout << "[GraphAudioProcessor] 状态变化：" << message << std::endl;
    
    if (stateCallback) {
        stateCallback(message);
    }
}

bool GraphAudioProcessor::isValidNodeID(NodeID nodeID) const {
    return audioGraph.getNodeForId(nodeID) != nullptr;
}

NodeID GraphAudioProcessor::getNextNodeID() {
    return NodeID{nodeIDCounter.fetch_add(1)};
}

//==============================================================================
// 节点管理实现
//==============================================================================

NodeID GraphAudioProcessor::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin,
                                      const std::string& name) {
    if (!plugin) {
        handleError("尝试添加空的插件");
        return NodeID{0};
    }

    std::string pluginName = name.empty() ? plugin->getName().toStdString() : name;
    std::cout << "[GraphAudioProcessor] 添加插件：" << pluginName << std::endl;

    try {
        // 直接添加到AudioProcessorGraph - 这就是JUCE的设计！
        auto node = audioGraph.addNode(std::move(plugin));
        if (!node) {
            handleError("无法添加插件到音频图");
            return NodeID{0};
        }

        // 如果图已经准备就绪，需要准备新节点
        if (isGraphReady()) {
            node->getProcessor()->prepareToPlay(currentConfig.sampleRate,
                                               currentConfig.samplesPerBlock);
        }

        notifyStateChange("插件已添加：" + pluginName);
        return node->nodeID;

    } catch (const std::exception& e) {
        handleError("添加插件时发生异常：" + std::string(e.what()));
        return NodeID{0};
    }
}

bool GraphAudioProcessor::removeNode(NodeID nodeID) {
    if (!isValidNodeID(nodeID)) {
        handleError("尝试删除无效的节点ID");
        return false;
    }

    // 不允许删除I/O节点
    if (nodeID == audioInputNodeID || nodeID == audioOutputNodeID ||
        nodeID == midiInputNodeID || nodeID == midiOutputNodeID) {
        handleError("不能删除I/O节点");
        return false;
    }

    std::cout << "[GraphAudioProcessor] 删除节点：" << nodeID.uid << std::endl;

    try {
        auto removedNode = audioGraph.removeNode(nodeID);
        if (removedNode) {
            notifyStateChange("节点已删除");
            return true;
        } else {
            handleError("无法删除节点");
            return false;
        }
    } catch (const std::exception& e) {
        handleError("删除节点时发生异常：" + std::string(e.what()));
        return false;
    }
}

std::vector<NodeInfo> GraphAudioProcessor::getAllNodes() const {
    std::vector<NodeInfo> nodeInfos;

    for (auto* node : audioGraph.getNodes()) {
        if (node && node->getProcessor()) {
            NodeInfo info;
            info.nodeID = node->nodeID;
            info.name = node->getProcessor()->getName().toStdString();
            info.numInputChannels = node->getProcessor()->getTotalNumInputChannels();
            info.numOutputChannels = node->getProcessor()->getTotalNumOutputChannels();
            info.acceptsMidi = node->getProcessor()->acceptsMidi();
            info.producesMidi = node->getProcessor()->producesMidi();
            info.latencyInSamples = node->getProcessor()->getLatencySamples();
            info.bypassed = node->isBypassed();

            nodeInfos.push_back(info);
        }
    }

    return nodeInfos;
}

NodeInfo GraphAudioProcessor::getNodeInfo(NodeID nodeID) const {
    auto* node = audioGraph.getNodeForId(nodeID);
    if (!node || !node->getProcessor()) {
        return NodeInfo{};
    }

    NodeInfo info;
    info.nodeID = nodeID;
    info.name = node->getProcessor()->getName().toStdString();
    info.numInputChannels = node->getProcessor()->getTotalNumInputChannels();
    info.numOutputChannels = node->getProcessor()->getTotalNumOutputChannels();
    info.acceptsMidi = node->getProcessor()->acceptsMidi();
    info.producesMidi = node->getProcessor()->producesMidi();
    info.latencyInSamples = node->getProcessor()->getLatencySamples();
    info.bypassed = node->isBypassed();

    return info;
}

bool GraphAudioProcessor::setNodeBypassed(NodeID nodeID, bool bypassed) {
    auto* node = audioGraph.getNodeForId(nodeID);
    if (!node) {
        handleError("无法找到指定的节点");
        return false;
    }

    node->setBypassed(bypassed);
    notifyStateChange("节点旁路状态已更新");
    return true;
}

bool GraphAudioProcessor::setNodeEnabled(NodeID nodeID, bool enabled) {
    // JUCE AudioProcessorGraph没有直接的enabled概念，
    // 我们可以通过旁路来模拟
    return setNodeBypassed(nodeID, !enabled);
}

//==============================================================================
// 连接管理实现
//==============================================================================

bool GraphAudioProcessor::connectAudio(NodeID sourceNode, int sourceChannel,
                                      NodeID destNode, int destChannel) {
    if (!isValidNodeID(sourceNode) || !isValidNodeID(destNode)) {
        handleError("连接中包含无效的节点ID");
        return false;
    }

    Connection connection = makeAudioConnection(sourceNode, sourceChannel, destNode, destChannel);

    if (!audioGraph.isConnectionLegal(connection)) {
        handleError("尝试创建非法的音频连接");
        return false;
    }

    bool success = audioGraph.addConnection(connection);
    if (success) {
        notifyStateChange("音频连接已创建");
    } else {
        handleError("无法创建音频连接");
    }

    return success;
}

bool GraphAudioProcessor::connectMidi(NodeID sourceNode, NodeID destNode) {
    if (!isValidNodeID(sourceNode) || !isValidNodeID(destNode)) {
        handleError("连接中包含无效的节点ID");
        return false;
    }

    Connection connection = makeMidiConnection(sourceNode, destNode);

    if (!audioGraph.isConnectionLegal(connection)) {
        handleError("尝试创建非法的MIDI连接");
        return false;
    }

    bool success = audioGraph.addConnection(connection);
    if (success) {
        notifyStateChange("MIDI连接已创建");
    } else {
        handleError("无法创建MIDI连接");
    }

    return success;
}

bool GraphAudioProcessor::disconnect(const Connection& connection) {
    bool success = audioGraph.removeConnection(connection);
    if (success) {
        notifyStateChange("连接已断开");
    } else {
        handleError("无法断开连接");
    }

    return success;
}

bool GraphAudioProcessor::disconnectNode(NodeID nodeID) {
    if (!isValidNodeID(nodeID)) {
        handleError("尝试断开无效节点的连接");
        return false;
    }

    bool success = audioGraph.disconnectNode(nodeID);
    if (success) {
        notifyStateChange("节点的所有连接已断开");
    } else {
        handleError("无法断开节点连接");
    }

    return success;
}

std::vector<ConnectionInfo> GraphAudioProcessor::getAllConnections() const {
    std::vector<ConnectionInfo> connectionInfos;

    for (const auto& connection : audioGraph.getConnections()) {
        ConnectionInfo info;
        info.connection = connection;
        info.isAudioConnection = isAudioConnection(connection);

        // 获取源节点和目标节点的名称
        auto* sourceNode = audioGraph.getNodeForId(connection.source.nodeID);
        auto* destNode = audioGraph.getNodeForId(connection.destination.nodeID);

        if (sourceNode && sourceNode->getProcessor()) {
            info.sourceName = sourceNode->getProcessor()->getName().toStdString();
        }

        if (destNode && destNode->getProcessor()) {
            info.destinationName = destNode->getProcessor()->getName().toStdString();
        }

        connectionInfos.push_back(info);
    }

    return connectionInfos;
}

//==============================================================================
// 性能监控实现
//==============================================================================

GraphPerformanceStats GraphAudioProcessor::getPerformanceStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return performanceStats;
}

void GraphAudioProcessor::resetPerformanceStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    performanceStats.reset();
    processingTimeHistory.clear();
}

void GraphAudioProcessor::setPerformanceCallback(PerformanceCallback callback) {
    performanceCallback = std::move(callback);
}

//==============================================================================
// 错误处理和状态
//==============================================================================

void GraphAudioProcessor::setErrorCallback(GraphErrorCallback callback) {
    errorCallback = std::move(callback);
}

void GraphAudioProcessor::setStateCallback(GraphStateCallback callback) {
    stateCallback = std::move(callback);
}

std::string GraphAudioProcessor::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

} // namespace WindsynthVST::AudioGraph
