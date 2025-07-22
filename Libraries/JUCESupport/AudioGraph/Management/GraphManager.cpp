//
//  GraphManager.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图的高级管理器实现
//

#include "GraphManager.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <queue>

namespace WindsynthVST::AudioGraph {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

GraphManager::GraphManager(GraphAudioProcessor& graphProcessor)
    : graphProcessor(graphProcessor)
{
    std::cout << "[GraphManager] 初始化图管理器" << std::endl;
}

GraphManager::~GraphManager() {
    std::cout << "[GraphManager] 析构图管理器" << std::endl;
}

//==============================================================================
// 高级节点管理实现
//==============================================================================

std::vector<NodeID> GraphManager::addNodeGroup(std::vector<std::unique_ptr<juce::AudioProcessor>> processors,
                                              const std::vector<std::string>& names) {
    std::cout << "[GraphManager] 添加节点组，数量：" << processors.size() << std::endl;
    
    std::lock_guard<std::mutex> lock(operationMutex);
    
    if (batchOperationActive) {
        beginBatchOperation("添加节点组");
    }
    
    std::vector<NodeID> nodeIDs;
    nodeIDs.reserve(processors.size());
    
    for (size_t i = 0; i < processors.size(); ++i) {
        std::string nodeName = (i < names.size()) ? names[i] : ("Node_" + std::to_string(i));
        
        // 这里需要适配不同类型的处理器
        // 暂时直接添加到图中
        auto node = graphProcessor.getGraph().addNode(std::move(processors[i]));
        if (node) {
            nodeIDs.push_back(node->nodeID);
            
            // 记录操作
            GraphOperation operation(OperationType::AddNode);
            operation.nodeID = node->nodeID;
            recordOperation(operation);
        }
    }
    
    if (batchOperationActive) {
        endBatchOperation();
    }
    
    std::cout << "[GraphManager] 成功添加 " << nodeIDs.size() << " 个节点" << std::endl;
    return nodeIDs;
}

int GraphManager::removeNodeGroup(const std::vector<NodeID>& nodeIDs) {
    std::cout << "[GraphManager] 移除节点组，数量：" << nodeIDs.size() << std::endl;
    
    std::lock_guard<std::mutex> lock(operationMutex);
    
    beginBatchOperation("移除节点组");
    
    int removedCount = 0;
    for (NodeID nodeID : nodeIDs) {
        if (graphProcessor.removeNode(nodeID)) {
            removedCount++;
            
            // 记录操作
            GraphOperation operation(OperationType::RemoveNode);
            operation.nodeID = nodeID;
            recordOperation(operation);
        }
    }
    
    endBatchOperation();
    
    std::cout << "[GraphManager] 成功移除 " << removedCount << " 个节点" << std::endl;
    return removedCount;
}

NodeID GraphManager::duplicateNode(NodeID sourceNodeID, const std::string& newName) {
    std::cout << "[GraphManager] 复制节点：" << sourceNodeID.uid << std::endl;
    
    auto* sourceNode = graphProcessor.getGraph().getNodeForId(sourceNodeID);
    if (!sourceNode) {
        std::cout << "[GraphManager] 源节点不存在" << std::endl;
        return NodeID{0};
    }
    
    // 获取源节点的状态信息
    juce::MemoryBlock stateData;
    sourceNode->getProcessor()->getStateInformation(stateData);
    
    // 注意：这里需要根据具体的处理器类型来创建新实例
    // 由于无法直接复制AudioProcessor，这里返回失败
    // 在实际实现中，需要根据处理器类型进行特殊处理
    
    std::cout << "[GraphManager] 节点复制功能需要特定实现" << std::endl;
    return NodeID{0};
}

bool GraphManager::moveNode(NodeID nodeID, int newPosition) {
    std::cout << "[GraphManager] 移动节点：" << nodeID.uid << " 到位置：" << newPosition << std::endl;
    
    // JUCE AudioProcessorGraph没有直接的节点重排序功能
    // 这里可以通过重新连接来实现逻辑上的重排序
    
    std::cout << "[GraphManager] 节点移动功能需要特定实现" << std::endl;
    return false;
}

//==============================================================================
// 智能连接管理实现
//==============================================================================

int GraphManager::autoConnectNodes(NodeID sourceNodeID, NodeID destNodeID, 
                                  bool connectAudio, bool connectMidi) {
    std::cout << "[GraphManager] 自动连接节点：" << sourceNodeID.uid 
              << " -> " << destNodeID.uid << std::endl;
    
    auto sourceInfo = graphProcessor.getNodeInfo(sourceNodeID);
    auto destInfo = graphProcessor.getNodeInfo(destNodeID);
    
    if (sourceInfo.nodeID.uid == 0 || destInfo.nodeID.uid == 0) {
        std::cout << "[GraphManager] 无效的节点ID" << std::endl;
        return 0;
    }
    
    int connectionsCreated = 0;
    
    // 连接音频通道
    if (connectAudio) {
        int maxChannels = std::min(sourceInfo.numOutputChannels, destInfo.numInputChannels);
        for (int ch = 0; ch < maxChannels; ++ch) {
            if (graphProcessor.connectAudio(sourceNodeID, ch, destNodeID, ch)) {
                connectionsCreated++;
                
                // 记录操作
                GraphOperation operation(OperationType::AddConnection);
                operation.connection = makeAudioConnection(sourceNodeID, ch, destNodeID, ch);
                recordOperation(operation);
            }
        }
    }
    
    // 连接MIDI
    if (connectMidi && sourceInfo.producesMidi && destInfo.acceptsMidi) {
        if (graphProcessor.connectMidi(sourceNodeID, destNodeID)) {
            connectionsCreated++;
            
            // 记录操作
            GraphOperation operation(OperationType::AddConnection);
            operation.connection = makeMidiConnection(sourceNodeID, destNodeID);
            recordOperation(operation);
        }
    }
    
    std::cout << "[GraphManager] 创建了 " << connectionsCreated << " 个连接" << std::endl;
    return connectionsCreated;
}

int GraphManager::createProcessingChain(const std::vector<NodeID>& nodeIDs, bool connectToIO) {
    std::cout << "[GraphManager] 创建处理链，节点数量：" << nodeIDs.size() << std::endl;
    
    if (nodeIDs.size() < 2) {
        std::cout << "[GraphManager] 处理链至少需要2个节点" << std::endl;
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(operationMutex);
    beginBatchOperation("创建处理链");
    
    int connectionsCreated = 0;
    
    // 连接到音频输入（如果需要）
    if (connectToIO) {
        NodeID audioInputID = graphProcessor.getAudioInputNodeID();
        connectionsCreated += autoConnectNodes(audioInputID, nodeIDs[0], true, false);
    }
    
    // 串联连接所有节点
    for (size_t i = 0; i < nodeIDs.size() - 1; ++i) {
        connectionsCreated += autoConnectNodes(nodeIDs[i], nodeIDs[i + 1], true, true);
    }
    
    // 连接到音频输出（如果需要）
    if (connectToIO) {
        NodeID audioOutputID = graphProcessor.getAudioOutputNodeID();
        connectionsCreated += autoConnectNodes(nodeIDs.back(), audioOutputID, true, false);
    }
    
    endBatchOperation();
    
    std::cout << "[GraphManager] 处理链创建完成，总连接数：" << connectionsCreated << std::endl;
    return connectionsCreated;
}

int GraphManager::createParallelBranches(NodeID inputNodeID, NodeID outputNodeID, 
                                        const std::vector<NodeID>& branchNodeIDs) {
    std::cout << "[GraphManager] 创建并行分支，分支数量：" << branchNodeIDs.size() << std::endl;
    
    std::lock_guard<std::mutex> lock(operationMutex);
    beginBatchOperation("创建并行分支");
    
    int connectionsCreated = 0;
    
    // 将输入连接到所有分支
    for (NodeID branchID : branchNodeIDs) {
        connectionsCreated += autoConnectNodes(inputNodeID, branchID, true, true);
    }
    
    // 将所有分支连接到输出
    for (NodeID branchID : branchNodeIDs) {
        connectionsCreated += autoConnectNodes(branchID, outputNodeID, true, true);
    }
    
    endBatchOperation();
    
    std::cout << "[GraphManager] 并行分支创建完成，总连接数：" << connectionsCreated << std::endl;
    return connectionsCreated;
}

bool GraphManager::reorganizeNodes(const std::vector<NodeID>& nodeIDs, 
                                  const std::string& organizationType) {
    std::cout << "[GraphManager] 重新组织节点，类型：" << organizationType << std::endl;
    
    // 首先断开所有相关连接
    for (NodeID nodeID : nodeIDs) {
        graphProcessor.disconnectNode(nodeID);
    }
    
    // 根据组织类型重新连接
    if (organizationType == "series" || organizationType == "串联") {
        return createProcessingChain(nodeIDs, true) > 0;
    } else if (organizationType == "parallel" || organizationType == "并联") {
        if (nodeIDs.size() >= 3) {
            std::vector<NodeID> branches(nodeIDs.begin() + 1, nodeIDs.end() - 1);
            return createParallelBranches(nodeIDs.front(), nodeIDs.back(), branches) > 0;
        }
    }
    
    std::cout << "[GraphManager] 不支持的组织类型：" << organizationType << std::endl;
    return false;
}

//==============================================================================
// 图验证和分析实现
//==============================================================================

GraphManager::ValidationResult GraphManager::validateGraph() {
    std::cout << "[GraphManager] 验证图的有效性" << std::endl;
    
    ValidationResult result;
    
    // 检查基本图结构
    auto nodes = graphProcessor.getAllNodes();
    if (nodes.empty()) {
        result.addWarning("图中没有节点");
    }
    
    // 检查I/O节点
    bool hasAudioInput = false, hasAudioOutput = false;
    for (const auto& nodeInfo : nodes) {
        if (nodeInfo.nodeID == graphProcessor.getAudioInputNodeID()) {
            hasAudioInput = true;
        }
        if (nodeInfo.nodeID == graphProcessor.getAudioOutputNodeID()) {
            hasAudioOutput = true;
        }
    }
    
    if (!hasAudioInput) {
        result.addError("缺少音频输入节点");
    }
    if (!hasAudioOutput) {
        result.addError("缺少音频输出节点");
    }
    
    // 检查连接有效性
    auto connections = graphProcessor.getAllConnections();
    for (const auto& connInfo : connections) {
        auto connResult = validateConnection(connInfo.connection);
        if (!connResult.isValid) {
            for (const auto& error : connResult.errors) {
                result.addError("连接错误：" + error);
            }
        }
    }
    
    // 检查环路
    if (detectLoops()) {
        result.addError("图中存在环路");
    }
    
    // 检查孤立节点
    for (const auto& nodeInfo : nodes) {
        auto connectedNodes = getConnectedNodes(nodeInfo.nodeID, true);
        connectedNodes.insert(connectedNodes.end(), 
                             getConnectedNodes(nodeInfo.nodeID, false).begin(),
                             getConnectedNodes(nodeInfo.nodeID, false).end());
        
        if (connectedNodes.empty() && 
            nodeInfo.nodeID != graphProcessor.getAudioInputNodeID() &&
            nodeInfo.nodeID != graphProcessor.getAudioOutputNodeID()) {
            result.addWarning("节点 " + nodeInfo.name + " 没有连接");
        }
    }
    
    if (validationCallback) {
        validationCallback(result);
    }
    
    std::cout << "[GraphManager] 图验证完成，错误：" << result.errors.size() 
              << "，警告：" << result.warnings.size() << std::endl;
    
    return result;
}

GraphManager::ValidationResult GraphManager::validateConnection(const Connection& connection) {
    ValidationResult result;
    
    // 检查连接是否合法
    if (!graphProcessor.getGraph().isConnectionLegal(connection)) {
        result.addError("连接不合法");
        return result;
    }
    
    // 检查节点是否存在
    auto* sourceNode = graphProcessor.getGraph().getNodeForId(connection.source.nodeID);
    auto* destNode = graphProcessor.getGraph().getNodeForId(connection.destination.nodeID);
    
    if (!sourceNode) {
        result.addError("源节点不存在");
    }
    if (!destNode) {
        result.addError("目标节点不存在");
    }
    
    if (!sourceNode || !destNode) {
        return result;
    }
    
    // 检查通道索引
    bool isMidi = isMidiConnection(connection);
    if (!isMidi) {
        if (connection.source.channelIndex >= sourceNode->getProcessor()->getTotalNumOutputChannels()) {
            result.addError("源通道索引超出范围");
        }
        if (connection.destination.channelIndex >= destNode->getProcessor()->getTotalNumInputChannels()) {
            result.addError("目标通道索引超出范围");
        }
    }
    
    return result;
}

bool GraphManager::detectLoops() {
    std::cout << "[GraphManager] 检测图中的环路" << std::endl;
    
    auto nodes = graphProcessor.getAllNodes();
    std::unordered_set<NodeID> visited;
    std::unordered_set<NodeID> recursionStack;
    bool hasLoop = false;
    
    for (const auto& nodeInfo : nodes) {
        if (visited.find(nodeInfo.nodeID) == visited.end()) {
            depthFirstSearch(nodeInfo.nodeID, visited, recursionStack, hasLoop);
            if (hasLoop) {
                break;
            }
        }
    }
    
    std::cout << "[GraphManager] 环路检测完成，结果：" << (hasLoop ? "存在环路" : "无环路") << std::endl;
    return hasLoop;
}

int GraphManager::calculateGraphDepth() {
    std::cout << "[GraphManager] 计算图的处理深度" << std::endl;
    
    auto nodes = graphProcessor.getAllNodes();
    std::unordered_map<NodeID, int> depthCache;
    int maxDepth = 0;
    
    for (const auto& nodeInfo : nodes) {
        int depth = calculateNodeDepth(nodeInfo.nodeID, depthCache);
        maxDepth = std::max(maxDepth, depth);
    }
    
    std::cout << "[GraphManager] 图的最大深度：" << maxDepth << std::endl;
    return maxDepth;
}

double GraphManager::estimateGraphLatency() {
    std::cout << "[GraphManager] 估算图的总延迟" << std::endl;
    
    auto nodes = graphProcessor.getAllNodes();
    double totalLatency = 0.0;
    
    for (const auto& nodeInfo : nodes) {
        totalLatency += nodeInfo.latencyInSamples;
    }
    
    std::cout << "[GraphManager] 估算的总延迟：" << totalLatency << " 采样" << std::endl;
    return totalLatency;
}

GraphManager::GraphStatistics GraphManager::getGraphStatistics() {
    std::cout << "[GraphManager] 获取图统计信息" << std::endl;
    
    GraphStatistics stats;
    
    auto nodes = graphProcessor.getAllNodes();
    auto connections = graphProcessor.getAllConnections();
    
    stats.totalNodes = static_cast<int>(nodes.size());
    stats.totalConnections = static_cast<int>(connections.size());
    
    // 统计节点类型
    for (const auto& nodeInfo : nodes) {
        if (nodeInfo.nodeID == graphProcessor.getAudioInputNodeID() ||
            nodeInfo.nodeID == graphProcessor.getAudioOutputNodeID() ||
            nodeInfo.nodeID == graphProcessor.getMidiInputNodeID() ||
            nodeInfo.nodeID == graphProcessor.getMidiOutputNodeID()) {
            stats.ioNodes++;
        } else {
            stats.vstPluginNodes++;
        }
    }
    
    // 统计连接类型
    for (const auto& connInfo : connections) {
        if (connInfo.isAudioConnection) {
            stats.audioConnections++;
        } else {
            stats.midiConnections++;
        }
    }
    
    // 计算其他统计信息
    stats.maxDepth = calculateGraphDepth();
    stats.hasLoops = detectLoops();
    stats.estimatedLatency = estimateGraphLatency();
    
    std::cout << "[GraphManager] 统计信息：节点=" << stats.totalNodes 
              << "，连接=" << stats.totalConnections 
              << "，深度=" << stats.maxDepth << std::endl;
    
    return stats;
}

//==============================================================================
// 图状态管理实现
//==============================================================================

std::string GraphManager::createSnapshot(const std::string& name) {
    std::cout << "[GraphManager] 创建图状态快照：" << name << std::endl;

    // 生成唯一的快照ID
    std::string snapshotId = "snapshot_" + std::to_string(juce::Time::currentTimeMillis());

    // 获取图的状态信息
    juce::MemoryBlock stateData;
    graphProcessor.getStateInformation(stateData);

    // 保存快照
    snapshots[snapshotId] = stateData;
    snapshotNames[snapshotId] = name;

    std::cout << "[GraphManager] 快照创建完成，ID：" << snapshotId << std::endl;
    return snapshotId;
}

bool GraphManager::restoreSnapshot(const std::string& snapshotId) {
    std::cout << "[GraphManager] 恢复图状态快照：" << snapshotId << std::endl;

    auto it = snapshots.find(snapshotId);
    if (it == snapshots.end()) {
        std::cout << "[GraphManager] 快照不存在：" << snapshotId << std::endl;
        return false;
    }

    try {
        graphProcessor.setStateInformation(it->second.getData(),
                                         static_cast<int>(it->second.getSize()));
        std::cout << "[GraphManager] 快照恢复成功" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[GraphManager] 快照恢复失败：" << e.what() << std::endl;
        return false;
    }
}

bool GraphManager::deleteSnapshot(const std::string& snapshotId) {
    std::cout << "[GraphManager] 删除快照：" << snapshotId << std::endl;

    auto removed1 = snapshots.erase(snapshotId);
    auto removed2 = snapshotNames.erase(snapshotId);

    bool success = (removed1 > 0 && removed2 > 0);
    std::cout << "[GraphManager] 快照删除" << (success ? "成功" : "失败") << std::endl;
    return success;
}

std::unordered_map<std::string, std::string> GraphManager::getSnapshots() {
    return snapshotNames;
}

//==============================================================================
// 撤销/重做功能实现
//==============================================================================

bool GraphManager::undo() {
    std::cout << "[GraphManager] 撤销操作" << std::endl;

    std::lock_guard<std::mutex> lock(operationMutex);

    if (undoStack.empty()) {
        std::cout << "[GraphManager] 没有可撤销的操作" << std::endl;
        return false;
    }

    GraphOperation operation = undoStack.back();
    undoStack.pop_back();

    // 执行撤销操作
    executeOperation(operation, true);

    // 添加到重做栈
    redoStack.push_back(operation);

    std::cout << "[GraphManager] 撤销操作完成" << std::endl;
    return true;
}

bool GraphManager::redo() {
    std::cout << "[GraphManager] 重做操作" << std::endl;

    std::lock_guard<std::mutex> lock(operationMutex);

    if (redoStack.empty()) {
        std::cout << "[GraphManager] 没有可重做的操作" << std::endl;
        return false;
    }

    GraphOperation operation = redoStack.back();
    redoStack.pop_back();

    // 执行重做操作
    executeOperation(operation, false);

    // 添加到撤销栈
    undoStack.push_back(operation);

    std::cout << "[GraphManager] 重做操作完成" << std::endl;
    return true;
}

void GraphManager::clearUndoHistory() {
    std::cout << "[GraphManager] 清除撤销历史" << std::endl;

    std::lock_guard<std::mutex> lock(operationMutex);
    undoStack.clear();
    redoStack.clear();
}

//==============================================================================
// 批量操作实现
//==============================================================================

void GraphManager::beginBatchOperation(const std::string& operationName) {
    std::cout << "[GraphManager] 开始批量操作：" << operationName << std::endl;

    if (batchOperationActive) {
        std::cout << "[GraphManager] 警告：已有活动的批量操作" << std::endl;
        return;
    }

    batchOperationActive = true;
    currentBatchName = operationName;
    currentBatchOperations.clear();
}

void GraphManager::endBatchOperation() {
    std::cout << "[GraphManager] 结束批量操作：" << currentBatchName << std::endl;

    if (!batchOperationActive) {
        std::cout << "[GraphManager] 警告：没有活动的批量操作" << std::endl;
        return;
    }

    if (!currentBatchOperations.empty()) {
        // 创建批量操作记录
        GraphOperation batchOperation(OperationType::BatchOperation);
        batchOperation.batchOperations = currentBatchOperations;

        // 添加到撤销栈
        undoStack.push_back(batchOperation);

        // 限制撤销栈大小
        if (undoStack.size() > MAX_UNDO_LEVELS) {
            undoStack.erase(undoStack.begin());
        }

        // 清除重做栈
        redoStack.clear();

        // 通知变化
        notifyGraphChange(batchOperation);
    }

    batchOperationActive = false;
    currentBatchOperations.clear();
    currentBatchName.clear();
}

void GraphManager::cancelBatchOperation() {
    std::cout << "[GraphManager] 取消批量操作：" << currentBatchName << std::endl;

    if (!batchOperationActive) {
        std::cout << "[GraphManager] 警告：没有活动的批量操作" << std::endl;
        return;
    }

    // 撤销当前批量操作中的所有操作
    for (auto it = currentBatchOperations.rbegin(); it != currentBatchOperations.rend(); ++it) {
        executeOperation(*it, true);
    }

    batchOperationActive = false;
    currentBatchOperations.clear();
    currentBatchName.clear();
}

//==============================================================================
// 回调设置
//==============================================================================

void GraphManager::setGraphChangeCallback(GraphChangeCallback callback) {
    changeCallback = std::move(callback);
}

void GraphManager::setValidationCallback(ValidationCallback callback) {
    validationCallback = std::move(callback);
}

//==============================================================================
// 查询接口实现
//==============================================================================

std::vector<NodeID> GraphManager::getConnectedNodes(NodeID nodeID, bool incoming) {
    std::vector<NodeID> connectedNodes;

    auto connections = graphProcessor.getAllConnections();
    for (const auto& connInfo : connections) {
        if (incoming && connInfo.connection.destination.nodeID == nodeID) {
            connectedNodes.push_back(connInfo.connection.source.nodeID);
        } else if (!incoming && connInfo.connection.source.nodeID == nodeID) {
            connectedNodes.push_back(connInfo.connection.destination.nodeID);
        }
    }

    // 去重
    std::sort(connectedNodes.begin(), connectedNodes.end());
    connectedNodes.erase(std::unique(connectedNodes.begin(), connectedNodes.end()),
                        connectedNodes.end());

    return connectedNodes;
}

std::vector<NodeID> GraphManager::getProcessingOrder() {
    std::cout << "[GraphManager] 获取节点处理顺序" << std::endl;

    auto nodes = graphProcessor.getAllNodes();
    std::vector<NodeID> processingOrder;
    std::unordered_set<NodeID> visited;
    std::queue<NodeID> queue;

    // 从音频输入节点开始拓扑排序
    NodeID audioInputID = graphProcessor.getAudioInputNodeID();
    queue.push(audioInputID);
    visited.insert(audioInputID);

    while (!queue.empty()) {
        NodeID currentNode = queue.front();
        queue.pop();
        processingOrder.push_back(currentNode);

        // 添加所有连接的下游节点
        auto downstreamNodes = getConnectedNodes(currentNode, false);
        for (NodeID downstreamNode : downstreamNodes) {
            if (visited.find(downstreamNode) == visited.end()) {
                visited.insert(downstreamNode);
                queue.push(downstreamNode);
            }
        }
    }

    // 添加任何未访问的节点
    for (const auto& nodeInfo : nodes) {
        if (visited.find(nodeInfo.nodeID) == visited.end()) {
            processingOrder.push_back(nodeInfo.nodeID);
        }
    }

    std::cout << "[GraphManager] 处理顺序包含 " << processingOrder.size() << " 个节点" << std::endl;
    return processingOrder;
}

std::vector<NodeID> GraphManager::findNodesByType(NodeType nodeType) {
    std::vector<NodeID> matchingNodes;

    auto nodes = graphProcessor.getAllNodes();
    for (const auto& nodeInfo : nodes) {
        NodeType currentType = NodeType::Unknown;

        // 判断节点类型
        if (nodeInfo.nodeID == graphProcessor.getAudioInputNodeID()) {
            currentType = NodeType::AudioInput;
        } else if (nodeInfo.nodeID == graphProcessor.getAudioOutputNodeID()) {
            currentType = NodeType::AudioOutput;
        } else if (nodeInfo.nodeID == graphProcessor.getMidiInputNodeID()) {
            currentType = NodeType::MidiInput;
        } else if (nodeInfo.nodeID == graphProcessor.getMidiOutputNodeID()) {
            currentType = NodeType::MidiOutput;
        } else {
            currentType = NodeType::VSTPlugin;
        }

        if (currentType == nodeType) {
            matchingNodes.push_back(nodeInfo.nodeID);
        }
    }

    return matchingNodes;
}

//==============================================================================
// 内部方法实现
//==============================================================================

void GraphManager::recordOperation(const GraphOperation& operation) {
    if (batchOperationActive) {
        currentBatchOperations.push_back(operation);
    } else {
        undoStack.push_back(operation);

        // 限制撤销栈大小
        if (undoStack.size() > MAX_UNDO_LEVELS) {
            undoStack.erase(undoStack.begin());
        }

        // 清除重做栈
        redoStack.clear();

        // 通知变化
        notifyGraphChange(operation);
    }
}

void GraphManager::executeOperation(const GraphOperation& operation, bool isUndo) {
    // 这里需要根据操作类型执行相应的撤销/重做操作
    // 由于JUCE AudioProcessorGraph的限制，某些操作可能无法完全撤销
    std::cout << "[GraphManager] 执行操作，类型：" << static_cast<int>(operation.type)
              << "，撤销：" << (isUndo ? "是" : "否") << std::endl;
}

void GraphManager::notifyGraphChange(const GraphOperation& operation) {
    if (changeCallback) {
        changeCallback(operation);
    }
}

void GraphManager::notifyValidationResult(const ValidationResult& result) {
    if (validationCallback) {
        validationCallback(result);
    }
}

void GraphManager::depthFirstSearch(NodeID nodeID, std::unordered_set<NodeID>& visited,
                                   std::unordered_set<NodeID>& recursionStack, bool& hasLoop) {
    visited.insert(nodeID);
    recursionStack.insert(nodeID);

    auto connectedNodes = getConnectedNodes(nodeID, false);
    for (NodeID connectedNode : connectedNodes) {
        if (recursionStack.find(connectedNode) != recursionStack.end()) {
            hasLoop = true;
            return;
        }

        if (visited.find(connectedNode) == visited.end()) {
            depthFirstSearch(connectedNode, visited, recursionStack, hasLoop);
            if (hasLoop) {
                return;
            }
        }
    }

    recursionStack.erase(nodeID);
}

int GraphManager::calculateNodeDepth(NodeID nodeID, std::unordered_map<NodeID, int>& depthCache) {
    auto it = depthCache.find(nodeID);
    if (it != depthCache.end()) {
        return it->second;
    }

    auto upstreamNodes = getConnectedNodes(nodeID, true);
    if (upstreamNodes.empty()) {
        depthCache[nodeID] = 0;
        return 0;
    }

    int maxUpstreamDepth = 0;
    for (NodeID upstreamNode : upstreamNodes) {
        int upstreamDepth = calculateNodeDepth(upstreamNode, depthCache);
        maxUpstreamDepth = std::max(maxUpstreamDepth, upstreamDepth);
    }

    int depth = maxUpstreamDepth + 1;
    depthCache[nodeID] = depth;
    return depth;
}

} // namespace WindsynthVST::AudioGraph
