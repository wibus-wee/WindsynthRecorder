//
//  NodeParameterController.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  节点参数控制器
//

#pragma once

#include "../Interfaces/INodeParameterController.hpp"
#include "../Core/EngineContext.hpp"
#include "../Core/EngineObserver.hpp"
#include "../AudioGraph/Core/AudioGraphTypes.hpp"
#include <memory>

namespace WindsynthVST::Engine::Managers {

/**
 * 节点参数控制器实现
 * 
 * 负责插件节点参数的获取和设置
 * 遵循单一职责原则，只处理参数控制相关的逻辑
 */
class NodeParameterController : public Interfaces::INodeParameterController {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param context 共享引擎上下文
     * @param notifier 事件通知器
     */
    explicit NodeParameterController(std::shared_ptr<Core::EngineContext> context,
                                   std::shared_ptr<Core::EngineNotifier> notifier);
    
    ~NodeParameterController() override;
    
    //==============================================================================
    // INodeParameterController 接口实现
    //==============================================================================
    
    bool setNodeParameter(uint32_t nodeID, int parameterIndex, float value) override;
    float getNodeParameter(uint32_t nodeID, int parameterIndex) const override;
    int getNodeParameterCount(uint32_t nodeID) const override;
    std::optional<Interfaces::ParameterInfo> getNodeParameterInfo(uint32_t nodeID, int parameterIndex) const override;
    bool resetNodeParameter(uint32_t nodeID, int parameterIndex = -1) override;
    std::vector<Interfaces::ParameterInfo> getAllParameterInfo(uint32_t nodeID) const override;

private:
    //==============================================================================
    // 成员变量
    //==============================================================================
    
    std::shared_ptr<Core::EngineContext> context_;
    std::shared_ptr<Core::EngineNotifier> notifier_;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void notifyError(const std::string& error);
    AudioGraph::NodeID convertToNodeID(uint32_t nodeID) const;
    juce::AudioProcessor* getPluginInstance(uint32_t nodeID) const;
    bool isValidParameterIndex(juce::AudioProcessor* instance, int parameterIndex) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeParameterController)
};

} // namespace WindsynthVST::Engine::Managers
