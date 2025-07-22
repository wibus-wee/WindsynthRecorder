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

    // 设置初始的I/O节点父图引用
    updateIONodesParentGraph();

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

    // 准备传输源（如果存在）
    if (transportSource) {
        transportSource->prepareToPlay(samplesPerBlock, sampleRate);
    }

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

    // 释放传输源资源
    if (transportSource) {
        transportSource->releaseResources();
    }

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

    // 如果有音频文件播放，先处理transportSource
    if (transportSource != nullptr) {
        // 确保缓冲区大小匹配
        if (transportBuffer.getNumChannels() != buffer.getNumChannels() ||
            transportBuffer.getNumSamples() != buffer.getNumSamples()) {
            transportBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples());
        }

        // 清空传输缓冲区
        transportBuffer.clear();

        // 从transportSource获取音频数据
        juce::AudioSourceChannelInfo channelInfo(&transportBuffer, 0, buffer.getNumSamples());
        transportSource->getNextAudioBlock(channelInfo);

        // 检查是否有音频数据
        float maxLevel = 0.0f;
        for (int channel = 0; channel < transportBuffer.getNumChannels(); ++channel) {
            auto* channelData = transportBuffer.getReadPointer(channel);
            for (int sample = 0; sample < transportBuffer.getNumSamples(); ++sample) {
                maxLevel = std::max(maxLevel, std::abs(channelData[sample]));
            }
        }

        static int debugCounter = 0;
        if (++debugCounter % 1000 == 0 && maxLevel > 0.001f) { // 每1000个块输出一次，且有信号时
            std::cout << "[GraphAudioProcessor] 音频文件信号电平: " << maxLevel << std::endl;
        }

        // 将传输音频添加到主缓冲区
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            if (channel < transportBuffer.getNumChannels()) {
                buffer.addFrom(channel, 0, transportBuffer, channel, 0, buffer.getNumSamples());
            }
        }
    }

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

void GraphAudioProcessor::processBlockWithInput(const juce::AudioBuffer<float>& inputBuffer,
                                               juce::AudioBuffer<float>& outputBuffer,
                                               juce::MidiBuffer& midiMessages) {
    if (!isGraphReady()) {
        outputBuffer.clear();
        return;
    }

    // 记录处理开始时间
    auto startTime = juce::Time::getHighResolutionTicks();

    // 创建一个临时缓冲区来处理音频图
    // 音频图需要一个可读写的缓冲区，但我们不希望直接修改输入
    juce::AudioBuffer<float> processingBuffer;

    // 设置处理缓冲区的大小以匹配输出
    processingBuffer.setSize(outputBuffer.getNumChannels(), outputBuffer.getNumSamples());
    processingBuffer.clear();

    // 将输入数据复制到处理缓冲区（仅用于音频图内部处理）
    int channelsToCopy = std::min(inputBuffer.getNumChannels(), processingBuffer.getNumChannels());
    for (int ch = 0; ch < channelsToCopy; ++ch) {
        processingBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, inputBuffer.getNumSamples());
    }

    // 处理音频图
    audioGraph.processBlock(processingBuffer, midiMessages);

    // 将处理结果复制到输出缓冲区
    outputBuffer.makeCopyOf(processingBuffer);

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

void GraphAudioProcessor::setTransportSource(juce::AudioTransportSource* source) {
    std::cout << "[GraphAudioProcessor] 设置传输源: " << (source ? "有效" : "空") << std::endl;
    transportSource = source;

    if (source && isConfigured.load()) {
        // 如果已经配置过，需要准备传输源
        source->prepareToPlay(currentConfig.samplesPerBlock, currentConfig.sampleRate);
    }
}

//==============================================================================
// AudioIODeviceCallback 接口实现
//==============================================================================

void GraphAudioProcessor::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                          int numInputChannels,
                                                          float* const* outputChannelData,
                                                          int numOutputChannels,
                                                          int numSamples,
                                                          const juce::AudioIODeviceCallbackContext& context) {
    // 创建输入和输出缓冲区
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    juce::MidiBuffer midiBuffer;

    // 清空输出缓冲区
    outputBuffer.clear();

    // 如果有输入，创建输入缓冲区（只读）
    if (inputChannelData != nullptr && numInputChannels > 0) {
        // 创建输入缓冲区的只读视图
        inputBuffer = juce::AudioBuffer<float>(const_cast<float**>(inputChannelData), numInputChannels, numSamples);

        // 将输入数据提供给音频图处理
        // 注意：这里不直接复制到输出，而是让音频图决定如何处理
        processBlockWithInput(inputBuffer, outputBuffer, midiBuffer);
    } else {
        // 没有输入时，只处理输出
        processBlock(outputBuffer, midiBuffer);
    }
}

void GraphAudioProcessor::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    std::cout << "[GraphAudioProcessor] 音频设备即将启动" << std::endl;

    if (device) {
        double sampleRate = device->getCurrentSampleRate();
        int bufferSize = device->getCurrentBufferSizeSamples();

        std::cout << "[GraphAudioProcessor] 设备参数: " << sampleRate << "Hz, " << bufferSize << " samples" << std::endl;

        // 不要在这里调用prepareToPlay，因为设备管理器已经在准备过程中
        // prepareToPlay应该由AudioIOManager在适当的时候调用
    }
}

void GraphAudioProcessor::audioDeviceStopped() {
    std::cout << "[GraphAudioProcessor] 音频设备已停止" << std::endl;
    // 不要在这里调用releaseResources，因为可能导致资源管理冲突
    // releaseResources应该由AudioIOManager在适当的时候调用
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
    // 创建XML来保存状态
    auto xml = std::make_unique<juce::XmlElement>("GraphAudioProcessorState");

    // 保存配置
    auto configXml = xml->createNewChildElement("Configuration");
    configXml->setAttribute("sampleRate", currentConfig.sampleRate);
    configXml->setAttribute("samplesPerBlock", currentConfig.samplesPerBlock);
    configXml->setAttribute("numInputChannels", currentConfig.numInputChannels);
    configXml->setAttribute("numOutputChannels", currentConfig.numOutputChannels);
    configXml->setAttribute("enableMidi", currentConfig.enableMidi);

    // 保存图状态
    juce::MemoryBlock graphData;
    audioGraph.getStateInformation(graphData);
    if (graphData.getSize() > 0) {
        auto graphXml = xml->createNewChildElement("GraphState");
        graphXml->addTextElement(juce::Base64::toBase64(graphData.getData(), graphData.getSize()));
    }

    // 转换为内存块
    copyXmlToBinary(*xml, destData);
}

void GraphAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // 从内存块解析XML
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || xml->getTagName() != "GraphAudioProcessorState") {
        return;
    }

    // 恢复配置
    auto configXml = xml->getChildByName("Configuration");
    if (configXml) {
        GraphConfig newConfig;
        newConfig.sampleRate = configXml->getDoubleAttribute("sampleRate", 44100.0);
        newConfig.samplesPerBlock = configXml->getIntAttribute("samplesPerBlock", 512);
        newConfig.numInputChannels = configXml->getIntAttribute("numInputChannels", 2);
        newConfig.numOutputChannels = configXml->getIntAttribute("numOutputChannels", 2);
        newConfig.enableMidi = configXml->getBoolAttribute("enableMidi", true);

        configure(newConfig);
    }

    // 恢复图状态
    auto graphXml = xml->getChildByName("GraphState");
    if (graphXml) {
        juce::String base64Data = graphXml->getAllSubText();
        if (base64Data.isNotEmpty()) {
            juce::MemoryOutputStream stream;
            if (juce::Base64::convertFromBase64(stream, base64Data)) {
                audioGraph.setStateInformation(stream.getData(), static_cast<int>(stream.getDataSize()));
            }
        }
    }
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

    // 更新AudioProcessorGraph的通道配置
    updateGraphChannelConfiguration(config);

    if (needsReinitialization && isConfigured.load()) {
        // 如果已经配置过，需要重新初始化
        releaseResources();
        prepareToPlay(config.sampleRate, config.samplesPerBlock);
    }

    // 在配置更新后创建默认连接
    createDefaultPassthroughConnections();

    notifyStateChange("音频图配置已更新");
}

//==============================================================================
// 内部方法
//==============================================================================

void GraphAudioProcessor::initializeIONodes() {
    std::cout << "[GraphAudioProcessor] 初始化I/O节点" << std::endl;

    // 创建音频输入节点（不立即设置父图）
    auto audioInputProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    audioInputNodeID = audioGraph.addNode(std::move(audioInputProcessor))->nodeID;

    // 创建音频输出节点（不立即设置父图）
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

void GraphAudioProcessor::updateIONodesParentGraph() {
    std::cout << "[GraphAudioProcessor] 更新I/O节点父图引用" << std::endl;

    // 更新I/O节点的父图引用，这会触发它们重新配置通道数
    auto* inputNode = audioGraph.getNodeForId(audioInputNodeID);
    auto* outputNode = audioGraph.getNodeForId(audioOutputNodeID);
    auto* midiInputNode = audioGraph.getNodeForId(midiInputNodeID);
    auto* midiOutputNode = audioGraph.getNodeForId(midiOutputNodeID);

    if (inputNode && inputNode->getProcessor()) {
        if (auto* ioProcessor = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(inputNode->getProcessor())) {
            ioProcessor->setParentGraph(&audioGraph);
        }
    }

    if (outputNode && outputNode->getProcessor()) {
        if (auto* ioProcessor = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(outputNode->getProcessor())) {
            ioProcessor->setParentGraph(&audioGraph);
        }
    }

    if (midiInputNode && midiInputNode->getProcessor()) {
        if (auto* ioProcessor = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(midiInputNode->getProcessor())) {
            ioProcessor->setParentGraph(&audioGraph);
        }
    }

    if (midiOutputNode && midiOutputNode->getProcessor()) {
        if (auto* ioProcessor = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(midiOutputNode->getProcessor())) {
            ioProcessor->setParentGraph(&audioGraph);
        }
    }

    std::cout << "[GraphAudioProcessor] I/O节点父图引用更新完成" << std::endl;
}

void GraphAudioProcessor::createDefaultPassthroughConnections() {
    std::cout << "[GraphAudioProcessor] 创建默认直通连接" << std::endl;
    std::cout << "[GraphAudioProcessor] 音频输入节点ID: " << audioInputNodeID.uid << std::endl;
    std::cout << "[GraphAudioProcessor] 音频输出节点ID: " << audioOutputNodeID.uid << std::endl;

    // 检查I/O节点的通道配置
    auto* inputNode = audioGraph.getNodeForId(audioInputNodeID);
    auto* outputNode = audioGraph.getNodeForId(audioOutputNodeID);

    if (inputNode && inputNode->getProcessor()) {
        auto* inputProcessor = inputNode->getProcessor();
        std::cout << "[GraphAudioProcessor] 输入节点通道数: 输入=" << inputProcessor->getTotalNumInputChannels()
                  << ", 输出=" << inputProcessor->getTotalNumOutputChannels() << std::endl;
    }

    if (outputNode && outputNode->getProcessor()) {
        auto* outputProcessor = outputNode->getProcessor();
        std::cout << "[GraphAudioProcessor] 输出节点通道数: 输入=" << outputProcessor->getTotalNumInputChannels()
                  << ", 输出=" << outputProcessor->getTotalNumOutputChannels() << std::endl;
    }

    // 首先检查 audioGraph 的总线配置
    std::cout << "[GraphAudioProcessor] audioGraph 总线配置 - 输入通道: " << audioGraph.getTotalNumInputChannels()
              << ", 输出通道: " << audioGraph.getTotalNumOutputChannels() << std::endl;

    // 创建立体声直通连接（左声道和右声道）
    Connection leftConnection = makeAudioConnection(audioInputNodeID, 0, audioOutputNodeID, 0);
    Connection rightConnection = makeAudioConnection(audioInputNodeID, 1, audioOutputNodeID, 1);

    std::cout << "[GraphAudioProcessor] 检查左声道连接合法性..." << std::endl;
    std::cout << "[GraphAudioProcessor] 左声道连接: 输入节点" << audioInputNodeID.uid << "[通道0] -> 输出节点" << audioOutputNodeID.uid << "[通道0]" << std::endl;

    // 详细检查连接合法性的各个条件
    auto* sourceNode = audioGraph.getNodeForId(audioInputNodeID);
    auto* destNode = audioGraph.getNodeForId(audioOutputNodeID);

    if (!sourceNode) {
        std::cout << "[GraphAudioProcessor] 错误: 源节点不存在" << std::endl;
    } else {
        auto* sourceProcessor = sourceNode->getProcessor();
        std::cout << "[GraphAudioProcessor] 源节点输出通道数: " << sourceProcessor->getTotalNumOutputChannels() << std::endl;
    }

    if (!destNode) {
        std::cout << "[GraphAudioProcessor] 错误: 目标节点不存在" << std::endl;
    } else {
        auto* destProcessor = destNode->getProcessor();
        std::cout << "[GraphAudioProcessor] 目标节点输入通道数: " << destProcessor->getTotalNumInputChannels() << std::endl;
    }

    if (audioGraph.isConnectionLegal(leftConnection)) {
        bool leftSuccess = audioGraph.addConnection(leftConnection);
        std::cout << "[GraphAudioProcessor] 左声道直通连接: " << (leftSuccess ? "成功" : "失败") << std::endl;
    } else {
        std::cout << "[GraphAudioProcessor] 左声道连接不合法" << std::endl;
    }

    std::cout << "[GraphAudioProcessor] 检查右声道连接合法性..." << std::endl;
    if (audioGraph.isConnectionLegal(rightConnection)) {
        bool rightSuccess = audioGraph.addConnection(rightConnection);
        std::cout << "[GraphAudioProcessor] 右声道直通连接: " << (rightSuccess ? "成功" : "失败") << std::endl;
    } else {
        std::cout << "[GraphAudioProcessor] 右声道连接不合法" << std::endl;
    }

    // 创建MIDI直通连接
    Connection midiConnection = makeMidiConnection(midiInputNodeID, midiOutputNodeID);
    std::cout << "[GraphAudioProcessor] 检查MIDI连接合法性..." << std::endl;
    if (audioGraph.isConnectionLegal(midiConnection)) {
        bool midiSuccess = audioGraph.addConnection(midiConnection);
        std::cout << "[GraphAudioProcessor] MIDI直通连接: " << (midiSuccess ? "成功" : "失败") << std::endl;
    } else {
        std::cout << "[GraphAudioProcessor] MIDI连接不合法" << std::endl;
    }

    // 输出当前连接状态
    auto connections = audioGraph.getConnections();
    std::cout << "[GraphAudioProcessor] 当前连接数量: " << connections.size() << std::endl;

    std::cout << "[GraphAudioProcessor] 默认直通连接创建完成" << std::endl;
}

void GraphAudioProcessor::autoConnectPluginToAudioPath(NodeID pluginNodeID) {
    std::cout << "[GraphAudioProcessor] 自动连接插件到音频路径：" << pluginNodeID.uid << std::endl;

    // 获取插件信息
    auto pluginInfo = getNodeInfo(pluginNodeID);
    if (pluginInfo.nodeID.uid == 0) {
        std::cout << "[GraphAudioProcessor] 插件节点无效" << std::endl;
        return;
    }

    std::cout << "[GraphAudioProcessor] 插件信息 - 输入通道: " << pluginInfo.numInputChannels
              << ", 输出通道: " << pluginInfo.numOutputChannels << std::endl;

    // 如果插件有音频输入输出，将其插入到音频路径中
    if (pluginInfo.numInputChannels > 0 && pluginInfo.numOutputChannels > 0) {
        // 断开现有的直通连接
        std::cout << "[GraphAudioProcessor] 断开现有的直通连接" << std::endl;
        auto connections = getAllConnections();
        for (const auto& connInfo : connections) {
            const auto& conn = connInfo.connection;
            // 查找输入到输出的直通连接
            if (conn.source.nodeID == audioInputNodeID &&
                conn.destination.nodeID == audioOutputNodeID &&
                conn.source.channelIndex != juce::AudioProcessorGraph::midiChannelIndex) {
                audioGraph.removeConnection(conn);
                std::cout << "[GraphAudioProcessor] 已断开直通连接：通道 " << conn.source.channelIndex << std::endl;
            }
        }

        // 连接：音频输入 → 插件 → 音频输出
        int maxInputChannels = std::min(2, pluginInfo.numInputChannels);  // 最多连接立体声
        int maxOutputChannels = std::min(2, pluginInfo.numOutputChannels);

        std::cout << "[GraphAudioProcessor] 连接音频输入到插件" << std::endl;
        for (int ch = 0; ch < maxInputChannels; ++ch) {
            if (connectAudio(audioInputNodeID, ch, pluginNodeID, ch)) {
                std::cout << "[GraphAudioProcessor] 已连接输入通道 " << ch << " 到插件" << std::endl;
            }
        }

        std::cout << "[GraphAudioProcessor] 连接插件到音频输出" << std::endl;
        for (int ch = 0; ch < maxOutputChannels; ++ch) {
            if (connectAudio(pluginNodeID, ch, audioOutputNodeID, ch)) {
                std::cout << "[GraphAudioProcessor] 已连接插件通道 " << ch << " 到输出" << std::endl;
            }
        }

        std::cout << "[GraphAudioProcessor] 插件已成功插入音频路径" << std::endl;
    } else {
        std::cout << "[GraphAudioProcessor] 插件没有音频输入输出，跳过音频连接" << std::endl;
    }
}

void GraphAudioProcessor::updateGraphChannelConfiguration(const GraphConfig& config) {
    std::cout << "[GraphAudioProcessor] 更新音频图通道配置" << std::endl;

    // 设置AudioProcessorGraph的通道配置
    // 这会影响AudioGraphIOProcessor的通道数
    juce::AudioChannelSet inputChannelSet = juce::AudioChannelSet::canonicalChannelSet(config.numInputChannels);
    juce::AudioChannelSet outputChannelSet = juce::AudioChannelSet::canonicalChannelSet(config.numOutputChannels);

    // 更新 GraphAudioProcessor 的总线配置
    if (getBusCount(true) > 0) {
        setChannelLayoutOfBus(true, 0, inputChannelSet);
    }
    if (getBusCount(false) > 0) {
        setChannelLayoutOfBus(false, 0, outputChannelSet);
    }

    // 强制更新缓存的通道数
    setBusesLayout(getBusesLayout());

    std::cout << "[GraphAudioProcessor] 当前总线配置 - 输入通道: " << getTotalNumInputChannels()
              << ", 输出通道: " << getTotalNumOutputChannels() << std::endl;

    // 关键：为内部的 audioGraph 设置相同的总线配置
    juce::AudioProcessor::BusesLayout graphLayout;
    graphLayout.inputBuses.add(inputChannelSet);
    graphLayout.outputBuses.add(outputChannelSet);

    std::cout << "[GraphAudioProcessor] 设置 audioGraph 总线配置..." << std::endl;
    bool layoutSuccess = audioGraph.setBusesLayout(graphLayout);
    std::cout << "[GraphAudioProcessor] audioGraph 总线配置设置" << (layoutSuccess ? "成功" : "失败") << std::endl;
    std::cout << "[GraphAudioProcessor] audioGraph 总线配置 - 输入通道: " << audioGraph.getTotalNumInputChannels()
              << ", 输出通道: " << audioGraph.getTotalNumOutputChannels() << std::endl;

    // 更新I/O节点的父图引用，这会触发它们重新配置通道数
    updateIONodesParentGraph();

    std::cout << "[GraphAudioProcessor] 音频图通道配置更新完成" << std::endl;
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
    return NodeID{static_cast<juce::uint32>(nodeIDCounter.fetch_add(1))};
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

        // 自动将插件连接到音频路径
        autoConnectPluginToAudioPath(node->nodeID);

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
            info.pluginName = node->getProcessor()->getName().toStdString(); // 修复：设置插件名称
            info.enabled = !node->isBypassed(); // 修复：设置启用状态
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
    info.pluginName = node->getProcessor()->getName().toStdString();
    info.enabled = !node->isBypassed();
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
