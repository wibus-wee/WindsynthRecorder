//
//  AudioMixerView.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频混音台界面 - 专业的音频播放和VST处理界面
//

import SwiftUI
import UniformTypeIdentifiers

/// 音频混音台主界面
struct AudioMixerView: View {
    @StateObject private var mixerService = AudioMixerService()
    
    // UI状态
    @State private var showingFilePicker = false
    @State private var showingVSTProcessor = false
    @State private var showingPluginParameters = false
    @State private var selectedPluginName: String?
    @State private var outputGain: Float = 1.0
    @State private var isMonitoring = true
    
    var body: some View {
        VStack(spacing: 0) {
            // 标题栏
            headerSection
            
            Divider()
            
            // 主要内容区域
            HStack(spacing: 0) {
                // 左侧：文件和播放控制
                leftPanel
                
                Divider()
                
                // 右侧：VST插件和参数控制
                rightPanel
            }
            
            Divider()
            
            // 底部：输出控制和状态
            bottomPanel
        }
        .navigationTitle("音频混音台")
        .fileImporter(
            isPresented: $showingFilePicker,
            allowedContentTypes: [.audio],
            allowsMultipleSelection: false
        ) { result in
            handleFileSelection(result)
        }
        .sheet(isPresented: $showingVSTProcessor) {
            VSTProcessorView()
        }
        .sheet(isPresented: $showingPluginParameters) {
            if let pluginName = selectedPluginName {
                PluginParameterView(
                    pluginName: pluginName,
                    vstManager: mixerService.getVSTManager()
                )
            }
        }
        .onReceive(mixerService.$errorMessage) { error in
            if let error = error {
                // 这里可以显示错误提示
                print("Mixer error: \(error)")
            }
        }
    }
    
    // MARK: - 视图组件
    
    private var headerSection: some View {
        HStack {
            Text("音频混音台")
                .font(.title2)
                .fontWeight(.semibold)
            
            Spacer()
            
            // 状态指示器
            HStack(spacing: 12) {
                StatusIndicator(
                    title: "VST",
                    isActive: mixerService.isVSTProcessingEnabled,
                    color: .blue
                )
                
                StatusIndicator(
                    title: "播放",
                    isActive: mixerService.playbackState == .playing,
                    color: .green
                )
            }
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 12)
        .background(Color(.controlBackgroundColor))
    }
    
    private var leftPanel: some View {
        VStack(alignment: .leading, spacing: 16) {
            // 文件加载区域
            fileLoadSection
            
            Divider()
            
            // 播放控制区域
            playbackControlSection
            
            Divider()
            
            // 时间和进度显示
            timeDisplaySection
            
            Spacer()
        }
        .padding(16)
        .frame(minWidth: 300, maxWidth: 400)
    }
    
    private var rightPanel: some View {
        VStack(alignment: .leading, spacing: 16) {
            // VST插件管理
            vstPluginSection
            
            Divider()
            
            // 插件列表和控制
            pluginListSection
            
            Spacer()
        }
        .padding(16)
        .frame(minWidth: 300)
    }
    
    private var bottomPanel: some View {
        HStack {
            // 输出增益控制
            outputGainSection
            
            Spacer()
            
            // 输出电平显示
            outputLevelSection
            
            Spacer()
            
            // 监听控制
            monitoringSection
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 12)
        .background(Color(.controlBackgroundColor))
    }
    
    // MARK: - 文件加载区域
    
    private var fileLoadSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("音频文件")
                .font(.headline)
            
            if mixerService.currentFileName.isEmpty {
                Button(action: { showingFilePicker = true }) {
                    HStack {
                        Image(systemName: "folder.badge.plus")
                        Text("选择音频文件")
                    }
                    .frame(maxWidth: .infinity)
                    .padding()
                    .background(Color.blue.opacity(0.1))
                    .foregroundColor(.blue)
                    .cornerRadius(8)
                }
                .buttonStyle(.plain)
            } else {
                VStack(alignment: .leading, spacing: 4) {
                    Text(mixerService.currentFileName)
                        .font(.system(.body, design: .monospaced))
                        .lineLimit(2)
                    
                    Text("时长: \(formatTime(mixerService.duration))")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Button("更换文件") {
                        showingFilePicker = true
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                }
                .padding()
                .background(Color(.controlBackgroundColor))
                .cornerRadius(8)
            }
        }
    }
    
    // MARK: - 播放控制区域
    
    private var playbackControlSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("播放控制")
                .font(.headline)
            
            HStack(spacing: 16) {
                // 播放/暂停按钮
                Button(action: togglePlayback) {
                    Image(systemName: playbackButtonIcon)
                        .font(.title2)
                        .frame(width: 44, height: 44)
                        .background(playbackButtonColor)
                        .foregroundColor(.white)
                        .clipShape(Circle())
                }
                .disabled(mixerService.currentFileName.isEmpty)
                
                // 停止按钮
                Button(action: { mixerService.stop() }) {
                    Image(systemName: "stop.fill")
                        .font(.title3)
                        .frame(width: 36, height: 36)
                        .background(Color.red)
                        .foregroundColor(.white)
                        .clipShape(Circle())
                }
                .disabled(mixerService.playbackState == .stopped)
                
                Spacer()
                
                // 状态文本
                Text(playbackStateText)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
    
    // MARK: - 时间显示区域
    
    private var timeDisplaySection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("时间")
                .font(.headline)
            
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text("当前:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(formatTime(mixerService.currentTime))
                        .font(.system(.body, design: .monospaced))
                }
                
                HStack {
                    Text("总长:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(formatTime(mixerService.duration))
                        .font(.system(.body, design: .monospaced))
                }
                
                // 进度条
                if mixerService.duration > 0 {
                    ProgressView(value: mixerService.currentTime, total: mixerService.duration)
                        .progressViewStyle(LinearProgressViewStyle())
                }
            }
            .padding()
            .background(Color(.controlBackgroundColor))
            .cornerRadius(8)
        }
    }
    
    // MARK: - VST插件区域
    
    private var vstPluginSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("VST 插件")
                    .font(.headline)
                
                Spacer()
                
                Button("管理插件") {
                    showingVSTProcessor = true
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
            }
            
            HStack {
                Text("已加载: \(mixerService.getVSTManager().loadedPlugins.count)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                if mixerService.isVSTProcessingEnabled {
                    Text("✓ 处理中")
                        .font(.caption)
                        .foregroundColor(.green)
                } else {
                    Text("○ 未激活")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private var pluginListSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            if mixerService.getVSTManager().loadedPlugins.isEmpty {
                Text("暂无加载的插件")
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .italic()
                    .frame(maxWidth: .infinity, alignment: .center)
                    .padding()
            } else {
                ForEach(mixerService.getVSTManager().loadedPlugins, id: \.self) { pluginIdentifier in
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            // 显示插件名称（从identifier中提取）
                            Text(getPluginDisplayName(identifier: pluginIdentifier))
                                .font(.system(.body, design: .monospaced))
                                .lineLimit(1)

                            Text(pluginIdentifier)
                                .font(.system(.caption2, design: .monospaced))
                                .foregroundColor(.secondary)
                                .lineLimit(1)
                        }

                        Spacer()

                        HStack(spacing: 4) {
                            Button("UI") {
                                mixerService.getVSTManager().showPluginEditor(identifier: pluginIdentifier)
                            }
                            .buttonStyle(.bordered)
                            .controlSize(.mini)

                            Button("参数") {
                                selectedPluginName = pluginIdentifier
                                showingPluginParameters = true
                            }
                            .buttonStyle(.bordered)
                            .controlSize(.mini)
                        }
                    }
                    .padding(.vertical, 4)
                }
            }
        }
    }
    
    // MARK: - 底部控制区域
    
    private var outputGainSection: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("输出增益")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Slider(value: $outputGain, in: 0...2) { _ in
                    mixerService.setOutputGain(outputGain)
                }
                .frame(width: 120)
                
                Text("\(Int(outputGain * 100))%")
                    .font(.caption)
                    .frame(width: 40, alignment: .trailing)
            }
        }
    }
    
    private var outputLevelSection: some View {
        VStack(alignment: .center, spacing: 4) {
            Text("输出电平")
                .font(.caption)
                .foregroundColor(.secondary)
            
            LevelMeter(level: mixerService.outputLevel)
                .frame(width: 100, height: 8)
        }
    }
    
    private var monitoringSection: some View {
        VStack(alignment: .trailing, spacing: 4) {
            Text("监听")
                .font(.caption)
                .foregroundColor(.secondary)
            
            Toggle("", isOn: $isMonitoring)
                .toggleStyle(.switch)
                .controlSize(.mini)
        }
    }
    
    // MARK: - 辅助方法
    
    private func handleFileSelection(_ result: Result<[URL], Error>) {
        switch result {
        case .success(let urls):
            if let url = urls.first {
                mixerService.loadAudioFile(url: url)
            }
        case .failure(let error):
            print("File selection error: \(error)")
        }
    }
    
    private func togglePlayback() {
        switch mixerService.playbackState {
        case .stopped, .paused:
            mixerService.play()
        case .playing:
            mixerService.pause()
        case .loading:
            break
        }
    }
    
    private var playbackButtonIcon: String {
        switch mixerService.playbackState {
        case .playing:
            return "pause.fill"
        case .loading:
            return "hourglass"
        default:
            return "play.fill"
        }
    }
    
    private var playbackButtonColor: Color {
        switch mixerService.playbackState {
        case .playing:
            return .orange
        case .loading:
            return .gray
        default:
            return .green
        }
    }
    
    private var playbackStateText: String {
        switch mixerService.playbackState {
        case .stopped:
            return "已停止"
        case .playing:
            return "播放中"
        case .paused:
            return "已暂停"
        case .loading:
            return "加载中..."
        }
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }

    private func getPluginDisplayName(identifier: String) -> String {
        // 从文件路径中提取插件名称
        if let fileName = identifier.split(separator: "/").last {
            let nameWithoutExtension = fileName.replacingOccurrences(of: ".vst3", with: "")
                .replacingOccurrences(of: ".vst", with: "")
            return String(nameWithoutExtension)
        }
        return identifier
    }
}

// MARK: - 辅助视图组件

/// 状态指示器
struct StatusIndicator: View {
    let title: String
    let isActive: Bool
    let color: Color
    
    var body: some View {
        HStack(spacing: 4) {
            Circle()
                .fill(isActive ? color : Color.gray)
                .frame(width: 8, height: 8)
            
            Text(title)
                .font(.caption)
                .foregroundColor(isActive ? color : .secondary)
        }
    }
}

/// 电平表
struct LevelMeter: View {
    let level: Float
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // 背景
                Rectangle()
                    .fill(Color.gray.opacity(0.3))
                
                // 电平条
                Rectangle()
                    .fill(levelColor)
                    .frame(width: geometry.size.width * CGFloat(level))
            }
        }
        .cornerRadius(4)
    }
    
    private var levelColor: Color {
        if level > 0.8 {
            return .red
        } else if level > 0.6 {
            return .orange
        } else {
            return .green
        }
    }
}

#Preview {
    NavigationView {
        AudioMixerView()
    }
}
