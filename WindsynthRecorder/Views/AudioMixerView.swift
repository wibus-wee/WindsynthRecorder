//
//  AudioMixerView.swift
//  WindsynthRecorder
//
//  专业音频混音台主界面 - 模块化设计
//

import SwiftUI
import UniformTypeIdentifiers

/// 专业音频混音台主界面
struct AudioMixerView: View {
    @StateObject private var audioGraphService = AudioGraphService.shared

    // UI状态
    @State private var showingFilePicker = false
    @State private var showingVSTProcessor = false
    @State private var showingPluginParameters = false
    @State private var showingRenderDialog = false
    @State private var selectedPluginName: String?
    @State private var selectedNodeID: UInt32?
    @State private var outputGain: Float = 0.75
    @State private var isMonitoring = true
    @State private var masterVolume: Float = 0.8
    @State private var inputGain: Float = 0.6

    // 波形数据
    @State private var waveformData: [Float] = []
    @State private var isLoadingWaveform = false

    // 离线渲染状态
    @State private var isRendering = false
    @State private var renderProgress: Float = 0.0
    @State private var renderMessage: String = ""
    @State private var renderSettings = AudioGraphService.RenderSettings()

    // 音频文件状态（适配新架构）
    @State private var currentFileName: String = ""
    @State private var currentAudioURL: URL?
    @State private var duration: Double = 0.0
    @State private var currentTime: Double = 0.0
    @State private var playbackState: PlaybackState = .stopped
    @State private var outputLevel: Double = 0.0

    enum PlaybackState {
        case stopped, playing, paused, loading
    }

    // 更新定时器
    @State private var updateTimer: Timer?

    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // 专业深色背景
                LinearGradient(
                    colors: [
                        Color(red: 0.12, green: 0.12, blue: 0.12),
                        Color(red: 0.08, green: 0.08, blue: 0.08)
                    ],
                    startPoint: .top,
                    endPoint: .bottom
                )
                .ignoresSafeArea()

                VStack(spacing: 0) {
                    // 顶部工具栏
                    professionalToolbar

                    // 主混音台区域
                    HStack(spacing: 0) {
                        // 左侧：传输控制
                        transportSection
                            .frame(width: 280)

                        // 分隔线
                        professionalDivider

                        // 中央：波形显示区域
                        waveformSection

                        // 分隔线
                        professionalDivider

                        // 右侧：VST 插件机架
                        vstRackSection
                            .frame(width: 320)
                    }

                    // 底部：主输出和监听
                    masterOutputSection
                }
            }
        }
        .preferredColorScheme(.dark)
        .fileImporter(
            isPresented: $showingFilePicker,
            allowedContentTypes: [.audio],
            allowsMultipleSelection: false
        ) { result in
            handleFileSelection(result)
        }
        .sheet(isPresented: $showingRenderDialog) {
            OfflineRenderDialog(
                isPresented: $showingRenderDialog,
                currentFileName: currentFileName,
                currentAudioURL: currentAudioURL,
                renderSettings: $renderSettings,
                isRendering: $isRendering,
                renderProgress: $renderProgress,
                renderMessage: $renderMessage,
                audioGraphService: audioGraphService
            )
        }
        .sheet(isPresented: $showingVSTProcessor) {
            VSTProcessorView()
        }
        .sheet(isPresented: $showingPluginParameters) {
            if let pluginName = selectedPluginName, let nodeID = selectedNodeID {
                PluginParameterView(
                    pluginName: pluginName,
                    nodeID: nodeID,
                    audioGraphService: audioGraphService
                )
            }
        }
        .onReceive(audioGraphService.$errorMessage) { error in
            if let error = error {
                print("AudioGraph error: \(error)")
            }
        }
        .onAppear {
            startUpdateTimer()
        }
        .onDisappear {
            stopUpdateTimer()
        }
        .onAppear {
            loadWaveformData()
        }
    }

    // MARK: - 界面组件

    private var professionalToolbar: some View {
        HStack {
            // Logo 和标题
            HStack(spacing: 8) {
                Image(systemName: "waveform.circle.fill")
                    .font(.title2)
                    .foregroundColor(.blue)

                Text("WindsynthRecorder")
                    .font(.system(.title3, design: .rounded, weight: .semibold))
                    .foregroundColor(.white)
            }

            Spacer()

            // 导出按钮
            Button(action: { showingRenderDialog = true }) {
                HStack(spacing: 6) {
                    Image(systemName: "square.and.arrow.up")
                        .font(.system(.caption, weight: .medium))
                    Text("EXPORT")
                        .font(.system(.caption, design: .rounded, weight: .bold))
                        .tracking(0.5)
                }
                .foregroundColor(.white)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.blue.opacity(0.8))
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(Color.blue, lineWidth: 1)
                )
            }
            .buttonStyle(.plain)
            .disabled(currentFileName.isEmpty || isRendering)
            .opacity(currentFileName.isEmpty ? 0.5 : 1.0)

            // 专业状态指示器
            HStack(spacing: 16) {
                ProfessionalStatusLED(
                    label: "REC",
                    isActive: false,
                    color: .red
                )

                ProfessionalStatusLED(
                    label: "PLAY",
                    isActive: playbackState == .playing,
                    color: .green
                )

                ProfessionalStatusLED(
                    label: "VST",
                    isActive: !audioGraphService.loadedPlugins.isEmpty,
                    color: .blue
                )

                // 时间码显示
                Text(formatTimeCode(currentTime))
                    .font(.system(.title3, design: .monospaced, weight: .medium))
                    .foregroundColor(.white)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 4)
                    .background(Color.black.opacity(0.6))
                    .cornerRadius(4)
            }
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 12)
        .background(
            LinearGradient(
                colors: [
                    Color(red: 0.15, green: 0.15, blue: 0.15),
                    Color(red: 0.10, green: 0.10, blue: 0.10)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        )
        .overlay(
            Rectangle()
                .frame(height: 1)
                .foregroundColor(Color.white.opacity(0.1)),
            alignment: .bottom
        )
    }

    private var transportSection: some View {
        VStack(spacing: 0) {
            // 传输控制标题
            HStack {
                Text("TRANSPORT")
                    .font(.system(.caption, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                    .tracking(1)

                Spacer()
            }
            .padding(.horizontal, 16)
            .padding(.top, 16)
            .padding(.bottom, 8)

            VStack(spacing: 20) {
                // 文件加载区域
                fileLoadSection

                // 传输控制按钮
                transportControls

                // 时间显示
                timeDisplay

                Spacer()
            }
            .padding(.horizontal, 16)
            .padding(.bottom, 16)
        }
        .background(
            Color(red: 0.10, green: 0.10, blue: 0.10)
        )
    }

    private var waveformSection: some View {
        VStack(spacing: 0) {
            // 波形区域标题
            HStack {
                Text("WAVEFORM")
                    .font(.system(.caption, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                    .tracking(1)

                Spacer()

                if !currentFileName.isEmpty {
                    Text(currentFileName)
                        .font(.system(.caption2, design: .rounded))
                        .foregroundColor(.gray)
                        .lineLimit(1)
                }
            }
            .padding(.horizontal, 16)
            .padding(.top, 16)
            .padding(.bottom, 8)

            // 高性能原生波形显示
            ZStack {
                NativeWaveformView(
                    audioData: waveformData,
                    duration: duration,
                    currentTime: currentTime,
                    onSeek: { time in
                        _ = audioGraphService.seekTo(timeInSeconds: time)
                    }
                )

                // 加载指示器
                if isLoadingWaveform {
                    VStack(spacing: 12) {
                        ProgressView()
                            .progressViewStyle(CircularProgressViewStyle(tint: .blue))
                            .scaleEffect(1.2)

                        Text("Extracting Waveform...")
                            .font(.system(.caption, design: .rounded))
                            .foregroundColor(.gray)
                    }
                    .padding(20)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.black.opacity(0.8))
                    )
                }
            }
            .padding(.horizontal, 16)
            .padding(.bottom, 16)
        }
        .background(
            Color(red: 0.08, green: 0.08, blue: 0.08)
        )
    }

    private var vstRackSection: some View {
        VStack(spacing: 0) {
            // VST 机架标题
            HStack {
                Text("VST RACK")
                    .font(.system(.caption, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                    .tracking(1)

                Spacer()

                Button(action: { showingVSTProcessor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(.blue)
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 16)
            .padding(.top, 16)
            .padding(.bottom, 8)

            if audioGraphService.loadedPlugins.isEmpty {
                VStack(spacing: 12) {
                    Image(systemName: "music.note.list")
                        .font(.system(size: 32))
                        .foregroundColor(.gray)

                    Text("No Plugins Loaded")
                        .font(.system(.caption, design: .rounded))
                        .foregroundColor(.gray)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {

                List {
                    ForEach(actualPlugins, id: \.nodeID) { plugin in
                        // 简化的插件槽显示，避免依赖VSTManagerExample
                        HStack(spacing: 12) {
                            // 拖拽指示器
                            Image(systemName: "line.3.horizontal")
                                .font(.caption)
                                .foregroundColor(.gray.opacity(0.6))
                                .frame(width: 16)

                            // 插件信息
                            VStack(alignment: .leading, spacing: 2) {
                                Text(plugin.pluginName)
                                    .font(.system(.caption, design: .rounded, weight: .medium))
                                    .foregroundColor(.white)
                                    .lineLimit(1)

                                Text("Node ID: \(plugin.nodeID)")
                                    .font(.system(.caption2, design: .rounded))
                                    .foregroundColor(.gray)
                            }

                            Spacer()

                            // 控制按钮
                            HStack(spacing: 6) {
                                // 开关按钮
                                Button(action: {
                                    let newState = !plugin.isEnabled
                                    audioGraphService.setNodeEnabled(nodeID: plugin.nodeID, enabled: newState) { success in
                                        if success {
                                            print("✅ 插件状态已更新: \(plugin.pluginName) -> \(newState ? "启用" : "禁用")")
                                        } else {
                                            print("❌ 插件状态更新失败: \(plugin.pluginName)")
                                        }
                                    }
                                }) {
                                    Image(systemName: plugin.isEnabled ? "power.circle.fill" : "power.circle")
                                        .font(.caption)
                                        .foregroundColor(plugin.isEnabled ? .green : .gray)
                                        .frame(width: 24, height: 24)
                                }
                                .buttonStyle(.plain)

                                // 参数按钮
                                Button(action: {
                                    selectedPluginName = plugin.pluginName
                                    selectedNodeID = plugin.nodeID
                                    showingPluginParameters = true
                                }) {
                                    Image(systemName: "gear")
                                        .font(.caption)
                                        .foregroundColor(.gray)
                                        .frame(width: 24, height: 24)
                                }
                                .buttonStyle(.plain)
                                .help("插件参数")

                                // 编辑器按钮
                                Button(action: {
                                    if audioGraphService.nodeHasEditor(nodeID: plugin.nodeID) {
                                        let success = audioGraphService.showNodeEditor(nodeID: plugin.nodeID)
                                        if !success {
                                            print("❌ 无法显示插件编辑器: \(plugin.pluginName)")
                                        } else {
                                            print("✅ 显示插件编辑器: \(plugin.pluginName)")
                                        }
                                    } else {
                                        print("ℹ️ 插件没有编辑器界面: \(plugin.pluginName)")
                                    }
                                }) {
                                    Image(systemName: "slider.horizontal.3")
                                        .font(.caption)
                                        .foregroundColor(.blue)
                                        .frame(width: 24, height: 24)
                                }
                                .buttonStyle(.plain)
                                .help("插件编辑器")

                                // 删除按钮
                                Button(action: {
                                    audioGraphService.removeNode(nodeID: plugin.nodeID) { success in
                                        if success {
                                            print("✅ 成功移除插件: \(plugin.pluginName)")
                                        } else {
                                            print("❌ 移除插件失败: \(plugin.pluginName)")
                                        }
                                    }
                                }) {
                                    Image(systemName: "xmark")
                                        .font(.caption)
                                        .foregroundColor(.red)
                                        .frame(width: 24, height: 24)
                                }
                                .buttonStyle(.plain)
                            }
                        }
                        .padding(12)
                        .background(
                            RoundedRectangle(cornerRadius: 8)
                                .fill(Color.black.opacity(plugin.isEnabled ? 0.2 : 0.4))
                        )
                        .listRowBackground(Color.clear)
                        .listRowSeparator(.hidden)
                    }
                    .onMove(perform: movePlugins)
                }
                .listStyle(.plain)
                .background(Color.clear)
                .scrollContentBackground(.hidden)
                
            }

            Spacer()
        }
        .background(
            Color(red: 0.10, green: 0.10, blue: 0.10)
        )
    }

    private var masterOutputSection: some View {
        HStack {
            // 主输出控制
            HStack(spacing: 20) {
                VStack(spacing: 4) {
                    Text("MASTER OUT")
                        .font(.system(.caption2, design: .rounded, weight: .bold))
                        .foregroundColor(.gray)
                        .tracking(0.5)

                    ProfessionalKnob(
                        value: $outputGain,
                        range: 0...2,
                        label: "GAIN"
                    )
                }

                VStack(spacing: 4) {
                    Text("MONITOR")
                        .font(.system(.caption2, design: .rounded, weight: .bold))
                        .foregroundColor(.gray)
                        .tracking(0.5)

                    ProfessionalToggle(isOn: $isMonitoring)
                }
            }

            Spacer()

            // 主电平表
            ProfessionalMasterMeter(
                leftLevel: Float(outputLevel),
                rightLevel: Float(outputLevel)
            )

            Spacer()

            // 系统状态
            VStack(alignment: .trailing, spacing: 4) {
                Text("SYSTEM")
                    .font(.system(.caption2, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                    .tracking(0.5)

                HStack(spacing: 8) {
                    Text("44.1kHz")
                        .font(.system(.caption2, design: .monospaced))
                        .foregroundColor(.green)

                    Text("24bit")
                        .font(.system(.caption2, design: .monospaced))
                        .foregroundColor(.green)
                }
            }
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 16)
        .background(
            LinearGradient(
                colors: [
                    Color(red: 0.15, green: 0.15, blue: 0.15),
                    Color(red: 0.10, green: 0.10, blue: 0.10)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        )
        .overlay(
            Rectangle()
                .frame(height: 1)
                .foregroundColor(Color.white.opacity(0.1)),
            alignment: .top
        )
    }

    private var fileLoadSection: some View {
        VStack(spacing: 12) {
            if currentFileName.isEmpty {
                Button(action: { showingFilePicker = true }) {
                    VStack(spacing: 8) {
                        Image(systemName: "doc.badge.plus")
                            .font(.system(size: 24))
                            .foregroundColor(.blue)

                        Text("Load Audio File")
                            .font(.system(.caption, design: .rounded, weight: .medium))
                            .foregroundColor(.blue)
                    }
                    .frame(maxWidth: .infinity)
                    .frame(height: 60)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.blue.opacity(0.5), style: StrokeStyle(lineWidth: 1, dash: [5]))
                    )
                }
                .buttonStyle(.plain)
            } else {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Image(systemName: "waveform")
                            .foregroundColor(.green)

                        VStack(alignment: .leading, spacing: 2) {
                            Text(currentFileName)
                                .font(.system(.caption, design: .rounded, weight: .medium))
                                .foregroundColor(.white)
                                .lineLimit(1)

                            Text("Duration: \(formatTime(duration))")
                                .font(.system(.caption2, design: .monospaced))
                                .foregroundColor(.gray)
                        }

                        Spacer()

                        Button(action: { showingFilePicker = true }) {
                            Image(systemName: "arrow.clockwise")
                                .font(.caption)
                                .foregroundColor(.blue)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(12)
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.black.opacity(0.3))
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                )
            }
        }
    }

    private var transportControls: some View {
        HStack(spacing: 12) {
            // 倒带
            ProfessionalTransportButton(
                icon: "backward.end.fill",
                action: {
                    audioGraphService.stopPlayback()
                    playbackState = .stopped
                },
                isEnabled: playbackState != .stopped,
                color: .gray
            )

            // 播放/暂停
            ProfessionalTransportButton(
                icon: playbackButtonIcon,
                action: togglePlayback,
                isEnabled: !currentFileName.isEmpty,
                color: playbackButtonColor,
                isLarge: true
            )

            // 停止
            ProfessionalTransportButton(
                icon: "stop.fill",
                action: {
                    audioGraphService.stopPlayback()
                    playbackState = .stopped
                },
                isEnabled: playbackState != .stopped,
                color: .red
            )

            // 录音
            ProfessionalTransportButton(
                icon: "record.circle",
                action: { /* TODO: 录音功能 */ },
                isEnabled: false,
                color: .red
            )
        }
        .padding(.vertical, 8)
    }

    private var timeDisplay: some View {
        VStack(spacing: 12) {
            // 时间码显示
            VStack(spacing: 8) {
                HStack {
                    Text("POSITION")
                        .font(.system(.caption2, design: .rounded, weight: .bold))
                        .foregroundColor(.gray)
                        .tracking(0.5)

                    Spacer()

                    Text("DURATION")
                        .font(.system(.caption2, design: .rounded, weight: .bold))
                        .foregroundColor(.gray)
                        .tracking(0.5)
                }

                HStack {
                    Text(formatTimeCode(currentTime))
                        .font(.system(.title3, design: .monospaced, weight: .medium))
                        .foregroundColor(.white)

                    Spacer()

                    Text(formatTimeCode(duration))
                        .font(.system(.title3, design: .monospaced, weight: .medium))
                        .foregroundColor(.gray)
                }
            }
            .padding(12)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(Color.black.opacity(0.5))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
            )
        }
    }

    private var professionalDivider: some View {
        Rectangle()
            .fill(
                LinearGradient(
                    colors: [
                        Color.clear,
                        Color.white.opacity(0.1),
                        Color.clear
                    ],
                    startPoint: .top,
                    endPoint: .bottom
                )
            )
            .frame(width: 1)
    }
    

    // MARK: - 辅助方法

    private func handleFileSelection(_ result: Result<[URL], Error>) {
        switch result {
        case .success(let urls):
            if let url = urls.first {
                let success = audioGraphService.loadAudioFile(filePath: url.path)
                if success {
                    currentAudioURL = url
                    currentFileName = url.lastPathComponent
                    duration = audioGraphService.getDuration()
                    loadWaveformData()
                }
            }
        case .failure(let error):
            print("File selection error: \(error)")
        }
    }

    private func togglePlayback() {
        switch playbackState {
        case .stopped, .paused:
            if audioGraphService.play() {
                playbackState = .playing
            }
        case .playing:
            audioGraphService.pause()
            playbackState = .paused
        case .loading:
            break
        }
    }

    private var playbackButtonIcon: String {
        switch playbackState {
        case .playing:
            return "pause.fill"
        case .loading:
            return "hourglass"
        default:
            return "play.fill"
        }
    }

    private var playbackButtonColor: Color {
        switch playbackState {
        case .playing:
            return .orange
        case .loading:
            return .gray
        default:
            return .green
        }
    }

    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }

    private func formatTimeCode(_ time: TimeInterval) -> String {
        let hours = Int(time) / 3600
        let minutes = Int(time) / 60 % 60
        let seconds = Int(time) % 60
        let frames = Int((time.truncatingRemainder(dividingBy: 1)) * 30) // 30fps
        return String(format: "%02d:%02d:%02d:%02d", hours, minutes, seconds, frames)
    }

    private func getPluginDisplayName(identifier: String) -> String {
        if let fileName = identifier.split(separator: "/").last {
            let nameWithoutExtension = fileName.replacingOccurrences(of: ".vst3", with: "")
                .replacingOccurrences(of: ".vst", with: "")
            return String(nameWithoutExtension)
        }
        return identifier
    }

    private func loadWaveformData() {
        // 从真实音频文件异步加载波形数据
        guard !currentFileName.isEmpty,
              let currentURL = currentAudioURL else {
            waveformData = []
            return
        }

        // 设置加载状态
        isLoadingWaveform = true
        print("🎵 开始异步提取波形数据: \(currentURL.lastPathComponent)")

        // 异步提取波形数据
        WaveformDataGenerator.generateFromAudioFileAsync(
            url: currentURL,
            targetSamples: 2000
        ) { result in
            // 这个回调已经在主线程中执行
            isLoadingWaveform = false

            switch result {
            case .success(let data):
                waveformData = data
                print("✅ 波形数据提取完成，样本数: \(data.count)")

            case .failure(let error):
                print("❌ 波形数据提取失败: \(error.localizedDescription)")
                waveformData = [] // 失败时显示空波形

                // TODO: 可以显示错误提示给用户
                // 例如：显示 Toast 或 Alert
            }
        }
    }

    // MARK: - 计算属性

    /// 过滤掉IO节点，只显示真实的插件
    private var actualPlugins: [NodeInfo] {
        return audioGraphService.loadedPlugins.filter { plugin in
            // 过滤掉系统IO节点
            !plugin.pluginName.contains("Audio Input") &&
            !plugin.pluginName.contains("Audio Output") &&
            !plugin.pluginName.contains("MIDI Input") &&
            !plugin.pluginName.contains("MIDI Output") &&
            !plugin.name.contains("Audio Input") &&
            !plugin.name.contains("Audio Output") &&
            !plugin.name.contains("MIDI Input") &&
            !plugin.name.contains("MIDI Output")
        }
    }

    // MARK: - 拖拽重新排序

    /// 处理VST插件的拖拽重新排序（SwiftUI List版本）
    private func movePlugins(from source: IndexSet, to destination: Int) {
        guard let sourceIndex = source.first else { return }

        // 计算目标索引（SwiftUI的onMove会传递插入位置）
        let destinationIndex = destination > sourceIndex ? destination - 1 : destination

        handlePluginMove(from: sourceIndex, to: destinationIndex)
    }

    /// 处理插件移动的核心逻辑（AppKit和SwiftUI共用）
    private func handlePluginMove(from sourceIndex: Int, to destinationIndex: Int) {
        print("🔄 拖拽移动插件: from \(sourceIndex) to \(destinationIndex)")

        let loadedPlugins = actualPlugins

        // 检查索引有效性
        guard sourceIndex >= 0 && sourceIndex < loadedPlugins.count &&
              destinationIndex >= 0 && destinationIndex < loadedPlugins.count &&
              sourceIndex != destinationIndex else {
            print("❌ 无效的移动索引")
            return
        }

        let sourcePlugin = loadedPlugins[sourceIndex]
        let success = audioGraphService.moveNode(nodeID: sourcePlugin.nodeID, newPosition: destinationIndex)

        if success {
            print("✅ 插件移动成功: \(sourcePlugin.name) -> 位置 \(destinationIndex)")
        } else {
            print("❌ 插件移动失败: \(sourcePlugin.name)")
        }
    }

    // MARK: - 定时器管理

    private func startUpdateTimer() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            updatePlaybackStatus()
        }
    }

    private func stopUpdateTimer() {
        updateTimer?.invalidate()
        updateTimer = nil
    }

    private func updatePlaybackStatus() {
        // 更新当前播放时间
        currentTime = audioGraphService.getCurrentTime()

        // 更新输出电平
        outputLevel = audioGraphService.getOutputLevel()

        // 检查播放状态
        if playbackState == .playing && currentTime >= duration && duration > 0 {
            playbackState = .stopped
            audioGraphService.stopPlayback()
        }
    }
}

#Preview {
    NavigationView {
        AudioMixerView()
    }
    .preferredColorScheme(.dark)
}
