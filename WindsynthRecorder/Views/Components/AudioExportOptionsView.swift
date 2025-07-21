//
//  AudioExportOptionsView.swift
//  WindsynthRecorder
//
//  音频导出选项配置界面
//

import SwiftUI
import UniformTypeIdentifiers

struct AudioExportOptionsView: View {
    @Binding var config: AudioExportConfig
    @ObservedObject var exportService: AudioExportService
    let currentAudioURL: URL?
    @Binding var isPresented: Bool
    
    @State private var showingFilePicker = false
    @State private var selectedOutputURL: URL?
    @State private var showingAdvancedOptions = false
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // 顶部信息
                headerSection
                
                ScrollView {
                    VStack(spacing: 20) {
                        // 基本设置
                        basicSettingsSection
                        
                        // 质量设置
                        qualitySettingsSection
                        
                        // 输出设置
                        outputSettingsSection
                        
                        // 高级选项
                        if showingAdvancedOptions {
                            advancedOptionsSection
                        }
                        
                        // 预设选择
                        presetsSection
                    }
                    .padding(20)
                }
                
                // 底部操作按钮
                bottomActionSection
            }
        }
        .frame(width: 600, height: 700)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
        .fileExporter(
            isPresented: $showingFilePicker,
            document: AudioExportDocument(),
            contentType: .audio,
            defaultFilename: defaultFilename
        ) { result in
            handleFileExport(result)
        }
    }
    
    // MARK: - 界面组件
    
    private var headerSection: some View {
        VStack(spacing: 8) {
            HStack {
                Image(systemName: "square.and.arrow.up.circle.fill")
                    .font(.title2)
                    .foregroundColor(.blue)
                
                Text("音频导出")
                    .font(.title2)
                    .fontWeight(.semibold)
                
                Spacer()
                
                Button("关闭") {
                    isPresented = false
                }
                .keyboardShortcut(.escape)
            }
            
            if let url = currentAudioURL {
                Text("源文件: \(url.lastPathComponent)")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }
    
    private var basicSettingsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("基本设置")
                .font(.headline)
                .foregroundColor(.primary)
            
            VStack(spacing: 16) {
                // 输出格式
                HStack {
                    Text("格式:")
                        .frame(width: 80, alignment: .leading)
                    
                    Picker("格式", selection: $config.outputFormat) {
                        ForEach(AudioExportFormat.allCases, id: \.self) { format in
                            VStack(alignment: .leading) {
                                Text(format.displayName)
                                Text(format.description)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                            .tag(format)
                        }
                    }
                    .pickerStyle(.menu)
                    .onChange(of: config.outputFormat) { _ in
                        config.applyQualitySettings()
                    }
                    
                    Spacer()
                }
                
                // 采样率
                HStack {
                    Text("采样率:")
                        .frame(width: 80, alignment: .leading)
                    
                    Picker("采样率", selection: $config.sampleRate) {
                        Text("44.1 kHz").tag(44100.0)
                        Text("48 kHz").tag(48000.0)
                        Text("88.2 kHz").tag(88200.0)
                        Text("96 kHz").tag(96000.0)
                        Text("192 kHz").tag(192000.0)
                    }
                    .pickerStyle(.menu)
                    
                    Spacer()
                }
                
                // 位深度
                HStack {
                    Text("位深度:")
                        .frame(width: 80, alignment: .leading)
                    
                    Picker("位深度", selection: $config.outputBitDepth) {
                        ForEach(config.outputFormat.supportedBitDepths, id: \.self) { depth in
                            Text("\(depth) bit").tag(depth)
                        }
                    }
                    .pickerStyle(.segmented)
                    
                    Spacer()
                }
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
    }
    
    private var qualitySettingsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("质量设置")
                .font(.headline)
                .foregroundColor(.primary)
            
            VStack(spacing: 16) {
                // 质量预设
                HStack {
                    Text("质量:")
                        .frame(width: 80, alignment: .leading)
                    
                    Picker("质量", selection: $config.quality) {
                        ForEach(AudioExportQuality.allCases, id: \.self) { quality in
                            VStack(alignment: .leading) {
                                Text(quality.displayName)
                                Text(quality.description)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                            .tag(quality)
                        }
                    }
                    .pickerStyle(.menu)
                    .onChange(of: config.quality) { _ in
                        config.applyQualitySettings()
                    }
                    
                    Spacer()
                }
                
                // 缓冲区大小
                HStack {
                    Text("缓冲区:")
                        .frame(width: 80, alignment: .leading)
                    
                    Text("\(config.bufferSize) 样本")
                        .foregroundColor(.secondary)
                    
                    Spacer()
                }
                
                // 抖动
                HStack {
                    Text("抖动:")
                        .frame(width: 80, alignment: .leading)
                    
                    Toggle("启用抖动", isOn: $config.enableDithering)
                        .toggleStyle(.switch)
                    
                    Spacer()
                }
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
    }
    
    private var outputSettingsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("输出设置")
                .font(.headline)
                .foregroundColor(.primary)
            
            VStack(spacing: 16) {
                // 归一化
                HStack {
                    Text("归一化:")
                        .frame(width: 80, alignment: .leading)
                    
                    Toggle("启用归一化", isOn: $config.normalizeOutput)
                        .toggleStyle(.switch)
                    
                    Spacer()
                }
                
                // 输出增益
                HStack {
                    Text("增益:")
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $config.outputGain, in: 0.1...2.0, step: 0.1) {
                        Text("增益")
                    } minimumValueLabel: {
                        Text("0.1x")
                            .font(.caption)
                    } maximumValueLabel: {
                        Text("2.0x")
                            .font(.caption)
                    }
                    
                    Text(String(format: "%.1fx", config.outputGain))
                        .frame(width: 40)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                // 限制器
                HStack {
                    Text("限制器:")
                        .frame(width: 80, alignment: .leading)
                    
                    Toggle("启用限制器", isOn: $config.enableLimiter)
                        .toggleStyle(.switch)
                    
                    Spacer()
                }
                
                if config.enableLimiter {
                    HStack {
                        Text("阈值:")
                            .frame(width: 80, alignment: .leading)
                        
                        Slider(value: $config.limiterThreshold, in: -20.0...0.0, step: 0.1) {
                            Text("阈值")
                        } minimumValueLabel: {
                            Text("-20dB")
                                .font(.caption)
                        } maximumValueLabel: {
                            Text("0dB")
                                .font(.caption)
                        }
                        
                        Text(String(format: "%.1f dB", config.limiterThreshold))
                            .frame(width: 50)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
    }
    
    private var advancedOptionsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("高级选项")
                .font(.headline)
                .foregroundColor(.primary)
            
            VStack(spacing: 16) {
                Toggle("高质量重采样", isOn: $config.enableHighQualityResampling)
                Toggle("抗混叠", isOn: $config.enableAntiAliasing)
                Toggle("噪声整形", isOn: $config.enableNoiseShaping)
                
                HStack {
                    Text("过采样:")
                        .frame(width: 80, alignment: .leading)
                    
                    Picker("过采样", selection: $config.oversamplingFactor) {
                        Text("1x").tag(1)
                        Text("2x").tag(2)
                        Text("4x").tag(4)
                        Text("8x").tag(8)
                    }
                    .pickerStyle(.segmented)
                    
                    Spacer()
                }
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
    }
    
    private var presetsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("预设配置")
                .font(.headline)
                .foregroundColor(.primary)
            
            HStack(spacing: 12) {
                Button("预览质量") {
                    config = AudioExportConfig.forPreview()
                }
                .buttonStyle(.bordered)
                
                Button("网络分享") {
                    config = AudioExportConfig.forWebSharing()
                }
                .buttonStyle(.bordered)
                
                Button("母带处理") {
                    config = AudioExportConfig.forMastering()
                }
                .buttonStyle(.bordered)
                
                Button("存档质量") {
                    config = AudioExportConfig.forArchival()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
    }
    
    private var bottomActionSection: some View {
        HStack {
            Button("高级选项") {
                showingAdvancedOptions.toggle()
            }
            .buttonStyle(.bordered)
            
            Spacer()
            
            Button("取消") {
                isPresented = false
            }
            .buttonStyle(.bordered)
            
            Button("开始导出") {
                startExport()
            }
            .buttonStyle(.borderedProminent)
            .disabled(currentAudioURL == nil)
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }
    
    // MARK: - 计算属性
    
    private var defaultFilename: String {
        guard let url = currentAudioURL else { return "export" }
        return url.deletingPathExtension().lastPathComponent + config.outputFormat.fileExtension
    }
    
    // MARK: - 方法
    
    private func handleFileExport(_ result: Result<URL, Error>) {
        switch result {
        case .success(let url):
            selectedOutputURL = url
            performExport(to: url)
        case .failure(let error):
            print("File export error: \(error)")
        }
    }
    
    private func startExport() {
        showingFilePicker = true
    }
    
    private func performExport(to outputURL: URL) {
        guard let inputURL = currentAudioURL else { return }
        
        if let taskId = exportService.addExportTask(inputURL: inputURL, outputURL: outputURL, config: config) {
            exportService.startProcessing()
            isPresented = false
            // 这里可以显示进度界面
        }
    }
}

// MARK: - 辅助类型

struct AudioExportDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.audio] }
    
    init() {}
    
    init(configuration: ReadConfiguration) throws {}
    
    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        return FileWrapper(regularFileWithContents: Data())
    }
}

#Preview {
    AudioExportOptionsView(
        config: .constant(AudioExportConfig.forWebSharing()),
        exportService: AudioExportService(),
        currentAudioURL: nil,
        isPresented: .constant(true)
    )
}
