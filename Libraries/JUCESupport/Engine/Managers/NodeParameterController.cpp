//
//  NodeParameterController.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  节点参数控制器实现
//

#include "NodeParameterController.hpp"
#include <iostream>

namespace WindsynthVST::Engine::Managers {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

NodeParameterController::NodeParameterController(std::shared_ptr<Core::EngineContext> context,
                                               std::shared_ptr<Core::EngineNotifier> notifier)
    : context_(std::move(context))
    , notifier_(std::move(notifier)) {
    std::cout << "[NodeParameterController] 构造函数" << std::endl;
}

NodeParameterController::~NodeParameterController() {
    std::cout << "[NodeParameterController] 析构函数" << std::endl;
}

//==============================================================================
// INodeParameterController 接口实现
//==============================================================================

bool NodeParameterController::setNodeParameter(uint32_t nodeID, int parameterIndex, float value) {
    if (!context_ || !context_->isInitialized()) {
        notifyError("引擎上下文未初始化");
        return false;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (!instance) {
            notifyError("找不到指定的插件实例");
            return false;
        }
        
        if (!isValidParameterIndex(instance, parameterIndex)) {
            notifyError("无效的参数索引");
            return false;
        }
        
        auto* param = instance->getParameters()[parameterIndex];
        if (param) {
            param->setValueNotifyingHost(value);
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        notifyError("设置节点参数失败: " + std::string(e.what()));
        return false;
    }
}

float NodeParameterController::getNodeParameter(uint32_t nodeID, int parameterIndex) const {
    if (!context_ || !context_->isInitialized()) {
        return -1.0f;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (!instance) {
            return -1.0f;
        }
        
        if (!isValidParameterIndex(instance, parameterIndex)) {
            return -1.0f;
        }
        
        auto* param = instance->getParameters()[parameterIndex];
        if (param) {
            return param->getValue();
        }
        
        return -1.0f;
    } catch (const std::exception& e) {
        std::cerr << "[NodeParameterController] 获取节点参数失败: " << e.what() << std::endl;
        return -1.0f;
    }
}

int NodeParameterController::getNodeParameterCount(uint32_t nodeID) const {
    if (!context_ || !context_->isInitialized()) {
        return 0;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (instance) {
            return static_cast<int>(instance->getParameters().size());
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[NodeParameterController] 获取节点参数数量失败: " << e.what() << std::endl;
        return 0;
    }
}

std::optional<Interfaces::ParameterInfo> NodeParameterController::getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const {
    if (!context_ || !context_->isInitialized()) {
        return std::nullopt;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (!instance) {
            return std::nullopt;
        }
        
        if (!isValidParameterIndex(instance, parameterIndex)) {
            return std::nullopt;
        }
        
        auto* param = instance->getParameters()[parameterIndex];
        if (!param) {
            return std::nullopt;
        }
        
        Interfaces::ParameterInfo info;
        info.name = param->getName(256).toStdString();
        info.label = param->getLabel().toStdString();
        info.minValue = 0.0f;
        info.maxValue = 1.0f;
        info.defaultValue = param->getDefaultValue();
        info.currentValue = param->getValue();
        info.isDiscrete = param->isDiscrete();
        info.numSteps = param->getNumSteps();
        info.units = param->getLabel().toStdString();
        
        return info;
    } catch (const std::exception& e) {
        std::cerr << "[NodeParameterController] 获取节点参数信息失败: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool NodeParameterController::resetNodeParameter(uint32_t nodeID, int parameterIndex) {
    if (!context_ || !context_->isInitialized()) {
        notifyError("引擎上下文未初始化");
        return false;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (!instance) {
            notifyError("找不到指定的插件实例");
            return false;
        }
        
        const auto& parameters = instance->getParameters();
        
        if (parameterIndex == -1) {
            // 重置所有参数
            for (auto* param : parameters) {
                if (param) {
                    param->setValueNotifyingHost(param->getDefaultValue());
                }
            }
            return true;
        } else {
            // 重置指定参数
            if (!isValidParameterIndex(instance, parameterIndex)) {
                notifyError("无效的参数索引");
                return false;
            }
            
            auto* param = parameters[parameterIndex];
            if (param) {
                param->setValueNotifyingHost(param->getDefaultValue());
                return true;
            }
        }
        
        return false;
    } catch (const std::exception& e) {
        notifyError("重置节点参数失败: " + std::string(e.what()));
        return false;
    }
}

std::vector<Interfaces::ParameterInfo> NodeParameterController::getAllParameterInfo(uint32_t nodeID) const {
    std::vector<Interfaces::ParameterInfo> result;
    
    if (!context_ || !context_->isInitialized()) {
        return result;
    }
    
    try {
        auto* instance = getPluginInstance(nodeID);
        if (!instance) {
            return result;
        }
        
        const auto& parameters = instance->getParameters();
        result.reserve(parameters.size());
        
        for (size_t i = 0; i < parameters.size(); ++i) {
            auto* param = parameters[i];
            if (param) {
                Interfaces::ParameterInfo info;
                info.name = param->getName(256).toStdString();
                info.label = param->getLabel().toStdString();
                info.minValue = 0.0f;
                info.maxValue = 1.0f;
                info.defaultValue = param->getDefaultValue();
                info.currentValue = param->getValue();
                info.isDiscrete = param->isDiscrete();
                info.numSteps = param->getNumSteps();
                info.units = param->getLabel().toStdString();
                
                result.push_back(info);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[NodeParameterController] 获取所有参数信息失败: " << e.what() << std::endl;
    }
    
    return result;
}

//==============================================================================
// 内部方法
//==============================================================================

void NodeParameterController::notifyError(const std::string& error) {
    if (notifier_) {
        notifier_->notifyError(error);
    }
    std::cerr << "[NodeParameterController] 错误: " << error << std::endl;
}

AudioGraph::NodeID NodeParameterController::convertToNodeID(uint32_t nodeID) const {
    juce::AudioProcessorGraph::NodeID id;
    id.uid = nodeID;
    return id;
}

juce::AudioProcessor* NodeParameterController::getPluginInstance(uint32_t nodeID) const {
    if (!context_) {
        return nullptr;
    }
    
    auto pluginManager = context_->getPluginManager();
    if (!pluginManager) {
        return nullptr;
    }
    
    AudioGraph::NodeID graphNodeID = convertToNodeID(nodeID);
    return pluginManager->getPluginInstance(graphNodeID);
}

bool NodeParameterController::isValidParameterIndex(juce::AudioProcessor* instance, int parameterIndex) const {
    if (!instance) {
        return false;
    }
    
    const auto& parameters = instance->getParameters();
    return parameterIndex >= 0 && parameterIndex < static_cast<int>(parameters.size());
}

} // namespace WindsynthVST::Engine::Managers
