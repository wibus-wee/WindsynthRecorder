//
//  AudioGraphService.swift
//  WindsynthRecorder
//
//  新架构音频图服务 - 基于C++核心的现代化音频处理
//  阶段二：真实的C++引擎集成
//

import Foundation
import Combine

/// 插件加载回调上下文
private class PluginLoadCallbackContext {
    let service: AudioGraphService
    let identifier: String
    let completion: (Bool, String?) -> Void

    init(service: AudioGraphService, identifier: String, completion: @escaping (Bool, String?) -> Void) {
        self.service = service
        self.identifier = identifier
        self.completion = completion
    }
}

/// 插件描述信息（Swift版本）
struct PluginDescription {
    let identifier: String
    let name: String
    let manufacturer: String
    let category: String
    let format: String
    let filePath: String
    let isValid: Bool

    /// 从C结构转换
    init(from cStruct: SimplePluginInfo_C) {
        self.identifier = withUnsafeBytes(of: cStruct.identifier) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.name = withUnsafeBytes(of: cStruct.name) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.manufacturer = withUnsafeBytes(of: cStruct.manufacturer) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.category = withUnsafeBytes(of: cStruct.category) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.format = withUnsafeBytes(of: cStruct.format) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.filePath = withUnsafeBytes(of: cStruct.filePath) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.isValid = cStruct.isValid
    }
}

/// 节点信息（Swift版本）
struct NodeInfo {
    let nodeID: UInt32
    let name: String
    let pluginName: String
    let isEnabled: Bool
    let isBypassed: Bool
    let numInputChannels: Int
    let numOutputChannels: Int

    /// 从C结构转换
    init(from cStruct: SimpleNodeInfo_C) {
        self.nodeID = cStruct.nodeID
        self.name = withUnsafeBytes(of: cStruct.name) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.pluginName = withUnsafeBytes(of: cStruct.pluginName) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.isEnabled = cStruct.isEnabled
        self.isBypassed = cStruct.isBypassed
        self.numInputChannels = Int(cStruct.numInputChannels)
        self.numOutputChannels = Int(cStruct.numOutputChannels)
    }
}

/// 音频统计信息（Swift版本）
struct AudioStatistics {
    let cpuUsage: Double
    let memoryUsage: Double
    let inputLevel: Double
    let outputLevel: Double
    let latency: Double
    let dropouts: Int
    let activeNodes: Int
    let totalConnections: Int

    /// 从C结构转换
    init(from cStruct: EngineStatistics_C) {
        self.cpuUsage = cStruct.cpuUsage
        self.memoryUsage = cStruct.memoryUsage
        self.inputLevel = cStruct.inputLevel
        self.outputLevel = cStruct.outputLevel
        self.latency = cStruct.latency
        self.dropouts = Int(cStruct.dropouts)
        self.activeNodes = Int(cStruct.activeNodes)
        self.totalConnections = Int(cStruct.totalConnections)
    }
}

/// 参数信息（Swift版本）
struct ParameterInfo {
    let name: String
    let label: String
    let minValue: Float
    let maxValue: Float
    let defaultValue: Float
    let currentValue: Float
    let isDiscrete: Bool
    let numSteps: Int
    let units: String

    /// 从C结构转换
    init(from cStruct: ParameterInfo_C) {
        self.name = withUnsafeBytes(of: cStruct.name) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.label = withUnsafeBytes(of: cStruct.label) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.units = withUnsafeBytes(of: cStruct.units) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.minValue = cStruct.minValue
        self.maxValue = cStruct.maxValue
        self.defaultValue = cStruct.defaultValue
        self.currentValue = cStruct.currentValue
        self.isDiscrete = cStruct.isDiscrete
        self.numSteps = Int(cStruct.numSteps)
    }
}

/// 引擎配置
struct EngineConfiguration {
    let sampleRate: Double
    let bufferSize: Int
    let numInputChannels: Int
    let numOutputChannels: Int
    let enableRealtimeProcessing: Bool
    let audioDeviceName: String

    /// 转换为C结构
    func toCStruct() -> EngineConfig_C {
        var config = EngineConfig_C()
        config.sampleRate = sampleRate
        config.bufferSize = Int32(bufferSize)
        config.numInputChannels = Int32(numInputChannels)
        config.numOutputChannels = Int32(numOutputChannels)
        config.enableRealtimeProcessing = enableRealtimeProcessing

        // 安全地复制字符串
        if let deviceNameCString = audioDeviceName.cString(using: .utf8) {
            let maxLength = min(deviceNameCString.count, 255)
            withUnsafeMutableBytes(of: &config.audioDeviceName) { buffer in
                let uint8Buffer = buffer.bindMemory(to: UInt8.self)
                for i in 0..<maxLength {
                    uint8Buffer[i] = UInt8(bitPattern: deviceNameCString[i])
                }
                if maxLength < 256 {
                    uint8Buffer[maxLength] = 0 // null terminator
                }
            }
        }

        return config
    }
}

/// 现代化音频图服务 - 真实的C++引擎集成
class AudioGraphService: ObservableObject {

    // MARK: - Published Properties

    @Published var loadedPlugins: [NodeInfo] = []
    @Published var availablePlugins: [PluginDescription] = []
    @Published var isRunning: Bool = false
    @Published var isScanning: Bool = false
    @Published var scanProgress: Float = 0.0
    @Published var currentConfiguration: EngineConfiguration
    @Published var errorMessage: String?
    @Published var statistics: AudioStatistics?

    // MARK: - Private Properties

    private let logger = AudioProcessingLogger.shared
    private var cancellables = Set<AnyCancellable>()

    /// 音频引擎句柄（真实的C++引擎）
    private var engineHandle: WindsynthEngineHandle?

    /// 统计信息更新定时器
    private var statisticsTimer: Timer?
    
    // MARK: - Singleton

    static let shared = AudioGraphService()

    private init() {
        // 初始化默认配置
        self.currentConfiguration = EngineConfiguration(
            sampleRate: 44100.0,
            bufferSize: 512,
            numInputChannels: 2,
            numOutputChannels: 2,
            enableRealtimeProcessing: true,
            audioDeviceName: ""
        )

        setupEngine()
        setupCallbacks()

        // 自动启动引擎
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            if self.start() {
                self.logger.info("AudioGraphService初始化完成", details: "引擎已自动启动")
            } else {
                self.logger.error("AudioGraphService初始化失败", details: "引擎启动失败")
            }
        }
    }

    deinit {
        cleanup()
    }
    
    // MARK: - Setup

    private func setupEngine() {
        // 创建真实的C++引擎
        engineHandle = Engine_Create()

        guard let handle = engineHandle else {
            logger.error("引擎创建失败", details: "无法创建C++音频引擎")
            errorMessage = "Failed to create audio engine"
            return
        }

        // 初始化引擎
        var config = currentConfiguration.toCStruct()
        let success = Engine_Initialize(handle, &config)

        if success {
            logger.info("音频引擎创建成功", details: "C++引擎已初始化")
        } else {
            logger.error("引擎初始化失败", details: "配置参数可能有误")
            errorMessage = "Failed to initialize audio engine"
        }
    }

    private func setupCallbacks() {
        guard let handle = engineHandle else { return }

        // 设置状态变化回调
        Engine_SetStateCallback(handle, { state, message, userData in
            guard let userData = userData else { return }
            let service = Unmanaged<AudioGraphService>.fromOpaque(userData).takeUnretainedValue()

            DispatchQueue.main.async {
                let messageStr = message != nil ? String(cString: message!) : ""
                service.handleStateChange(state: state, message: messageStr)
            }
        }, Unmanaged.passUnretained(self).toOpaque())

        // 设置错误回调
        Engine_SetErrorCallback(handle, { error, userData in
            guard let userData = userData else { return }
            let service = Unmanaged<AudioGraphService>.fromOpaque(userData).takeUnretainedValue()

            DispatchQueue.main.async {
                let errorStr = error != nil ? String(cString: error!) : "Unknown error"
                service.handleError(error: errorStr)
            }
        }, Unmanaged.passUnretained(self).toOpaque())
    }

    private func cleanup() {
        // 停止统计信息定时器
        statisticsTimer?.invalidate()
        statisticsTimer = nil

        // 销毁引擎
        if let handle = engineHandle {
            Engine_Shutdown(handle)
            Engine_Destroy(handle)
            engineHandle = nil
            logger.info("音频引擎已销毁", details: "资源清理完成")
        }
    }
    
    // MARK: - Public Methods

    /// 启动音频处理
    func start() -> Bool {
        guard let handle = engineHandle else {
            errorMessage = "Audio engine not initialized"
            return false
        }

        let success = Engine_Start(handle)

        if success {
            isRunning = true
            startStatisticsTimer()
            logger.info("音频处理已启动", details: "采样率: \(currentConfiguration.sampleRate)Hz, 缓冲区: \(currentConfiguration.bufferSize)")
        } else {
            errorMessage = "Failed to start audio engine"
            logger.error("音频处理启动失败", details: "引擎状态异常")
        }

        return success
    }

    /// 停止音频处理
    func stop() {
        guard let handle = engineHandle else { return }

        Engine_Stop(handle)
        isRunning = false
        stopStatisticsTimer()
        logger.info("音频处理已停止", details: "引擎已安全停止")
    }
    
    /// 扫描可用插件
    func scanPlugins(searchPaths: [String] = []) -> Int {
        guard let handle = engineHandle else {
            errorMessage = "Audio engine not initialized"
            return 0
        }

        // 使用withCString确保内存安全
        var count: Int32 = 0

        // 为每个路径创建C字符串，并确保在调用期间保持有效
        let cStrings = searchPaths.map { $0.cString(using: .utf8) ?? [] }

        // 创建指针数组
        cStrings.withUnsafeBufferPointer { buffer in
            let pointers = buffer.map { $0.withUnsafeBufferPointer { UnsafePointer($0.baseAddress) } }
            let nullTerminatedPointers = pointers + [nil]

            count = Engine_ScanPlugins(handle, nullTerminatedPointers)
        }

        // 更新可用插件列表
        refreshAvailablePlugins()

        logger.info("插件扫描完成", details: "找到 \(count) 个插件")
        return Int(count)
    }

    /// 通过标识符加载插件（异步）
    func loadPlugin(identifier: String, displayName: String = "", completion: @escaping (Bool, String?) -> Void) {
        guard let handle = engineHandle else {
            completion(false, "Audio engine not initialized")
            return
        }

        logger.info("开始加载插件", details: "标识符: \(identifier)")

        // 创建回调上下文
        let callbackContext = PluginLoadCallbackContext(
            service: self,
            identifier: identifier,
            completion: completion
        )

        // 保存回调上下文（防止被释放）
        let contextPointer = Unmanaged.passRetained(callbackContext).toOpaque()

        // 调用异步加载，传递回调
        Engine_LoadPluginByIdentifier(
            handle,
            identifier,
            displayName.isEmpty ? nil : displayName,
            { nodeID, success, error, userData in
                // C回调函数
                guard let userData = userData else { return }

                let context = Unmanaged<PluginLoadCallbackContext>.fromOpaque(userData).takeRetainedValue()

                DispatchQueue.main.async {
                    if success {
                        // 刷新已加载插件列表
                        context.service.refreshLoadedPlugins()
                        context.completion(true, nil)
                        context.service.logger.info("插件加载成功", details: "节点ID: \(nodeID)")
                    } else {
                        let errorMsg = error != nil ? String(cString: error!) : "未知错误"
                        context.completion(false, errorMsg)
                        context.service.logger.error("插件加载失败", details: errorMsg)
                    }
                }
            },
            contextPointer
        )
    }

    /// 通过标识符加载插件（同步版本，用于向后兼容）
    @available(*, deprecated, message: "使用异步版本 loadPlugin(identifier:displayName:completion:)")
    func loadPlugin(identifier: String, displayName: String = "") -> Bool {
        var result = false
        let semaphore = DispatchSemaphore(value: 0)

        loadPlugin(identifier: identifier, displayName: displayName) { success, _ in
            result = success
            semaphore.signal()
        }

        // 等待最多3秒
        _ = semaphore.wait(timeout: .now() + 3.0)
        return result
    }

    /// 移除节点（异步）
    func removeNode(nodeID: UInt32, completion: @escaping (Bool) -> Void) {
        guard let handle = engineHandle else {
            completion(false)
            return
        }

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            let success = Engine_RemoveNode(handle, nodeID)

            DispatchQueue.main.async {
                if success {
                    self?.refreshLoadedPlugins()
                    self?.logger.info("节点移除成功", details: "节点ID: \(nodeID)")
                } else {
                    self?.logger.error("节点移除失败", details: "节点ID: \(nodeID)")
                }
                completion(success)
            }
        }
    }

    /// 移除节点（同步版本，用于向后兼容）
    @available(*, deprecated, message: "使用异步版本 removeNode(nodeID:completion:)")
    func removeNode(nodeID: UInt32) -> Bool {
        var result = false
        let semaphore = DispatchSemaphore(value: 0)

        removeNode(nodeID: nodeID) { success in
            result = success
            semaphore.signal()
        }

        // 等待最多2秒
        _ = semaphore.wait(timeout: .now() + 2.0)
        return result
    }
    
    /// 设置节点参数
    func setNodeParameter(nodeID: UInt32, parameterIndex: Int, value: Float) -> Bool {
        guard let handle = engineHandle else {
            errorMessage = "Audio engine not initialized"
            return false
        }

        let success = Engine_SetNodeParameter(handle, nodeID, Int32(parameterIndex), value)

        if success {
            logger.info("节点参数已设置", details: "节点ID: \(nodeID), 参数索引: \(parameterIndex), 值: \(value)")
        } else {
            logger.error("节点参数设置失败", details: "节点ID: \(nodeID), 参数索引: \(parameterIndex)")
        }

        return success
    }

    /// 获取节点参数
    func getNodeParameter(nodeID: UInt32, parameterIndex: Int) -> Float? {
        guard let handle = engineHandle else {
            return nil
        }

        let value = Engine_GetNodeParameter(handle, nodeID, Int32(parameterIndex))
        return value >= 0 ? value : nil
    }

    /// 获取节点参数数量
    func getNodeParameterCount(nodeID: UInt32) -> Int {
        guard let handle = engineHandle else {
            return 0
        }

        return Int(Engine_GetNodeParameterCount(handle, nodeID))
    }

    /// 获取节点参数信息
    func getNodeParameterInfo(nodeID: UInt32, parameterIndex: Int) -> ParameterInfo? {
        guard let handle = engineHandle else {
            return nil
        }

        var cInfo = ParameterInfo_C()
        let success = Engine_GetNodeParameterInfo(handle, nodeID, Int32(parameterIndex), &cInfo)

        if success {
            return ParameterInfo(from: cInfo)
        }

        return nil
    }

    /// 设置节点旁路状态
    func setNodeBypassed(nodeID: UInt32, bypassed: Bool) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_SetNodeBypassed(handle, nodeID, bypassed)

        if success {
            refreshLoadedPlugins() // 更新状态
            logger.info("节点旁路状态已更新", details: "节点ID: \(nodeID), 旁路: \(bypassed)")
        }

        return success
    }

    /// 设置节点启用状态（异步）
    func setNodeEnabled(nodeID: UInt32, enabled: Bool, completion: @escaping (Bool) -> Void) {
        guard let handle = engineHandle else {
            completion(false)
            return
        }

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            let success = Engine_SetNodeEnabled(handle, nodeID, enabled)

            DispatchQueue.main.async {
                if success {
                    self?.refreshLoadedPlugins() // 更新状态
                    self?.logger.info("节点启用状态已更新", details: "节点ID: \(nodeID), 启用: \(enabled)")
                }
                completion(success)
            }
        }
    }

    /// 设置节点启用状态（同步版本，用于向后兼容）
    @available(*, deprecated, message: "使用异步版本 setNodeEnabled(nodeID:enabled:completion:)")
    func setNodeEnabled(nodeID: UInt32, enabled: Bool) -> Bool {
        var result = false
        let semaphore = DispatchSemaphore(value: 0)

        setNodeEnabled(nodeID: nodeID, enabled: enabled) { success in
            result = success
            semaphore.signal()
        }

        // 等待最多1秒
        _ = semaphore.wait(timeout: .now() + 1.0)
        return result
    }

    // MARK: - 插件编辑器管理

    /// 检查节点是否有编辑器
    func nodeHasEditor(nodeID: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        return Engine_NodeHasEditor(handle, nodeID)
    }

    /// 显示节点编辑器
    func showNodeEditor(nodeID: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_ShowNodeEditor(handle, nodeID)

        if success {
            logger.info("节点编辑器已显示", details: "节点ID: \(nodeID)")
        } else {
            logger.error("显示节点编辑器失败", details: "节点ID: \(nodeID)")
        }

        return success
    }

    /// 隐藏节点编辑器
    func hideNodeEditor(nodeID: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_HideNodeEditor(handle, nodeID)

        if success {
            logger.info("节点编辑器已隐藏", details: "节点ID: \(nodeID)")
        }

        return success
    }

    /// 检查节点编辑器是否可见
    func isNodeEditorVisible(nodeID: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        return Engine_IsNodeEditorVisible(handle, nodeID)
    }

    // MARK: - 节点位置管理

    /// 移动节点在处理链中的位置
    func moveNode(nodeID: UInt32, newPosition: Int) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_MoveNode(handle, nodeID, Int32(newPosition))

        if success {
            refreshLoadedPlugins() // 更新插件列表顺序
            logger.info("节点已移动", details: "节点ID: \(nodeID), 新位置: \(newPosition)")
        } else {
            logger.error("移动节点失败", details: "节点ID: \(nodeID), 新位置: \(newPosition)")
        }

        return success
    }

    /// 交换两个节点的位置
    func swapNodes(nodeID1: UInt32, nodeID2: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_SwapNodes(handle, nodeID1, nodeID2)

        if success {
            refreshLoadedPlugins() // 更新插件列表顺序
            logger.info("节点已交换", details: "节点1: \(nodeID1), 节点2: \(nodeID2)")
        } else {
            logger.error("交换节点失败", details: "节点1: \(nodeID1), 节点2: \(nodeID2)")
        }

        return success
    }
    
    /// 加载音频文件
    func loadAudioFile(filePath: String) -> Bool {
        guard let handle = engineHandle else {
            errorMessage = "Audio engine not initialized"
            return false
        }

        let success = Engine_LoadAudioFile(handle, filePath)

        if success {
            logger.info("音频文件加载成功", details: "路径: \(filePath)")
        } else {
            logger.error("音频文件加载失败", details: "路径: \(filePath)")
            errorMessage = "Failed to load audio file: \(filePath)"
        }

        return success
    }

    /// 播放音频
    func play() -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        return Engine_Play(handle)
    }

    /// 暂停音频
    func pause() {
        guard let handle = engineHandle else {
            return
        }

        Engine_Pause(handle)
    }

    /// 停止音频播放
    func stopPlayback() {
        guard let handle = engineHandle else {
            return
        }

        Engine_StopPlayback(handle)
    }

    /// 跳转到指定时间
    func seekTo(timeInSeconds: Double) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        return Engine_SeekTo(handle, timeInSeconds)
    }

    /// 获取当前播放时间
    func getCurrentTime() -> Double {
        guard let handle = engineHandle else {
            return 0.0
        }

        return Engine_GetCurrentTime(handle)
    }

    /// 获取音频文件时长
    func getDuration() -> Double {
        guard let handle = engineHandle else {
            return 0.0
        }

        return Engine_GetDuration(handle)
    }

    /// 创建处理链
    func createProcessingChain(nodeIDs: [UInt32]) -> Int {
        guard let handle = engineHandle else {
            return 0
        }

        let count = Engine_CreateProcessingChain(handle, nodeIDs, Int32(nodeIDs.count))

        if count > 0 {
            logger.info("处理链创建成功", details: "连接数: \(count), 节点: \(nodeIDs)")
        }

        return Int(count)
    }

    /// 自动连接节点到I/O
    func autoConnectToIO(nodeID: UInt32) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        let success = Engine_AutoConnectToIO(handle, nodeID)

        if success {
            logger.info("节点已自动连接到I/O", details: "节点ID: \(nodeID)")
        }

        return success
    }

    // MARK: - Private Helper Methods

    /// 刷新可用插件列表
    private func refreshAvailablePlugins() {
        guard let handle = engineHandle else { return }

        let count = Engine_GetAvailablePluginCount(handle)
        var plugins: [PluginDescription] = []

        for i in 0..<count {
            var pluginInfo = SimplePluginInfo_C()
            if Engine_GetAvailablePluginInfo(handle, i, &pluginInfo) {
                plugins.append(PluginDescription(from: pluginInfo))
            }
        }

        DispatchQueue.main.async { [weak self] in
            self?.availablePlugins = plugins
        }

        logger.info("可用插件列表已刷新", details: "找到 \(plugins.count) 个插件")
    }

    /// 刷新已加载节点列表
    private func refreshLoadedPlugins() {
        guard let handle = engineHandle else { return }

        let count = Engine_GetLoadedNodeCount(handle)
        var nodes: [NodeInfo] = []

        for i in 0..<count {
            var nodeInfo = SimpleNodeInfo_C()
            if Engine_GetLoadedNodeInfo(handle, i, &nodeInfo) {
                nodes.append(NodeInfo(from: nodeInfo))
            }
        }

        DispatchQueue.main.async { [weak self] in
            self?.loadedPlugins = nodes
        }

        logger.info("已加载节点列表已刷新", details: "找到 \(nodes.count) 个节点")
    }

    /// 更新统计信息
    private func updateStatistics() {
        guard let handle = engineHandle else { return }

        var stats = EngineStatistics_C()
        if Engine_GetStatistics(handle, &stats) {
            DispatchQueue.main.async { [weak self] in
                self?.statistics = AudioStatistics(from: stats)
            }
        }
    }

    /// 启动统计信息定时器
    private func startStatisticsTimer() {
        statisticsTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            self?.updateStatistics()
        }
    }

    /// 停止统计信息定时器
    private func stopStatisticsTimer() {
        statisticsTimer?.invalidate()
        statisticsTimer = nil
    }

    // MARK: - Callback Handlers

    /// 处理引擎状态变化
    private func handleStateChange(state: EngineState_C, message: String) {
        switch state {
        case EngineState_Running:
            isRunning = true
        case EngineState_Stopped, EngineState_Error:
            isRunning = false
        default:
            break
        }

        logger.info("引擎状态变化", details: "状态: \(state.rawValue), 消息: \(message)")
    }

    /// 处理引擎错误
    private func handleError(error: String) {
        errorMessage = error
        logger.error("引擎错误", details: error)
    }

    // MARK: - Configuration Management

    /// 更新引擎配置
    func updateConfiguration(_ newConfig: EngineConfiguration) -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        var config = newConfig.toCStruct()
        let success = Engine_UpdateConfiguration(handle, &config)

        if success {
            currentConfiguration = newConfig
            logger.info("引擎配置已更新", details: "采样率: \(newConfig.sampleRate)Hz, 缓冲区: \(newConfig.bufferSize)")
        } else {
            logger.error("引擎配置更新失败", details: "配置参数可能无效")
        }

        return success
    }

    /// 获取当前引擎状态
    func getEngineState() -> EngineState_C {
        guard let handle = engineHandle else {
            return EngineState_Error
        }

        return Engine_GetState(handle)
    }

    /// 检查引擎是否正在运行
    func isEngineRunning() -> Bool {
        guard let handle = engineHandle else {
            return false
        }

        return Engine_IsRunning(handle)
    }
}

// MARK: - Extensions

extension AudioGraphService {
    /// 便捷方法：通过路径加载插件
    func loadPluginByPath(_ path: String) -> Bool {
        // 首先扫描该路径
        let count = scanPlugins(searchPaths: [path])

        if count > 0 {
            // 获取第一个找到的插件
            if let firstPlugin = availablePlugins.first {
                return loadPlugin(identifier: firstPlugin.identifier, displayName: firstPlugin.name)
            }
        }

        return false
    }

    /// 便捷方法：获取输出电平
    func getOutputLevel() -> Double {
        guard let handle = engineHandle else {
            return 0.0
        }

        return Engine_GetOutputLevel(handle)
    }

    /// 便捷方法：获取输入电平
    func getInputLevel() -> Double {
        guard let handle = engineHandle else {
            return 0.0
        }

        return Engine_GetInputLevel(handle)
    }
}
