import SwiftUI

struct ContentView: View {
    @StateObject private var audioDeviceManager = AudioDeviceManager.shared
    @StateObject private var audioRecorder = AudioRecorder.shared
    @StateObject private var notificationManager = NotificationManager.shared
    @StateObject private var audioGraphService = AudioGraphService.shared

    // MARK: - Window Management
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.openWindow) private var openWindow

    @State private var showingSaveDialog = false
    @State private var fileName = ""
    @State private var maximizeLoudness = false
    @State private var loudnessMethod: LoudnessMethod = .aggressive
    @State private var loudnessSettings = LoudnessSettings()
    @State private var showingLoudnessSettings = false
    @State private var loudnormSettings = LoudnormSettings()
    @State private var showingLoudnormSettings = false

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

        func generateFilter(loudnessSettings: LoudnessSettings = LoudnessSettings(), loudnormSettings: LoudnormSettings = LoudnormSettings()) -> String {
            switch self {
            case .dynaudnorm:
                return loudnessSettings.toDynaudnormString()
            case .loudnorm:
                return loudnormSettings.toLoudnormString()
            case .limiter:
                return "alimiter=level_in=2:level_out=0.95:limit=0.95:attack=5:release=50"
            case .aggressive:
                return "loudnorm=I=-12:LRA=5:TP=-1,alimiter=level_in=1.5:level_out=0.98:limit=0.98:attack=3:release=30"
            }
        }
    }
    @State private var showingFFmpegSettings = false
    @State private var showingDeviceList = false
    @State private var autoRefreshTimer: Timer?
    @ObservedObject private var ffmpegManager = FFmpegManager.shared

    // Audio Settings
    @State private var selectedSampleRate: SampleRate = .rate44100
    @State private var selectedBitDepth: BitDepth = .bit16
    @State private var selectedFileFormat: FileFormat = .wav
    @State private var saveLocation: URL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
    @State private var autoNormalize = true
    @State private var enableNoiseGate = false
    @State private var noiseGateThreshold: Double = -40.0

    enum SampleRate: String, CaseIterable {
        case rate44100 = "44.1 kHz"
        case rate48000 = "48 kHz"
        case rate96000 = "96 kHz"
        case rate192000 = "192 kHz"

        var value: Double {
            switch self {
            case .rate44100: return 44100
            case .rate48000: return 48000
            case .rate96000: return 96000
            case .rate192000: return 192000
            }
        }
    }

    enum BitDepth: String, CaseIterable {
        case bit16 = "16-bit"
        case bit24 = "24-bit"
        case bit32 = "32-bit"

        var value: Int {
            switch self {
            case .bit16: return 16
            case .bit24: return 24
            case .bit32: return 32
            }
        }
    }

    enum FileFormat: String, CaseIterable {
        case wav = "WAV"
        case aiff = "AIFF"
        case m4a = "M4A"

        var fileExtension: String {
            switch self {
            case .wav: return "wav"
            case .aiff: return "aiff"
            case .m4a: return "m4a"
            }
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            // Main Content
            ScrollView {
                VStack(spacing: 16) {
                    recordingControlSection
                    deviceStatusSection
                    audioSettingsSection
                    fileSettingsSection
                }
                .padding(20)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(.windowBackgroundColor))
        .overlay(notificationOverlay)
        .onAppear {
            // 启动自动刷新（初始化已在 StartupInitializationView 中完成）
            startAutoRefresh()
        }
        .onDisappear {
            stopAutoRefresh()
        }
        .sheet(isPresented: $showingSaveDialog) {
            modernSaveDialog
        }
        .sheet(isPresented: $showingDeviceList) {
            modernDeviceListView
        }
        .sheet(isPresented: $showingLoudnessSettings) {
            LoudnessSettingsView(settings: $loudnessSettings, isPresented: $showingLoudnessSettings)
        }
        .sheet(isPresented: $showingLoudnormSettings) {
            LoudnormSettingsView(settings: $loudnormSettings, isPresented: $showingLoudnormSettings)
        }

        .sheet(isPresented: $showingFFmpegSettings) {
            FFmpegSettingsView(isPresented: $showingFFmpegSettings)
        }
    }
    // MARK: - Recording Control Section
    private var recordingControlSection: some View {
        VStack(spacing: 12) {
            // Header with app title
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("WindsynthRecorder")
                        .font(.system(size: 18, weight: .semibold))
                    Text("专业电吹管录音工具")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }

                Spacer()

                HStack(spacing: 8) {
                    // FFmpeg 状态指示器
                    HStack(spacing: 4) {
                        Image(systemName: ffmpegManager.isFFmpegAvailable ? "checkmark.circle.fill" : "exclamationmark.triangle.fill")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundStyle(ffmpegManager.isFFmpegAvailable ? .green : .orange)

                        Button(action: {
                            showingFFmpegSettings = true
                        }) {
                            Text("FFmpeg")
                                .font(.system(size: 10, weight: .medium))
                                .foregroundStyle(ffmpegManager.isFFmpegAvailable ? .green : .orange)
                        }
                        .buttonStyle(.plain)
                        .help(ffmpegManager.isFFmpegAvailable ? "FFmpeg 可用 - 点击查看设置" : "FFmpeg 不可用 - 点击配置")
                    }

                    // VST 处理器按钮
                    Button(action: {
                        openWindow(id: WindowManager.WindowConfig.vstManager.id)
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: !audioGraphService.loadedPlugins.isEmpty ? "music.note.list" : "music.note.list")
                                .font(.system(size: 12, weight: .medium))
                            Text("VST 插件")
                                .font(.system(size: 11, weight: .medium))

                            // 显示已加载插件数量（绿色）或可用插件数量（蓝色）
                            if !audioGraphService.loadedPlugins.isEmpty {
                                Text("(\(audioGraphService.loadedPlugins.count))")
                                    .font(.system(size: 10, weight: .bold))
                                    .foregroundStyle(.green)
                            } else if !audioGraphService.availablePlugins.isEmpty {
                                Text("(\(audioGraphService.availablePlugins.count))")
                                    .font(.system(size: 10, weight: .bold))
                                    .foregroundStyle(.blue)
                            }
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(!audioGraphService.loadedPlugins.isEmpty ? Color.green.opacity(0.1) : (!audioGraphService.availablePlugins.isEmpty ? Color.blue.opacity(0.1) : Color.gray.opacity(0.1)), in: RoundedRectangle(cornerRadius: 4))
                        .foregroundStyle(!audioGraphService.loadedPlugins.isEmpty ? .green : (!audioGraphService.availablePlugins.isEmpty ? .blue : .gray))
                    }
                    .buttonStyle(.plain)
                    .help(!audioGraphService.loadedPlugins.isEmpty ? "VST 插件处理器 - 已加载 \(audioGraphService.loadedPlugins.count) 个插件" : (!audioGraphService.availablePlugins.isEmpty ? "VST 插件处理器 - 发现 \(audioGraphService.availablePlugins.count) 个可用插件" : "VST 插件处理器"))

                    // 音频混音台按钮
                    Button(action: {
                        openWindow(id: WindowManager.WindowConfig.audioMixer.id)
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "slider.horizontal.3")
                                .font(.system(size: 12, weight: .medium))
                            Text("混音台")
                                .font(.system(size: 11, weight: .medium))
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(Color.orange.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
                        .foregroundStyle(.orange)
                    }
                    .buttonStyle(.plain)
                    .help("音频混音台 - 实时播放和VST处理")

                    // 音频后处理按钮
                    Button(action: {
                        openWindow(id: WindowManager.WindowConfig.audioProcessor.id)
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "waveform.badge.plus")
                                .font(.system(size: 12, weight: .medium))
                            Text("音频处理")
                                .font(.system(size: 11, weight: .medium))
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(Color.purple.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
                        .foregroundStyle(.purple)
                    }
                    .buttonStyle(.plain)
                    .help("对已有音频文件进行后处理")
                }

                // Quick device status
                HStack(spacing: 12) {
                    deviceStatusIndicator(
                        title: "电吹管",
                        isConnected: audioDeviceManager.currentOutputDevice?.name == "SR_EWI-0964"
                    )

                    deviceStatusIndicator(
                        title: "录音线",
                        isConnected: audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" })
                    )
                }
            }

            Divider()

            // Recording controls and time display
            HStack(spacing: 20) {
                // Recording button and status
                VStack(alignment: .leading, spacing: 8) {
                    HStack(spacing: 12) {
                        Button(action: audioRecorder.recordingState == .idle ? startRecording : stopRecording) {
                            HStack(spacing: 8) {
                                Image(systemName: audioRecorder.recordingState == .recording ? "stop.circle.fill" : "record.circle")
                                    .font(.system(size: 18, weight: .medium))
                                    .foregroundStyle(audioRecorder.recordingState == .recording ? .red : .primary)

                                Text(audioRecorder.recordingState == .recording ? "停止录音" : "开始录音")
                                    .font(.system(size: 14, weight: .medium))
                            }
                            .padding(.horizontal, 16)
                            .padding(.vertical, 10)
                            .background(Color.primary.opacity(0.05), in: RoundedRectangle(cornerRadius: 8))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(audioRecorder.recordingState == .recording ? Color.red.opacity(0.3) : Color.primary.opacity(0.1), lineWidth: 1)
                            )
                        }
                        .buttonStyle(.plain)
                        .disabled(audioRecorder.recordingState == .processing)

                        if audioRecorder.recordingState == .recording {
                            HStack(spacing: 4) {
                                Circle()
                                    .fill(Color.red)
                                    .frame(width: 6, height: 6)
                                    .opacity(0.8)
                                    .animation(.easeInOut(duration: 0.8).repeatForever(autoreverses: true), value: audioRecorder.recordingState)

                                Text("REC")
                                    .font(.system(size: 11, weight: .bold))
                                    .foregroundStyle(.red)
                            }
                        }
                    }

                    Text(recordingStatusText)
                        .font(.system(size: 11))
                        .foregroundStyle(.secondary)
                }

                Spacer()

                // Time display
                VStack(alignment: .trailing, spacing: 4) {
                    Text("录音时长")
                        .font(.system(size: 11, weight: .medium))
                        .foregroundStyle(.secondary)

                    Text(timeString(from: audioRecorder.currentTime))
                        .font(.system(size: 24, design: .monospaced))
                        .fontWeight(.medium)
                        .foregroundStyle(audioRecorder.recordingState == .recording ? .red : .primary)
                }
            }

            // Recording actions (when available)
            if audioRecorder.recordingState == .idle && audioRecorder.temporaryAudioURL != nil {
                HStack(spacing: 12) {
                    Button(action: {
                        fileName = defaultFileName()
                        showingSaveDialog = true
                    }) {
                        HStack(spacing: 6) {
                            Image(systemName: "square.and.arrow.down")
                                .font(.system(size: 12, weight: .medium))
                            Text("保存录音")
                                .font(.system(size: 12, weight: .medium))
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(Color.green.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))
                        .foregroundStyle(.green)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(Color.green.opacity(0.3), lineWidth: 1)
                        )
                    }
                    .buttonStyle(.plain)

                    Button(action: audioRecorder.deleteRecording) {
                        HStack(spacing: 6) {
                            Image(systemName: "trash")
                                .font(.system(size: 12, weight: .medium))
                            Text("删除录音")
                                .font(.system(size: 12, weight: .medium))
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(Color.red.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))
                        .foregroundStyle(.red)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(Color.red.opacity(0.3), lineWidth: 1)
                        )
                    }
                    .buttonStyle(.plain)

                    Spacer()
                }
            }

            // Processing indicator
            if audioRecorder.recordingState == .processing {
                HStack(spacing: 12) {
                    ProgressView()
                        .progressViewStyle(.circular)
                        .scaleEffect(0.8)

                    VStack(alignment: .leading, spacing: 2) {
                        Text("正在处理音频文件...")
                            .font(.system(size: 12, weight: .medium))
                        Text("请稍候，这可能需要一些时间")
                            .font(.system(size: 10))
                            .foregroundStyle(.secondary)
                    }

                    Spacer()
                }
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }

    private func deviceStatusIndicator(title: String, isConnected: Bool) -> some View {
        HStack(spacing: 4) {
            Circle()
                .fill(isConnected ? Color.green : Color.red)
                .frame(width: 6, height: 6)

            Text(title)
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(isConnected ? .green : .red)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(isConnected ? Color.green.opacity(0.1) : Color.red.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
    }

    // MARK: - Device Status Section
    private var deviceStatusSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("音频设备管理")
                    .font(.system(size: 15, weight: .semibold))

                Spacer()

                Button(action: checkDevices) {
                    Image(systemName: "arrow.clockwise")
                        .font(.system(size: 12, weight: .medium))
                }
                .buttonStyle(.bordered)
                .controlSize(.mini)
                .help("刷新设备状态")

                Button(action: { showingDeviceList = true }) {
                    HStack(spacing: 4) {
                        Image(systemName: "list.bullet")
                            .font(.system(size: 11, weight: .medium))
                        Text("查看全部")
                            .font(.system(size: 11, weight: .medium))
                    }
                }
                .buttonStyle(.bordered)
                .controlSize(.mini)

                Button(action: {
                    openWindow(id: WindowManager.WindowConfig.logs.id)
                }) {
                    HStack(spacing: 4) {
                        Image(systemName: "doc.text")
                            .font(.system(size: 11, weight: .medium))
                        Text("查看日志")
                            .font(.system(size: 11, weight: .medium))
                    }
                }
                .buttonStyle(.bordered)
                .controlSize(.mini)
                .help("查看系统日志和命令执行记录")
            }

            HStack(spacing: 16) {
                // Output Device
                VStack(alignment: .leading, spacing: 8) {
                    Text("输出设备")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    HStack(spacing: 10) {
                        Image(systemName: "speaker.wave.3")
                            .font(.system(size: 14, weight: .medium))
                            .foregroundStyle(.blue)
                            .frame(width: 20)

                        VStack(alignment: .leading, spacing: 1) {
                            Text("电吹管蓝牙")
                                .font(.system(size: 13, weight: .medium))

                            Text(audioDeviceManager.currentOutputDevice?.name ?? "未连接")
                                .font(.system(size: 11))
                                .foregroundStyle(.secondary)
                                .lineLimit(1)
                                .truncationMode(.middle)
                        }

                        Spacer()

                        Button(action: {
                            let result = audioDeviceManager.checkAndSetupSRBlueDevice()
                            if !result.success {
                                notificationManager.showError(message: result.message)
                            } else {
                                notificationManager.showSuccess(message: result.message)
                            }
                        }) {
                            Text("设为默认")
                                .font(.system(size: 10, weight: .medium))
                        }
                        .buttonStyle(.borderedProminent)
                        // .controlSize(.mini)
                        .disabled(audioDeviceManager.currentOutputDevice?.name == "SR_EWI-0964")
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(audioDeviceManager.currentOutputDevice?.name == "SR_EWI-0964" ? Color.green.opacity(0.3) : Color.primary.opacity(0.08), lineWidth: 1)
                    )
                }

                // Input Device
                VStack(alignment: .leading, spacing: 8) {
                    Text("输入设备")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    HStack(spacing: 10) {
                        Image(systemName: "mic")
                            .font(.system(size: 14, weight: .medium))
                            .foregroundStyle(.red)
                            .frame(width: 20)

                        VStack(alignment: .leading, spacing: 1) {
                            Text("电吹管录音线")
                                .font(.system(size: 13, weight: .medium))

                            Text(audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" }) ? "SR-REC" : "未连接")
                                .font(.system(size: 11))
                                .foregroundStyle(.secondary)
                        }

                        Spacer()

                        HStack(spacing: 4) {
                            Circle()
                                .fill(audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" }) ? Color.green : Color.red)
                                .frame(width: 6, height: 6)

                            Text(audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" }) ? "已连接" : "未连接")
                                .font(.system(size: 10, weight: .medium))
                                .foregroundStyle(audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" }) ? .green : .red)
                        }
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(audioDeviceManager.availableInputDevices.contains(where: { $0.name == "SR-REC" }) ? Color.green.opacity(0.3) : Color.primary.opacity(0.08), lineWidth: 1)
                    )
                }
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Audio Settings Section
    private var audioSettingsSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("音频设置")
                    .font(.system(size: 15, weight: .semibold))

                Spacer()

                Button("重置为默认") {
                    resetAudioSettings()
                }
                .font(.system(size: 11))
                .buttonStyle(.bordered)
                .controlSize(.mini)
            }

            HStack(spacing: 16) {
                // Sample Rate
                VStack(alignment: .leading, spacing: 6) {
                    Text("采样率")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Picker("采样率", selection: $selectedSampleRate) {
                        ForEach(SampleRate.allCases, id: \.self) { rate in
                            Text(rate.rawValue).tag(rate)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 200)
                }

                // Bit Depth
                VStack(alignment: .leading, spacing: 6) {
                    Text("位深度")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Picker("位深度", selection: $selectedBitDepth) {
                        ForEach(BitDepth.allCases, id: \.self) { depth in
                            Text(depth.rawValue).tag(depth)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 200)
                }

                // File Format
                VStack(alignment: .leading, spacing: 6) {
                    Text("文件格式")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.secondary)

                    Picker("文件格式", selection: $selectedFileFormat) {
                        ForEach(FileFormat.allCases, id: \.self) { format in
                            Text(format.rawValue).tag(format)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 200)
                }

                Spacer()
            }

            // Audio Processing Options
            VStack(spacing: 8) {
                HStack {
                    Toggle(isOn: $autoNormalize) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("自动标准化")
                                .font(.system(size: 12, weight: .medium))
                            Text("自动调整音频电平到最佳范围")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }
                    }
                    .toggleStyle(.switch)

                    Spacer()
                }

                HStack {
                    Toggle(isOn: $enableNoiseGate) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("噪声门")
                                .font(.system(size: 12, weight: .medium))
                            Text("自动消除低音量背景噪声")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }
                    }
                    .toggleStyle(.switch)

                    if enableNoiseGate {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("阈值: \(Int(noiseGateThreshold)) dB")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)

                            Slider(value: $noiseGateThreshold, in: -60...(-20), step: 1)
                                .frame(width: 100)
                        }
                    }

                    Spacer()
                }

                // 响度最大化设置
                VStack(spacing: 8) {
                    HStack {
                        Toggle(isOn: $maximizeLoudness) {
                            VStack(alignment: .leading, spacing: 1) {
                                Text("响度最大化")
                                    .font(.system(size: 12, weight: .medium))
                                Text("提升音频音量到最佳水平（推荐）")
                                    .font(.system(size: 10))
                                    .foregroundStyle(.secondary)
                            }
                        }
                        .toggleStyle(.switch)

                        Spacer()
                    }

                    if maximizeLoudness {
                        VStack(spacing: 8) {
                            // 响度处理方法选择
                            HStack {
                                Text("处理方法:")
                                    .font(.system(size: 11, weight: .medium))
                                    .foregroundStyle(.secondary)

                                Picker("", selection: $loudnessMethod) {
                                    ForEach(LoudnessMethod.allCases, id: \.self) { method in
                                        Text(method.rawValue).tag(method)
                                    }
                                }
                                .pickerStyle(.menu)
                                .frame(width: 120)

                                Spacer()
                            }

                            // 方法描述
                            HStack {
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(loudnessMethod.description)
                                        .font(.system(size: 10))
                                        .foregroundStyle(.blue)

                                    if loudnessMethod == .dynaudnorm {
                                        Button("高级设置") {
                                            showingLoudnessSettings = true
                                        }
                                        .font(.system(size: 9))
                                        .buttonStyle(.bordered)
                                        .controlSize(.mini)
                                    } else if loudnessMethod == .loudnorm {
                                        Button("高级设置") {
                                            showingLoudnormSettings = true
                                        }
                                        .font(.system(size: 9))
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
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(Color.blue.opacity(0.2), lineWidth: 1)
                        )
                    }
                }
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - File Settings Section
    private var fileSettingsSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("文件设置")
                    .font(.system(size: 15, weight: .semibold))

                Spacer()
            }

            VStack(spacing: 12) {
                // Save Location
                HStack {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("保存位置")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundStyle(.secondary)

                        Text(saveLocation.path)
                            .font(.system(size: 11))
                            .foregroundStyle(.primary)
                            .lineLimit(1)
                            .truncationMode(.middle)
                    }

                    Spacer()

                    Button("选择文件夹") {
                        selectSaveLocation()
                    }
                    .font(.system(size: 11))
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))

                // File Naming
                HStack {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("文件命名")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundStyle(.secondary)

                        Text("格式: Recording_YYYY-MM-DD_HH-MM-SS")
                            .font(.system(size: 11))
                            .foregroundStyle(.primary)
                    }

                    Spacer()

                    Button("预览") {
                        let preview = defaultFileName()
                        notificationManager.showSuccess(message: "文件名预览: \(preview).\(selectedFileFormat.fileExtension)")
                    }
                    .font(.system(size: 11))
                    .buttonStyle(.bordered)
                    .controlSize(.mini)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 6))

                // Auto-save option
                HStack {
                    Toggle(isOn: .constant(false)) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("录音完成后自动保存")
                                .font(.system(size: 12, weight: .medium))
                            Text("跳过保存对话框，直接保存到指定位置")
                                .font(.system(size: 10))
                                .foregroundStyle(.secondary)
                        }
                    }
                    .toggleStyle(.switch)
                    .disabled(true) // 暂时禁用，未来功能

                    Spacer()
                }
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 10))
    }


    
    // MARK: - Helper Functions
    private func resetAudioSettings() {
        selectedSampleRate = .rate44100
        selectedBitDepth = .bit16
        selectedFileFormat = .wav
        autoNormalize = false
        enableNoiseGate = false
        noiseGateThreshold = -40.0
    }

    private func selectSaveLocation() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.prompt = "选择保存位置"

        if panel.runModal() == .OK {
            if let url = panel.url {
                saveLocation = url
            }
        }
    }



    private var recordingStatusText: String {
        switch audioRecorder.recordingState {
        case .idle:
            return audioRecorder.temporaryAudioURL != nil ? "录音已完成，可以保存或删除" : "点击录音按钮开始"
        case .recording:
            return "录音进行中，点击停止按钮结束"
        case .processing:
            return "正在处理音频文件..."
        }
    }


    
    private var modernSaveDialog: some View {
        VStack(spacing: 20) {
            // Header
            VStack(spacing: 6) {
                Text("保存录音")
                    .font(.system(size: 16, weight: .semibold))

                Text("确认文件设置并保存录音")
                    .font(.system(size: 12))
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }

            // Form
            VStack(spacing: 16) {
                VStack(alignment: .leading, spacing: 6) {
                    Text("文件名")
                        .font(.system(size: 13, weight: .medium))

                    TextField("输入文件名", text: $fileName)
                        .textFieldStyle(.roundedBorder)
                        .font(.system(size: 13))
                }

                // File format info
                VStack(alignment: .leading, spacing: 6) {
                    Text("文件格式")
                        .font(.system(size: 13, weight: .medium))

                    HStack {
                        Text("\(selectedFileFormat.rawValue) • \(selectedSampleRate.rawValue) • \(selectedBitDepth.rawValue)")
                            .font(.system(size: 12))
                            .foregroundStyle(.secondary)

                        Spacer()
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color.primary.opacity(0.05), in: RoundedRectangle(cornerRadius: 6))
                }

                VStack(alignment: .leading, spacing: 6) {
                    Text("保存设置")
                        .font(.system(size: 13, weight: .medium))

                    Text("文件将使用上述音频设置进行处理和保存")
                        .font(.system(size: 11))
                        .foregroundStyle(.secondary)
                }
            }
            .padding(16)
            .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 8))

            // Actions
            HStack(spacing: 12) {
                Button("取消") {
                    showingSaveDialog = false
                }
                .buttonStyle(.bordered)
                .keyboardShortcut(.cancelAction)

                Button("保存录音") {
                    if !fileName.isEmpty {
                        saveRecordingWithCurrentSettings()
                        showingSaveDialog = false
                        fileName = ""
                    }
                }
                .buttonStyle(.borderedProminent)
                .keyboardShortcut(.return, modifiers: [])
                .disabled(fileName.isEmpty)
            }
        }
        .padding(24)
        .frame(width: 420)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
    }

    private func saveRecordingWithCurrentSettings() {
        // 使用当前的响度最大化设置来保存文件
        audioRecorder.saveRecording(fileName: fileName, maximizeLoudness: maximizeLoudness, loudnessMethod: loudnessMethod, loudnessSettings: loudnessSettings, loudnormSettings: loudnormSettings)
    }
    
    private var notificationOverlay: some View {
        GeometryReader { geometry in
            if notificationManager.isShowing {
                VStack {
                    Spacer()
                    HStack(spacing: 12) {
                        Image(systemName: notificationManager.isError ? "exclamationmark.triangle.fill" : "checkmark.circle.fill")
                            .font(.system(size: 16, weight: .medium))
                            .foregroundStyle(.white)

                        Text(notificationManager.message)
                            .font(.system(size: 14, weight: .medium))
                            .foregroundStyle(.white)
                    }
                    .padding(.horizontal, 20)
                    .padding(.vertical, 12)
                    .background(
                        notificationManager.isError ? Color.red.gradient : Color.green.gradient,
                        in: RoundedRectangle(cornerRadius: 12)
                    )
                    .shadow(color: .black.opacity(0.1), radius: 8, x: 0, y: 4)
                    .padding(.bottom, 32)
                }
                .frame(maxWidth: .infinity)
                .transition(.move(edge: .bottom).combined(with: .opacity))
                .animation(.spring(response: 0.5, dampingFraction: 0.8), value: notificationManager.isShowing)
            }
        }
        .allowsHitTesting(false)
    }
    
    private var modernDeviceListView: some View {
        VStack(spacing: 20) {
            // Header
            VStack(spacing: 6) {
                Text("系统音频设备")
                    .font(.system(size: 16, weight: .semibold))

                Text("查看所有可用的音频输入和输出设备")
                    .font(.system(size: 12))
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }

            ScrollView {
                VStack(spacing: 16) {
                    // Output Devices Section
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Image(systemName: "speaker.wave.3")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.blue)

                            Text("输出设备")
                                .font(.system(size: 14, weight: .medium))

                            Spacer()
                        }

                        LazyVStack(spacing: 4) {
                            ForEach(audioDeviceManager.availableOutputDevices) { device in
                                CompactDeviceRow(
                                    name: device.name,
                                    icon: "speaker.wave.3",
                                    isActive: audioDeviceManager.currentOutputDevice?.id == device.id
                                )
                            }
                        }
                    }

                    Divider()

                    // Input Devices Section
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Image(systemName: "mic")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.red)

                            Text("输入设备")
                                .font(.system(size: 14, weight: .medium))

                            Spacer()
                        }

                        LazyVStack(spacing: 4) {
                            ForEach(audioDeviceManager.availableInputDevices) { device in
                                CompactDeviceRow(
                                    name: device.name,
                                    icon: "mic",
                                    isActive: audioDeviceManager.currentInputDevice?.id == device.id
                                )
                            }
                        }
                    }
                }
                .padding(16)
            }
            .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 8))
            .frame(maxHeight: 350)

            // Close Button
            Button("关闭") {
                showingDeviceList = false
            }
            .buttonStyle(.borderedProminent)
            .keyboardShortcut(.return, modifiers: [])
        }
        .padding(24)
        .frame(width: 450)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
    }

    private struct CompactDeviceRow: View {
        let name: String
        let icon: String
        let isActive: Bool

        var body: some View {
            HStack(spacing: 8) {
                Image(systemName: icon)
                    .font(.system(size: 12, weight: .medium))
                    .foregroundStyle(isActive ? .green : .secondary)
                    .frame(width: 16)

                Text(name)
                    .font(.system(size: 12, weight: .medium))
                    .lineLimit(1)
                    .truncationMode(.middle)

                Spacer()

                if isActive {
                    HStack(spacing: 3) {
                        Circle()
                            .fill(Color.green)
                            .frame(width: 4, height: 4)

                        Text("当前")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundStyle(.green)
                    }
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(isActive ? Color.green.opacity(0.08) : Color.clear, in: RoundedRectangle(cornerRadius: 4))
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(isActive ? Color.green.opacity(0.2) : Color.clear, lineWidth: 1)
            )
        }
    }

    // MARK: - Helper Functions

    private func timeString(from timeInterval: TimeInterval) -> String {
        let minutes = Int(timeInterval) / 60
        let seconds = Int(timeInterval) % 60
        let milliseconds = Int((timeInterval.truncatingRemainder(dividingBy: 1)) * 10)
        return String(format: "%02d:%02d.%01d", minutes, seconds, milliseconds)
    }
    
    private func defaultFileName() -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        return "Recording_\(formatter.string(from: Date()))"
    }
    
    private func startAutoRefresh() {
        // 每3秒自动刷新一次设备状态
        autoRefreshTimer = Timer.scheduledTimer(withTimeInterval: 3.0, repeats: true) { _ in
            audioDeviceManager.refreshDeviceList()
        }
    }
    
    private func stopAutoRefresh() {
        autoRefreshTimer?.invalidate()
        autoRefreshTimer = nil
    }
    
    private func checkDevices() {
        AudioProcessingLogger.shared.info("开始设备检查", details: "刷新设备列表并检查 SR 设备状态")

        audioDeviceManager.refreshDeviceList()
        let (success, message) = audioDeviceManager.checkAndSetupSRBlueDevice()
        if !success {
            notificationManager.showError(message: message)
        }

        let (available, recMessage) = audioDeviceManager.checkSRRecDevice()
        if !available {
            notificationManager.showError(message: recMessage)
        }

        AudioProcessingLogger.shared.info("设备检查完成", details: "SR Blue: \(success ? "成功" : "失败"), SR-REC: \(available ? "可用" : "不可用")")
    }
    
    private func startRecording() {
        AudioProcessingLogger.shared.info("用户启动录音", details: "开始录音流程")
        checkDevices()
        // Reset file name for new recording
        fileName = ""
        maximizeLoudness = false
        audioRecorder.startRecording()
    }
    
    private func stopRecording() {
        AudioProcessingLogger.shared.info("用户停止录音", details: "停止录音流程")
        audioRecorder.stopRecording()
    }
} 


struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
} 
