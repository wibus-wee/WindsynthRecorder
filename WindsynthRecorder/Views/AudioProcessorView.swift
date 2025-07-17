import SwiftUI
import UniformTypeIdentifiers

struct AudioProcessorView: View {
    @Binding var isPresented: Bool

    // 服务类
    @StateObject private var fileManager = BatchFileManager()
    @StateObject private var dropHandler = FileDropHandler()
    @StateObject private var audioAnalyzer = AudioAnalyzer()
    @StateObject private var audioProcessor = AudioProcessor()
    @ObservedObject private var logger = AudioProcessingLogger.shared

    // UI状态
    @State private var showingFilePicker = false
    @State private var showingLogs = false
    @State private var showingAudioAnalysis = false
    @State private var showingBatchAnalysisResults = false
    @State private var audioAnalysisResult: AudioAnalysisResult?

    // 批量分析状态
    @State private var isBatchAnalyzing = false
    @State private var batchAnalysisCount = 0
    @State private var totalAnalysisCount = 0

    // 音频处理配置
    @State private var enableLoudnessMaximization = true
    @State private var loudnessMethod: LoudnessMethod = .aggressive
    @State private var loudnessSettings = LoudnessSettings()
    @State private var showingLoudnessSettings = false
    @State private var loudnormSettings = LoudnormSettings()
    @State private var showingLoudnormSettings = false
    @State private var enableNoiseReduction = false
    @State private var noiseReductionStrength: Double = 0.5
    @State private var enableNormalization = false
    @State private var normalizationTarget: Double = -3.0
    @State private var outputFormat: OutputFormat = .mp3
    @State private var outputQuality: OutputQuality = .high

    enum LoudnessMethod: String, CaseIterable {
        case dynaudnorm = "动态均衡"
        case loudnorm = "标准化"
        case limiter = "硬限制"
        case aggressive = "激进模式"

        var description: String {
            switch self {
            case .dynaudnorm: return "平滑的动态响度调整"
            case .loudnorm: return "EBU R128 标准响度标准化"
            case .limiter: return "硬限制器最大化音量"
            case .aggressive: return "标准化 + 限制器组合"
            }
        }
    }

    enum OutputFormat: String, CaseIterable {
        case mp3 = "MP3"
        case wav = "WAV"
        case aiff = "AIFF"
        case m4a = "M4A"

        var fileExtension: String {
            switch self {
            case .mp3: return "mp3"
            case .wav: return "wav"
            case .aiff: return "aiff"
            case .m4a: return "m4a"
            }
        }
    }

    enum OutputQuality: String, CaseIterable {
        case low = "标准质量"
        case medium = "高质量"
        case high = "无损质量"

        var ffmpegParams: [String] {
            switch self {
            case .low: return ["-q:a", "5"]
            case .medium: return ["-q:a", "2"]
            case .high: return ["-q:a", "0"]
            }
        }
    }

    // 输出设置
    @State private var outputLocation = FileManager.default.urls(for: .desktopDirectory, in: .userDomainMask)[0]



    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                VStack(alignment: .leading, spacing: 6) {
                    Text("音频后处理工具")
                        .font(.system(size: 18, weight: .semibold))

                    Text("对已有音频文件应用响度最大化、降噪等效果")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }

                Spacer()

                HStack(spacing: 8) {
                    if !fileManager.files.isEmpty {
                        Button(action: {
                            batchAnalyzeFiles()
                        }) {
                            HStack(spacing: 4) {
                                Image(systemName: "waveform.path.ecg")
                                    .font(.system(size: 12, weight: .medium))
                                Text("批量分析")
                                    .font(.system(size: 11, weight: .medium))
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(Color.green.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
                            .foregroundStyle(.green)
                        }
                        .buttonStyle(.plain)
                        .help("分析所有音频文件的响度信息")
                        .disabled(fileManager.isProcessing)
                    }

                    Button(action: {
                        showingLogs = true
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "doc.text")
                                .font(.system(size: 12, weight: .medium))
                            Text("查看日志")
                                .font(.system(size: 11, weight: .medium))
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(Color.blue.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
                        .foregroundStyle(.blue)
                    }
                    .buttonStyle(.plain)
                    .help("查看详细的处理日志")
                }
            }
            .padding()
            
            ScrollView {
                VStack(spacing: 20) {
                    // 文件选择区域
                    fileSelectionSection
                    
                    if !fileManager.files.isEmpty {
                        Divider()

                        // 批量分析按钮和结果
                        batchAnalysisSection

                        Divider()

                        // 音频处理选项
                        audioProcessingSection

                        Divider()

                        // 输出设置
                        outputSettingsSection

                        Divider()

                        // 处理进度
                        if fileManager.isProcessing {
                            processingSection
                        }

                        // 分析进度
                        if isBatchAnalyzing {
                            analysisProgressSection
                        }
                    }
                }
                .padding(20)
            }
            
            // 底部按钮
            HStack(spacing: 12) {
                Button("取消") {
                    isPresented = false
                }
                .buttonStyle(.bordered)
                .keyboardShortcut(.cancelAction)
                
                Spacer()
                
                if !fileManager.files.isEmpty {
                    Button("开始批处理") {
                        startBatchProcessing()
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(fileManager.isProcessing)
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 600, height: 700)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
        .fileImporter(
            isPresented: $showingFilePicker,
            allowedContentTypes: [.audio],
            allowsMultipleSelection: true
        ) { result in
            switch result {
            case .success(let files):
                fileManager.addFiles(files)
            case .failure(let error):
                logger.error("文件选择失败", details: error.localizedDescription)
            }
        }
        .sheet(isPresented: $showingLoudnessSettings) {
            LoudnessSettingsView(settings: $loudnessSettings, isPresented: $showingLoudnessSettings)
        }
        .sheet(isPresented: $showingLoudnormSettings) {
            LoudnormSettingsView(settings: $loudnormSettings, isPresented: $showingLoudnormSettings)
        }
        .sheet(isPresented: $showingLogs) {
            AudioProcessingLogView(isPresented: $showingLogs)
        }
        .sheet(isPresented: $showingBatchAnalysisResults) {
            BatchAnalysisView(files: fileManager.files, isPresented: $showingBatchAnalysisResults)
        }
    }
    
    // MARK: - File Selection Section
    private var fileSelectionSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("音频文件列表")
                    .font(.system(size: 15, weight: .semibold))

                if !fileManager.files.isEmpty {
                    Text("(\(fileManager.files.count) 个文件)")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }

                Spacer()

                if !fileManager.files.isEmpty {
                    Button("清空列表") {
                        fileManager.clearAllFiles()
                    }
                    .font(.system(size: 11))
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                    .disabled(fileManager.isProcessing)
                }
            }

            if fileManager.files.isEmpty {
                // 文件选择按钮和拖放区域
                Button(action: {
                    showingFilePicker = true
                }) {
                    VStack(spacing: 8) {
                        Image(systemName: dropHandler.isDragTargeted ? "arrow.down.circle.fill" : "plus.circle.dashed")
                            .font(.system(size: 32, weight: .light))
                            .foregroundStyle(dropHandler.isDragTargeted ? .green : .blue)

                        Text(dropHandler.isDragTargeted ? "释放文件到此处" : "选择音频文件")
                            .font(.system(size: 14, weight: .medium))
                            .foregroundStyle(dropHandler.isDragTargeted ? .green : .primary)

                        if !dropHandler.isDragTargeted {
                            Text("点击选择或拖拽文件到此处")
                                .font(.system(size: 12, weight: .medium))
                                .foregroundStyle(.blue)

                            Text("支持多选，格式：MP3, WAV, AIFF, M4A 等")
                                .font(.system(size: 11))
                                .foregroundStyle(.secondary)
                        } else {
                            Text("支持音频文件格式")
                                .font(.system(size: 11))
                                .foregroundStyle(.green)
                        }
                    }
                    .frame(maxWidth: .infinity)
                    .padding(24)
                    .background(
                        dropHandler.isDragTargeted ? 
                        Color.green.opacity(0.1) : Color.blue.opacity(0.05), 
                        in: RoundedRectangle(cornerRadius: 8)
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(
                                dropHandler.isDragTargeted ? 
                                Color.green.opacity(0.5) : Color.blue.opacity(0.2), 
                                style: StrokeStyle(lineWidth: dropHandler.isDragTargeted ? 2 : 1, dash: [5])
                            )
                    )
                    .scaleEffect(dropHandler.isDragTargeted ? 1.02 : 1.0)
                    .animation(.easeInOut(duration: 0.2), value: dropHandler.isDragTargeted)
                }
                .buttonStyle(.plain)
                .onDrop(of: [.audio, .fileURL], isTargeted: $dropHandler.isDragTargeted) { providers in
                    return dropHandler.handleDrop(providers: providers) { urls in
                        fileManager.addFiles(urls)
                    }
                }
            } else {
                // 文件列表显示
                VStack(spacing: 8) {
                    // 添加更多文件按钮
                    HStack {
                        Button("添加更多文件") {
                            showingFilePicker = true
                        }
                        .font(.system(size: 11))
                        .buttonStyle(.bordered)
                        .controlSize(.mini)
                        .disabled(fileManager.isProcessing)

                        if !dropHandler.isDragTargeted {
                            Text("或拖拽文件到下方列表")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        } else {
                            Text("释放文件到列表区域")
                                .font(.system(size: 10, weight: .medium))
                                .foregroundStyle(.green)
                        }

                        Spacer()
                    }

                    // 文件列表
                    ScrollView {
                        LazyVStack(spacing: 6) {
                            ForEach(Array(fileManager.files.enumerated()), id: \.element.id) { index, file in
                                FileItemView(file: file, index: index, fileManager: fileManager)
                            }
                        }
                        .padding(8)
                    }
                    .frame(maxHeight: 200)
                    .background(
                        dropHandler.isDragTargeted ? 
                        Color.green.opacity(0.05) : Color.primary.opacity(0.02), 
                        in: RoundedRectangle(cornerRadius: 8)
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(
                                dropHandler.isDragTargeted ? 
                                Color.green.opacity(0.3) : Color.primary.opacity(0.1), 
                                lineWidth: dropHandler.isDragTargeted ? 2 : 1
                            )
                    )
                    .onDrop(of: [.audio, .fileURL], isTargeted: $dropHandler.isDragTargeted) { providers in
                        return dropHandler.handleDrop(providers: providers) { urls in
                            fileManager.addFiles(urls)
                        }
                    }
                }
            }
        }
    }

    // MARK: - Batch Analysis Section
    private var batchAnalysisSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("音频分析")
                    .font(.system(size: 15, weight: .semibold))
                Spacer()

                if hasAnalyzedFiles {
                    Button("查看分析结果") {
                        showingBatchAnalysisResults = true
                    }
                    .font(.system(size: 11))
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                }
            }

            HStack(spacing: 12) {
                if hasAnalyzedFiles {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("已分析: \(analyzedFilesCount)/\(fileManager.files.count)")
                            .font(.system(size: 11, weight: .medium))

                        if let stats = getAnalysisStatistics() {
                            Text("平均响度: \(String(format: "%.1f", stats.avgLoudness)) LUFS")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }
                    }
                } else {
                    Text("请先分析音频文件")
                        .font(.system(size: 11, weight: .medium))
                        .foregroundStyle(.secondary)
                }

                Spacer()
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Analysis Progress Section
    private var analysisProgressSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("分析进度")
                    .font(.system(size: 15, weight: .semibold))
                Spacer()
                Text("\(batchAnalysisCount)/\(totalAnalysisCount)")
                    .font(.system(size: 12))
                    .foregroundStyle(.secondary)
            }

            ProgressView(value: totalAnalysisCount > 0 ? Double(batchAnalysisCount) / Double(totalAnalysisCount) : 0)
                .progressViewStyle(.linear)

            Text("正在分析音频文件...")
                .font(.system(size: 11))
                .foregroundStyle(.secondary)
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Audio Processing Section
    private var audioProcessingSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("音频处理选项")
                    .font(.system(size: 15, weight: .semibold))

                Spacer()
            }

            VStack(spacing: 12) {
                // 响度最大化
                VStack(spacing: 8) {
                    HStack {
                        Toggle(isOn: $enableLoudnessMaximization) {
                            VStack(alignment: .leading, spacing: 1) {
                                Text("响度最大化")
                                    .font(.system(size: 13, weight: .medium))
                                Text("提升音频音量到最佳水平")
                                    .font(.system(size: 11))
                                    .foregroundStyle(.secondary)
                            }
                        }
                        .toggleStyle(.switch)

                        Spacer()
                    }

                    if enableLoudnessMaximization {
                        VStack(spacing: 8) {
                            // 响度处理方法选择
                            HStack {
                                Text("处理方法:")
                                    .font(.system(size: 12, weight: .medium))
                                    .foregroundStyle(.secondary)

                                Picker("", selection: $loudnessMethod) {
                                    ForEach(LoudnessMethod.allCases, id: \.self) { method in
                                        Text(method.rawValue).tag(method)
                                    }
                                }
                                .pickerStyle(.menu)
                                .frame(width: 180)

                                Spacer()
                            }

                            // 方法描述
                            HStack {
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(loudnessMethod.description)
                                        .font(.system(size: 11))
                                        .foregroundStyle(.blue)

                                    if loudnessMethod == .dynaudnorm {
                                        Button("高级设置") {
                                            showingLoudnessSettings = true
                                        }
                                        .font(.system(size: 10))
                                        .buttonStyle(.bordered)
                                        .controlSize(.mini)
                                    } else if loudnessMethod == .loudnorm {
                                        Button("高级设置") {
                                            showingLoudnormSettings = true
                                        }
                                        .font(.system(size: 10))
                                        .buttonStyle(.bordered)
                                        .controlSize(.mini)
                                    }
                                }

                                Spacer()
                            }
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                        .background(Color.blue.opacity(0.05), in: RoundedRectangle(cornerRadius: 6))
                    }
                }

                // 音频标准化
                HStack {
                    Toggle(isOn: $enableNormalization) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("音频标准化")
                                .font(.system(size: 13, weight: .medium))
                            Text("将音频峰值标准化到指定电平")
                                .font(.system(size: 11))
                                .foregroundStyle(.secondary)
                        }
                    }
                    .toggleStyle(.switch)

                    if enableNormalization {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("目标: \(normalizationTarget, specifier: "%.1f") dB")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)

                            Slider(value: $normalizationTarget, in: -12...0, step: 0.5)
                                .frame(width: 200)
                        }
                    }

                    Spacer()
                }

                // 降噪处理
                HStack {
                    Toggle(isOn: $enableNoiseReduction) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("降噪处理")
                                .font(.system(size: 13, weight: .medium))
                            Text("减少背景噪声和杂音")
                                .font(.system(size: 11))
                                .foregroundStyle(.secondary)
                        }
                    }
                    .toggleStyle(.switch)

                    if enableNoiseReduction {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("强度: \(Int(noiseReductionStrength * 100))%")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)

                            Slider(value: $noiseReductionStrength, in: 0...1, step: 0.1)
                                .frame(width: 200)
                        }
                    }

                    Spacer()
                }
            }
        }
    }
    
    // MARK: - Output Settings Section
    private var outputSettingsSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("输出设置")
                    .font(.system(size: 15, weight: .semibold))

                Spacer()
            }

            HStack(spacing: 16) {
                // 输出格式
                VStack(alignment: .leading, spacing: 6) {
                    Text("输出格式")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Picker("输出格式", selection: $outputFormat) {
                        ForEach(OutputFormat.allCases, id: \.self) { format in
                            Text(format.rawValue).tag(format)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 200)
                }

                // 输出质量
                VStack(alignment: .leading, spacing: 6) {
                    Text("输出质量")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Picker("输出质量", selection: $outputQuality) {
                        ForEach(OutputQuality.allCases, id: \.self) { quality in
                            Text(quality.rawValue).tag(quality)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 180)
                }

                Spacer()
            }

            // 输出位置
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("输出位置")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Text(outputLocation.path)
                        .font(.system(size: 11))
                        .foregroundStyle(.primary)
                        .lineLimit(1)
                        .truncationMode(.middle)
                }

                Spacer()

                Button("选择文件夹") {
                    selectOutputLocation()
                }
                .font(.system(size: 11))
                .buttonStyle(.bordered)
                .controlSize(.mini)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))
        }
    }
    
    // MARK: - Processing Section
    private var processingSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("处理进度")
                    .font(.system(size: 15, weight: .semibold))
                Spacer()
                Text("\(fileManager.completedCount)/\(fileManager.totalCount)")
                    .font(.system(size: 12))
                    .foregroundStyle(.secondary)
            }
            
            ProgressView(value: fileManager.overallProgress)
                .progressViewStyle(.linear)
            
            Text("正在处理音频文件...")
                .font(.system(size: 11))
                .foregroundStyle(.secondary)
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }
    
    // MARK: - Helper Functions
    private func selectOutputLocation() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.prompt = "选择输出位置"
        
        if panel.runModal() == .OK {
            if let url = panel.url {
                outputLocation = url
            }
        }
    }
    
    private func startBatchProcessing() {
        fileManager.startBatchProcessing()

        // 创建处理配置
        let config = AudioProcessingConfig(
            enableLoudnessNormalization: enableLoudnessMaximization,
            enableNoiseReduction: enableNoiseReduction,
            enableEqualizer: false, // 暂时禁用
            outputFormat: AudioOutputFormat.mp3, // 使用服务类的枚举
            outputQuality: AudioOutputQuality.high // 使用服务类的枚举
        )

        // 验证输出目录
        guard audioProcessor.validateOutputDirectory(outputLocation) else {
            logger.error("输出目录无效", details: outputLocation.path)
            fileManager.completeBatchProcessing()
            return
        }

        let inputURLs = fileManager.files.map { $0.url }

        audioProcessor.processFiles(
            inputURLs: inputURLs,
            outputDirectory: outputLocation,
            config: config,
            fileProgressCallback: { index, progress in
                self.fileManager.updateFileProgress(at: index, progress: progress)
                if progress >= 1.0 {
                    self.fileManager.updateFileStatus(at: index, status: .completed)
                }
            },
            overallProgressCallback: { progress in
                self.fileManager.overallProgress = progress
            },
            completion: { processedURLs, errors in
                // 处理错误
                for (index, error) in errors.enumerated() {
                    if index < self.fileManager.files.count {
                        self.fileManager.updateFileStatus(at: index, status: .failed, error: error.localizedDescription)
                    }
                }

                self.fileManager.completeBatchProcessing()

                let successCount = processedURLs.count
                let failureCount = errors.count

                if successCount > 0 {
                    NotificationManager.shared.showSuccess(message: "批量处理完成：成功 \(successCount) 个，失败 \(failureCount) 个")
                } else {
                    NotificationManager.shared.showError(message: "批量处理失败：所有文件处理失败")
                }
            }
        )
    }

    // MARK: - Analysis Helper Methods
    private var hasAnalyzedFiles: Bool {
        fileManager.files.contains { $0.analysisResult != nil }
    }

    private var analyzedFilesCount: Int {
        fileManager.files.filter { $0.analysisResult != nil }.count
    }

    private func getAnalysisStatistics() -> (avgLoudness: Double, minLoudness: Double, maxLoudness: Double, avgPeak: Double, minPeak: Double, maxPeak: Double)? {
        let analyzedFiles = fileManager.files.compactMap { $0.analysisResult }

        guard !analyzedFiles.isEmpty else { return nil }

        let loudnessValues = analyzedFiles.map { $0.originalLoudness }
        let peakValues = analyzedFiles.map { $0.originalPeak }

        let avgLoudness = loudnessValues.reduce(0, +) / Double(loudnessValues.count)
        let minLoudness = loudnessValues.min() ?? 0
        let maxLoudness = loudnessValues.max() ?? 0

        let avgPeak = peakValues.reduce(0, +) / Double(peakValues.count)
        let minPeak = peakValues.min() ?? 0
        let maxPeak = peakValues.max() ?? 0

        return (avgLoudness, minLoudness, maxLoudness, avgPeak, minPeak, maxPeak)
    }

    private func batchAnalyzeFiles() {
        let filesToAnalyze = fileManager.files.filter { $0.analysisResult == nil }
        guard !filesToAnalyze.isEmpty else { return }

        isBatchAnalyzing = true
        batchAnalysisCount = 0
        totalAnalysisCount = filesToAnalyze.count

        logger.info("开始批量音频分析", details: "文件数量: \(filesToAnalyze.count)")

        // 更新文件状态为分析中
        for (index, file) in fileManager.files.enumerated() {
            if file.analysisResult == nil {
                fileManager.updateFileStatus(at: index, status: .analyzing)
            }
        }

        let urls = filesToAnalyze.map { $0.url }

        audioAnalyzer.analyzeFiles(urls, progressCallback: { completed, total in
            self.batchAnalysisCount = completed
        }, completion: { results, errors in
            self.isBatchAnalyzing = false

            // 更新文件的分析结果
            for (index, file) in self.fileManager.files.enumerated() {
                if let result = results[file.url] {
                    self.fileManager.updateFileAnalysisResult(at: index, result: result)
                } else if errors[file.url] != nil {
                    self.fileManager.updateFileStatus(at: index, status: .failed, error: "分析失败")
                }
            }

            self.logger.info("批量音频分析完成", details: "成功: \(results.count), 失败: \(errors.count)")
        })
    }
}

// MARK: - File Item View
struct FileItemView: View {
    let file: BatchFileItem
    let index: Int
    let fileManager: BatchFileManager

    var body: some View {
        HStack(spacing: 12) {
            // 状态图标
            Image(systemName: file.status.icon)
                .font(.system(size: 16, weight: .medium))
                .foregroundStyle(file.status.color)
                .frame(width: 20)

            // 文件信息
            VStack(alignment: .leading, spacing: 2) {
                Text(file.fileName)
                    .font(.system(size: 12, weight: .medium))
                    .lineLimit(1)

                HStack {
                    Text(file.filePath)
                        .font(.system(size: 10))
                        .foregroundStyle(.secondary)
                        .lineLimit(1)
                        .truncationMode(.middle)
                }
            }

            Spacer()

            VStack {
                Text(file.status.displayText)
                        .font(.system(size: 10, weight: .medium))
                        .foregroundStyle(file.status.color)

                // 进度条（仅在处理时显示）
                if file.status == .processing || file.status == .analyzing {
                    ProgressView(value: file.progress)
                        .progressViewStyle(.linear)
                        .frame(width: 60)
                }
            }

            // 操作按钮
            HStack(spacing: 4) {
                if file.status == .pending && !fileManager.isProcessing {
                    Button(action: {
                        // 单个文件分析功能
                        // analyzeFile(at: index)
                    }) {
                        Image(systemName: "waveform.path.ecg")
                            .font(.system(size: 10))
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                    .help("分析音频")
                }

                if !fileManager.isProcessing {
                    Button(action: {
                        fileManager.removeFile(at: index)
                    }) {
                        Image(systemName: "trash")
                            .font(.system(size: 10))
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                    .foregroundStyle(.red)
                    .help("移除文件")
                }
            }
        }
        .padding(8)
        .background(Color.gray.opacity(0.05), in: RoundedRectangle(cornerRadius: 6))
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.gray.opacity(0.2), lineWidth: 0.5)
        )
    }
}

// MARK: - Batch Analysis View
struct BatchAnalysisView: View {
    let files: [BatchFileItem]
    @Binding var isPresented: Bool

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("批量分析结果")
                    .font(.system(size: 16, weight: .semibold))

                Spacer()

                Button("完成") {
                    isPresented = false
                }
                .buttonStyle(.bordered)
            }
            .padding()

            // Statistics
            if let stats = getStatistics() {
                VStack(spacing: 12) {
                    Text("统计信息")
                        .font(.system(size: 14, weight: .semibold))

                    HStack(spacing: 20) {
                        VStack {
                            Text("响度 (LUFS)")
                                .font(.system(size: 11, weight: .medium))
                                .foregroundStyle(.secondary)
                            Text("平均: \(String(format: "%.1f", stats.avgLoudness))")
                                .font(.system(size: 12))
                            Text("范围: \(String(format: "%.1f", stats.minLoudness)) ~ \(String(format: "%.1f", stats.maxLoudness))")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }

                        VStack {
                            Text("峰值 (dBTP)")
                                .font(.system(size: 11, weight: .medium))
                                .foregroundStyle(.secondary)
                            Text("平均: \(String(format: "%.1f", stats.avgPeak))")
                                .font(.system(size: 12))
                            Text("范围: \(String(format: "%.1f", stats.minPeak)) ~ \(String(format: "%.1f", stats.maxPeak))")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }
                    }
                }
                .padding()
                .background(Color.primary.opacity(0.05), in: RoundedRectangle(cornerRadius: 8))
                .padding(.horizontal)
            }

            // File list
            ScrollView {
                LazyVStack(spacing: 8) {
                    ForEach(files.filter { $0.analysisResult != nil }, id: \.id) { file in
                        fileAnalysisRow(file: file)
                    }
                }
                .padding()
            }
        }
        .frame(width: 600, height: 500)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
    }

    private func fileAnalysisRow(file: BatchFileItem) -> some View {
        HStack(spacing: 12) {
            VStack(alignment: .leading, spacing: 2) {
                Text(file.fileName)
                    .font(.system(size: 12, weight: .medium))
                    .lineLimit(1)

                Text(file.filePath)
                    .font(.system(size: 10))
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }

            Spacer()

            if let result = file.analysisResult {
                VStack(alignment: .trailing, spacing: 2) {
                    Text("\(String(format: "%.1f", result.originalLoudness)) LUFS")
                        .font(.system(size: 11, weight: .medium))

                    Text("\(String(format: "%.1f", result.originalPeak)) dBTP")
                        .font(.system(size: 10))
                        .foregroundStyle(.secondary)
                }
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))
    }

    private func getStatistics() -> (avgLoudness: Double, minLoudness: Double, maxLoudness: Double, avgPeak: Double, minPeak: Double, maxPeak: Double)? {
        let analyzedFiles = files.compactMap { $0.analysisResult }

        guard !analyzedFiles.isEmpty else { return nil }

        let loudnessValues = analyzedFiles.map { $0.originalLoudness }
        let peakValues = analyzedFiles.map { $0.originalPeak }

        let avgLoudness = loudnessValues.reduce(0, +) / Double(loudnessValues.count)
        let minLoudness = loudnessValues.min() ?? 0
        let maxLoudness = loudnessValues.max() ?? 0

        let avgPeak = peakValues.reduce(0, +) / Double(peakValues.count)
        let minPeak = peakValues.min() ?? 0
        let maxPeak = peakValues.max() ?? 0

        return (avgLoudness, minLoudness, maxLoudness, avgPeak, minPeak, maxPeak)
    }
}



#Preview {
    AudioProcessorView(isPresented: .constant(true))
}
