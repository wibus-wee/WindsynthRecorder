//
//  OfflineRenderDialog.swift
//  WindsynthRecorder
//
//  离线音频渲染对话框 - 类似 Logic Pro 的并轨界面
//

import SwiftUI
import UniformTypeIdentifiers

/// 离线音频渲染对话框
struct OfflineRenderDialog: View {
    @Binding var isPresented: Bool
    let currentFileName: String
    let currentAudioURL: URL?
    @Binding var renderSettings: AudioGraphService.RenderSettings
    @Binding var isRendering: Bool
    @Binding var renderProgress: Float
    @Binding var renderMessage: String
    let audioGraphService: AudioGraphService
    
    @State private var showingFileSaver = false
    @State private var outputURL: URL?
    @State private var estimatedFileSize: String = "计算中..."
    
    var body: some View {
        VStack(spacing: 0) {
            // 标题栏
            titleBar

            // 主内容区域 - 两栏布局
            HStack(spacing: 0) {
                // 左侧：设置面板
                VStack(spacing: 0) {
                    leftPanelContent
                }
                .frame(width: 320)
                .background(Color(NSColor.controlBackgroundColor))

                // 分隔线
                Rectangle()
                    .fill(Color(NSColor.separatorColor))
                    .frame(width: 1)

                // 右侧：预览和信息面板
                VStack(spacing: 0) {
                    rightPanelContent
                }
                .frame(minWidth: 280)
                .background(Color(NSColor.windowBackgroundColor))
            }

            // 底部操作栏
            bottomActionBar
        }
        .frame(width: 720, height: 580)
        .background(Color(NSColor.windowBackgroundColor))
        .onAppear {
            calculateEstimatedFileSize()
        }
        .onChange(of: renderSettings) { _ in
            calculateEstimatedFileSize()
        }
        .fileSaver(
            isPresented: $showingFileSaver,
            document: EmptyDocument(),
            contentType: .audio,
            defaultFilename: generateDefaultFilename(),
            onCompletion: handleFileSaveResult
        )
    }
    
    // MARK: - 左侧设置面板

    private var leftPanelContent: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // 渲染设置
                compactRenderSettingsSection

                // 输出选项
                compactOutputSettingsSection
            }
            .padding(20)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .frame(maxWidth: .infinity)
    }

    // MARK: - 右侧预览面板

    private var rightPanelContent: some View {
        VStack(spacing: 0) {
            // 源文件信息
            sourceFileInfoPanel

            // 分隔线
            Rectangle()
                .fill(Color(NSColor.separatorColor))
                .frame(height: 1)

            // 预估信息和预览
            estimationAndPreviewPanel
        }
    }

    private var titleBar: some View {
        HStack {
            Image(systemName: "waveform.path.badge.plus")
                .font(.title2)
                .foregroundColor(.blue)

            Text("离线音频渲染")
                .font(.title2)
                .fontWeight(.semibold)

            Spacer()

            Button("取消") {
                isPresented = false
            }
            .keyboardShortcut(.escape)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 16)
        .background(Color(NSColor.controlBackgroundColor))
        .overlay(
            Rectangle()
                .frame(height: 1)
                .foregroundColor(Color(NSColor.separatorColor)),
            alignment: .bottom
        )
    }
    
    private var compactRenderSettingsSection: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("渲染设置")
                .font(.headline)
                .foregroundColor(.primary)

            VStack(spacing: 12) {
                // 音频格式
                VStack(alignment: .leading, spacing: 6) {
                    Text("格式")
                        .font(.subheadline)
                        .fontWeight(.medium)

                    Picker("格式", selection: $renderSettings.format) {
                        ForEach(AudioGraphService.RenderSettings.AudioFormat.allCases, id: \.self) { format in
                            Text(format.displayName).tag(format)
                        }
                    }
                    .pickerStyle(.segmented)
                }

                // 采样率和位深度
                HStack(spacing: 12) {
                    VStack(alignment: .leading, spacing: 6) {
                        Text("采样率")
                            .font(.subheadline)
                            .fontWeight(.medium)

                        Picker("采样率", selection: $renderSettings.sampleRate) {
                            Text("44.1k").tag(44100)
                            Text("48k").tag(48000)
                            Text("96k").tag(96000)
                            Text("192k").tag(192000)
                        }
                        .pickerStyle(.menu)
                    }

                    VStack(alignment: .leading, spacing: 6) {
                        Text("位深度")
                            .font(.subheadline)
                            .fontWeight(.medium)

                        Picker("位深度", selection: $renderSettings.bitDepth) {
                            Text("16 bit").tag(16)
                            Text("24 bit").tag(24)
                            Text("32 bit").tag(32)
                        }
                        .pickerStyle(.menu)
                    }
                }

                // 声道数
                VStack(alignment: .leading, spacing: 6) {
                    Text("声道")
                        .font(.subheadline)
                        .fontWeight(.medium)

                    Picker("声道", selection: $renderSettings.numChannels) {
                        Text("单声道").tag(1)
                        Text("立体声").tag(2)
                    }
                    .pickerStyle(.segmented)
                }
            }
            .padding(16)
            .background(Color(NSColor.windowBackgroundColor))
            .cornerRadius(8)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var compactOutputSettingsSection: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("输出选项")
                .font(.headline)
                .foregroundColor(.primary)

            VStack(alignment: .leading, spacing: 12) {
                Toggle("正常化输出", isOn: $renderSettings.normalizeOutput)
                    .help("将音频电平正常化到最大值，避免削波")

                Toggle("包含插件尾音", isOn: $renderSettings.includePluginTails)
                    .help("渲染插件的延迟和混响尾音")
            }
            .padding(16)
            .background(Color(NSColor.windowBackgroundColor))
            .cornerRadius(8)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    
    private var sourceFileInfoPanel: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("源音频文件")
                .font(.headline)
                .foregroundColor(.primary)
                .padding(.horizontal, 20)
                .padding(.top, 20)

            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Image(systemName: "waveform")
                        .foregroundColor(.blue)
                        .font(.title2)

                    VStack(alignment: .leading, spacing: 4) {
                        Text(currentFileName)
                            .font(.system(.body, design: .monospaced))
                            .fontWeight(.medium)
                            .foregroundColor(.primary)

                        if let url = currentAudioURL {
                            Text(url.path)
                                .font(.caption)
                                .foregroundColor(.secondary)
                                .lineLimit(2)
                        }
                    }

                    Spacer()
                }

                // 文件信息
                if let url = currentAudioURL {
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Text("文件大小:")
                                .foregroundColor(.secondary)
                            Spacer()
                            Text(getFileSize(url: url))
                                .fontWeight(.medium)
                        }

                        HStack {
                            Text("时长:")
                                .foregroundColor(.secondary)
                            Spacer()
                            Text(formatDuration(audioGraphService.getDuration()))
                                .fontWeight(.medium)
                        }
                    }
                    .font(.caption)
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
    }
    
    private var estimationAndPreviewPanel: some View {
        VStack(alignment: .leading, spacing: 0) {
            // 预估信息标题
            Text("渲染预估")
                .font(.headline)
                .foregroundColor(.primary)
                .padding(.horizontal, 20)
                .padding(.top, 20)

            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    // 输出文件信息
                    VStack(alignment: .leading, spacing: 12) {
                        Text("输出文件")
                            .font(.subheadline)
                            .fontWeight(.semibold)
                            .foregroundColor(.secondary)

                        VStack(alignment: .leading, spacing: 8) {
                            HStack {
                                Text("文件名:")
                                    .foregroundColor(.secondary)
                                Spacer()
                                Text(generateDefaultFilename())
                                    .font(.system(.caption, design: .monospaced))
                                    .fontWeight(.medium)
                            }

                            HStack {
                                Text("预估大小:")
                                    .foregroundColor(.secondary)
                                Spacer()
                                Text(estimatedFileSize)
                                    .fontWeight(.medium)
                                    .foregroundColor(.blue)
                            }

                            HStack {
                                Text("格式:")
                                    .foregroundColor(.secondary)
                                Spacer()
                                Text("\(renderSettings.format.displayName)")
                                    .fontWeight(.medium)
                            }

                            HStack {
                                Text("质量:")
                                    .foregroundColor(.secondary)
                                Spacer()
                                Text("\(renderSettings.sampleRate/1000)kHz / \(renderSettings.bitDepth)bit")
                                    .fontWeight(.medium)
                            }
                        }
                        .font(.caption)
                    }

                    Divider()

                    // 处理选项预览
                    VStack(alignment: .leading, spacing: 12) {
                        Text("处理选项")
                            .font(.subheadline)
                            .fontWeight(.semibold)
                            .foregroundColor(.secondary)

                        VStack(alignment: .leading, spacing: 6) {
                            HStack {
                                Image(systemName: renderSettings.normalizeOutput ? "checkmark.circle.fill" : "circle")
                                    .foregroundColor(renderSettings.normalizeOutput ? .green : .gray)
                                Text("正常化输出")
                                    .font(.caption)
                                Spacer()
                            }

                            HStack {
                                Image(systemName: renderSettings.includePluginTails ? "checkmark.circle.fill" : "circle")
                                    .foregroundColor(renderSettings.includePluginTails ? .green : .gray)
                                Text("包含插件尾音")
                                    .font(.caption)
                                Spacer()
                            }
                        }
                    }

                    Divider()

                    // 渲染模式
                    VStack(alignment: .leading, spacing: 8) {
                        Text("渲染模式")
                            .font(.subheadline)
                            .fontWeight(.semibold)
                            .foregroundColor(.secondary)

                        HStack {
                            Image(systemName: "bolt.fill")
                                .foregroundColor(.orange)
                            Text("离线高质量渲染")
                                .font(.caption)
                                .fontWeight(.medium)
                            Spacer()
                        }

                        Text("使用最高质量设置进行离线渲染，确保最佳音质输出。")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 20)
            }
        }
    }
    

    
    private var bottomActionBar: some View {
        HStack {
            if isRendering {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        ProgressView()
                            .scaleEffect(0.8)
                        Text(renderMessage)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    ProgressView(value: renderProgress)
                        .frame(width: 200)
                }
            }
            
            Spacer()
            
            Button("取消") {
                isPresented = false
            }
            .disabled(isRendering)
            
            Button("开始渲染") {
                showingFileSaver = true
            }
            .buttonStyle(.borderedProminent)
            .disabled(isRendering || currentAudioURL == nil)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 16)
        .background(Color(NSColor.controlBackgroundColor))
        .overlay(
            Rectangle()
                .frame(height: 1)
                .foregroundColor(Color(NSColor.separatorColor)),
            alignment: .top
        )
    }
    
    // MARK: - Helper Methods

    private func generateDefaultFilename() -> String {
        let baseName = currentFileName.replacingOccurrences(of: "\\.[^.]*$", with: "", options: .regularExpression)
        return "\(baseName)_rendered.\(renderSettings.format.fileExtension)"
    }

    private func getFileSize(url: URL) -> String {
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            if let fileSize = attributes[.size] as? Int64 {
                return ByteCountFormatter.string(fromByteCount: fileSize, countStyle: .file)
            }
        } catch {
            return "未知"
        }
        return "未知"
    }

    private func formatDuration(_ duration: Double) -> String {
        let minutes = Int(duration) / 60
        let seconds = Int(duration) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }
    
    private func calculateEstimatedFileSize() {
        guard let url = currentAudioURL else {
            estimatedFileSize = "无法计算"
            return
        }
        
        // 简单的文件大小估算
        let duration = audioGraphService.getDuration()
        let bytesPerSecond = Double(renderSettings.sampleRate * renderSettings.numChannels * renderSettings.bitDepth) / 8.0
        let estimatedBytes = Int64(duration * bytesPerSecond)
        
        estimatedFileSize = ByteCountFormatter.string(fromByteCount: estimatedBytes, countStyle: .file)
    }
    
    private func handleFileSaveResult(_ result: Result<URL, Error>) {
        switch result {
        case .success(let url):
            startRendering(to: url)
        case .failure(let error):
            print("File save error: \(error)")
        }
    }
    
    private func startRendering(to outputURL: URL) {
        guard let inputURL = currentAudioURL else { return }
        
        isRendering = true
        renderProgress = 0.0
        renderMessage = "准备渲染..."
        
        audioGraphService.renderToFile(
            inputPath: inputURL.path,
            outputPath: outputURL.path,
            settings: renderSettings,
            progressCallback: { progress, message in
                // 确保在主线程更新 UI
                DispatchQueue.main.async {
                    renderProgress = progress
                    renderMessage = message
                }
            }
        ) { success, error in
            // 确保在主线程更新 UI
            DispatchQueue.main.async {
                isRendering = false

                if success {
                    renderMessage = "渲染完成"
                    // 延迟关闭对话框
                    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                        isPresented = false
                    }
                } else {
                    renderMessage = "渲染失败: \(error ?? "未知错误")"
                }
            }
        }
    }
}

// MARK: - Supporting Types

/// 空文档类型（用于文件保存）
struct EmptyDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.audio] }
    
    init() {}
    
    init(configuration: ReadConfiguration) throws {}
    
    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        return FileWrapper(regularFileWithContents: Data())
    }
}

/// 文件保存器修饰符
extension View {
    func fileSaver<T: FileDocument>(
        isPresented: Binding<Bool>,
        document: T,
        contentType: UTType,
        defaultFilename: String,
        onCompletion: @escaping (Result<URL, Error>) -> Void
    ) -> some View {
        self.fileExporter(
            isPresented: isPresented,
            document: document,
            contentType: contentType,
            defaultFilename: defaultFilename,
            onCompletion: onCompletion
        )
    }
}
