//
//  AudioMixerService.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频混音台服务 - 支持音频文件播放和VST实时处理
//

import Foundation
import AVFoundation
import Combine

/// 播放状态枚举
enum PlaybackState {
    case stopped
    case playing
    case paused
    case loading
}

/// 音频混音台配置
struct AudioMixerConfig {
    var sampleRate: Double = 44100.0
    var bufferSize: Int = 512
    var numChannels: Int = 2
    var enableVSTProcessing: Bool = true
    var monitoringEnabled: Bool = true
    var outputGain: Float = 1.0
}

/// 音频混音台服务 - 独立的音频播放和VST处理系统
class AudioMixerService: NSObject, ObservableObject {
    
    // MARK: - Published Properties
    
    @Published var playbackState: PlaybackState = .stopped
    @Published var currentTime: TimeInterval = 0
    @Published var duration: TimeInterval = 0
    @Published var isVSTProcessingEnabled: Bool = false
    @Published var outputLevel: Float = 0.0
    @Published var currentFileName: String = ""
    @Published var errorMessage: String?
    
    // MARK: - Private Properties
    
    // JUCE 音频引擎
    private var juceAudioEngine: JUCEAudioEngine

    // 保留 AVFoundation 用于文件信息读取
    private var audioFile: AVAudioFile?
    private var vstManager: VSTManagerExample
    private var config: AudioMixerConfig
    
    // 定时器和监控
    private var playbackTimer: Timer?
    private var levelTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    // MARK: - Initialization
    
    override init() {
        self.juceAudioEngine = JUCEAudioEngine()
        self.vstManager = VSTManagerExample.shared
        self.config = AudioMixerConfig()

        super.init()

        setupObservers()
        syncWithJUCEEngine()
    }
    
    deinit {
        cleanup()
    }
    
    // MARK: - Setup Methods
    
    private func syncWithJUCEEngine() {
        // 同步 JUCE 引擎的状态到 AudioMixerService
        juceAudioEngine.$playbackState
            .sink { [weak self] state in
                self?.playbackState = state
            }
            .store(in: &cancellables)

        juceAudioEngine.$currentTime
            .sink { [weak self] time in
                self?.currentTime = time
            }
            .store(in: &cancellables)

        juceAudioEngine.$duration
            .sink { [weak self] duration in
                self?.duration = duration
            }
            .store(in: &cancellables)

        juceAudioEngine.$isVSTProcessingEnabled
            .sink { [weak self] enabled in
                self?.isVSTProcessingEnabled = enabled
            }
            .store(in: &cancellables)

        juceAudioEngine.$outputLevel
            .sink { [weak self] level in
                self?.outputLevel = level
            }
            .store(in: &cancellables)

        juceAudioEngine.$currentFileName
            .sink { [weak self] fileName in
                self?.currentFileName = fileName
            }
            .store(in: &cancellables)

        juceAudioEngine.$errorMessage
            .sink { [weak self] error in
                self?.errorMessage = error
            }
            .store(in: &cancellables)

        print("✅ JUCE Audio Engine synchronized with AudioMixerService")
    }
    
    private func setupObservers() {
        // 移除循环依赖 - 不要在这里监听 isVSTProcessingEnabled
        // VST 状态变化应该通过直接调用方法来处理，而不是通过 Publisher
    }
    
    // MARK: - Public Methods
    
    /// 加载音频文件
    func loadAudioFile(url: URL) {
        // 委托给 JUCE 音频引擎
        juceAudioEngine.loadAudioFile(from: url)

        // 同时保留 AVAudioFile 用于兼容性
        do {
            audioFile = try AVAudioFile(forReading: url)
        } catch {
            print("⚠️ Failed to create AVAudioFile for compatibility: \(error)")
        }
    }
    
    /// 开始播放
    func play() {
        juceAudioEngine.play()
    }

    /// 暂停播放
    func pause() {
        juceAudioEngine.pause()
    }
    
    /// 停止播放
    func stop() {
        juceAudioEngine.stop()
    }

    /// 跳转到指定时间
    func seek(to time: TimeInterval) {
        juceAudioEngine.seek(to: time)
    }

    /// 设置输出音量
    func setOutputGain(_ gain: Float) {
        config.outputGain = max(0.0, min(2.0, gain))
        // TODO: 实现 JUCE 音频引擎的音量控制
    }

    /// 获取VST管理器
    func getVSTManager() -> VSTManagerExample {
        return juceAudioEngine.getVSTManager()
    }

    /// 启动实时音频处理（不需要播放文件）
    func startRealtimeProcessing() {
        juceAudioEngine.startRealtimeProcessing()
    }

    /// 停止实时音频处理
    func stopRealtimeProcessing() {
        juceAudioEngine.stopRealtimeProcessing()
    }
    
    // MARK: - Private Methods

    private func cleanup() {
        // JUCE 引擎会自动清理
        cancellables.removeAll()
        print("🗑️ Audio mixer service cleaned up")
    }


}
