import Foundation
import AVFAudio
import CoreAudio
import AVFoundation

enum RecordingState {
    case idle
    case recording
    case processing
}

class AudioRecorder: NSObject, ObservableObject {
    static let shared = AudioRecorder()

    @Published var recordingState: RecordingState = .idle
    @Published var currentTime: TimeInterval = 0
    @Published var vstProcessingEnabled: Bool = false
    @Published var vstProcessingActive: Bool = false

    private var audioEngine: AVAudioEngine?
    private var audioFile: AVAudioFile?
    private var timer: Timer?
    var temporaryAudioURL: URL?

    // VST 处理相关
    private var vstManager: VSTManagerExample?
    private var audioBuffer: UnsafeMutablePointer<Float>?
    private var bufferSize: Int = 0
    
    private override init() {
        super.init()
        setupAudioEngine()
    }
    
    private func setupAudioEngine() {
        audioEngine = AVAudioEngine()
    }
    
    func startRecording() {
        guard recordingState == .idle else { return }

        AudioProcessingLogger.shared.info("开始录音流程", details: "检查 SR-REC 设备可用性")

        guard let srRecDevice = AudioDeviceManager.shared.availableInputDevices.first(where: { $0.name == "SR-REC" }) else {
            AudioProcessingLogger.shared.error("录音启动失败", details: "未找到 SR-REC 设备")
            NotificationManager.shared.showError(message: "未找到 SR-REC 设备（音频线是否已连接？）")
            return
        }

        AudioProcessingLogger.shared.info("找到 SR-REC 设备", details: "设备ID: \(srRecDevice.id)")

        if !AudioDeviceManager.shared.setDefaultInputDevice(deviceID: srRecDevice.id) {
            AudioProcessingLogger.shared.error("录音启动失败", details: "无法设置 SR-REC 为输入设备")
            NotificationManager.shared.showError(message: "无法设置 SR-REC 为输入设备")
            return
        }

        AudioProcessingLogger.shared.success("音频设备设置成功", details: "SR-REC 已设为默认输入设备")
        
        do {
            audioEngine = AVAudioEngine()
            guard let audioEngine = audioEngine else {
                AudioProcessingLogger.shared.error("录音启动失败", details: "无法创建音频引擎")
                return
            }

            let inputNode = audioEngine.inputNode
            let format = inputNode.outputFormat(forBus: 0)

            let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            temporaryAudioURL = documentsPath.appendingPathComponent("temp_recording.wav")

            AudioProcessingLogger.shared.info("设置录音文件", details: "临时文件路径: \(temporaryAudioURL?.path ?? "unknown")\n音频格式: \(format.description)")

            try? FileManager.default.removeItem(at: temporaryAudioURL!)

            let settings = inputNode.outputFormat(forBus: 0).settings

            audioFile = try AVAudioFile(forWriting: temporaryAudioURL!, settings: settings)

            inputNode.installTap(onBus: 0, bufferSize: 1024, format: format) { [weak self] (buffer, time) in
                try? self?.audioFile?.write(from: buffer)
            }

            try audioEngine.start()
            recordingState = .recording
            currentTime = 0
            startTimer()

            AudioProcessingLogger.shared.success("录音已开始", details: "音频引擎已启动，开始录制音频数据")

        } catch {
            AudioProcessingLogger.shared.error("录音启动异常", details: error.localizedDescription)
            NotificationManager.shared.showError(message: "录音启动失败: \(error.localizedDescription)")
        }
    }
    
    func stopRecording() {
        AudioProcessingLogger.shared.info("停止录音", details: "正在停止音频引擎和保存文件")

        audioEngine?.stop()
        audioEngine?.inputNode.removeTap(onBus: 0)
        audioFile = nil // Close the file
        stopTimer()
        recordingState = .idle

        if let url = temporaryAudioURL {
            do {
                let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
                let fileSize = attributes[.size] as? UInt64 ?? 0
                let fileSizeMB = Double(fileSize) / (1024 * 1024)
                AudioProcessingLogger.shared.success("录音已停止", details: "文件路径: \(url.path)\n文件大小: \(fileSize) 字节 (\(String(format: "%.2f", fileSizeMB)) MB)")
            } catch {
                AudioProcessingLogger.shared.warning("录音已停止", details: "无法获取文件信息: \(error.localizedDescription)")
            }
        } else {
            AudioProcessingLogger.shared.warning("录音已停止", details: "临时文件路径为空")
        }
    }
    
    func saveRecording(fileName: String, maximizeLoudness: Bool, loudnessMethod: ContentView.LoudnessMethod = .aggressive, loudnessSettings: LoudnessSettings = LoudnessSettings(), loudnormSettings: LoudnormSettings = LoudnormSettings()) {
        guard let sourceURL = temporaryAudioURL else { return }

        recordingState = .processing

        let destinationURL = getFinalDestinationURL(fileName: fileName)
        convertToMP3(sourceURL: sourceURL, destinationURL: destinationURL, maximizeLoudness: maximizeLoudness, loudnessMethod: loudnessMethod, loudnessSettings: loudnessSettings, loudnormSettings: loudnormSettings)
    }
    
    private func getFinalDestinationURL(fileName: String) -> URL {
        let documentsPath = FileManager.default.urls(for: .desktopDirectory, in: .userDomainMask)[0]
        return documentsPath.appendingPathComponent("\(fileName).mp3")
    }

    func deleteRecording() {
        if let url = temporaryAudioURL {
            try? FileManager.default.removeItem(at: url)
        }
        temporaryAudioURL = nil
        currentTime = 0
    }
    

    
    private func convertToMP3(sourceURL: URL, destinationURL: URL, maximizeLoudness: Bool = false, loudnessMethod: ContentView.LoudnessMethod = .aggressive, loudnessSettings: LoudnessSettings = LoudnessSettings(), loudnormSettings: LoudnormSettings = LoudnormSettings()) {
        let process = Process()
        let logger = AudioProcessingLogger.shared
        let ffmpegManager = FFmpegManager.shared

        guard let ffmpegPath = ffmpegManager.getFFmpegPath() else {
            logger.error("FFmpeg 不可用", details: "请在设置中配置 FFmpeg 路径")
            NotificationManager.shared.showError(message: "FFmpeg 不可用，请检查设置")
            DispatchQueue.main.async {
                self.recordingState = .idle
            }
            return
        }

        logger.info("开始录音文件转换", details: "源文件: \(sourceURL.lastPathComponent)\n目标文件: \(destinationURL.lastPathComponent)")

        process.executableURL = URL(fileURLWithPath: ffmpegPath)

        // Build arguments based on whether loudness maximization is enabled
        var arguments = [
            "-i", sourceURL.path,
            "-y" // Overwrite output file if it exists
        ]

        if maximizeLoudness {
            let loudnessFilter = loudnessMethod.generateFilter(loudnessSettings: loudnessSettings, loudnormSettings: loudnormSettings)
            arguments.append(contentsOf: [
                "-af", loudnessFilter
            ])
            logger.info("应用响度最大化", details: "滤镜: \(loudnessFilter)\n方法: \(loudnessMethod.rawValue)")
        }

        // Add encoding parameters
        arguments.append(contentsOf: [
            "-acodec", "libmp3lame",
            "-q:a", "0", // Highest quality VBR
            "-ar", "44100",
            destinationURL.path
        ])

        process.arguments = arguments

        let fullCommand = "ffmpeg " + arguments.joined(separator: " ")
        logger.info("执行 FFmpeg 命令", details: fullCommand)

        // 设置管道来捕获错误输出
        let errorPipe = Pipe()
        process.standardError = errorPipe

        do {
            logger.info("开始执行 FFmpeg 进程", details: "PID 将在进程启动后分配")
            try process.run()
            logger.info("FFmpeg 进程已启动", details: "PID: \(process.processIdentifier)")
            process.waitUntilExit()

            // 读取错误输出
            let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
            let errorOutput = String(data: errorData, encoding: .utf8) ?? ""

            DispatchQueue.main.async {
                if process.terminationStatus == 0 {
                    let message = maximizeLoudness ? "录音已保存到桌面（已应用响度最大化）" : "录音已保存到桌面"
                    logger.success("FFmpeg 命令执行成功", details: "命令: \(fullCommand)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)\n文件: \(destinationURL.lastPathComponent)\n位置: \(destinationURL.path)")
                    NotificationManager.shared.showSuccess(message: message)
                    // Clean up original temp file after successful conversion
                    try? FileManager.default.removeItem(at: self.temporaryAudioURL!)
                    self.temporaryAudioURL = nil
                    self.currentTime = 0
                } else {
                    logger.error("FFmpeg 命令执行失败", details: "命令: \(fullCommand)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)\nFFmpeg 错误输出:\n\(errorOutput)")
                    NotificationManager.shared.showError(message: "MP3 转换失败")
                }
                self.recordingState = .idle
            }
        } catch {
            DispatchQueue.main.async {
                logger.error("FFmpeg 进程启动异常", details: "命令: \(fullCommand)\n错误: \(error.localizedDescription)")
                NotificationManager.shared.showError(message: "MP3 转换失败: \(error.localizedDescription)")
                self.recordingState = .idle
            }
        }
    }
    
    private func startTimer() {
        stopTimer()
        timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            if self.recordingState == .recording {
                self.currentTime += 0.1
            }
        }
    }
    
    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
} 
