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
    @StateObject private var mixerService = AudioMixerService()

    // UI状态
    @State private var showingFilePicker = false
    @State private var showingVSTProcessor = false
    @State private var showingPluginParameters = false
    @State private var selectedPluginName: String?
    @State private var outputGain: Float = 0.75
    @State private var isMonitoring = true
    @State private var masterVolume: Float = 0.8
    @State private var inputGain: Float = 0.6

    // 波形数据
    @State private var waveformData: [Float] = []
    @State private var isLoadingWaveform = false

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
                print("Mixer error: \(error)")
            }
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

            // 专业状态指示器
            HStack(spacing: 16) {
                ProfessionalStatusLED(
                    label: "REC",
                    isActive: false,
                    color: .red
                )

                ProfessionalStatusLED(
                    label: "PLAY",
                    isActive: mixerService.playbackState == .playing,
                    color: .green
                )

                ProfessionalStatusLED(
                    label: "VST",
                    isActive: mixerService.isVSTProcessingEnabled,
                    color: .blue
                )

                // 时间码显示
                Text(formatTimeCode(mixerService.currentTime))
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

                if !mixerService.currentFileName.isEmpty {
                    Text(mixerService.currentFileName)
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
                    duration: mixerService.duration,
                    currentTime: mixerService.currentTime,
                    onSeek: { time in
                        mixerService.seek(to: time)
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

            if mixerService.loadedPlugins.isEmpty {
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
                    ForEach(mixerService.loadedPlugins.indices, id: \.self) { index in
                        let pluginIdentifier = mixerService.loadedPlugins[index]
                        ProfessionalPluginSlot(
                            pluginName: getPluginDisplayName(identifier: pluginIdentifier),
                            identifier: pluginIdentifier,
                            vstManager: mixerService.getVSTManager(),
                            onParametersPressed: {
                                selectedPluginName = pluginIdentifier
                                showingPluginParameters = true
                            }
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
                leftLevel: mixerService.outputLevel,
                rightLevel: mixerService.outputLevel
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
            if mixerService.currentFileName.isEmpty {
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
                            Text(mixerService.currentFileName)
                                .font(.system(.caption, design: .rounded, weight: .medium))
                                .foregroundColor(.white)
                                .lineLimit(1)

                            Text("Duration: \(formatTime(mixerService.duration))")
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
                action: { mixerService.stop() },
                isEnabled: mixerService.playbackState != .stopped,
                color: .gray
            )

            // 播放/暂停
            ProfessionalTransportButton(
                icon: playbackButtonIcon,
                action: togglePlayback,
                isEnabled: !mixerService.currentFileName.isEmpty,
                color: playbackButtonColor,
                isLarge: true
            )

            // 停止
            ProfessionalTransportButton(
                icon: "stop.fill",
                action: { mixerService.stop() },
                isEnabled: mixerService.playbackState != .stopped,
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
                    Text(formatTimeCode(mixerService.currentTime))
                        .font(.system(.title3, design: .monospaced, weight: .medium))
                        .foregroundColor(.white)

                    Spacer()

                    Text(formatTimeCode(mixerService.duration))
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
                mixerService.loadAudioFile(url: url)
                loadWaveformData()
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
        guard !mixerService.currentFileName.isEmpty,
              let currentURL = mixerService.currentAudioURL else {
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

        // 调用VST管理器移动插件
        let vstManager = mixerService.getVSTManager()
        let success = vstManager.movePlugin(from: sourceIndex, to: destinationIndex)

        if success {
            print("✅ 插件移动成功")
            // VST管理器会自动更新loadedPlugins数组并触发UI更新
        } else {
            print("❌ 插件移动失败")
            // 可以在这里显示错误提示
        }
    }
}

#Preview {
    NavigationView {
        AudioMixerView()
    }
    .preferredColorScheme(.dark)
}
