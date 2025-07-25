//
//  INodeParameterController.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  节点参数控制接口
//

#pragma once

#include <optional>
#include <string>

namespace WindsynthVST::Engine::Interfaces {

/**
 * 参数信息结构
 */
struct ParameterInfo {
    std::string name;
    std::string label;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float currentValue = 0.0f;
    bool isDiscrete = false;
    int numSteps = 0;
    std::string units;
};

/**
 * 节点参数控制接口
 * 
 * 负责插件节点参数的获取和设置
 */
class INodeParameterController {
public:
    virtual ~INodeParameterController() = default;
    
    /**
     * 设置节点参数
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @param value 参数值（0.0-1.0）
     * @return 成功返回true
     */
    virtual bool setNodeParameter(uint32_t nodeID, int parameterIndex, float value) = 0;
    
    /**
     * 获取节点参数
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数值（0.0-1.0），失败返回-1.0
     */
    virtual float getNodeParameter(uint32_t nodeID, int parameterIndex) const = 0;
    
    /**
     * 获取节点参数数量
     * @param nodeID 节点ID
     * @return 参数数量
     */
    virtual int getNodeParameterCount(uint32_t nodeID) const = 0;
    
    /**
     * 获取节点参数信息
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引
     * @return 参数信息，失败返回空的optional
     */
    virtual std::optional<ParameterInfo> getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const = 0;
    
    /**
     * 重置节点参数到默认值
     * @param nodeID 节点ID
     * @param parameterIndex 参数索引，-1表示重置所有参数
     * @return 成功返回true
     */
    virtual bool resetNodeParameter(uint32_t nodeID, int parameterIndex = -1) = 0;
    
    /**
     * 获取节点的所有参数信息
     * @param nodeID 节点ID
     * @return 参数信息列表
     */
    virtual std::vector<ParameterInfo> getAllParameterInfo(uint32_t nodeID) const = 0;
};

} // namespace WindsynthVST::Engine::Interfaces
