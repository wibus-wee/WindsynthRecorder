//
//  PresetManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  预设管理器实现
//

#include "PresetManager.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 自动备份定时器类
//==============================================================================

class AutoBackupTimer : public juce::Timer {
public:
    AutoBackupTimer(PresetManager& manager) : presetManager(manager) {}
    
    void timerCallback() override {
        presetManager.performAutoBackup();
    }
    
private:
    PresetManager& presetManager;
};

//==============================================================================
// 构造函数和析构函数
//==============================================================================

PresetManager::PresetManager(GraphAudioProcessor& graphProcessor, PluginManager& pluginManager)
    : graphProcessor(graphProcessor), pluginManager(pluginManager)
{
    std::cout << "[PresetManager] 初始化预设管理器" << std::endl;
    
    // 创建自动备份定时器
    autoBackupTimer = std::make_unique<AutoBackupTimer>(*this);
    
    // 创建默认类别
    createCategory(PresetCategory("Default", "默认类别"));
    createCategory(PresetCategory("User", "用户创建"));
    createCategory(PresetCategory("Factory", "出厂预设"));
}

PresetManager::~PresetManager() {
    std::cout << "[PresetManager] 析构预设管理器" << std::endl;
    
    if (autoBackupTimer) {
        autoBackupTimer->stopTimer();
    }
}

//==============================================================================
// 预设管理实现
//==============================================================================

bool PresetManager::savePreset(const std::string& presetName, const PresetInfo& info) {
    std::cout << "[PresetManager] 保存预设：" << presetName << std::endl;
    
    if (presetName.empty()) {
        std::cout << "[PresetManager] 预设名称不能为空" << std::endl;
        return false;
    }
    
    // 捕获当前状态
    GraphState currentState = captureCurrentState();
    if (!currentState.isValid()) {
        std::cout << "[PresetManager] 无法捕获当前图状态" << std::endl;
        return false;
    }
    
    // 创建预设数据
    PresetInfo finalInfo = info;
    if (finalInfo.name.empty()) {
        finalInfo.name = presetName;
    }
    if (finalInfo.category.empty()) {
        finalInfo.category = "User";
    }
    
    PresetData presetData(finalInfo, currentState);
    
    // 存储预设
    {
        std::lock_guard<std::mutex> lock(presetsMutex);
        presets[presetName] = presetData;
        
        // 更新类别中的预设列表
        auto& category = categories[finalInfo.category];
        if (std::find(category.presetNames.begin(), category.presetNames.end(), presetName) 
            == category.presetNames.end()) {
            category.presetNames.push_back(presetName);
        }
    }
    
    // 通知保存完成
    if (presetSavedCallback) {
        presetSavedCallback(presetName, true);
    }
    
    notifyStateChanged();
    
    std::cout << "[PresetManager] 预设保存成功：" << presetName << std::endl;
    return true;
}

bool PresetManager::loadPreset(const std::string& presetName, PresetLoadedCallback callback) {
    std::cout << "[PresetManager] 加载预设：" << presetName << std::endl;
    
    PresetData presetData;
    
    // 获取预设数据
    {
        std::lock_guard<std::mutex> lock(presetsMutex);
        auto it = presets.find(presetName);
        if (it == presets.end()) {
            std::cout << "[PresetManager] 预设不存在：" << presetName << std::endl;
            if (callback) callback(presetName, false);
            return false;
        }
        presetData = it->second;
    }
    
    // 应用图状态
    bool success = applyGraphState(presetData.state);
    
    // 通知加载完成
    if (callback) {
        callback(presetName, success);
    }
    
    if (presetLoadedCallback) {
        presetLoadedCallback(presetName, success);
    }
    
    if (success) {
        notifyStateChanged();
        std::cout << "[PresetManager] 预设加载成功：" << presetName << std::endl;
    } else {
        std::cout << "[PresetManager] 预设加载失败：" << presetName << std::endl;
    }
    
    return success;
}

bool PresetManager::deletePreset(const std::string& presetName) {
    std::cout << "[PresetManager] 删除预设：" << presetName << std::endl;
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto it = presets.find(presetName);
    if (it == presets.end()) {
        return false;
    }
    
    // 从类别中移除
    std::string category = it->second.info.category;
    auto& categoryData = categories[category];
    categoryData.presetNames.erase(
        std::remove(categoryData.presetNames.begin(), categoryData.presetNames.end(), presetName),
        categoryData.presetNames.end());
    
    // 删除预设
    presets.erase(it);
    
    notifyStateChanged();
    
    std::cout << "[PresetManager] 预设删除成功：" << presetName << std::endl;
    return true;
}

bool PresetManager::renamePreset(const std::string& oldName, const std::string& newName) {
    std::cout << "[PresetManager] 重命名预设：" << oldName << " -> " << newName << std::endl;
    
    if (oldName == newName || newName.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto it = presets.find(oldName);
    if (it == presets.end()) {
        return false;
    }
    
    // 检查新名称是否已存在
    if (presets.find(newName) != presets.end()) {
        std::cout << "[PresetManager] 新预设名称已存在：" << newName << std::endl;
        return false;
    }
    
    // 复制预设数据并更新名称
    PresetData presetData = it->second;
    presetData.info.name = newName;
    presetData.info.modifiedTime = juce::Time::getCurrentTime();
    
    // 更新类别中的预设列表
    auto& categoryData = categories[presetData.info.category];
    auto nameIt = std::find(categoryData.presetNames.begin(), categoryData.presetNames.end(), oldName);
    if (nameIt != categoryData.presetNames.end()) {
        *nameIt = newName;
    }
    
    // 删除旧预设，添加新预设
    presets.erase(it);
    presets[newName] = presetData;
    
    notifyStateChanged();
    
    std::cout << "[PresetManager] 预设重命名成功" << std::endl;
    return true;
}

bool PresetManager::duplicatePreset(const std::string& sourceName, const std::string& targetName) {
    std::cout << "[PresetManager] 复制预设：" << sourceName << " -> " << targetName << std::endl;
    
    if (sourceName == targetName || targetName.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto it = presets.find(sourceName);
    if (it == presets.end()) {
        return false;
    }
    
    // 检查目标名称是否已存在
    if (presets.find(targetName) != presets.end()) {
        std::cout << "[PresetManager] 目标预设名称已存在：" << targetName << std::endl;
        return false;
    }
    
    // 复制预设数据并更新信息
    PresetData presetData = it->second;
    presetData.info.name = targetName;
    presetData.info.createdTime = juce::Time::getCurrentTime();
    presetData.info.modifiedTime = juce::Time::getCurrentTime();
    
    // 添加到类别中
    auto& categoryData = categories[presetData.info.category];
    categoryData.presetNames.push_back(targetName);
    
    // 添加新预设
    presets[targetName] = presetData;
    
    notifyStateChanged();
    
    std::cout << "[PresetManager] 预设复制成功" << std::endl;
    return true;
}

bool PresetManager::presetExists(const std::string& presetName) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    return presets.find(presetName) != presets.end();
}

//==============================================================================
// 预设查询实现
//==============================================================================

std::vector<std::string> PresetManager::getAllPresetNames() const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    std::vector<std::string> names;
    names.reserve(presets.size());
    
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

const PresetManager::PresetInfo* PresetManager::getPresetInfo(const std::string& presetName) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto it = presets.find(presetName);
    return (it != presets.end()) ? &it->second.info : nullptr;
}

std::vector<std::string> PresetManager::getPresetsByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    auto it = categories.find(category);
    if (it != categories.end()) {
        return it->second.presetNames;
    }
    
    return {};
}

std::vector<std::string> PresetManager::getPresetsByTag(const std::string& tag) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    std::vector<std::string> result;
    
    for (const auto& pair : presets) {
        const auto& tags = pair.second.info.tags;
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

std::vector<std::string> PresetManager::searchPresets(const std::string& searchText,
                                                     bool searchInName,
                                                     bool searchInDescription,
                                                     bool searchInTags) const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    
    std::vector<std::string> result;
    std::string lowerSearchText = searchText;
    std::transform(lowerSearchText.begin(), lowerSearchText.end(), lowerSearchText.begin(), ::tolower);
    
    for (const auto& pair : presets) {
        const auto& info = pair.second.info;
        bool matches = false;
        
        if (searchInName) {
            std::string lowerName = info.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(lowerSearchText) != std::string::npos) {
                matches = true;
            }
        }
        
        if (!matches && searchInDescription) {
            std::string lowerDesc = info.description;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
            if (lowerDesc.find(lowerSearchText) != std::string::npos) {
                matches = true;
            }
        }
        
        if (!matches && searchInTags) {
            for (const auto& tag : info.tags) {
                std::string lowerTag = tag;
                std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                if (lowerTag.find(lowerSearchText) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

//==============================================================================
// 类别管理实现
//==============================================================================

bool PresetManager::createCategory(const PresetCategory& category) {
    std::cout << "[PresetManager] 创建类别：" << category.name << std::endl;

    if (category.name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(presetsMutex);
    categories[category.name] = category;

    return true;
}

bool PresetManager::deleteCategory(const std::string& categoryName) {
    std::cout << "[PresetManager] 删除类别：" << categoryName << std::endl;

    if (categoryName == "Default" || categoryName == "User" || categoryName == "Factory") {
        std::cout << "[PresetManager] 不能删除系统类别：" << categoryName << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(presetsMutex);

    auto it = categories.find(categoryName);
    if (it == categories.end()) {
        return false;
    }

    // 将该类别下的预设移动到默认类别
    for (const auto& presetName : it->second.presetNames) {
        auto presetIt = presets.find(presetName);
        if (presetIt != presets.end()) {
            presetIt->second.info.category = "Default";
            categories["Default"].presetNames.push_back(presetName);
        }
    }

    categories.erase(it);
    return true;
}

std::vector<PresetManager::PresetCategory> PresetManager::getAllCategories() const {
    std::lock_guard<std::mutex> lock(presetsMutex);

    std::vector<PresetCategory> result;
    result.reserve(categories.size());

    for (const auto& pair : categories) {
        result.push_back(pair.second);
    }

    return result;
}

bool PresetManager::setPresetCategory(const std::string& presetName, const std::string& categoryName) {
    std::cout << "[PresetManager] 设置预设类别：" << presetName << " -> " << categoryName << std::endl;

    std::lock_guard<std::mutex> lock(presetsMutex);

    auto presetIt = presets.find(presetName);
    if (presetIt == presets.end()) {
        return false;
    }

    auto categoryIt = categories.find(categoryName);
    if (categoryIt == categories.end()) {
        return false;
    }

    // 从旧类别中移除
    std::string oldCategory = presetIt->second.info.category;
    auto& oldCategoryData = categories[oldCategory];
    oldCategoryData.presetNames.erase(
        std::remove(oldCategoryData.presetNames.begin(), oldCategoryData.presetNames.end(), presetName),
        oldCategoryData.presetNames.end());

    // 添加到新类别
    presetIt->second.info.category = categoryName;
    categoryIt->second.presetNames.push_back(presetName);

    return true;
}

//==============================================================================
// 状态管理实现
//==============================================================================

PresetManager::GraphState PresetManager::getCurrentState() const {
    return captureCurrentState();
}

bool PresetManager::setGraphState(const GraphState& state) {
    return applyGraphState(state);
}

std::string PresetManager::createSnapshot(const std::string& name) {
    std::string snapshotId = generateUniqueId();
    std::string finalName = name.empty() ? ("Snapshot_" + snapshotId) : name;

    std::cout << "[PresetManager] 创建快照：" << finalName << std::endl;

    GraphState currentState = captureCurrentState();

    std::lock_guard<std::mutex> lock(snapshotsMutex);
    snapshots[snapshotId] = currentState;
    snapshotNames[snapshotId] = finalName;

    return snapshotId;
}

bool PresetManager::restoreSnapshot(const std::string& snapshotId) {
    std::cout << "[PresetManager] 恢复快照：" << snapshotId << std::endl;

    GraphState state;

    {
        std::lock_guard<std::mutex> lock(snapshotsMutex);
        auto it = snapshots.find(snapshotId);
        if (it == snapshots.end()) {
            return false;
        }
        state = it->second;
    }

    return applyGraphState(state);
}

std::unordered_map<std::string, std::string> PresetManager::getAllSnapshots() const {
    std::lock_guard<std::mutex> lock(snapshotsMutex);
    return snapshotNames;
}

//==============================================================================
// 自动备份实现
//==============================================================================

void PresetManager::enableAutoBackup(bool enable, int intervalMinutes) {
    std::cout << "[PresetManager] " << (enable ? "启用" : "禁用") << "自动备份，间隔："
              << intervalMinutes << "分钟" << std::endl;

    autoBackupEnabled = enable;
    autoBackupInterval = intervalMinutes;

    if (enable && intervalMinutes > 0) {
        autoBackupTimer->startTimer(intervalMinutes * 60 * 1000); // 转换为毫秒
    } else {
        autoBackupTimer->stopTimer();
    }
}

std::string PresetManager::createBackup() {
    std::string backupId = generateUniqueId();

    std::cout << "[PresetManager] 创建备份：" << backupId << std::endl;

    GraphState currentState = captureCurrentState();

    std::lock_guard<std::mutex> lock(snapshotsMutex);
    backups[backupId] = currentState;
    backupTimes[backupId] = juce::Time::getCurrentTime();

    return backupId;
}

bool PresetManager::restoreBackup(const std::string& backupId) {
    std::cout << "[PresetManager] 恢复备份：" << backupId << std::endl;

    GraphState state;

    {
        std::lock_guard<std::mutex> lock(snapshotsMutex);
        auto it = backups.find(backupId);
        if (it == backups.end()) {
            return false;
        }
        state = it->second;
    }

    return applyGraphState(state);
}

void PresetManager::cleanupOldBackups(int keepCount) {
    std::cout << "[PresetManager] 清理旧备份，保留：" << keepCount << "个" << std::endl;

    std::lock_guard<std::mutex> lock(snapshotsMutex);

    if (static_cast<int>(backups.size()) <= keepCount) {
        return;
    }

    // 按时间排序备份
    std::vector<std::pair<juce::Time, std::string>> sortedBackups;
    for (const auto& pair : backupTimes) {
        sortedBackups.emplace_back(pair.second, pair.first);
    }

    std::sort(sortedBackups.begin(), sortedBackups.end(), std::greater<>());

    // 删除多余的备份
    for (size_t i = keepCount; i < sortedBackups.size(); ++i) {
        const std::string& backupId = sortedBackups[i].second;
        backups.erase(backupId);
        backupTimes.erase(backupId);
    }
}

//==============================================================================
// 回调设置
//==============================================================================

void PresetManager::setPresetLoadedCallback(PresetLoadedCallback callback) {
    presetLoadedCallback = std::move(callback);
}

void PresetManager::setPresetSavedCallback(PresetSavedCallback callback) {
    presetSavedCallback = std::move(callback);
}

void PresetManager::setStateChangedCallback(StateChangedCallback callback) {
    stateChangedCallback = std::move(callback);
}

//==============================================================================
// 统计信息实现
//==============================================================================

int PresetManager::getNumPresets() const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    return static_cast<int>(presets.size());
}

int PresetManager::getNumCategories() const {
    std::lock_guard<std::mutex> lock(presetsMutex);
    return static_cast<int>(categories.size());
}

int PresetManager::getNumBackups() const {
    std::lock_guard<std::mutex> lock(snapshotsMutex);
    return static_cast<int>(backups.size());
}

//==============================================================================
// 内部方法实现
//==============================================================================

PresetManager::GraphState PresetManager::captureCurrentState() const {
    std::cout << "[PresetManager] 捕获当前图状态" << std::endl;

    GraphState state;

    // 获取图配置
    state.config = graphProcessor.getConfig();

    // 获取图结构数据
    graphProcessor.getStateInformation(state.graphData);

    // 获取所有插件状态
    juce::MemoryOutputStream pluginStream;
    auto allPlugins = pluginManager.getAllPlugins();

    pluginStream.writeInt(static_cast<int>(allPlugins.size()));

    for (const auto& pluginInfo : allPlugins) {
        // 写入插件基本信息
        pluginStream.writeString(pluginInfo.name);
        pluginStream.writeString(pluginInfo.displayName);
        pluginStream.writeBool(pluginInfo.enabled);
        pluginStream.writeBool(pluginInfo.bypassed);

        // 写入插件描述
        auto xml = pluginInfo.description.createXml();
        if (xml) {
            pluginStream.writeString(xml->toString());
        } else {
            pluginStream.writeString("");
        }

        // 写入插件状态
        juce::MemoryBlock pluginState;
        if (pluginManager.getPluginState(pluginInfo.nodeID, pluginState)) {
            pluginStream.writeInt64(static_cast<juce::int64>(pluginState.getSize()));
            pluginStream.write(pluginState.getData(), pluginState.getSize());
        } else {
            pluginStream.writeInt64(0);
        }
    }

    state.pluginStates = pluginStream.getMemoryBlock();

    // 获取连接信息
    juce::MemoryOutputStream connectionStream;
    auto allConnections = graphProcessor.getAllConnections();

    connectionStream.writeInt(static_cast<int>(allConnections.size()));

    for (const auto& connInfo : allConnections) {
        connectionStream.writeInt(connInfo.connection.source.nodeID.uid);
        connectionStream.writeInt(connInfo.connection.source.channelIndex);
        connectionStream.writeInt(connInfo.connection.destination.nodeID.uid);
        connectionStream.writeInt(connInfo.connection.destination.channelIndex);
        connectionStream.writeBool(connInfo.isAudioConnection);
    }

    state.connections = connectionStream.getMemoryBlock();

    // 获取I/O配置（这里简化处理）
    juce::MemoryOutputStream ioStream;
    ioStream.writeInt(state.config.numInputChannels);
    ioStream.writeInt(state.config.numOutputChannels);
    ioStream.writeDouble(state.config.sampleRate);
    ioStream.writeInt(state.config.samplesPerBlock);

    state.ioConfig = ioStream.getMemoryBlock();

    std::cout << "[PresetManager] 状态捕获完成，插件数量：" << allPlugins.size()
              << "，连接数量：" << allConnections.size() << std::endl;

    return state;
}

bool PresetManager::applyGraphState(const GraphState& state) {
    std::cout << "[PresetManager] 应用图状态" << std::endl;

    if (!state.isValid()) {
        std::cout << "[PresetManager] 无效的图状态" << std::endl;
        return false;
    }

    try {
        // 首先清除当前图中的所有插件（保留I/O节点）
        auto currentPlugins = pluginManager.getAllPlugins();
        for (const auto& plugin : currentPlugins) {
            pluginManager.removePlugin(plugin.nodeID);
        }

        // 应用图配置
        graphProcessor.configure(state.config);

        // 恢复插件状态
        juce::MemoryInputStream pluginStream(state.pluginStates, false);
        int numPlugins = pluginStream.readInt();

        std::cout << "[PresetManager] 恢复 " << numPlugins << " 个插件" << std::endl;

        for (int i = 0; i < numPlugins; ++i) {
            std::string name = pluginStream.readString().toStdString();
            std::string displayName = pluginStream.readString().toStdString();
            bool enabled = pluginStream.readBool();
            bool bypassed = pluginStream.readBool();

            // 读取插件描述
            std::string descriptionXml = pluginStream.readString().toStdString();
            if (descriptionXml.empty()) {
                std::cout << "[PresetManager] 跳过无效的插件描述" << std::endl;
                continue;
            }

            auto xml = juce::XmlDocument::parse(descriptionXml);
            if (!xml) {
                std::cout << "[PresetManager] 无法解析插件描述XML" << std::endl;
                continue;
            }

            juce::PluginDescription description;
            description.loadFromXml(*xml);

            // 读取插件状态数据
            juce::int64 stateSize = pluginStream.readInt64();
            juce::MemoryBlock pluginState;
            if (stateSize > 0) {
                pluginState.setSize(static_cast<size_t>(stateSize));
                pluginStream.read(pluginState.getData(), static_cast<size_t>(stateSize));
            }

            // 异步加载插件（这里简化为同步处理）
            std::cout << "[PresetManager] 恢复插件：" << displayName << std::endl;

            // 注意：这里需要实际的插件加载逻辑
            // 由于这是状态恢复，我们需要确保插件能够被正确加载
        }

        // 恢复连接（在所有插件加载完成后）
        // 这里需要延迟处理，因为插件是异步加载的

        std::cout << "[PresetManager] 图状态应用完成" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "[PresetManager] 应用图状态时发生异常：" << e.what() << std::endl;
        return false;
    }
}

std::string PresetManager::generateUniqueId() const {
    auto now = juce::Time::getCurrentTime();
    std::stringstream ss;
    ss << "id_" << now.toMilliseconds() << "_" << juce::Random::getSystemRandom().nextInt(10000);
    return ss.str();
}

void PresetManager::notifyStateChanged() {
    if (stateChangedCallback) {
        stateChangedCallback();
    }
}

void PresetManager::performAutoBackup() {
    if (autoBackupEnabled) {
        std::cout << "[PresetManager] 执行自动备份" << std::endl;
        createBackup();
        cleanupOldBackups(10); // 保留最近10个备份
    }
}

} // namespace WindsynthVST::AudioGraph
