//
//  AudioExportService.swift
//  WindsynthRecorder
//
//  音频导出服务 - 封装离线处理器功能，提供高质量音频导出
//

import Foundation
import Combine
import AVFoundation

// MARK: - C Bridge Types

/// 离线处理器句柄类型
typealias OfflineProcessorHandle = UnsafeMutableRawPointer

/// 离线处理配置C结构体
struct OfflineProcessingConfig_C {
    var sampleRate: Double
    var bufferSize: Int32
    var numChannels: Int32
    var normalizeOutput: Bool
    var outputGain: Double
    var enableDithering: Bool
    var outputBitDepth: Int32
}

/// 任务状态C枚举
typealias TaskStatus_C = Int32

let TASK_STATUS_PENDING: TaskStatus_C = 0
let TASK_STATUS_PROCESSING: TaskStatus_C = 1
let TASK_STATUS_COMPLETED: TaskStatus_C = 2
let TASK_STATUS_FAILED: TaskStatus_C = 3
let TASK_STATUS_CANCELLED: TaskStatus_C = 4

/// 音频处理链句柄类型（从桥接头文件引用）
// AudioProcessingChainHandle 已在桥接头文件中定义为 void*

/// 导出任务状态
enum ExportTaskStatus {
    case pending
    case processing
    case completed
    case failed
    case cancelled
}

/// 导出质量设置
enum AudioExportQuality: String, CaseIterable {
    case draft = "draft"
    case standard = "standard"
    case high = "high"
    case maximum = "maximum"

    var displayName: String {
        switch self {
        case .draft: return "草稿质量"
        case .standard: return "标准质量"
        case .high: return "高质量"
        case .maximum: return "最高质量"
        }
    }

    var description: String {
        switch self {
        case .draft: return "快速导出，适合预览"
        case .standard: return "平衡质量和速度"
        case .high: return "高质量，适合最终输出"
        case .maximum: return "最高质量，处理时间较长"
        }
    }

    var bufferSize: Int {
        switch self {
        case .draft: return 2048
        case .standard: return 4096
        case .high: return 8192
        case .maximum: return 16384
        }
    }

    var enableDithering: Bool {
        switch self {
        case .draft, .standard: return false
        case .high, .maximum: return true
        }
    }

    var oversamplingFactor: Int {
        switch self {
        case .draft: return 1
        case .standard: return 1
        case .high: return 2
        case .maximum: return 4
        }
    }
}

/// 导出配置
struct AudioExportConfig {
    // 基本音频设置
    var sampleRate: Double = 44100.0
    var numChannels: Int = 2
    var outputFormat: AudioExportFormat = .wav
    var outputBitDepth: Int = 24

    // 质量设置
    var quality: AudioExportQuality = .standard
    var bufferSize: Int = 4096
    var enableDithering: Bool = false
    var oversamplingFactor: Int = 1

    // 音频处理
    var normalizeOutput: Bool = false
    var outputGain: Double = 1.0
    var enableLimiter: Bool = false
    var limiterThreshold: Double = -0.1 // dB

    // 高级选项
    var enableHighQualityResampling: Bool = true
    var enableAntiAliasing: Bool = true
    var enableNoiseShaping: Bool = false

    // 自动从质量设置更新相关参数
    mutating func applyQualitySettings() {
        bufferSize = quality.bufferSize
        enableDithering = quality.enableDithering
        oversamplingFactor = quality.oversamplingFactor

        // 根据格式调整位深度
        if !outputFormat.supportedBitDepths.contains(outputBitDepth) {
            outputBitDepth = outputFormat.defaultBitDepth
        }
    }

    // 验证配置有效性
    func validate() -> [String] {
        var errors: [String] = []

        if sampleRate < 8000 || sampleRate > 192000 {
            errors.append("采样率必须在 8000-192000 Hz 之间")
        }

        if numChannels < 1 || numChannels > 8 {
            errors.append("声道数必须在 1-8 之间")
        }

        if !outputFormat.supportedBitDepths.contains(outputBitDepth) {
            errors.append("所选格式不支持 \(outputBitDepth) 位深度")
        }

        if outputGain < 0.0 || outputGain > 10.0 {
            errors.append("输出增益必须在 0.0-10.0 之间")
        }

        if limiterThreshold > 0.0 || limiterThreshold < -20.0 {
            errors.append("限制器阈值必须在 -20.0-0.0 dB 之间")
        }

        return errors
    }
}

// MARK: - 配置预设扩展

extension AudioExportConfig {

    /// 创建用于快速预览的配置
    static func forPreview() -> AudioExportConfig {
        var config = AudioExportConfig()
        config.quality = .draft
        config.outputFormat = .wav
        config.outputBitDepth = 16
        config.sampleRate = 44100.0
        config.applyQualitySettings()
        return config
    }

    /// 创建用于最终发布的高质量配置
    static func forMastering() -> AudioExportConfig {
        var config = AudioExportConfig()
        config.quality = .maximum
        config.outputFormat = .wav
        config.outputBitDepth = 24
        config.sampleRate = 48000.0
        config.normalizeOutput = true
        config.enableLimiter = true
        config.enableHighQualityResampling = true
        config.enableAntiAliasing = true
        config.enableNoiseShaping = true
        config.applyQualitySettings()
        return config
    }

    /// 创建用于存档的无损配置
    static func forArchival() -> AudioExportConfig {
        var config = AudioExportConfig()
        config.quality = .high
        config.outputFormat = .flac
        config.outputBitDepth = 24
        config.sampleRate = 96000.0
        config.normalizeOutput = false
        config.outputGain = 1.0
        config.applyQualitySettings()
        return config
    }

    /// 创建用于网络分享的配置
    static func forWebSharing() -> AudioExportConfig {
        var config = AudioExportConfig()
        config.quality = .standard
        config.outputFormat = .wav
        config.outputBitDepth = 16
        config.sampleRate = 44100.0
        config.normalizeOutput = true
        config.enableLimiter = true
        config.limiterThreshold = -1.0
        config.applyQualitySettings()
        return config
    }

    /// 根据输入文件格式创建匹配的配置
    static func matching(inputSampleRate: Double, inputChannels: Int) -> AudioExportConfig {
        var config = AudioExportConfig()
        config.sampleRate = inputSampleRate
        config.numChannels = inputChannels
        config.quality = .high
        config.applyQualitySettings()
        return config
    }
}

/// 支持的导出格式
enum AudioExportFormat: String, CaseIterable {
    case wav = "wav"
    case aiff = "aiff"
    case flac = "flac"

    var fileExtension: String {
        return "." + rawValue
    }

    var displayName: String {
        switch self {
        case .wav: return "WAV"
        case .aiff: return "AIFF"
        case .flac: return "FLAC"
        }
    }

    var description: String {
        switch self {
        case .wav: return "WAV - 无损音频格式，兼容性最好"
        case .aiff: return "AIFF - Apple无损音频格式"
        case .flac: return "FLAC - 无损压缩音频格式"
        }
    }

    var supportedBitDepths: [Int] {
        switch self {
        case .wav, .aiff:
            return [16, 24, 32]
        case .flac:
            return [16, 24]
        }
    }

    var defaultBitDepth: Int {
        switch self {
        case .wav, .aiff, .flac:
            return 24
        }
    }

    var supportsCompression: Bool {
        switch self {
        case .wav, .aiff:
            return false
        case .flac:
            return true
        }
    }
}

/// 导出任务信息
struct ExportTask {
    let id: String
    let inputURL: URL
    let outputURL: URL
    let config: AudioExportConfig
    var status: ExportTaskStatus = .pending
    var progress: Double = 0.0
    var error: String?
    
    init(id: String, inputURL: URL, outputURL: URL, config: AudioExportConfig) {
        self.id = id
        self.inputURL = inputURL
        self.outputURL = outputURL
        self.config = config
    }
}

/// 音频导出服务
class AudioExportService: NSObject, ObservableObject {
    
    // MARK: - Published Properties
    
    @Published var isProcessing: Bool = false
    @Published var tasks: [ExportTask] = []
    @Published var overallProgress: Double = 0.0
    @Published var errorMessage: String?
    
    // MARK: - Private Properties

    private var offlineProcessor: UnsafeMutablePointer<OfflineProcessorHandle>?
    private var vstManager: VSTManagerExample
    private let logger = AudioProcessingLogger.shared
    
    // MARK: - Initialization
    
    override init() {
        self.vstManager = VSTManagerExample.shared
        super.init()
        setupOfflineProcessor()
    }
    
    deinit {
        cleanup()
    }
    
    // MARK: - Setup Methods
    
    private func setupOfflineProcessor() {
        offlineProcessor = offlineProcessor_create()
        guard let processor = offlineProcessor else {
            errorMessage = "Failed to create offline processor"
            logger.error("音频导出服务初始化失败", details: "无法创建离线处理器")
            return
        }

        // 设置错误回调
        let errorCallback: @convention(c) (UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void = { errorPtr, userData in
            guard let errorPtr = errorPtr else { return }
            let errorString = String(cString: errorPtr)

            // 获取AudioExportService实例
            if let userData = userData {
                let service = Unmanaged<AudioExportService>.fromOpaque(userData).takeUnretainedValue()
                DispatchQueue.main.async {
                    service.handleError(errorString)
                }
            }
        }

        let selfPtr = Unmanaged.passUnretained(self).toOpaque()
        offlineProcessor_setErrorCallback(processor, errorCallback, selfPtr)

        logger.info("音频导出服务初始化成功", details: "离线处理器已创建")
    }

    private func cleanup() {
        if let processor = offlineProcessor {
            offlineProcessor_destroy(processor)
            offlineProcessor = nil
        }
    }
    
    // MARK: - Public Methods
    
    /// 添加导出任务
    func addExportTask(inputURL: URL, outputURL: URL, config: AudioExportConfig) -> String? {
        guard let processor = offlineProcessor else {
            errorMessage = "Offline processor not available"
            return nil
        }

        // 验证配置
        let validationErrors = config.validate()
        if !validationErrors.isEmpty {
            errorMessage = "配置验证失败: " + validationErrors.joined(separator: ", ")
            logger.error("导出配置验证失败", details: validationErrors.joined(separator: "\n"))
            return nil
        }

        // 验证输入文件
        guard inputURL.isFileURL && FileManager.default.fileExists(atPath: inputURL.path) else {
            errorMessage = "输入文件不存在"
            logger.error("输入文件不存在", details: inputURL.path)
            return nil
        }

        // 确保输出目录存在
        let outputDirectory = outputURL.deletingLastPathComponent()
        do {
            try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
        } catch {
            errorMessage = "无法创建输出目录: \(error.localizedDescription)"
            logger.error("创建输出目录失败", details: "目录: \(outputDirectory.path)\n错误: \(error.localizedDescription)")
            return nil
        }

        // 确保输出文件有正确的扩展名
        var finalOutputURL = outputURL
        if finalOutputURL.pathExtension.lowercased() != config.outputFormat.rawValue {
            finalOutputURL = finalOutputURL.deletingPathExtension().appendingPathExtension(config.outputFormat.rawValue)
        }

        // 转换配置为C结构体
        var cConfig = OfflineProcessingConfig_C(
            sampleRate: config.sampleRate,
            bufferSize: Int32(config.bufferSize),
            numChannels: Int32(config.numChannels),
            normalizeOutput: config.normalizeOutput,
            outputGain: config.outputGain,
            enableDithering: config.enableDithering,
            outputBitDepth: Int32(config.outputBitDepth)
        )
        
        // 获取VST处理链
        let processingChain = vstManager.getProcessingChain()
        
        // 调用C接口添加任务
        // 记录路径信息用于调试
        logger.info("准备添加导出任务", details: """
            输入路径: \(inputURL.path)
            输出路径: \(finalOutputURL.path)
            输入文件存在: \(FileManager.default.fileExists(atPath: inputURL.path))
            """)

        guard let taskIdPtr = offlineProcessor_addTask(
            processor,
            inputURL.path,
            finalOutputURL.path,
            &cConfig,
            processingChain
        ) else {
            errorMessage = "Failed to add export task"
            logger.error("添加导出任务失败", details: "输入: \(inputURL.lastPathComponent)")
            return nil
        }

        let taskId = String(cString: taskIdPtr)

        // 创建任务对象
        let task = ExportTask(id: taskId, inputURL: inputURL, outputURL: finalOutputURL, config: config)
        tasks.append(task)

        logger.info("添加导出任务", details: """
            任务ID: \(taskId)
            输入: \(inputURL.lastPathComponent)
            输出: \(finalOutputURL.lastPathComponent)
            格式: \(config.outputFormat.displayName)
            质量: \(config.quality.displayName)
            采样率: \(config.sampleRate) Hz
            位深度: \(config.outputBitDepth) bit
            """)

        return taskId
    }

    /// 使用预设配置添加导出任务
    func addExportTask(inputURL: URL, outputURL: URL, preset: AudioExportConfig) -> String? {
        return addExportTask(inputURL: inputURL, outputURL: outputURL, config: preset)
    }

    /// 快速导出（使用标准配置）
    func quickExport(inputURL: URL, outputDirectory: URL, format: AudioExportFormat = .wav) -> String? {
        let fileName = inputURL.deletingPathExtension().lastPathComponent + format.fileExtension
        let outputURL = outputDirectory.appendingPathComponent(fileName)

        var config = AudioExportConfig.forWebSharing()
        config.outputFormat = format
        config.applyQualitySettings()

        return addExportTask(inputURL: inputURL, outputURL: outputURL, config: config)
    }

    /// 高质量导出（用于最终发布）
    func masteringExport(inputURL: URL, outputDirectory: URL, format: AudioExportFormat = .wav) -> String? {
        let fileName = inputURL.deletingPathExtension().lastPathComponent + "_mastered" + format.fileExtension
        let outputURL = outputDirectory.appendingPathComponent(fileName)

        var config = AudioExportConfig.forMastering()
        config.outputFormat = format
        config.applyQualitySettings()

        return addExportTask(inputURL: inputURL, outputURL: outputURL, config: config)
    }

    /// 批量导出多个文件
    func batchExport(inputURLs: [URL], outputDirectory: URL, config: AudioExportConfig) -> [String] {
        var taskIds: [String] = []

        for inputURL in inputURLs {
            let fileName = inputURL.deletingPathExtension().lastPathComponent + config.outputFormat.fileExtension
            let outputURL = outputDirectory.appendingPathComponent(fileName)

            if let taskId = addExportTask(inputURL: inputURL, outputURL: outputURL, config: config) {
                taskIds.append(taskId)
            }
        }

        logger.info("批量导出任务", details: "添加了 \(taskIds.count)/\(inputURLs.count) 个任务")
        return taskIds
    }
    
    /// 移除导出任务
    func removeTask(taskId: String) {
        guard let processor = offlineProcessor else { return }
        
        if offlineProcessor_removeTask(processor, taskId) {
            tasks.removeAll { $0.id == taskId }
            logger.info("移除导出任务", details: "任务ID: \(taskId)")
        }
    }
    
    /// 清空所有任务
    func clearAllTasks() {
        guard let processor = offlineProcessor else { return }
        
        offlineProcessor_clearTasks(processor)
        tasks.removeAll()
        logger.info("清空所有导出任务", details: "已移除 \(tasks.count) 个任务")
    }
    
    /// 开始处理
    func startProcessing() {
        guard let processor = offlineProcessor else {
            errorMessage = "Offline processor not available"
            return
        }
        
        guard !tasks.isEmpty else {
            errorMessage = "No tasks to process"
            return
        }
        
        isProcessing = true
        offlineProcessor_startProcessing(processor)
        
        // 开始监控进度
        startProgressMonitoring()
        
        logger.info("开始导出处理", details: "任务数量: \(tasks.count)")
    }
    
    /// 停止处理
    func stopProcessing() {
        guard let processor = offlineProcessor else { return }
        
        offlineProcessor_stopProcessing(processor)
        isProcessing = false
        
        logger.info("停止导出处理", details: "用户手动停止")
    }
    
    // MARK: - Private Methods
    
    private func startProgressMonitoring() {
        // 使用定时器监控进度
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }
            
            self.updateProgress()
            
            // 检查是否完成
            if !self.isProcessing {
                timer.invalidate()
            }
        }
    }
    
    private func updateProgress() {
        guard let processor = offlineProcessor else { return }
        
        // 更新总体进度
        overallProgress = offlineProcessor_getOverallProgress(processor)
        
        // 更新各个任务的状态和进度
        for i in 0..<tasks.count {
            let taskId = tasks[i].id
            
            // 获取任务状态
            let cStatus = offlineProcessor_getTaskStatus(processor, taskId)
            let status = convertTaskStatus(cStatus)
            tasks[i].status = status
            
            // 获取任务进度
            tasks[i].progress = offlineProcessor_getTaskProgress(processor, taskId)
        }
        
        // 检查是否所有任务都完成
        let allCompleted = tasks.allSatisfy { task in
            task.status == .completed || task.status == .failed || task.status == .cancelled
        }
        
        if allCompleted && isProcessing {
            isProcessing = false
            let completedCount = tasks.filter { $0.status == .completed }.count
            let failedCount = tasks.filter { $0.status == .failed }.count
            
            logger.info("导出处理完成", details: "成功: \(completedCount), 失败: \(failedCount), 总计: \(tasks.count)")
        }
    }
    
    private func convertTaskStatus(_ cStatus: TaskStatus_C) -> ExportTaskStatus {
        switch cStatus {
        case TASK_STATUS_PENDING:
            return .pending
        case TASK_STATUS_PROCESSING:
            return .processing
        case TASK_STATUS_COMPLETED:
            return .completed
        case TASK_STATUS_FAILED:
            return .failed
        case TASK_STATUS_CANCELLED:
            return .cancelled
        default:
            return .failed
        }
    }

    private func handleError(_ error: String) {
        errorMessage = error
        logger.error("音频导出错误", details: error)

        // 如果正在处理，停止处理
        if isProcessing {
            isProcessing = false
        }
    }
}

// MARK: - C Bridge Function Declarations

// 这些函数在VSTBridge.h中声明，在VSTBridge.mm中实现

@_silgen_name("offlineProcessor_create")
func offlineProcessor_create() -> UnsafeMutablePointer<OfflineProcessorHandle>?

@_silgen_name("offlineProcessor_destroy")
func offlineProcessor_destroy(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>)

@_silgen_name("offlineProcessor_addTask")
func offlineProcessor_addTask(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>,
                            _ inputFilePath: UnsafePointer<CChar>,
                            _ outputFilePath: UnsafePointer<CChar>,
                            _ config: UnsafePointer<OfflineProcessingConfig_C>,
                            _ processingChain: AudioProcessingChainHandle?) -> UnsafePointer<CChar>?

@_silgen_name("offlineProcessor_removeTask")
func offlineProcessor_removeTask(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>, _ taskId: UnsafePointer<CChar>) -> Bool

@_silgen_name("offlineProcessor_clearTasks")
func offlineProcessor_clearTasks(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>)

@_silgen_name("offlineProcessor_startProcessing")
func offlineProcessor_startProcessing(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>)

@_silgen_name("offlineProcessor_stopProcessing")
func offlineProcessor_stopProcessing(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>)

@_silgen_name("offlineProcessor_isProcessing")
func offlineProcessor_isProcessing(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>) -> Bool

@_silgen_name("offlineProcessor_getTaskStatus")
func offlineProcessor_getTaskStatus(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>, _ taskId: UnsafePointer<CChar>) -> TaskStatus_C

@_silgen_name("offlineProcessor_getTaskProgress")
func offlineProcessor_getTaskProgress(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>, _ taskId: UnsafePointer<CChar>) -> Double

@_silgen_name("offlineProcessor_getOverallProgress")
func offlineProcessor_getOverallProgress(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>) -> Double

@_silgen_name("offlineProcessor_setErrorCallback")
func offlineProcessor_setErrorCallback(_ handle: UnsafeMutablePointer<OfflineProcessorHandle>,
                                     _ callback: @convention(c) (UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void,
                                     _ userData: UnsafeMutableRawPointer?)