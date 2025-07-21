//
//  PresetManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  预设管理器，管理整个音频图的预设和状态
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <mutex>
#include "../Core/GraphAudioProcessor.hpp"
#include "../Core/AudioGraphTypes.hpp"
#include "../Plugins/PluginManager.hpp"

namespace WindsynthVST::AudioGraph {

/**
 * 预设管理器
 * 
 * 管理整个音频图的预设和状态：
 * - 完整图状态的保存和加载
 * - 预设的分类和组织
 * - 预设的导入导出
 * - 版本控制和兼容性管理
 * - 自动备份和恢复
 */
class PresetManager {
public:
    //==============================================================================
    // 类型定义
    //==============================================================================
    
    /**
     * 预设信息
     */
    struct PresetInfo {
        std::string name;
        std::string description;
        std::string category;
        std::string author;
        juce::String version;
        juce::Time createdTime;
        juce::Time modifiedTime;
        std::vector<std::string> tags;
        
        PresetInfo() = default;
        PresetInfo(const std::string& n, const std::string& desc = "", const std::string& cat = "")
            : name(n), description(desc), category(cat), 
              createdTime(juce::Time::getCurrentTime()), modifiedTime(juce::Time::getCurrentTime()) {}
    };
    
    /**
     * 图状态数据
     */
    struct GraphState {
        juce::MemoryBlock graphData;        // 图结构数据
        juce::MemoryBlock pluginStates;    // 所有插件状态
        juce::MemoryBlock connections;     // 连接信息
        juce::MemoryBlock ioConfig;        // I/O配置
        GraphConfig config;                // 图配置
        
        bool isValid() const {
            return graphData.getSize() > 0;
        }
    };
    
    /**
     * 预设数据
     */
    struct PresetData {
        PresetInfo info;
        GraphState state;
        juce::String formatVersion = "1.0";
        
        PresetData() = default;
        PresetData(const PresetInfo& i, const GraphState& s) : info(i), state(s) {}
    };
    
    /**
     * 预设类别
     */
    struct PresetCategory {
        std::string name;
        std::string description;
        std::vector<std::string> presetNames;
        
        PresetCategory() = default;
        PresetCategory(const std::string& n, const std::string& desc = "") 
            : name(n), description(desc) {}
    };
    
    //==============================================================================
    // 回调类型定义
    //==============================================================================
    
    using PresetLoadedCallback = std::function<void(const std::string& presetName, bool success)>;
    using PresetSavedCallback = std::function<void(const std::string& presetName, bool success)>;
    using StateChangedCallback = std::function<void()>;
    
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param graphProcessor 音频图处理器
     * @param pluginManager 插件管理器
     */
    PresetManager(GraphAudioProcessor& graphProcessor, PluginManager& pluginManager);
    
    /**
     * 析构函数
     */
    ~PresetManager();
    
    //==============================================================================
    // 预设管理
    //==============================================================================
    
    /**
     * 保存当前状态为预设
     * @param presetName 预设名称
     * @param info 预设信息
     * @return 成功返回true
     */
    bool savePreset(const std::string& presetName, const PresetInfo& info = PresetInfo());
    
    /**
     * 加载预设
     * @param presetName 预设名称
     * @param callback 加载完成回调
     * @return 成功返回true
     */
    bool loadPreset(const std::string& presetName, PresetLoadedCallback callback = nullptr);
    
    /**
     * 删除预设
     * @param presetName 预设名称
     * @return 成功返回true
     */
    bool deletePreset(const std::string& presetName);
    
    /**
     * 重命名预设
     * @param oldName 旧名称
     * @param newName 新名称
     * @return 成功返回true
     */
    bool renamePreset(const std::string& oldName, const std::string& newName);
    
    /**
     * 复制预设
     * @param sourceName 源预设名称
     * @param targetName 目标预设名称
     * @return 成功返回true
     */
    bool duplicatePreset(const std::string& sourceName, const std::string& targetName);
    
    /**
     * 检查预设是否存在
     * @param presetName 预设名称
     * @return 存在返回true
     */
    bool presetExists(const std::string& presetName) const;
    
    //==============================================================================
    // 预设查询
    //==============================================================================
    
    /**
     * 获取所有预设名称
     * @return 预设名称列表
     */
    std::vector<std::string> getAllPresetNames() const;
    
    /**
     * 获取预设信息
     * @param presetName 预设名称
     * @return 预设信息，未找到时返回nullptr
     */
    const PresetInfo* getPresetInfo(const std::string& presetName) const;
    
    /**
     * 按类别获取预设
     * @param category 类别名称
     * @return 预设名称列表
     */
    std::vector<std::string> getPresetsByCategory(const std::string& category) const;
    
    /**
     * 按标签搜索预设
     * @param tag 标签
     * @return 预设名称列表
     */
    std::vector<std::string> getPresetsByTag(const std::string& tag) const;
    
    /**
     * 搜索预设
     * @param searchText 搜索文本
     * @param searchInName 是否在名称中搜索
     * @param searchInDescription 是否在描述中搜索
     * @param searchInTags 是否在标签中搜索
     * @return 匹配的预设名称列表
     */
    std::vector<std::string> searchPresets(const std::string& searchText,
                                          bool searchInName = true,
                                          bool searchInDescription = true,
                                          bool searchInTags = true) const;
    
    //==============================================================================
    // 类别管理
    //==============================================================================
    
    /**
     * 创建预设类别
     * @param category 类别信息
     * @return 成功返回true
     */
    bool createCategory(const PresetCategory& category);
    
    /**
     * 删除预设类别
     * @param categoryName 类别名称
     * @return 成功返回true
     */
    bool deleteCategory(const std::string& categoryName);
    
    /**
     * 获取所有类别
     * @return 类别列表
     */
    std::vector<PresetCategory> getAllCategories() const;
    
    /**
     * 设置预设类别
     * @param presetName 预设名称
     * @param categoryName 类别名称
     * @return 成功返回true
     */
    bool setPresetCategory(const std::string& presetName, const std::string& categoryName);
    
    //==============================================================================
    // 文件操作
    //==============================================================================
    
    /**
     * 导出预设到文件
     * @param presetName 预设名称
     * @param file 导出文件
     * @return 成功返回true
     */
    bool exportPreset(const std::string& presetName, const juce::File& file) const;
    
    /**
     * 从文件导入预设
     * @param file 导入文件
     * @param presetName 预设名称（可选，为空时使用文件中的名称）
     * @return 成功返回true
     */
    bool importPreset(const juce::File& file, const std::string& presetName = "");
    
    /**
     * 导出所有预设
     * @param directory 导出目录
     * @return 成功导出的预设数量
     */
    int exportAllPresets(const juce::File& directory) const;
    
    /**
     * 从目录导入预设
     * @param directory 导入目录
     * @return 成功导入的预设数量
     */
    int importPresetsFromDirectory(const juce::File& directory);
    
    //==============================================================================
    // 状态管理
    //==============================================================================
    
    /**
     * 获取当前图状态
     * @return 图状态数据
     */
    GraphState getCurrentState() const;
    
    /**
     * 设置图状态
     * @param state 图状态数据
     * @return 成功返回true
     */
    bool setGraphState(const GraphState& state);
    
    /**
     * 创建状态快照
     * @param name 快照名称
     * @return 快照ID
     */
    std::string createSnapshot(const std::string& name = "");
    
    /**
     * 恢复状态快照
     * @param snapshotId 快照ID
     * @return 成功返回true
     */
    bool restoreSnapshot(const std::string& snapshotId);
    
    /**
     * 获取所有快照
     * @return 快照ID到名称的映射
     */
    std::unordered_map<std::string, std::string> getAllSnapshots() const;
    
    //==============================================================================
    // 自动备份
    //==============================================================================
    
    /**
     * 启用自动备份
     * @param enable 是否启用
     * @param intervalMinutes 备份间隔（分钟）
     */
    void enableAutoBackup(bool enable, int intervalMinutes = 5);
    
    /**
     * 手动创建备份
     * @return 备份ID
     */
    std::string createBackup();
    
    /**
     * 恢复备份
     * @param backupId 备份ID
     * @return 成功返回true
     */
    bool restoreBackup(const std::string& backupId);
    
    /**
     * 清理旧备份
     * @param keepCount 保留的备份数量
     */
    void cleanupOldBackups(int keepCount = 10);
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置预设加载回调
     */
    void setPresetLoadedCallback(PresetLoadedCallback callback);
    
    /**
     * 设置预设保存回调
     */
    void setPresetSavedCallback(PresetSavedCallback callback);
    
    /**
     * 设置状态变化回调
     */
    void setStateChangedCallback(StateChangedCallback callback);
    
    //==============================================================================
    // 统计信息
    //==============================================================================
    
    /**
     * 获取预设数量
     * @return 预设数量
     */
    int getNumPresets() const;
    
    /**
     * 获取类别数量
     * @return 类别数量
     */
    int getNumCategories() const;
    
    /**
     * 获取备份数量
     * @return 备份数量
     */
    int getNumBackups() const;

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    GraphAudioProcessor& graphProcessor;
    PluginManager& pluginManager;
    
    // 预设存储
    mutable std::mutex presetsMutex;
    std::unordered_map<std::string, PresetData> presets;
    std::unordered_map<std::string, PresetCategory> categories;
    
    // 快照和备份
    mutable std::mutex snapshotsMutex;
    std::unordered_map<std::string, GraphState> snapshots;
    std::unordered_map<std::string, std::string> snapshotNames;
    std::unordered_map<std::string, GraphState> backups;
    std::unordered_map<std::string, juce::Time> backupTimes;
    
    // 自动备份
    std::unique_ptr<juce::Timer> autoBackupTimer;
    bool autoBackupEnabled = false;
    int autoBackupInterval = 5;
    
    // 回调函数
    PresetLoadedCallback presetLoadedCallback;
    PresetSavedCallback presetSavedCallback;
    StateChangedCallback stateChangedCallback;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    GraphState captureCurrentState() const;
    bool applyGraphState(const GraphState& state);
    std::string generateUniqueId() const;
    void notifyStateChanged();
    void performAutoBackup();
    
    // 序列化方法
    std::unique_ptr<juce::XmlElement> serializePresetData(const PresetData& data) const;
    PresetData deserializePresetData(const juce::XmlElement& xml) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace WindsynthVST::AudioGraph
