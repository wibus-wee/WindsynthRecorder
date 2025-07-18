#include "AudioProcessingChain.hpp"
#include <iostream>

namespace WindsynthVST {

// ProcessingNode 实现
ProcessingNode::ProcessingNode(std::unique_ptr<VSTPluginInstance> plugin)
    : plugin(std::move(plugin)) {
    if (this->plugin) {
        name = this->plugin->getName();
    }
}

ProcessingNode::~ProcessingNode() {
    if (isPrepared) {
        releaseResources();
    }
}

void ProcessingNode::prepareToPlay(double sampleRate, int samplesPerBlock) {
    if (plugin && enabled) {
        plugin->prepareToPlay(sampleRate, samplesPerBlock);
        isPrepared = true;
    }
}

void ProcessingNode::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (plugin && enabled && isPrepared && !bypassed) {
        plugin->processBlock(buffer, midiMessages);
    }
}

void ProcessingNode::releaseResources() {
    if (plugin && isPrepared) {
        plugin->releaseResources();
        isPrepared = false;
    }
}

void ProcessingNode::saveState(juce::MemoryBlock& destData) {
    if (plugin) {
        plugin->getStateInformation(destData);
    }
}

void ProcessingNode::loadState(const void* data, int sizeInBytes) {
    if (plugin) {
        plugin->setStateInformation(data, sizeInBytes);
    }
}

// AudioProcessingChain 实现
AudioProcessingChain::AudioProcessingChain() {
    processingTimes.reserve(100); // 预分配空间用于性能统计
}

AudioProcessingChain::~AudioProcessingChain() {
    if (isPrepared) {
        releaseResources();
    }
}

void AudioProcessingChain::configure(const ProcessingChainConfig& newConfig) {
    juce::ScopedLock sl(lock);
    config = newConfig;
    
    // 如果已经准备好了，需要重新准备
    if (isPrepared) {
        releaseResources();
        prepareToPlay(config.sampleRate, config.samplesPerBlock);
    }
}

void AudioProcessingChain::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ScopedLock sl(lock);
    
    config.sampleRate = sampleRate;
    config.samplesPerBlock = samplesPerBlock;
    
    // 准备内部缓冲区
    internalBuffer.setSize(config.numOutputChannels, samplesPerBlock);
    
    // 准备所有插件节点
    for (auto& node : nodes) {
        if (node) {
            node->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
    
    isPrepared = true;
    resetPerformanceStats();
}

void AudioProcessingChain::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (!isPrepared || !enabled || masterBypass) {
        return;
    }
    
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // 预处理回调
    if (preProcessingCallback) {
        preProcessingCallback(buffer, midiMessages);
    }
    
    // 处理每个插件节点
    {
        juce::ScopedLock sl(lock);
        
        for (auto& node : nodes) {
            if (node && node->isEnabled()) {
                try {
                    node->processBlock(buffer, midiMessages);
                } catch (const std::exception& e) {
                    onError("插件处理错误: " + node->getName() + " - " + e.what());
                    node->setEnabled(false); // 禁用有问题的插件
                }
            }
        }
    }
    
    // 后处理回调
    if (postProcessingCallback) {
        postProcessingCallback(buffer, midiMessages);
    }
    
    // 更新性能统计
    auto endTime = juce::Time::getHighResolutionTicks();
    double processingTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0; // 转换为毫秒
    updatePerformanceStats(processingTime);
}

void AudioProcessingChain::releaseResources() {
    juce::ScopedLock sl(lock);
    
    for (auto& node : nodes) {
        if (node) {
            node->releaseResources();
        }
    }
    
    isPrepared = false;
}

bool AudioProcessingChain::addPlugin(std::unique_ptr<VSTPluginInstance> plugin) {
    std::cout << "[AudioProcessingChain] addPlugin: Starting plugin addition" << std::endl;

    if (!plugin) {
        std::cout << "[AudioProcessingChain] addPlugin: Plugin is null" << std::endl;
        onError("尝试添加空插件");
        return false;
    }

    std::cout << "[AudioProcessingChain] addPlugin: Plugin is valid, acquiring lock" << std::endl;
    juce::ScopedLock sl(lock);

    std::cout << "[AudioProcessingChain] addPlugin: Creating ProcessingNode" << std::endl;
    auto node = std::make_unique<ProcessingNode>(std::move(plugin));

    if (isPrepared) {
        std::cout << "[AudioProcessingChain] addPlugin: Chain is prepared, preparing node" << std::endl;
        node->prepareToPlay(config.sampleRate, config.samplesPerBlock);
    } else {
        std::cout << "[AudioProcessingChain] addPlugin: Chain is not prepared, skipping node preparation" << std::endl;
    }

    std::cout << "[AudioProcessingChain] addPlugin: Adding node to chain" << std::endl;
    nodes.push_back(std::move(node));
    std::cout << "[AudioProcessingChain] addPlugin: Plugin successfully added, total plugins: " << nodes.size() << std::endl;
    return true;
}

bool AudioProcessingChain::insertPlugin(int index, std::unique_ptr<VSTPluginInstance> plugin) {
    if (!plugin || index < 0 || index > static_cast<int>(nodes.size())) {
        onError("插件插入位置无效");
        return false;
    }
    
    juce::ScopedLock sl(lock);
    
    auto node = std::make_unique<ProcessingNode>(std::move(plugin));
    
    if (isPrepared) {
        node->prepareToPlay(config.sampleRate, config.samplesPerBlock);
    }
    
    nodes.insert(nodes.begin() + index, std::move(node));
    return true;
}

bool AudioProcessingChain::removePlugin(int index) {
    if (!validateIndex(index)) {
        return false;
    }
    
    juce::ScopedLock sl(lock);
    nodes.erase(nodes.begin() + index);
    return true;
}

bool AudioProcessingChain::movePlugin(int fromIndex, int toIndex) {
    if (!validateIndex(fromIndex) || toIndex < 0 || toIndex >= static_cast<int>(nodes.size())) {
        return false;
    }
    
    juce::ScopedLock sl(lock);
    
    auto node = std::move(nodes[fromIndex]);
    nodes.erase(nodes.begin() + fromIndex);
    
    if (toIndex > fromIndex) {
        toIndex--; // 调整索引，因为我们已经删除了一个元素
    }
    
    nodes.insert(nodes.begin() + toIndex, std::move(node));
    return true;
}

void AudioProcessingChain::clearPlugins() {
    juce::ScopedLock sl(lock);
    nodes.clear();
}

ProcessingNode* AudioProcessingChain::getNode(int index) {
    if (!validateIndex(index)) {
        return nullptr;
    }
    return nodes[index].get();
}

const ProcessingNode* AudioProcessingChain::getNode(int index) const {
    if (!validateIndex(index)) {
        return nullptr;
    }
    return nodes[index].get();
}

int AudioProcessingChain::findPluginIndex(const std::string& pluginName) const {
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        if (nodes[i] && nodes[i]->getName() == pluginName) {
            return i;
        }
    }
    return -1;
}

ProcessingNode* AudioProcessingChain::findPlugin(const std::string& pluginName) {
    int index = findPluginIndex(pluginName);
    return index >= 0 ? getNode(index) : nullptr;
}

void AudioProcessingChain::setPluginBypassed(int index, bool bypassed) {
    if (auto* node = getNode(index)) {
        node->setBypass(bypassed);
    }
}

bool AudioProcessingChain::isPluginBypassed(int index) const {
    if (auto* node = getNode(index)) {
        return node->isBypassed();
    }
    return false;
}

int AudioProcessingChain::getTotalLatency() const {
    int totalLatency = 0;
    
    for (const auto& node : nodes) {
        if (node && node->isEnabled() && !node->isBypassed()) {
            if (auto* plugin = node->getPlugin()) {
                if (auto* rawInstance = plugin->getRawInstance()) {
                    totalLatency += rawInstance->getLatencySamples();
                }
            }
        }
    }
    
    return totalLatency;
}

void AudioProcessingChain::resetPerformanceStats() {
    stats = PerformanceStats();
    processingTimes.clear();
}

void AudioProcessingChain::updatePerformanceStats(double processingTime) {
    processingTimes.push_back(processingTime);
    
    // 保持最近100次的统计
    if (processingTimes.size() > 100) {
        processingTimes.erase(processingTimes.begin());
    }
    
    // 计算平均值和峰值
    double sum = 0.0;
    double peak = 0.0;
    
    for (double time : processingTimes) {
        sum += time;
        peak = std::max(peak, time);
    }
    
    stats.averageProcessingTime = sum / processingTimes.size();
    stats.peakProcessingTime = peak;
    
    // 计算CPU使用率（基于缓冲区大小和采样率）
    double bufferDuration = (config.samplesPerBlock / config.sampleRate) * 1000.0; // 毫秒
    stats.cpuUsagePercent = (stats.averageProcessingTime / bufferDuration) * 100.0;
}

AudioProcessingChain::ChainPreset AudioProcessingChain::savePreset(const std::string& name) const {
    ChainPreset preset;
    preset.name = name;
    preset.config = config;
    
    for (const auto& node : nodes) {
        if (node) {
            juce::MemoryBlock state;
            node->saveState(state);
            preset.pluginStates.push_back(state);
            preset.pluginBypassed.push_back(node->isBypassed());
        }
    }
    
    return preset;
}

bool AudioProcessingChain::loadPreset(const ChainPreset& preset) {
    if (preset.pluginStates.size() != nodes.size()) {
        onError("预设与当前插件链不匹配");
        return false;
    }
    
    juce::ScopedLock sl(lock);
    
    // 加载配置
    config = preset.config;
    
    // 加载插件状态
    for (size_t i = 0; i < nodes.size() && i < preset.pluginStates.size(); ++i) {
        if (nodes[i]) {
            nodes[i]->loadState(preset.pluginStates[i].getData(), 
                              static_cast<int>(preset.pluginStates[i].getSize()));
            
            if (i < preset.pluginBypassed.size()) {
                nodes[i]->setBypass(preset.pluginBypassed[i]);
            }
        }
    }
    
    return true;
}

void AudioProcessingChain::onError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
}

bool AudioProcessingChain::validateIndex(int index) const {
    return index >= 0 && index < static_cast<int>(nodes.size());
}

} // namespace WindsynthVST
