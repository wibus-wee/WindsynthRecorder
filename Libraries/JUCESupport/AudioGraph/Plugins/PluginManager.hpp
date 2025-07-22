//
//  PluginManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理器，管理已加载的插件实例
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
#include "ModernPluginLoader.hpp"

namespace WindsynthVST::AudioGraph {

/**
 * 插件管理器
 * 
 * 管理已加载的插件实例，提供高级插件管理功能：
 * - 插件实例的生命周期管理
 * - 插件参数管理和自动化
 * - 插件状态保存和恢复
 * - 插件性能监控
 * - 插件预设管理
 */
class PluginManager {
public:
    //==============================================================================
    // 类型定义
    //==============================================================================
    
    /**
     * 插件实例信息
     */
    struct PluginInstanceInfo {
        NodeID nodeID;
        std::string name;
        std::string displayName;
        juce::PluginDescription description;
        bool enabled = true;
        bool bypassed = false;
        double cpuUsage = 0.0;
        int latencySamples = 0;
        juce::Time loadTime;
        
        PluginInstanceInfo() = default;
        PluginInstanceInfo(NodeID id, const std::string& n, const juce::PluginDescription& desc)
            : nodeID(id), name(n), description(desc), loadTime(juce::Time::getCurrentTime()) {}
    };
    
    /**
     * 插件参数信息
     */
    struct ParameterInfo {
        int index;
        std::string name;
        std::string label;
        float value;
        float defaultValue;
        bool isAutomatable;
        bool isDiscrete;
        int numSteps;
        
        ParameterInfo() = default;
        ParameterInfo(int idx, const std::string& n, float val, float def = 0.0f)
            : index(idx), name(n), value(val), defaultValue(def), isAutomatable(true), 
              isDiscrete(false), numSteps(0) {}
    };
    
    /**
     * 插件预设信息
     */
    struct PresetInfo {
        std::string name;
        juce::MemoryBlock data;
        juce::Time createdTime;
        
        PresetInfo() = default;
        PresetInfo(const std::string& n, const juce::MemoryBlock& d)
            : name(n), data(d), createdTime(juce::Time::getCurrentTime()) {}
    };
    
    //==============================================================================
    // 回调类型定义
    //==============================================================================
    
    using PluginLoadedCallback = std::function<void(NodeID nodeID, const PluginInstanceInfo& info)>;
    using PluginRemovedCallback = std::function<void(NodeID nodeID)>;
    using ParameterChangedCallback = std::function<void(NodeID nodeID, int parameterIndex, float newValue)>;
    using PluginErrorCallback = std::function<void(NodeID nodeID, const std::string& error)>;
    
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param graphProcessor 音频图处理器
     * @param pluginLoader 插件加载器
     */
    PluginManager(GraphAudioProcessor& graphProcessor, ModernPluginLoader& pluginLoader);
    
    /**
     * 析构函数
     */
    ~PluginManager();
    
    //==============================================================================
    // 插件实例管理
    //==============================================================================
    
    /**
     * 异步加载插件
     * @param description 插件描述
     * @param displayName 显示名称（可选）
     * @param callback 加载完成回调
     */
    void loadPluginAsync(const juce::PluginDescription& description,
                        const std::string& displayName = "",
                        std::function<void(NodeID nodeID, const std::string& error)> callback = nullptr);
    
    /**
     * 移除插件
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool removePlugin(NodeID nodeID);
    
    /**
     * 获取所有插件实例信息
     * @return 插件实例信息列表
     */
    std::vector<PluginInstanceInfo> getAllPlugins() const;
    
    /**
     * 获取插件实例信息
     * @param nodeID 节点ID
     * @return 插件实例信息，未找到时返回nullptr
     */
    const PluginInstanceInfo* getPluginInfo(NodeID nodeID) const;
    
    /**
     * 获取插件实例
     * @param nodeID 节点ID
     * @return 插件实例，未找到时返回nullptr
     */
    juce::AudioPluginInstance* getPluginInstance(NodeID nodeID) const;
    
    /**
     * 设置插件启用状态
     * @param nodeID 节点ID
     * @param enabled 是否启用
     * @return 成功返回true
     */
    bool setPluginEnabled(NodeID nodeID, bool enabled);
    
    /**
     * 设置插件旁路状态
     * @param nodeID 节点ID
     * @param bypassed 是否旁路
     * @return 成功返回true
     */
    bool setPluginBypassed(NodeID nodeID, bool bypassed);
    
    /**
     * 重命名插件
     * @param nodeID 节点ID
     * @param newName 新名称
     * @return 成功返回true
     */
    bool renamePlugin(NodeID nodeID, const std::string& newName);
    
    //==============================================================================
    // 插件参数管理
    //==============================================================================
    
    /**
     * 获取插件参数列表
     * @param nodeID 节点ID
     * @return 参数信息列表
     */
    std::vector<ParameterInfo> getPluginParameters(NodeID nodeID) const;
    
    /**
     * 获取参数值
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数值，失败时返回0.0f
     */
    float getParameterValue(NodeID nodeID, int parameterIndex) const;
    
    /**
     * 设置参数值
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @param value 参数值
     * @return 成功返回true
     */
    bool setParameterValue(NodeID nodeID, int parameterIndex, float value);
    
    /**
     * 获取参数文本表示
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数文本
     */
    std::string getParameterText(NodeID nodeID, int parameterIndex) const;
    
    /**
     * 重置所有参数到默认值
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool resetParametersToDefault(NodeID nodeID);

    //==============================================================================
    // 编辑器窗口管理
    //==============================================================================

    /**
     * 显示插件编辑器
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool showEditor(NodeID nodeID);

    /**
     * 隐藏插件编辑器
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    bool hideEditor(NodeID nodeID);

    /**
     * 检查编辑器是否可见
     * @param nodeID 节点ID
     * @return 可见返回true
     */
    bool isEditorVisible(NodeID nodeID) const;

    //==============================================================================
    // 插件预设管理
    //==============================================================================
    
    /**
     * 保存插件预设
     * @param nodeID 节点ID
     * @param presetName 预设名称
     * @return 成功返回true
     */
    bool savePreset(NodeID nodeID, const std::string& presetName);
    
    /**
     * 加载插件预设
     * @param nodeID 节点ID
     * @param presetName 预设名称
     * @return 成功返回true
     */
    bool loadPreset(NodeID nodeID, const std::string& presetName);
    
    /**
     * 删除插件预设
     * @param nodeID 节点ID
     * @param presetName 预设名称
     * @return 成功返回true
     */
    bool deletePreset(NodeID nodeID, const std::string& presetName);
    
    /**
     * 获取插件预设列表
     * @param nodeID 节点ID
     * @return 预设名称列表
     */
    std::vector<std::string> getPresetNames(NodeID nodeID) const;
    
    /**
     * 导出预设到文件
     * @param nodeID 节点ID
     * @param presetName 预设名称
     * @param file 导出文件
     * @return 成功返回true
     */
    bool exportPreset(NodeID nodeID, const std::string& presetName, const juce::File& file) const;
    
    /**
     * 从文件导入预设
     * @param nodeID 节点ID
     * @param presetName 预设名称
     * @param file 导入文件
     * @return 成功返回true
     */
    bool importPreset(NodeID nodeID, const std::string& presetName, const juce::File& file);
    
    //==============================================================================
    // 插件状态管理
    //==============================================================================
    
    /**
     * 获取插件状态
     * @param nodeID 节点ID
     * @param stateData 状态数据输出
     * @return 成功返回true
     */
    bool getPluginState(NodeID nodeID, juce::MemoryBlock& stateData) const;
    
    /**
     * 设置插件状态
     * @param nodeID 节点ID
     * @param stateData 状态数据
     * @return 成功返回true
     */
    bool setPluginState(NodeID nodeID, const juce::MemoryBlock& stateData);
    
    //==============================================================================
    // 性能监控
    //==============================================================================
    
    /**
     * 更新插件性能统计
     */
    void updatePerformanceStats();
    
    /**
     * 获取插件CPU使用率
     * @param nodeID 节点ID
     * @return CPU使用率百分比
     */
    double getPluginCpuUsage(NodeID nodeID) const;
    
    /**
     * 获取插件延迟
     * @param nodeID 节点ID
     * @return 延迟采样数
     */
    int getPluginLatency(NodeID nodeID) const;
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置插件加载回调
     */
    void setPluginLoadedCallback(PluginLoadedCallback callback);
    
    /**
     * 设置插件移除回调
     */
    void setPluginRemovedCallback(PluginRemovedCallback callback);
    
    /**
     * 设置参数变化回调
     */
    void setParameterChangedCallback(ParameterChangedCallback callback);
    
    /**
     * 设置插件错误回调
     */
    void setPluginErrorCallback(PluginErrorCallback callback);
    
    //==============================================================================
    // 统计信息
    //==============================================================================
    
    /**
     * 获取已加载插件数量
     * @return 插件数量
     */
    int getNumLoadedPlugins() const;
    
    /**
     * 获取总CPU使用率
     * @return CPU使用率百分比
     */
    double getTotalCpuUsage() const;
    
    /**
     * 获取总延迟
     * @return 总延迟采样数
     */
    int getTotalLatency() const;

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    GraphAudioProcessor& graphProcessor;
    ModernPluginLoader& pluginLoader;
    
    // 插件实例管理
    mutable std::mutex pluginsMutex;
    std::unordered_map<NodeID, PluginInstanceInfo> pluginInstances;
    
    // 预设管理
    mutable std::mutex presetsMutex;
    std::unordered_map<NodeID, std::unordered_map<std::string, PresetInfo>> pluginPresets;
    
    // 性能监控
    mutable std::mutex performanceMutex;
    std::unordered_map<NodeID, double> cpuUsageMap;

    // 编辑器窗口管理
    mutable std::mutex editorsMutex;
    std::unordered_map<NodeID, std::unique_ptr<juce::DocumentWindow>> editorWindows;
    
    // 回调函数
    PluginLoadedCallback pluginLoadedCallback;
    PluginRemovedCallback pluginRemovedCallback;
    ParameterChangedCallback parameterChangedCallback;
    PluginErrorCallback pluginErrorCallback;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void handlePluginLoaded(NodeID nodeID, std::unique_ptr<juce::AudioPluginInstance> instance,
                           const std::string& displayName, const juce::PluginDescription& description);
    void notifyPluginLoaded(NodeID nodeID, const PluginInstanceInfo& info);
    void notifyPluginRemoved(NodeID nodeID);
    void notifyParameterChanged(NodeID nodeID, int parameterIndex, float newValue);
    void notifyPluginError(NodeID nodeID, const std::string& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};

} // namespace WindsynthVST::AudioGraph
