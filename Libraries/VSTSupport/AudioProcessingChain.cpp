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
        std::cout << "[ProcessingNode] Preparing plugin: " << sampleRate << "Hz, " << samplesPerBlock << " samples" << std::endl;
        plugin->prepareToPlay(sampleRate, samplesPerBlock);
        isPrepared = true;
        std::cout << "[ProcessingNode] Plugin prepared successfully" << std::endl;
    } else {
        std::cout << "[ProcessingNode] Cannot prepare: plugin=" << (plugin ? "exists" : "null")
                  << ", enabled=" << (enabled ? "true" : "false") << std::endl;
    }
}

void ProcessingNode::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (plugin && enabled && isPrepared && !bypassed) {
        plugin->processBlock(buffer, midiMessages);
    } else {
        static int logCount = 0;
        if (logCount++ % 1000 == 0) {
            std::cout << "[ProcessingNode] Cannot process: plugin=" << (plugin ? "exists" : "null")
                      << ", enabled=" << (enabled ? "true" : "false")
                      << ", prepared=" << (isPrepared ? "true" : "false") << std::endl;
        }
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

    std::cout << "[AudioProcessingChain] prepareToPlay: " << sampleRate << "Hz, " << samplesPerBlock
              << " samples, " << nodes.size() << " plugins" << std::endl;

    config.sampleRate = sampleRate;
    config.samplesPerBlock = samplesPerBlock;

    // 准备内部缓冲区
    internalBuffer.setSize(config.numOutputChannels, samplesPerBlock);

    // 准备所有插件节点
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]) {
            std::cout << "[AudioProcessingChain] Preparing plugin " << i << std::endl;
            nodes[i]->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }

    isPrepared = true;
    resetPerformanceStats();
    std::cout << "[AudioProcessingChain] prepareToPlay completed successfully" << std::endl;
}

void AudioProcessingChain::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (!isPrepared || !enabled || masterBypass) {
        // std::cout << "[AudioProcessingChain] processBlock: Skipping - isPrepared=" << isPrepared
        //           << ", enabled=" << enabled << ", masterBypass=" << masterBypass << std::endl;
        return;
    }

    // 验证缓冲区
    if (buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0) {
        // std::cout << "[AudioProcessingChain] processBlock: Invalid buffer - samples=" << buffer.getNumSamples()
        //           << ", channels=" << buffer.getNumChannels() << std::endl;
        return;
    }

    auto startTime = juce::Time::getHighResolutionTicks();

    // std::cout << "[AudioProcessingChain] processBlock: Processing " << buffer.getNumSamples()
    //           << " samples, " << buffer.getNumChannels() << " channels, " << nodes.size() << " plugins" << std::endl;

    // 预处理回调
    if (preProcessingCallback) {
        preProcessingCallback(buffer, midiMessages);
    }

    // 处理每个插件节点
    {
        juce::ScopedLock sl(lock);

        for (size_t i = 0; i < nodes.size(); ++i) {
            auto& node = nodes[i];
            if (node && node->isEnabled()) {
                try {
                    // std::cout << "[AudioProcessingChain] processBlock: Processing plugin " << i
                    //           << " (" << node->getName() << ")" << std::endl;
                    node->processBlock(buffer, midiMessages);
                    // std::cout << "[AudioProcessingChain] processBlock: Plugin " << i << " processed successfully" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "[AudioProcessingChain] processBlock: Plugin " << i << " error: " << e.what() << std::endl;
                    onError("插件处理错误: " + node->getName() + " - " + e.what());
                    node->setEnabled(false); // 禁用有问题的插件
                }
            } else {
                std::cout << "[AudioProcessingChain] processBlock: Skipping plugin " << i
                          << " (node=" << (node ? "valid" : "null")
                          << ", enabled=" << (node ? node->isEnabled() : false) << ")" << std::endl;
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

    // std::cout << "[AudioProcessingChain] processBlock: Completed in " << processingTime << "ms" << std::endl;
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

    // 检查对象是否有效
    if (this == nullptr) {
        std::cout << "[AudioProcessingChain] addPlugin: ERROR - this pointer is null!" << std::endl;
        return false;
    }

    // 验证 lock 对象是否有效
    try {
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
    } catch (const std::exception& e) {
        std::cout << "[AudioProcessingChain] addPlugin: Exception caught: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cout << "[AudioProcessingChain] addPlugin: Unknown exception caught" << std::endl;
        return false;
    }
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

// ============================================================================
// 插件编辑器管理实现
// ============================================================================

bool AudioProcessingChain::showPluginEditor(int index) {
    juce::ScopedLock scopedLock(lock);

    if (!validateIndex(index)) {
        onError("Invalid plugin index for editor: " + std::to_string(index));
        return false;
    }

    auto* node = nodes[index].get();
    if (!node) {
        onError("Plugin node not found at index: " + std::to_string(index));
        return false;
    }

    auto* plugin = node->getPlugin();
    if (!plugin) {
        onError("Plugin instance not found at index: " + std::to_string(index));
        return false;
    }

    if (!plugin->hasEditor()) {
        onError("Plugin at index " + std::to_string(index) + " has no editor");
        return false;
    }

    // 如果窗口已经存在，直接显示
    auto it = editorWindows.find(index);
    if (it != editorWindows.end() && it->second) {
        it->second->setVisible(true);
        it->second->toFront(true);
        return true;
    }

    // 创建新的编辑器窗口
    auto* editor = plugin->createEditor();
    if (!editor) {
        onError("Failed to create editor for plugin at index: " + std::to_string(index));
        return false;
    }

    // 创建自定义窗口类来管理编辑器
    class PluginEditorWindow : public juce::DocumentWindow {
    public:
        PluginEditorWindow(const juce::String& name, AudioProcessingChain* chain, int pluginIndex)
            : juce::DocumentWindow(name, juce::Colours::lightgrey, allButtons)
            , processingChain(chain)
            , index(pluginIndex) {
        }

        void closeButtonPressed() override {
            if (processingChain) {
                processingChain->hidePluginEditor(index);
            }
        }

    private:
        AudioProcessingChain* processingChain;
        int index;
    };

    auto window = std::make_unique<PluginEditorWindow>(
        plugin->getName() + " Editor", this, index);

    window->setContentOwned(editor, true);
    window->setResizable(editor->isResizable(), false);
    window->centreWithSize(editor->getWidth(), editor->getHeight());
    window->setVisible(true);

    // 存储窗口
    editorWindows[index] = std::move(window);

    return true;
}

void AudioProcessingChain::hidePluginEditor(int index) {
    juce::ScopedLock scopedLock(lock);
    cleanupEditorWindow(index);
}

bool AudioProcessingChain::hasPluginEditor(int index) const {
    juce::ScopedLock scopedLock(lock);

    if (!validateIndex(index)) {
        return false;
    }

    auto* node = nodes[index].get();
    if (!node) return false;

    auto* plugin = node->getPlugin();
    return plugin ? plugin->hasEditor() : false;
}

void AudioProcessingChain::hideAllEditors() {
    juce::ScopedLock scopedLock(lock);

    for (auto& pair : editorWindows) {
        if (pair.second) {
            pair.second->setVisible(false);
        }
    }
    editorWindows.clear();
}

void AudioProcessingChain::cleanupEditorWindow(int index) {
    auto it = editorWindows.find(index);
    if (it != editorWindows.end()) {
        if (it->second) {
            it->second->setVisible(false);
        }
        editorWindows.erase(it);
    }
}

} // namespace WindsynthVST
