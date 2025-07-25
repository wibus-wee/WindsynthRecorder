//
//  IPluginManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件管理接口
//

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace WindsynthVST::Engine::Interfaces {

/**
 * 插件信息结构（简化版）
 */
struct SimplePluginInfo {
    std::string identifier;
    std::string name;
    std::string manufacturer;
    std::string category;
    std::string format;
    std::string filePath;
    bool isValid = false;
};

/**
 * 节点信息结构（简化版）
 */
struct SimpleNodeInfo {
    uint32_t nodeID = 0;
    std::string name;
    std::string pluginName;
    bool isEnabled = true;
    bool isBypassed = false;
    int numInputChannels = 0;
    int numOutputChannels = 0;
};

/**
 * 插件加载回调函数类型
 */
using PluginLoadCallback = std::function<void(uint32_t nodeID, bool success, const std::string& error)>;

/**
 * 插件管理接口
 * 
 * 负责插件的扫描、加载和管理
 */
class IPluginManager {
public:
    virtual ~IPluginManager() = default;
    
    /**
     * 获取可用插件列表
     * @return 插件信息列表
     */
    virtual std::vector<SimplePluginInfo> getAvailablePlugins() const = 0;
    
    /**
     * 异步加载插件
     * @param pluginIdentifier 插件标识符
     * @param displayName 显示名称（可选）
     * @param callback 加载完成回调
     */
    virtual void loadPluginAsync(const std::string& pluginIdentifier,
                               const std::string& displayName = "",
                               PluginLoadCallback callback = nullptr) = 0;
    
    /**
     * 移除插件节点
     * @param nodeID 节点ID
     * @return 成功返回true
     */
    virtual bool removeNode(uint32_t nodeID) = 0;
    
    /**
     * 获取已加载的节点列表
     * @return 节点信息列表
     */
    virtual std::vector<SimpleNodeInfo> getLoadedNodes() const = 0;
    
    /**
     * 设置节点旁路状态
     * @param nodeID 节点ID
     * @param bypassed 是否旁路
     * @return 成功返回true
     */
    virtual bool setNodeBypassed(uint32_t nodeID, bool bypassed) = 0;
    
    /**
     * 设置节点启用状态
     * @param nodeID 节点ID
     * @param enabled 是否启用
     * @return 成功返回true
     */
    virtual bool setNodeEnabled(uint32_t nodeID, bool enabled) = 0;
    
    /**
     * 获取节点名称
     * @param nodeID 节点ID
     * @return 节点名称
     */
    virtual std::string getNodeName(uint32_t nodeID) const = 0;
    
    /**
     * 设置节点名称
     * @param nodeID 节点ID
     * @param name 新名称
     * @return 成功返回true
     */
    virtual bool setNodeName(uint32_t nodeID, const std::string& name) = 0;
};

} // namespace WindsynthVST::Engine::Interfaces
