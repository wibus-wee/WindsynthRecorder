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
        // 初始化默认配置 - 使用48kHz以匹配现代音频设备
        self.currentConfiguration = EngineConfiguration(
            sampleRate: 48000.0,
            bufferSize: 512,
            numInputChannels: 2,  // 录音应用需要立体声输入
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
        print("[AudioGraphService] 开始清理音频引擎资源...")
        logger.info("开始清理音频引擎资源...", details: "执行完整清理流程")

        // 首先确保引擎已停止
        if isRunning {
            print("[AudioGraphService] 引擎仍在运行，先停止")
            stop()
        }

        // 停止统计信息定时器
        statisticsTimer?.invalidate()
        statisticsTimer = nil

        // 销毁引擎
        if let handle = engineHandle {
            print("[AudioGraphService] 正在销毁音频引擎...")
            logger.info("正在销毁音频引擎...", details: "调用 Engine_Shutdown 和 Engine_Destroy")

            // 关闭引擎
            print("[AudioGraphService] 调用 Engine_Shutdown")
            Engine_Shutdown(handle)
            print("[AudioGraphService] Engine_Shutdown 完成")

            // 给一点时间让所有音频回调完成
            Thread.sleep(forTimeInterval: 0.2)

            // 销毁引擎
            print("[AudioGraphService] 调用 Engine_Destroy")
            Engine_Destroy(handle)
            engineHandle = nil
            print("[AudioGraphService] Engine_Destroy 完成")

            logger.info("音频引擎已销毁", details: "资源清理完成")
        }

        print("[AudioGraphService] 音频引擎资源清理完成")
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

        logger.info("正在停止音频处理...", details: "开始停止引擎")

        // 停止引擎
        Engine_Stop(handle)
        isRunning = false

        // 停止统计定时器
        stopStatisticsTimer()

        // 给音频线程一些时间来完全停止
        Thread.sleep(forTimeInterval: 0.05)

        logger.info("音频处理已停止", details: "引擎已安全停止")
    }

    /// 强制清理所有资源（用于应用退出时）
    func forceCleanup() {
        print("[AudioGraphService] 强制清理所有资源")
        cleanup()
    }
    
    /// 扫描插件（统一异步方法）
    func scanPluginsAsync(
        rescanExisting: Bool = false,
        progressCallback: ((Float, String) -> Void)? = nil,
        completion: @escaping (Int) -> Void
    ) {
        guard let handle = engineHandle else {
            errorMessage = "Audio engine not initialized"
            completion(0)
            return
        }

        logger.info("开始扫描插件", details: "重新扫描: \(rescanExisting)")

        // 更新扫描状态
        isScanning = true

        // 创建回调上下文
        let callbackContext = PluginScanCallbackContext(
            service: self,
            progressCallback: progressCallback,
            completion: completion
        )

        // 保存回调上下文（防止被释放）
        let contextPointer = Unmanaged.passRetained(callbackContext).toOpaque()

        Engine_ScanPluginsAsync(
            handle,
            rescanExisting,
            { progress, currentFile, userData in
                // 进度回调
                guard let userData = userData else { return }
                let context = Unmanaged<PluginScanCallbackContext>.fromOpaque(userData).takeUnretainedValue()

                DispatchQueue.main.async {
                    let fileName = currentFile != nil ? String(cString: currentFile!) : ""
                    context.progressCallback?(progress, fileName)
                }
            },
            { foundPlugins, userData in
                // 完成回调
                guard let userData = userData else { return }
                let context = Unmanaged<PluginScanCallbackContext>.fromOpaque(userData).takeRetainedValue()

                DispatchQueue.main.async {
                    guard let service = context.service else { return }
                    service.refreshAvailablePlugins()
                    service.isScanning = false
                    service.logger.info("插件扫描完成", details: "找到 \(foundPlugins) 个插件")
                    context.completion(Int(foundPlugins))
                }
            },
            contextPointer
        )
    }

    /// 停止插件扫描
    func stopPluginScan() {
        guard let handle = engineHandle else {
            return
        }

        Engine_StopPluginScan(handle)
        logger.info("已停止插件扫描")
    }

    /// 更新扫描状态
    private func updateScanningStatus() {
        guard let handle = engineHandle else {
            isScanning = false
            return
        }

        isScanning = Engine_IsScanning(handle)
    }

    // 注意：Dead Man's Pedal和黑名单功能已内置到扫描器中，无需手动管理

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

// MARK: - Callback Context Classes

/// 插件扫描回调上下文
private class PluginScanCallbackContext {
    weak var service: AudioGraphService?
    let progressCallback: ((Float, String) -> Void)?
    let completion: (Int) -> Void

    init(service: AudioGraphService, progressCallback: ((Float, String) -> Void)?, completion: @escaping (Int) -> Void) {
        self.service = service
        self.progressCallback = progressCallback
        self.completion = completion
    }
}

// MARK: - Extensions

extension AudioGraphService {
    /// 便捷方法：异步通过路径加载插件
    func loadPluginByPath(_ path: String, completion: @escaping (Bool) -> Void) {
        // 注意：现在使用默认路径扫描，不支持自定义路径
        // 如果需要特定路径，建议直接使用插件标识符
        scanPluginsAsync { [weak self] foundPlugins in
            guard let self = self, foundPlugins > 0 else {
                completion(false)
                return
            }

            // 获取第一个找到的插件
            if let firstPlugin = self.availablePlugins.first {
                self.loadPlugin(identifier: firstPlugin.identifier, displayName: firstPlugin.name) { success, _ in
                    completion(success)
                }
            } else {
                completion(false)
            }
        }
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

    //==============================================================================
    // MARK: - 离线音频渲染
    //==============================================================================

    /// 离线渲染配置
    struct RenderSettings: Equatable {
        var sampleRate: Int = 44100
        var bitDepth: Int = 24
        var numChannels: Int = 2
        var normalizeOutput: Bool = false
        var includePluginTails: Bool = false
        var format: AudioFormat = .wav

        enum AudioFormat: Int, CaseIterable {
            case wav = 0
            case aiff = 1

            var displayName: String {
                switch self {
                case .wav: return "WAV"
                case .aiff: return "AIFF"
                }
            }

            var fileExtension: String {
                switch self {
                case .wav: return "wav"
                case .aiff: return "aiff"
                }
            }
        }

        /// 转换为 C 结构
        func toCStruct() -> RenderSettings_C {
            return RenderSettings_C(
                sampleRate: Int32(sampleRate),
                bitDepth: Int32(bitDepth),
                numChannels: Int32(numChannels),
                normalizeOutput: normalizeOutput,
                includePluginTails: includePluginTails,
                format: Int32(format.rawValue)
            )
        }
    }

    /// 渲染进度回调类型
    typealias RenderProgressCallback = (Float, String) -> Void

    /// 离线渲染音频文件（通过 VST 处理链）
    /// - Parameters:
    ///   - inputPath: 输入音频文件路径
    ///   - outputPath: 输出音频文件路径
    ///   - settings: 渲染设置
    ///   - progressCallback: 进度回调（可选）
    ///   - completion: 完成回调
    func renderToFile(
        inputPath: String,
        outputPath: String,
        settings: RenderSettings = RenderSettings(),
        progressCallback: RenderProgressCallback? = nil,
        completion: @escaping (Bool, String?) -> Void
    ) {
        guard let handle = engineHandle else {
            completion(false, "Audio engine not initialized")
            return
        }

        logger.info("开始离线渲染", details: "输入: \(inputPath), 输出: \(outputPath)")

        // 在后台线程执行渲染
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else {
                DispatchQueue.main.async {
                    completion(false, "Service deallocated")
                }
                return
            }

            var cSettings = settings.toCStruct()

            // 创建进度回调包装器
            let progressCallbackWrapper: @convention(c) (Float, UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void = { progress, message, userData in
                guard let userData = userData else { return }

                // 安全地获取回调包装器
                let unmanagedWrapper = Unmanaged<RenderProgressCallbackWrapper>.fromOpaque(userData)
                let callback = unmanagedWrapper.takeUnretainedValue()

                let messageString = message != nil ? String(cString: message!) : ""

                // 使用包装器的安全方法调用回调
                callback.invokeCallback(progress: progress, message: messageString)
            }

            // 包装进度回调 - 使用 retained 引用确保内存安全
            var callbackWrapper: RenderProgressCallbackWrapper?
            var callbackPointer: UnsafeMutableRawPointer?

            if let progressCallback = progressCallback {
                callbackWrapper = RenderProgressCallbackWrapper(callback: progressCallback)
                // 使用 passRetained 确保对象在 C++ 调用期间保持存活
                callbackPointer = Unmanaged.passRetained(callbackWrapper!).toOpaque()
            }

            // 执行渲染
            let success = Engine_RenderToFile(
                handle,
                inputPath,
                outputPath,
                &cSettings,
                progressCallback != nil ? progressCallbackWrapper : nil,
                callbackPointer
            )

            // 回到主线程执行完成回调
            DispatchQueue.main.async {
                // 清理回调包装器内存
                if let callbackPointer = callbackPointer {
                    // 释放之前 passRetained 的对象
                    let _ = Unmanaged<RenderProgressCallbackWrapper>.fromOpaque(callbackPointer).takeRetainedValue()
                }

                if success {
                    self.logger.info("离线渲染完成", details: "输出文件: \(outputPath)")
                    completion(true, nil)
                } else {
                    self.logger.error("离线渲染失败", details: "请检查输入文件和输出路径")
                    completion(false, "Render failed")
                }
            }
        }
    }
}

/// 进度回调包装器（用于 C 回调）
private class RenderProgressCallbackWrapper {
    private let callback: AudioGraphService.RenderProgressCallback
    private let callbackQueue: DispatchQueue

    init(callback: @escaping AudioGraphService.RenderProgressCallback) {
        self.callback = callback
        self.callbackQueue = DispatchQueue.main
    }

    func invokeCallback(progress: Float, message: String) {
        callbackQueue.async { [weak self] in
            self?.callback(progress, message)
        }
    }
}
