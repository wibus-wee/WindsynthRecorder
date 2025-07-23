//
//  AudioIOManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频I/O管理器实现
//

#include "AudioIOManager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

AudioIOManager::AudioIOManager(GraphAudioProcessor& graphProcessor)
    : graphProcessor(graphProcessor)
{
    std::cout << "[AudioIOManager] 初始化音频I/O管理器" << std::endl;
    
    initializeDeviceManager();
    createDefaultMappings();
    
    // 初始化电平监控数组
    currentLevels.inputLevels.resize(currentConfig.numInputChannels, 0.0f);
    currentLevels.outputLevels.resize(currentConfig.numOutputChannels, 0.0f);
    currentLevels.inputPeaks.resize(currentConfig.numInputChannels, 0.0f);
    currentLevels.outputPeaks.resize(currentConfig.numOutputChannels, 0.0f);
    
    inputLevelSmoothers.resize(currentConfig.numInputChannels, 0.0f);
    outputLevelSmoothers.resize(currentConfig.numOutputChannels, 0.0f);
}

AudioIOManager::~AudioIOManager() {
    std::cout << "[AudioIOManager] 析构音频I/O管理器" << std::endl;

    if (deviceManager) {
        // 移除音频回调
        deviceManager->removeAudioCallback(&graphProcessor);
        deviceManager->closeAudioDevice();
    }
}

//==============================================================================
// 设备管理实现
//==============================================================================

std::vector<AudioIOManager::AudioDeviceInfo> AudioIOManager::scanAudioDevices() {
    std::cout << "[AudioIOManager] 扫描音频设备" << std::endl;
    
    std::vector<AudioDeviceInfo> devices;
    
    if (!deviceManager) {
        std::cout << "[AudioIOManager] 设备管理器未初始化" << std::endl;
        return devices;
    }
    
    // 获取音频设备类型
    auto& deviceTypes = deviceManager->getAvailableDeviceTypes();
    
    for (auto* deviceType : deviceTypes) {
        deviceType->scanForDevices();
        
        auto deviceNames = deviceType->getDeviceNames();
        for (const auto& deviceName : deviceNames) {
            AudioDeviceInfo info;
            info.name = deviceName.toStdString();
            info.type = deviceType->getTypeName().toStdString();
            
            // 获取设备详细信息
            auto device = deviceType->createDevice(deviceName, deviceName);
            if (device) {
                info.numInputChannels = device->getActiveInputChannels().countNumberOfSetBits();
                info.numOutputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
                
                auto sampleRates = device->getAvailableSampleRates();
                for (double rate : sampleRates) {
                    info.supportedSampleRates.push_back(rate);
                }
                
                auto bufferSizes = device->getAvailableBufferSizes();
                for (int size : bufferSizes) {
                    info.supportedBufferSizes.push_back(size);
                }
                
                info.isAvailable = true;
            }
            
            devices.push_back(info);
        }
    }
    
    std::cout << "[AudioIOManager] 找到 " << devices.size() << " 个音频设备" << std::endl;
    return devices;
}

bool AudioIOManager::setAudioDevice(const std::string& deviceName, 
                                   double sampleRate, 
                                   int bufferSize) {
    std::cout << "[AudioIOManager] 设置音频设备：" << deviceName 
              << "，采样率：" << sampleRate << "，缓冲区：" << bufferSize << std::endl;
    
    if (!deviceManager) {
        std::cout << "[AudioIOManager] 设备管理器未初始化" << std::endl;
        return false;
    }
    
    // 设置音频设备
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.outputDeviceName = deviceName;
    setup.inputDeviceName = deviceName;
    setup.sampleRate = sampleRate;
    setup.bufferSize = bufferSize;
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;
    
    juce::String error = deviceManager->setAudioDeviceSetup(setup, true);
    
    if (error.isEmpty()) {
        // 更新当前设备信息
        currentDevice.name = deviceName;
        currentDevice.isAvailable = true;
        
        // 更新配置
        currentConfig.sampleRate = sampleRate;
        currentConfig.bufferSize = bufferSize;
        
        // 通知图处理器
        graphProcessor.configure(GraphConfig{
            sampleRate,
            bufferSize,
            currentConfig.numInputChannels,
            currentConfig.numOutputChannels,
            true,
            true
        });
        
        notifyConfigChange();
        notifyDeviceChange(currentDevice, true);
        
        std::cout << "[AudioIOManager] 音频设备设置成功" << std::endl;
        return true;
    } else {
        std::cout << "[AudioIOManager] 音频设备设置失败：" << error.toStdString() << std::endl;
        return false;
    }
}

AudioIOManager::AudioDeviceInfo AudioIOManager::getCurrentDevice() const {
    return currentDevice;
}

bool AudioIOManager::isDeviceAvailable(const std::string& deviceName) const {
    auto devices = const_cast<AudioIOManager*>(this)->scanAudioDevices();
    for (const auto& device : devices) {
        if (device.name == deviceName) {
            return device.isAvailable;
        }
    }
    return false;
}

//==============================================================================
// I/O配置管理实现
//==============================================================================

bool AudioIOManager::configureIO(const IOConfiguration& config) {
    std::cout << "[AudioIOManager] 配置音频I/O：输入=" << config.numInputChannels 
              << "，输出=" << config.numOutputChannels << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    // 验证配置
    if (config.numInputChannels < 0 || config.numInputChannels > Constants::MAX_AUDIO_CHANNELS ||
        config.numOutputChannels < 0 || config.numOutputChannels > Constants::MAX_AUDIO_CHANNELS) {
        std::cout << "[AudioIOManager] 无效的通道配置" << std::endl;
        return false;
    }
    
    if (config.sampleRate <= 0 || config.bufferSize <= 0) {
        std::cout << "[AudioIOManager] 无效的采样率或缓冲区大小" << std::endl;
        return false;
    }
    
    // 更新配置
    currentConfig = config;
    
    // 调整电平监控数组大小
    currentLevels.inputLevels.resize(config.numInputChannels, 0.0f);
    currentLevels.outputLevels.resize(config.numOutputChannels, 0.0f);
    currentLevels.inputPeaks.resize(config.numInputChannels, 0.0f);
    currentLevels.outputPeaks.resize(config.numOutputChannels, 0.0f);
    
    inputLevelSmoothers.resize(config.numInputChannels, 0.0f);
    outputLevelSmoothers.resize(config.numOutputChannels, 0.0f);
    
    // 更新图处理器配置
    GraphConfig graphConfig{
        config.sampleRate,
        config.bufferSize,
        config.numInputChannels,
        config.numOutputChannels,
        true,
        true
    };
    
    graphProcessor.configure(graphConfig);
    
    // 更新通道映射
    updateChannelMappings();
    
    configured = true;
    notifyConfigChange();
    
    std::cout << "[AudioIOManager] I/O配置完成" << std::endl;
    return true;
}

bool AudioIOManager::setInputChannels(int numChannels) {
    if (numChannels < 0 || numChannels > Constants::MAX_AUDIO_CHANNELS) {
        return false;
    }
    
    IOConfiguration newConfig = currentConfig;
    newConfig.numInputChannels = numChannels;
    return configureIO(newConfig);
}

bool AudioIOManager::setOutputChannels(int numChannels) {
    if (numChannels < 0 || numChannels > Constants::MAX_AUDIO_CHANNELS) {
        return false;
    }
    
    IOConfiguration newConfig = currentConfig;
    newConfig.numOutputChannels = numChannels;
    return configureIO(newConfig);
}

bool AudioIOManager::setSampleRate(double sampleRate) {
    if (sampleRate <= 0) {
        return false;
    }
    
    IOConfiguration newConfig = currentConfig;
    newConfig.sampleRate = sampleRate;
    return configureIO(newConfig);
}

bool AudioIOManager::setBufferSize(int bufferSize) {
    if (bufferSize <= 0) {
        return false;
    }
    
    IOConfiguration newConfig = currentConfig;
    newConfig.bufferSize = bufferSize;
    return configureIO(newConfig);
}

//==============================================================================
// 通道映射管理实现
//==============================================================================

bool AudioIOManager::addInputMapping(const ChannelMapping& mapping) {
    std::cout << "[AudioIOManager] 添加输入通道映射：" << mapping.sourceChannel 
              << " -> " << mapping.destinationChannel << std::endl;
    
    if (mapping.sourceChannel < 0 || mapping.sourceChannel >= currentConfig.numInputChannels ||
        mapping.destinationChannel < 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    // 检查是否已存在映射
    auto it = std::find_if(currentConfig.inputMappings.begin(), currentConfig.inputMappings.end(),
        [&mapping](const ChannelMapping& existing) {
            return existing.sourceChannel == mapping.sourceChannel;
        });
    
    if (it != currentConfig.inputMappings.end()) {
        *it = mapping; // 更新现有映射
    } else {
        currentConfig.inputMappings.push_back(mapping); // 添加新映射
    }
    
    updateChannelMappings();
    notifyConfigChange();
    return true;
}

bool AudioIOManager::addOutputMapping(const ChannelMapping& mapping) {
    std::cout << "[AudioIOManager] 添加输出通道映射：" << mapping.sourceChannel 
              << " -> " << mapping.destinationChannel << std::endl;
    
    if (mapping.destinationChannel < 0 || mapping.destinationChannel >= currentConfig.numOutputChannels ||
        mapping.sourceChannel < 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    // 检查是否已存在映射
    auto it = std::find_if(currentConfig.outputMappings.begin(), currentConfig.outputMappings.end(),
        [&mapping](const ChannelMapping& existing) {
            return existing.destinationChannel == mapping.destinationChannel;
        });
    
    if (it != currentConfig.outputMappings.end()) {
        *it = mapping; // 更新现有映射
    } else {
        currentConfig.outputMappings.push_back(mapping); // 添加新映射
    }
    
    updateChannelMappings();
    notifyConfigChange();
    return true;
}

bool AudioIOManager::removeInputMapping(int sourceChannel) {
    std::cout << "[AudioIOManager] 移除输入通道映射：" << sourceChannel << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    auto it = std::remove_if(currentConfig.inputMappings.begin(), currentConfig.inputMappings.end(),
        [sourceChannel](const ChannelMapping& mapping) {
            return mapping.sourceChannel == sourceChannel;
        });
    
    bool removed = (it != currentConfig.inputMappings.end());
    currentConfig.inputMappings.erase(it, currentConfig.inputMappings.end());
    
    if (removed) {
        updateChannelMappings();
        notifyConfigChange();
    }
    
    return removed;
}

bool AudioIOManager::removeOutputMapping(int destinationChannel) {
    std::cout << "[AudioIOManager] 移除输出通道映射：" << destinationChannel << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    auto it = std::remove_if(currentConfig.outputMappings.begin(), currentConfig.outputMappings.end(),
        [destinationChannel](const ChannelMapping& mapping) {
            return mapping.destinationChannel == destinationChannel;
        });
    
    bool removed = (it != currentConfig.outputMappings.end());
    currentConfig.outputMappings.erase(it, currentConfig.outputMappings.end());
    
    if (removed) {
        updateChannelMappings();
        notifyConfigChange();
    }
    
    return removed;
}

void AudioIOManager::clearAllMappings() {
    std::cout << "[AudioIOManager] 清除所有通道映射" << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    currentConfig.inputMappings.clear();
    currentConfig.outputMappings.clear();
    
    updateChannelMappings();
    notifyConfigChange();
}

void AudioIOManager::createDefaultMappings() {
    std::cout << "[AudioIOManager] 创建默认通道映射" << std::endl;
    
    std::lock_guard<std::mutex> lock(configMutex);
    
    // 清除现有映射
    currentConfig.inputMappings.clear();
    currentConfig.outputMappings.clear();
    
    // 创建1:1映射
    for (int i = 0; i < currentConfig.numInputChannels; ++i) {
        currentConfig.inputMappings.emplace_back(i, i, 1.0f);
    }
    
    for (int i = 0; i < currentConfig.numOutputChannels; ++i) {
        currentConfig.outputMappings.emplace_back(i, i, 1.0f);
    }
    
    updateChannelMappings();
}

//==============================================================================
// 智能连接管理实现
//==============================================================================

int AudioIOManager::autoConnectToInput(NodeID nodeID, int channelOffset) {
    std::cout << "[AudioIOManager] 自动连接节点到输入：" << nodeID.uid
              << "，通道偏移：" << channelOffset << std::endl;

    auto nodeInfo = graphProcessor.getNodeInfo(nodeID);
    if (nodeInfo.nodeID.uid == 0) {
        std::cout << "[AudioIOManager] 无效的节点ID" << std::endl;
        return 0;
    }

    NodeID audioInputID = getAudioInputNodeID();
    int connectionsCreated = 0;

    // 连接音频通道
    int maxChannels = std::min(currentConfig.numInputChannels - channelOffset,
                              nodeInfo.numInputChannels);

    for (int ch = 0; ch < maxChannels; ++ch) {
        if (graphProcessor.connectAudio(audioInputID, ch + channelOffset, nodeID, ch)) {
            connectionsCreated++;
        }
    }

    std::cout << "[AudioIOManager] 创建了 " << connectionsCreated << " 个输入连接" << std::endl;
    return connectionsCreated;
}

int AudioIOManager::autoConnectToOutput(NodeID nodeID, int channelOffset) {
    std::cout << "[AudioIOManager] 自动连接节点到输出：" << nodeID.uid
              << "，通道偏移：" << channelOffset << std::endl;

    auto nodeInfo = graphProcessor.getNodeInfo(nodeID);
    if (nodeInfo.nodeID.uid == 0) {
        std::cout << "[AudioIOManager] 无效的节点ID" << std::endl;
        return 0;
    }

    NodeID audioOutputID = getAudioOutputNodeID();
    int connectionsCreated = 0;

    // 连接音频通道
    int maxChannels = std::min(nodeInfo.numOutputChannels,
                              currentConfig.numOutputChannels - channelOffset);

    for (int ch = 0; ch < maxChannels; ++ch) {
        if (graphProcessor.connectAudio(nodeID, ch, audioOutputID, ch + channelOffset)) {
            connectionsCreated++;
        }
    }

    std::cout << "[AudioIOManager] 创建了 " << connectionsCreated << " 个输出连接" << std::endl;
    return connectionsCreated;
}

bool AudioIOManager::connectMidiInput(NodeID nodeID) {
    std::cout << "[AudioIOManager] 连接MIDI输入到节点：" << nodeID.uid << std::endl;

    auto nodeInfo = graphProcessor.getNodeInfo(nodeID);
    if (nodeInfo.nodeID.uid == 0 || !nodeInfo.acceptsMidi) {
        std::cout << "[AudioIOManager] 节点不接受MIDI或无效" << std::endl;
        return false;
    }

    NodeID midiInputID = getMidiInputNodeID();
    return graphProcessor.connectMidi(midiInputID, nodeID);
}

bool AudioIOManager::connectMidiOutput(NodeID nodeID) {
    std::cout << "[AudioIOManager] 连接节点到MIDI输出：" << nodeID.uid << std::endl;

    auto nodeInfo = graphProcessor.getNodeInfo(nodeID);
    if (nodeInfo.nodeID.uid == 0 || !nodeInfo.producesMidi) {
        std::cout << "[AudioIOManager] 节点不产生MIDI或无效" << std::endl;
        return false;
    }

    NodeID midiOutputID = getMidiOutputNodeID();
    return graphProcessor.connectMidi(nodeID, midiOutputID);
}

bool AudioIOManager::disconnectAllIO(NodeID nodeID) {
    std::cout << "[AudioIOManager] 断开节点的所有I/O连接：" << nodeID.uid << std::endl;

    return graphProcessor.disconnectNode(nodeID);
}

//==============================================================================
// 音频监控和电平检测实现
//==============================================================================

void AudioIOManager::enableLevelMonitoring(bool enable) {
    std::cout << "[AudioIOManager] " << (enable ? "启用" : "禁用") << "电平监控" << std::endl;

    std::lock_guard<std::mutex> lock(levelMutex);
    levelMonitoringEnabled = enable;

    if (enable) {
        resetPeakLevels();
    }
}

AudioIOManager::AudioLevelInfo AudioIOManager::getCurrentLevels() const {
    std::lock_guard<std::mutex> lock(levelMutex);
    return currentLevels;
}

void AudioIOManager::resetPeakLevels() {
    std::cout << "[AudioIOManager] 重置峰值电平" << std::endl;

    std::lock_guard<std::mutex> lock(levelMutex);

    std::fill(currentLevels.inputPeaks.begin(), currentLevels.inputPeaks.end(), 0.0f);
    std::fill(currentLevels.outputPeaks.begin(), currentLevels.outputPeaks.end(), 0.0f);
    currentLevels.inputClipping = false;
    currentLevels.outputClipping = false;
}

void AudioIOManager::setLevelUpdateInterval(int intervalMs) {
    if (intervalMs > 0) {
        levelUpdateIntervalMs = intervalMs;
        std::cout << "[AudioIOManager] 设置电平更新间隔：" << intervalMs << "ms" << std::endl;
    }
}

//==============================================================================
// 音频处理控制实现
//==============================================================================

void AudioIOManager::setInputGain(float gain) {
    std::cout << "[AudioIOManager] 设置输入增益：" << gain << std::endl;

    std::lock_guard<std::mutex> lock(configMutex);
    currentConfig.inputGain = std::max(0.0f, gain);
    notifyConfigChange();
}

void AudioIOManager::setOutputGain(float gain) {
    std::cout << "[AudioIOManager] 设置输出增益：" << gain << std::endl;

    std::lock_guard<std::mutex> lock(configMutex);
    currentConfig.outputGain = std::max(0.0f, gain);
    notifyConfigChange();
}

void AudioIOManager::setInputMuted(bool muted) {
    std::cout << "[AudioIOManager] 设置输入静音：" << (muted ? "是" : "否") << std::endl;

    inputMuted = muted;
    notifyConfigChange();
}

void AudioIOManager::setOutputMuted(bool muted) {
    std::cout << "[AudioIOManager] 设置输出静音：" << (muted ? "是" : "否") << std::endl;

    outputMuted = muted;
    notifyConfigChange();
}

void AudioIOManager::enableInputMonitoring(bool enable) {
    std::cout << "[AudioIOManager] " << (enable ? "启用" : "禁用") << "输入监听" << std::endl;

    std::lock_guard<std::mutex> lock(configMutex);
    currentConfig.enableInputMonitoring = enable;
    inputMonitoringEnabled = enable;
    notifyConfigChange();
}

void AudioIOManager::enableOutputLimiting(bool enable) {
    std::cout << "[AudioIOManager] " << (enable ? "启用" : "禁用") << "输出限制器" << std::endl;

    std::lock_guard<std::mutex> lock(configMutex);
    currentConfig.enableOutputLimiting = enable;
    outputLimitingEnabled = enable;
    notifyConfigChange();
}

//==============================================================================
// 回调设置
//==============================================================================

void AudioIOManager::setDeviceChangeCallback(DeviceChangeCallback callback) {
    deviceChangeCallback = std::move(callback);
}

void AudioIOManager::setLevelUpdateCallback(LevelUpdateCallback callback) {
    levelUpdateCallback = std::move(callback);
}

void AudioIOManager::setConfigChangeCallback(ConfigChangeCallback callback) {
    configChangeCallback = std::move(callback);
}

//==============================================================================
// 状态查询实现
//==============================================================================

NodeID AudioIOManager::getAudioInputNodeID() const {
    return graphProcessor.getAudioInputNodeID();
}

NodeID AudioIOManager::getAudioOutputNodeID() const {
    return graphProcessor.getAudioOutputNodeID();
}

NodeID AudioIOManager::getMidiInputNodeID() const {
    return graphProcessor.getMidiInputNodeID();
}

NodeID AudioIOManager::getMidiOutputNodeID() const {
    return graphProcessor.getMidiOutputNodeID();
}

//==============================================================================
// 内部方法实现
//==============================================================================

void AudioIOManager::initializeDeviceManager() {
    std::cout << "[AudioIOManager] 初始化设备管理器" << std::endl;

    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    // deviceManager->initialiseWithDefaultDevices(2, 2);
    deviceManager->initialiseWithDefaultDevices(0, 2); // 0 input channels

    // 关键：连接GraphAudioProcessor到音频设备管理器
    deviceManager->addAudioCallback(&graphProcessor);
    std::cout << "[AudioIOManager] GraphAudioProcessor已连接到音频设备" << std::endl;

    // 设置默认设备信息
    auto* currentAudioDevice = deviceManager->getCurrentAudioDevice();
    if (currentAudioDevice) {
        currentDevice.name = currentAudioDevice->getName().toStdString();
        currentDevice.numInputChannels = currentAudioDevice->getActiveInputChannels().countNumberOfSetBits();
        currentDevice.numOutputChannels = currentAudioDevice->getActiveOutputChannels().countNumberOfSetBits();
        currentDevice.isDefault = true;
        currentDevice.isAvailable = true;
    }
}

void AudioIOManager::updateChannelMappings() {
    std::cout << "[AudioIOManager] 更新通道映射" << std::endl;

    // 这里可以实现具体的通道映射逻辑
    // 例如更新内部的音频路由矩阵
}

void AudioIOManager::updateAudioLevels(const juce::AudioBuffer<float>& buffer, bool isInput) {
    if (!levelMonitoringEnabled) {
        return;
    }

    std::lock_guard<std::mutex> lock(levelMutex);

    auto& levels = isInput ? currentLevels.inputLevels : currentLevels.outputLevels;
    auto& peaks = isInput ? currentLevels.inputPeaks : currentLevels.outputPeaks;
    auto& smoothers = isInput ? inputLevelSmoothers : outputLevelSmoothers;
    bool& clipping = isInput ? currentLevels.inputClipping : currentLevels.outputClipping;

    int numChannels = std::min(buffer.getNumChannels(), static_cast<int>(levels.size()));

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* channelData = buffer.getReadPointer(ch);
        int numSamples = buffer.getNumSamples();

        // 计算RMS电平
        float rmsLevel = calculateRMSLevel(channelData, numSamples);
        levels[ch] = smoothLevel(smoothers[ch], rmsLevel);
        smoothers[ch] = levels[ch];

        // 计算峰值电平
        float peakLevel = calculatePeakLevel(channelData, numSamples);
        peaks[ch] = std::max(peaks[ch], peakLevel);

        // 检测削波
        if (peakLevel >= 0.99f) {
            clipping = true;
        }
    }

    currentLevels.timestamp = juce::Time::getMillisecondCounterHiRes();

    // 检查是否需要更新回调
    auto now = juce::Time::getCurrentTime();
    if (now.toMilliseconds() - lastLevelUpdate.toMilliseconds() >= levelUpdateIntervalMs) {
        lastLevelUpdate = now;
        notifyLevelUpdate();
    }
}

void AudioIOManager::notifyConfigChange() {
    if (configChangeCallback) {
        configChangeCallback(currentConfig);
    }
}

void AudioIOManager::notifyDeviceChange(const AudioDeviceInfo& device, bool connected) {
    if (deviceChangeCallback) {
        deviceChangeCallback(device, connected);
    }
}

void AudioIOManager::notifyLevelUpdate() {
    if (levelUpdateCallback) {
        levelUpdateCallback(currentLevels);
    }
}

//==============================================================================
// 电平计算辅助方法实现
//==============================================================================

float AudioIOManager::calculateRMSLevel(const float* channelData, int numSamples) {
    if (numSamples == 0) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float sample = channelData[i];
        sum += sample * sample;
    }

    return std::sqrt(sum / numSamples);
}

float AudioIOManager::calculatePeakLevel(const float* channelData, int numSamples) {
    if (numSamples == 0) {
        return 0.0f;
    }

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(channelData[i]));
    }

    return peak;
}

float AudioIOManager::smoothLevel(float currentLevel, float newLevel, float smoothingFactor) {
    return currentLevel + smoothingFactor * (newLevel - currentLevel);
}

} // namespace WindsynthVST::AudioGraph
