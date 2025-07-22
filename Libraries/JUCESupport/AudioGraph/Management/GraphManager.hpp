//
//  GraphManager.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图的高级管理器
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include "../Core/GraphAudioProcessor.hpp"
#include "../Core/AudioGraphTypes.hpp"

namespace WindsynthVST::AudioGraph {

/**
 * 音频图的高级管理器
 * 
 * 这个类提供了音频图的高级管理功能：
 * - 智能节点管理和组织
 * - 高级连接验证和优化
 * - 图拓扑分析和验证
 * - 批量操作支持
 * - 撤销/重做功能
 * - 图状态快照和恢复
 */
class GraphManager {
public:
    //==============================================================================
    // 类型定义
    //==============================================================================
    
    /**
     * 图操作类型
     */
    enum class OperationType {
        AddNode,
        RemoveNode,
        AddConnection,
        RemoveConnection,
        SetNodeProperty,
        BatchOperation
    };
    
    /**
     * 图操作记录
     */
    struct GraphOperation {
        OperationType type;
        NodeID nodeID;
        Connection connection;
        std::string propertyName;
        juce::var oldValue;
        juce::var newValue;
        std::vector<GraphOperation> batchOperations;
        
        GraphOperation(OperationType t) : type(t) {}
    };
    
    /**
     * 图验证结果
     */
    struct ValidationResult {
        bool isValid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        void addError(const std::string& error) {
            errors.push_back(error);
            isValid = false;
        }
        
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }
    };
    
    /**
     * 图统计信息
     */
    struct GraphStatistics {
        int totalNodes = 0;
        int vstPluginNodes = 0;
        int ioNodes = 0;
        int totalConnections = 0;
        int audioConnections = 0;
        int midiConnections = 0;
        int maxDepth = 0;
        bool hasLoops = false;
        double estimatedLatency = 0.0;
    };
    
    //==============================================================================
    // 回调类型定义
    //==============================================================================
    
    using GraphChangeCallback = std::function<void(const GraphOperation& operation)>;
    using ValidationCallback = std::function<void(const ValidationResult& result)>;
    
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     * @param graphProcessor 音频图处理器
     */
    explicit GraphManager(GraphAudioProcessor& graphProcessor);
    
    /**
     * 析构函数
     */
    ~GraphManager();
    
    //==============================================================================
    // 高级节点管理
    //==============================================================================
    
    /**
     * 添加节点组（批量添加）
     * @param processors 处理器列表
     * @param names 节点名称列表
     * @return 添加的节点ID列表
     */
    std::vector<NodeID> addNodeGroup(std::vector<std::unique_ptr<juce::AudioProcessor>> processors,
                                    const std::vector<std::string>& names = {});
    
    /**
     * 移除节点组（批量移除）
     * @param nodeIDs 节点ID列表
     * @return 成功移除的节点数量
     */
    int removeNodeGroup(const std::vector<NodeID>& nodeIDs);
    
    /**
     * 复制节点
     * @param sourceNodeID 源节点ID
     * @param newName 新节点名称
     * @return 新节点ID，失败时返回NodeID{0}
     */
    NodeID duplicateNode(NodeID sourceNodeID, const std::string& newName = "");
    
    /**
     * 移动节点（重新排序）
     * @param nodeID 节点ID
     * @param newPosition 新位置索引
     * @return 成功返回true
     */
    bool moveNode(NodeID nodeID, int newPosition);
    
    //==============================================================================
    // 智能连接管理
    //==============================================================================
    
    /**
     * 自动连接节点（智能匹配通道）
     * @param sourceNodeID 源节点ID
     * @param destNodeID 目标节点ID
     * @param connectAudio 是否连接音频
     * @param connectMidi 是否连接MIDI
     * @return 成功创建的连接数量
     */
    int autoConnectNodes(NodeID sourceNodeID, NodeID destNodeID, 
                        bool connectAudio = true, bool connectMidi = true);
    
    /**
     * 创建处理链（串联多个节点）
     * @param nodeIDs 节点ID列表（按处理顺序）
     * @param connectToIO 是否连接到输入输出
     * @return 成功创建的连接数量
     */
    int createProcessingChain(const std::vector<NodeID>& nodeIDs, bool connectToIO = true);
    
    /**
     * 创建并行处理分支
     * @param inputNodeID 输入节点ID
     * @param outputNodeID 输出节点ID
     * @param branchNodeIDs 分支节点ID列表
     * @return 成功创建的连接数量
     */
    int createParallelBranches(NodeID inputNodeID, NodeID outputNodeID, 
                              const std::vector<NodeID>& branchNodeIDs);
    
    /**
     * 断开所有连接并重新组织
     * @param nodeIDs 要重新组织的节点ID列表
     * @param organizationType 组织类型（串联、并联等）
     * @return 成功返回true
     */
    bool reorganizeNodes(const std::vector<NodeID>& nodeIDs, const std::string& organizationType);
    
    //==============================================================================
    // 图验证和分析
    //==============================================================================
    
    /**
     * 验证整个图的有效性
     * @return 验证结果
     */
    ValidationResult validateGraph();
    
    /**
     * 检查连接的有效性
     * @param connection 要检查的连接
     * @return 验证结果
     */
    ValidationResult validateConnection(const Connection& connection);
    
    /**
     * 检测图中的环路
     * @return 如果存在环路返回true
     */
    bool detectLoops();
    
    /**
     * 计算图的处理深度
     * @return 最大处理深度
     */
    int calculateGraphDepth();
    
    /**
     * 估算图的总延迟
     * @return 估算的延迟（以采样为单位）
     */
    double estimateGraphLatency();
    
    /**
     * 获取图统计信息
     * @return 图统计信息
     */
    GraphStatistics getGraphStatistics();
    
    //==============================================================================
    // 图状态管理
    //==============================================================================
    
    /**
     * 创建图状态快照
     * @param name 快照名称
     * @return 快照ID
     */
    std::string createSnapshot(const std::string& name);
    
    /**
     * 恢复图状态快照
     * @param snapshotId 快照ID
     * @return 成功返回true
     */
    bool restoreSnapshot(const std::string& snapshotId);
    
    /**
     * 删除快照
     * @param snapshotId 快照ID
     * @return 成功返回true
     */
    bool deleteSnapshot(const std::string& snapshotId);
    
    /**
     * 获取所有快照列表
     * @return 快照ID和名称的映射
     */
    std::unordered_map<std::string, std::string> getSnapshots();
    
    //==============================================================================
    // 撤销/重做功能
    //==============================================================================
    
    /**
     * 撤销上一个操作
     * @return 成功返回true
     */
    bool undo();
    
    /**
     * 重做上一个撤销的操作
     * @return 成功返回true
     */
    bool redo();
    
    /**
     * 清除撤销历史
     */
    void clearUndoHistory();
    
    /**
     * 检查是否可以撤销
     * @return 可以撤销返回true
     */
    bool canUndo() const { return !undoStack.empty(); }
    
    /**
     * 检查是否可以重做
     * @return 可以重做返回true
     */
    bool canRedo() const { return !redoStack.empty(); }
    
    //==============================================================================
    // 批量操作
    //==============================================================================
    
    /**
     * 开始批量操作
     * @param operationName 操作名称
     */
    void beginBatchOperation(const std::string& operationName);
    
    /**
     * 结束批量操作
     */
    void endBatchOperation();
    
    /**
     * 取消批量操作
     */
    void cancelBatchOperation();
    
    /**
     * 检查是否在批量操作中
     * @return 在批量操作中返回true
     */
    bool isBatchOperationActive() const { return batchOperationActive; }
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置图变化回调
     */
    void setGraphChangeCallback(GraphChangeCallback callback);
    
    /**
     * 设置验证回调
     */
    void setValidationCallback(ValidationCallback callback);
    
    //==============================================================================
    // 查询接口
    //==============================================================================
    
    /**
     * 查找连接到指定节点的所有节点
     * @param nodeID 节点ID
     * @param incoming 是否查找输入连接
     * @return 连接的节点ID列表
     */
    std::vector<NodeID> getConnectedNodes(NodeID nodeID, bool incoming = true);
    
    /**
     * 获取节点的处理顺序
     * @return 按处理顺序排列的节点ID列表
     */
    std::vector<NodeID> getProcessingOrder();
    
    /**
     * 查找指定类型的节点
     * @param nodeType 节点类型
     * @return 匹配的节点ID列表
     */
    std::vector<NodeID> findNodesByType(NodeType nodeType);

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================
    
    GraphAudioProcessor& graphProcessor;
    
    // 撤销/重做栈
    std::vector<GraphOperation> undoStack;
    std::vector<GraphOperation> redoStack;
    static constexpr size_t MAX_UNDO_LEVELS = 50;
    
    // 批量操作状态
    bool batchOperationActive = false;
    std::vector<GraphOperation> currentBatchOperations;
    std::string currentBatchName;
    
    // 快照管理
    std::unordered_map<std::string, juce::MemoryBlock> snapshots;
    std::unordered_map<std::string, std::string> snapshotNames;
    
    // 回调函数
    GraphChangeCallback changeCallback;
    ValidationCallback validationCallback;
    
    // 内部状态
    mutable std::mutex operationMutex;
    
    //==============================================================================
    // 内部方法
    //==============================================================================
    
    void recordOperation(const GraphOperation& operation);
    void executeOperation(const GraphOperation& operation, bool isUndo = false);
    void notifyGraphChange(const GraphOperation& operation);
    void notifyValidationResult(const ValidationResult& result);
    
    // 图分析辅助方法
    void depthFirstSearch(NodeID nodeID, std::unordered_set<NodeID>& visited, 
                         std::unordered_set<NodeID>& recursionStack, bool& hasLoop);
    int calculateNodeDepth(NodeID nodeID, std::unordered_map<NodeID, int>& depthCache);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphManager)
};

} // namespace WindsynthVST::AudioGraph
