import Foundation
import SwiftUI

// 批处理文件状态
enum BatchFileStatus {
    case pending
    case analyzing
    case analyzed
    case processing
    case completed
    case failed
    
    var icon: String {
        switch self {
        case .pending: return "clock"
        case .analyzing: return "waveform.path.ecg"
        case .analyzed: return "checkmark.circle"
        case .processing: return "gear"
        case .completed: return "checkmark.circle.fill"
        case .failed: return "xmark.circle.fill"
        }
    }
    
    var color: Color {
        switch self {
        case .pending: return .secondary
        case .analyzing: return .blue
        case .analyzed: return .green
        case .processing: return .orange
        case .completed: return .green
        case .failed: return .red
        }
    }
    
    var displayText: String {
        switch self {
        case .pending: return "等待中"
        case .analyzing: return "分析中"
        case .analyzed: return "已分析"
        case .processing: return "处理中"
        case .completed: return "已完成"
        case .failed: return "失败"
        }
    }
}

// 音频分析结果
struct AudioAnalysisResult {
    let originalLoudness: Double
    let originalPeak: Double
    let targetLoudness: Double
    let targetPeak: Double
}

// 批处理文件项
struct BatchFileItem: Identifiable {
    let id = UUID()
    let url: URL
    var status: BatchFileStatus = .pending
    var progress: Double = 0.0
    var error: String?
    var analysisResult: AudioAnalysisResult?
    
    var fileName: String {
        url.lastPathComponent
    }
    
    var filePath: String {
        url.path
    }
    
    var fileExtension: String {
        url.pathExtension.lowercased()
    }
}

// 批处理文件管理器
class BatchFileManager: ObservableObject {
    @Published var files: [BatchFileItem] = []
    @Published var isProcessing = false
    @Published var currentProcessingIndex = 0
    @Published var overallProgress: Double = 0.0
    
    private let logger = AudioProcessingLogger.shared
    
    // 添加文件
    func addFiles(_ urls: [URL]) {
        for url in urls {
            // 检查是否已存在
            if !files.contains(where: { $0.url == url }) {
                let fileItem = BatchFileItem(url: url)
                files.append(fileItem)
                logger.info("添加文件", details: url.lastPathComponent)
            }
        }
    }
    
    // 移除文件
    func removeFile(at index: Int) {
        guard index < files.count else { return }
        let file = files[index]
        files.remove(at: index)
        logger.info("移除文件", details: file.url.lastPathComponent)
    }
    
    // 清空所有文件
    func clearAllFiles() {
        files.removeAll()
        logger.info("清空文件列表", details: "已移除所有文件")
    }
    
    // 获取指定状态的文件数量
    func getFileCount(with status: BatchFileStatus) -> Int {
        return files.filter { $0.status == status }.count
    }
    
    // 获取已完成的文件数量
    var completedCount: Int {
        getFileCount(with: .completed)
    }
    
    // 获取失败的文件数量
    var failedCount: Int {
        getFileCount(with: .failed)
    }
    
    // 获取总文件数量
    var totalCount: Int {
        files.count
    }
    
    // 更新文件状态
    func updateFileStatus(at index: Int, status: BatchFileStatus, error: String? = nil) {
        guard index < files.count else { return }
        files[index].status = status
        files[index].error = error
    }
    
    // 更新文件进度
    func updateFileProgress(at index: Int, progress: Double) {
        guard index < files.count else { return }
        files[index].progress = progress
    }
    
    // 更新文件分析结果
    func updateFileAnalysisResult(at index: Int, result: AudioAnalysisResult) {
        guard index < files.count else { return }
        files[index].analysisResult = result
        files[index].status = .analyzed
    }
    
    // 重置所有文件状态为pending（除了失败的）
    func resetFileStatuses() {
        for index in 0..<files.count {
            if files[index].status != .failed {
                files[index].status = .pending
                files[index].progress = 0.0
            }
        }
    }
    
    // 更新整体进度
    func updateOverallProgress() {
        let completedCount = files.filter { $0.status == .completed || $0.status == .failed }.count
        overallProgress = files.isEmpty ? 0.0 : Double(completedCount) / Double(files.count)
    }
    
    // 开始批处理
    func startBatchProcessing() {
        isProcessing = true
        currentProcessingIndex = 0
        overallProgress = 0.0
        resetFileStatuses()
        logger.info("开始批量处理", details: "总文件数: \(files.count)")
    }
    
    // 完成批处理
    func completeBatchProcessing() {
        isProcessing = false
        overallProgress = 1.0
        
        let completedCount = getFileCount(with: .completed)
        let failedCount = getFileCount(with: .failed)
        
        logger.info("批量处理完成", details: "成功: \(completedCount), 失败: \(failedCount), 总计: \(files.count)")
    }
    
    // 获取下一个待处理的文件索引
    func getNextProcessingIndex() -> Int? {
        for (index, file) in files.enumerated() {
            if file.status == .pending {
                return index
            }
        }
        return nil
    }
    
    // 检查是否所有文件都已处理完成
    var isAllFilesProcessed: Bool {
        return files.allSatisfy { $0.status == .completed || $0.status == .failed }
    }
    
    // 获取统计信息
    func getStatistics() -> (analyzed: Int, failed: Int, avgLoudness: Double, minLoudness: Double, maxLoudness: Double, avgPeak: Double, minPeak: Double, maxPeak: Double) {
        let analyzedFiles = files.compactMap { $0.analysisResult }
        
        guard !analyzedFiles.isEmpty else {
            return (0, failedCount, 0, 0, 0, 0, 0, 0)
        }
        
        let loudnessValues = analyzedFiles.map { $0.originalLoudness }
        let peakValues = analyzedFiles.map { $0.originalPeak }
        
        let avgLoudness = loudnessValues.reduce(0, +) / Double(loudnessValues.count)
        let minLoudness = loudnessValues.min() ?? 0
        let maxLoudness = loudnessValues.max() ?? 0
        
        let avgPeak = peakValues.reduce(0, +) / Double(peakValues.count)
        let minPeak = peakValues.min() ?? 0
        let maxPeak = peakValues.max() ?? 0
        
        return (analyzedFiles.count, failedCount, avgLoudness, minLoudness, maxLoudness, avgPeak, minPeak, maxPeak)
    }
}
