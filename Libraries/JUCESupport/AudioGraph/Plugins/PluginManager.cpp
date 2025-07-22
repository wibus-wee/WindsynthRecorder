//
//  PluginManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理器实现
//

#include "PluginManager.hpp"
#include <iostream>
#include <algorithm>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

PluginManager::PluginManager(GraphAudioProcessor& graphProcessor, ModernPluginLoader& pluginLoader)
    : graphProcessor(graphProcessor), pluginLoader(pluginLoader)
{
    std::cout << "[PluginManager] 初始化插件管理器" << std::endl;
}

PluginManager::~PluginManager() {
    std::cout << "[PluginManager] 析构插件管理器" << std::endl;
}

//==============================================================================
// 插件实例管理实现
//==============================================================================

void PluginManager::loadPluginAsync(const juce::PluginDescription& description,
                                   const std::string& displayName,
                                   std::function<void(NodeID nodeID, const std::string& error)> callback) {
    std::cout << "[PluginManager] 异步加载插件：" << description.name << std::endl;
    
    std::string finalDisplayName = displayName.empty() ? description.name.toStdString() : displayName;
    
    pluginLoader.loadPluginAsync(description, 
                                graphProcessor.getConfig().sampleRate,
                                graphProcessor.getConfig().samplesPerBlock,
        [this, description, finalDisplayName, callback](std::unique_ptr<juce::AudioPluginInstance> instance, 
                                                        const juce::String& error) {
            if (instance) {
                // 添加到图中
                NodeID nodeID = graphProcessor.addPlugin(std::move(instance), finalDisplayName);
                
                if (nodeID.uid != 0) {
                    handlePluginLoaded(nodeID, nullptr, finalDisplayName, description);
                    
                    if (callback) {
                        callback(nodeID, "");
                    }
                } else {
                    std::string errorMsg = "无法添加插件到音频图";
                    notifyPluginError(NodeID{0}, errorMsg);
                    
                    if (callback) {
                        callback(NodeID{0}, errorMsg);
                    }
                }
            } else {
                std::string errorMsg = error.toStdString();
                notifyPluginError(NodeID{0}, errorMsg);
                
                if (callback) {
                    callback(NodeID{0}, errorMsg);
                }
            }
        });
}

bool PluginManager::removePlugin(NodeID nodeID) {
    std::cout << "[PluginManager] 移除插件：" << nodeID.uid << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        auto it = pluginInstances.find(nodeID);
        if (it == pluginInstances.end()) {
            std::cout << "[PluginManager] 插件不存在：" << nodeID.uid << std::endl;
            return false;
        }
    }
    
    // 从图中移除
    bool success = graphProcessor.removeNode(nodeID);
    
    if (success) {
        // 清理内部数据
        {
            std::lock_guard<std::mutex> lock(pluginsMutex);
            pluginInstances.erase(nodeID);
        }
        
        {
            std::lock_guard<std::mutex> lock(presetsMutex);
            pluginPresets.erase(nodeID);
        }
        
        {
            std::lock_guard<std::mutex> lock(performanceMutex);
            cpuUsageMap.erase(nodeID);
        }
        
        notifyPluginRemoved(nodeID);
    }
    
    return success;
}

std::vector<PluginManager::PluginInstanceInfo> PluginManager::getAllPlugins() const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    std::vector<PluginInstanceInfo> plugins;
    plugins.reserve(pluginInstances.size());
    
    for (const auto& pair : pluginInstances) {
        plugins.push_back(pair.second);
    }
    
    return plugins;
}

const PluginManager::PluginInstanceInfo* PluginManager::getPluginInfo(NodeID nodeID) const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    auto it = pluginInstances.find(nodeID);
    return (it != pluginInstances.end()) ? &it->second : nullptr;
}

juce::AudioPluginInstance* PluginManager::getPluginInstance(NodeID nodeID) const {
    auto* node = graphProcessor.getGraph().getNodeForId(nodeID);
    if (!node) {
        return nullptr;
    }
    
    return dynamic_cast<juce::AudioPluginInstance*>(node->getProcessor());
}

bool PluginManager::setPluginEnabled(NodeID nodeID, bool enabled) {
    std::cout << "[PluginManager] 设置插件启用状态：" << nodeID.uid << " -> " << enabled << std::endl;
    
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = pluginInstances.find(nodeID);
    if (it == pluginInstances.end()) {
        return false;
    }
    
    it->second.enabled = enabled;
    
    // 通过旁路来实现启用/禁用
    return graphProcessor.setNodeBypassed(nodeID, !enabled);
}

bool PluginManager::setPluginBypassed(NodeID nodeID, bool bypassed) {
    std::cout << "[PluginManager] 设置插件旁路状态：" << nodeID.uid << " -> " << bypassed << std::endl;
    
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = pluginInstances.find(nodeID);
    if (it == pluginInstances.end()) {
        return false;
    }
    
    it->second.bypassed = bypassed;
    return graphProcessor.setNodeBypassed(nodeID, bypassed);
}

bool PluginManager::renamePlugin(NodeID nodeID, const std::string& newName) {
    std::cout << "[PluginManager] 重命名插件：" << nodeID.uid << " -> " << newName << std::endl;
    
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = pluginInstances.find(nodeID);
    if (it == pluginInstances.end()) {
        return false;
    }
    
    it->second.displayName = newName;
    return true;
}

//==============================================================================
// 插件参数管理实现
//==============================================================================

std::vector<PluginManager::ParameterInfo> PluginManager::getPluginParameters(NodeID nodeID) const {
    std::vector<ParameterInfo> parameters;

    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return parameters;
    }

    // 使用现代JUCE参数API
    auto& parameterArray = instance->getParameters();
    parameters.reserve(parameterArray.size());

    for (int i = 0; i < parameterArray.size(); ++i) {
        auto* param = parameterArray[i];
        if (!param) continue;

        ParameterInfo paramInfo;
        paramInfo.index = i;
        paramInfo.name = param->getName(256).toStdString();
        paramInfo.label = param->getLabel().toStdString();
        paramInfo.value = param->getValue();
        paramInfo.defaultValue = param->getDefaultValue();
        paramInfo.isAutomatable = true;
        paramInfo.isDiscrete = param->isDiscrete();
        paramInfo.numSteps = param->getNumSteps();

        parameters.push_back(paramInfo);
    }

    return parameters;
}

float PluginManager::getParameterValue(NodeID nodeID, int parameterIndex) const {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return 0.0f;
    }

    auto& params = instance->getParameters();
    if (parameterIndex < 0 || parameterIndex >= params.size()) {
        return 0.0f;
    }

    auto* param = params[parameterIndex];
    return param ? param->getValue() : 0.0f;
}

bool PluginManager::setParameterValue(NodeID nodeID, int parameterIndex, float value) {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return false;
    }

    auto& params = instance->getParameters();
    if (parameterIndex < 0 || parameterIndex >= params.size()) {
        return false;
    }

    auto* param = params[parameterIndex];
    if (param) {
        param->setValue(value);
        notifyParameterChanged(nodeID, parameterIndex, value);
        return true;
    }

    return false;
}

std::string PluginManager::getParameterText(NodeID nodeID, int parameterIndex) const {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return "";
    }

    auto& params = instance->getParameters();
    if (parameterIndex < 0 || parameterIndex >= params.size()) {
        return "";
    }

    auto* param = params[parameterIndex];
    return param ? param->getText(param->getValue(), 256).toStdString() : "";
}

bool PluginManager::resetParametersToDefault(NodeID nodeID) {
    std::cout << "[PluginManager] 重置插件参数到默认值：" << nodeID.uid << std::endl;

    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return false;
    }

    auto& params = instance->getParameters();
    for (int i = 0; i < params.size(); ++i) {
        auto* param = params[i];
        if (param) {
            float defaultValue = param->getDefaultValue();
            param->setValue(defaultValue);
            notifyParameterChanged(nodeID, i, defaultValue);
        }
    }

    return true;
}

//==============================================================================
// 插件预设管理实现
//==============================================================================

bool PluginManager::savePreset(NodeID nodeID, const std::string& presetName) {
    std::cout << "[PluginManager] 保存插件预设：" << nodeID.uid << " -> " << presetName << std::endl;
    
    juce::MemoryBlock stateData;
    if (!getPluginState(nodeID, stateData)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    pluginPresets[nodeID][presetName] = PresetInfo(presetName, stateData);
    
    return true;
}

bool PluginManager::loadPreset(NodeID nodeID, const std::string& presetName) {
    std::cout << "[PluginManager] 加载插件预设：" << nodeID.uid << " -> " << presetName << std::endl;
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto pluginIt = pluginPresets.find(nodeID);
    if (pluginIt == pluginPresets.end()) {
        return false;
    }
    
    auto presetIt = pluginIt->second.find(presetName);
    if (presetIt == pluginIt->second.end()) {
        return false;
    }
    
    return setPluginState(nodeID, presetIt->second.data);
}

bool PluginManager::deletePreset(NodeID nodeID, const std::string& presetName) {
    std::cout << "[PluginManager] 删除插件预设：" << nodeID.uid << " -> " << presetName << std::endl;
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto pluginIt = pluginPresets.find(nodeID);
    if (pluginIt == pluginPresets.end()) {
        return false;
    }
    
    return pluginIt->second.erase(presetName) > 0;
}

std::vector<std::string> PluginManager::getPresetNames(NodeID nodeID) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    std::vector<std::string> names;
    
    auto pluginIt = pluginPresets.find(nodeID);
    if (pluginIt != pluginPresets.end()) {
        names.reserve(pluginIt->second.size());
        for (const auto& preset : pluginIt->second) {
            names.push_back(preset.first);
        }
    }
    
    return names;
}

bool PluginManager::exportPreset(NodeID nodeID, const std::string& presetName, const juce::File& file) const {
    std::cout << "[PluginManager] 导出预设：" << presetName << " 到 " << file.getFullPathName() << std::endl;
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto pluginIt = pluginPresets.find(nodeID);
    if (pluginIt == pluginPresets.end()) {
        return false;
    }
    
    auto presetIt = pluginIt->second.find(presetName);
    if (presetIt == pluginIt->second.end()) {
        return false;
    }
    
    return file.replaceWithData(presetIt->second.data.getData(), presetIt->second.data.getSize());
}

bool PluginManager::importPreset(NodeID nodeID, const std::string& presetName, const juce::File& file) {
    std::cout << "[PluginManager] 导入预设：" << presetName << " 从 " << file.getFullPathName() << std::endl;
    
    if (!file.existsAsFile()) {
        return false;
    }
    
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    pluginPresets[nodeID][presetName] = PresetInfo(presetName, data);
    
    return true;
}

//==============================================================================
// 插件状态管理实现
//==============================================================================

bool PluginManager::getPluginState(NodeID nodeID, juce::MemoryBlock& stateData) const {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return false;
    }

    instance->getStateInformation(stateData);
    return true;
}

bool PluginManager::setPluginState(NodeID nodeID, const juce::MemoryBlock& stateData) {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return false;
    }

    instance->setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
    return true;
}

//==============================================================================
// 性能监控实现
//==============================================================================

void PluginManager::updatePerformanceStats() {
    // 这里可以实现性能统计的更新逻辑
    // 由于JUCE AudioProcessor没有直接的CPU使用率API，
    // 我们可以通过测量处理时间来估算
}

double PluginManager::getPluginCpuUsage(NodeID nodeID) const {
    std::lock_guard<std::mutex> lock(performanceMutex);

    auto it = cpuUsageMap.find(nodeID);
    return (it != cpuUsageMap.end()) ? it->second : 0.0;
}

int PluginManager::getPluginLatency(NodeID nodeID) const {
    auto* instance = getPluginInstance(nodeID);
    if (!instance) {
        return 0;
    }

    return instance->getLatencySamples();
}

//==============================================================================
// 回调设置
//==============================================================================

void PluginManager::setPluginLoadedCallback(PluginLoadedCallback callback) {
    pluginLoadedCallback = std::move(callback);
}

void PluginManager::setPluginRemovedCallback(PluginRemovedCallback callback) {
    pluginRemovedCallback = std::move(callback);
}

void PluginManager::setParameterChangedCallback(ParameterChangedCallback callback) {
    parameterChangedCallback = std::move(callback);
}

void PluginManager::setPluginErrorCallback(PluginErrorCallback callback) {
    pluginErrorCallback = std::move(callback);
}

//==============================================================================
// 统计信息实现
//==============================================================================

int PluginManager::getNumLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    return static_cast<int>(pluginInstances.size());
}

double PluginManager::getTotalCpuUsage() const {
    std::lock_guard<std::mutex> lock(performanceMutex);

    double total = 0.0;
    for (const auto& pair : cpuUsageMap) {
        total += pair.second;
    }

    return total;
}

int PluginManager::getTotalLatency() const {
    int totalLatency = 0;

    std::lock_guard<std::mutex> lock(pluginsMutex);
    for (const auto& pair : pluginInstances) {
        totalLatency += pair.second.latencySamples;
    }

    return totalLatency;
}

//==============================================================================
// 内部方法实现
//==============================================================================

void PluginManager::handlePluginLoaded(NodeID nodeID, std::unique_ptr<juce::AudioPluginInstance> instance,
                                      const std::string& displayName, const juce::PluginDescription& description) {
    std::cout << "[PluginManager] 处理插件加载完成：" << displayName << std::endl;

    // 创建插件实例信息
    PluginInstanceInfo info(nodeID, displayName, description);
    info.displayName = displayName;

    // 获取延迟信息
    auto* actualInstance = getPluginInstance(nodeID);
    if (actualInstance) {
        info.latencySamples = actualInstance->getLatencySamples();
    }

    // 存储插件信息
    {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        pluginInstances[nodeID] = info;
    }

    // 初始化性能监控
    {
        std::lock_guard<std::mutex> lock(performanceMutex);
        cpuUsageMap[nodeID] = 0.0;
    }

    notifyPluginLoaded(nodeID, info);
}

void PluginManager::notifyPluginLoaded(NodeID nodeID, const PluginInstanceInfo& info) {
    if (pluginLoadedCallback) {
        pluginLoadedCallback(nodeID, info);
    }
}

void PluginManager::notifyPluginRemoved(NodeID nodeID) {
    if (pluginRemovedCallback) {
        pluginRemovedCallback(nodeID);
    }
}

void PluginManager::notifyParameterChanged(NodeID nodeID, int parameterIndex, float newValue) {
    if (parameterChangedCallback) {
        parameterChangedCallback(nodeID, parameterIndex, newValue);
    }
}

void PluginManager::notifyPluginError(NodeID nodeID, const std::string& error) {
    std::cout << "[PluginManager] 插件错误：" << error << std::endl;

    if (pluginErrorCallback) {
        pluginErrorCallback(nodeID, error);
    }
}

} // namespace WindsynthVST::AudioGraph
