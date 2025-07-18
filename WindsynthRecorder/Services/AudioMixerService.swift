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
    
    private var audioEngine: AVAudioEngine
    private var audioPlayerNode: AVAudioPlayerNode
    private var audioFile: AVAudioFile?
    private var vstManager: VSTManagerExample
    private var config: AudioMixerConfig
    
    // 音频处理相关
    private var audioBuffer: UnsafeMutablePointer<Float>?
    private var bufferSize: Int = 0
    private var isProcessingSetup: Bool = false
    
    // 定时器和监控
    private var playbackTimer: Timer?
    private var levelTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    // MARK: - Initialization
    
    override init() {
        self.audioEngine = AVAudioEngine()
        self.audioPlayerNode = AVAudioPlayerNode()
        self.vstManager = VSTManagerExample.shared
        self.config = AudioMixerConfig()
        
        super.init()
        
        setupAudioEngine()
        setupVSTProcessing()
        setupObservers()
    }
    
    deinit {
        cleanup()
    }
    
    // MARK: - Setup Methods
    
    private func setupAudioEngine() {
        // 添加播放器节点到音频引擎
        audioEngine.attach(audioPlayerNode)
        
        // 连接播放器节点到主混音器
        let mainMixer = audioEngine.mainMixerNode
        audioEngine.connect(audioPlayerNode, to: mainMixer, format: nil)
        
        // 准备音频引擎
        audioEngine.prepare()
        
        print("Audio engine setup completed")
    }
    
    private func setupVSTProcessing() {
        // 配置VST音频处理
        vstManager.configureAudioProcessing(
            sampleRate: config.sampleRate,
            samplesPerBlock: config.bufferSize,
            numChannels: config.numChannels
        )
        
        print("VST processing setup completed")
    }
    
    private func setupObservers() {
        // 监听VST管理器状态变化
        vstManager.$loadedPlugins
            .sink { [weak self] plugins in
                DispatchQueue.main.async {
                    self?.isVSTProcessingEnabled = !plugins.isEmpty
                }
            }
            .store(in: &cancellables)
        
        vstManager.$errorMessage
            .sink { [weak self] error in
                DispatchQueue.main.async {
                    self?.errorMessage = error
                }
            }
            .store(in: &cancellables)
    }
    
    // MARK: - Public Methods
    
    /// 加载音频文件
    func loadAudioFile(url: URL) {
        do {
            // 停止当前播放
            stop()
            
            playbackState = .loading
            
            // 加载音频文件
            audioFile = try AVAudioFile(forReading: url)
            guard let file = audioFile else {
                throw NSError(domain: "AudioMixerService", code: 1, 
                            userInfo: [NSLocalizedDescriptionKey: "无法加载音频文件"])
            }
            
            // 更新文件信息
            duration = Double(file.length) / file.fileFormat.sampleRate
            currentFileName = url.lastPathComponent
            currentTime = 0
            
            // 设置音频格式
            let format = file.processingFormat
            audioEngine.connect(audioPlayerNode, to: audioEngine.mainMixerNode, format: format)
            
            // 如果启用VST处理，设置音频tap
            if config.enableVSTProcessing {
                setupVSTAudioTap(format: format)
            }
            
            playbackState = .stopped
            
            print("Audio file loaded: \(url.lastPathComponent), duration: \(duration)s")
            
        } catch {
            errorMessage = "加载音频文件失败: \(error.localizedDescription)"
            playbackState = .stopped
            print("Failed to load audio file: \(error)")
        }
    }
    
    /// 开始播放
    func play() {
        guard let file = audioFile, playbackState != .playing else { return }
        
        do {
            // 启动音频引擎
            if !audioEngine.isRunning {
                try audioEngine.start()
            }
            
            // 如果是暂停状态，直接继续播放
            if playbackState == .paused {
                audioPlayerNode.play()
                playbackState = .playing
                startTimers()
                return
            }
            
            // 从头开始播放
            audioPlayerNode.scheduleFile(file, at: nil) { [weak self] in
                DispatchQueue.main.async {
                    self?.playbackState = .stopped
                    self?.currentTime = 0
                    self?.stopTimers()
                }
            }
            
            audioPlayerNode.play()
            playbackState = .playing
            startTimers()
            
            print("Playback started")
            
        } catch {
            errorMessage = "播放失败: \(error.localizedDescription)"
            print("Failed to start playback: \(error)")
        }
    }
    
    /// 暂停播放
    func pause() {
        guard playbackState == .playing else { return }
        
        audioPlayerNode.pause()
        playbackState = .paused
        stopTimers()
        
        print("Playback paused")
    }
    
    /// 停止播放
    func stop() {
        audioPlayerNode.stop()
        playbackState = .stopped
        currentTime = 0
        stopTimers()
        
        print("Playback stopped")
    }
    
    /// 跳转到指定时间
    func seek(to time: TimeInterval) {
        guard let file = audioFile else { return }
        
        let wasPlaying = playbackState == .playing
        stop()
        
        // 计算帧位置
        let framePosition = AVAudioFramePosition(time * file.fileFormat.sampleRate)
        let frameCount = AVAudioFrameCount(file.length - framePosition)
        
        if framePosition < file.length && frameCount > 0 {
            audioPlayerNode.scheduleSegment(file, startingFrame: framePosition, frameCount: frameCount, at: nil) { [weak self] in
                DispatchQueue.main.async {
                    self?.playbackState = .stopped
                    self?.currentTime = 0
                    self?.stopTimers()
                }
            }
            
            currentTime = time
            
            if wasPlaying {
                play()
            }
        }
    }
    
    /// 设置输出音量
    func setOutputGain(_ gain: Float) {
        config.outputGain = max(0.0, min(2.0, gain))
        audioEngine.mainMixerNode.outputVolume = config.outputGain
    }
    
    /// 获取VST管理器
    func getVSTManager() -> VSTManagerExample {
        return vstManager
    }
    
    // MARK: - Private Methods
    
    private func setupVSTAudioTap(format: AVAudioFormat) {
        guard isVSTProcessingEnabled else { return }
        
        // 移除现有的tap
        audioPlayerNode.removeTap(onBus: 0)
        
        // 安装新的音频tap用于VST处理
        audioPlayerNode.installTap(onBus: 0, bufferSize: AVAudioFrameCount(config.bufferSize), format: format) { [weak self] buffer, time in
            self?.processAudioWithVST(buffer: buffer)
        }
        
        isProcessingSetup = true
        print("VST audio tap setup completed")
    }
    
    private func processAudioWithVST(buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData,
              buffer.frameLength > 0 else { return }
        
        let numChannels = Int(buffer.format.channelCount)
        let numSamples = Int(buffer.frameLength)
        
        // 处理每个通道
        for channel in 0..<numChannels {
            let channelBuffer = channelData[channel]
            
            // 应用VST处理
            if vstManager.processAudioBuffer(channelBuffer, numSamples: numSamples, numChannels: 1) {
                // VST处理成功
            }
        }
        
        // 更新输出电平
        updateOutputLevel(from: buffer)
    }
    
    private func updateOutputLevel(from buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData else { return }
        
        var maxLevel: Float = 0.0
        let numChannels = Int(buffer.format.channelCount)
        let numSamples = Int(buffer.frameLength)
        
        for channel in 0..<numChannels {
            let samples = channelData[channel]
            for sample in 0..<numSamples {
                maxLevel = max(maxLevel, abs(samples[sample]))
            }
        }
        
        DispatchQueue.main.async {
            self.outputLevel = maxLevel
        }
    }
    
    private func startTimers() {
        // 播放进度定时器
        playbackTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            self?.updatePlaybackTime()
        }
        
        // 电平监控定时器
        levelTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            // 电平更新在音频回调中处理
        }
    }
    
    private func stopTimers() {
        playbackTimer?.invalidate()
        playbackTimer = nil
        
        levelTimer?.invalidate()
        levelTimer = nil
    }
    
    private func updatePlaybackTime() {
        guard let file = audioFile, playbackState == .playing else { return }
        
        if let nodeTime = audioPlayerNode.lastRenderTime,
           let playerTime = audioPlayerNode.playerTime(forNodeTime: nodeTime) {
            currentTime = Double(playerTime.sampleTime) / file.fileFormat.sampleRate
        }
    }
    
    private func cleanup() {
        stop()
        
        if audioEngine.isRunning {
            audioEngine.stop()
        }
        
        audioPlayerNode.removeTap(onBus: 0)
        
        if let buffer = audioBuffer {
            buffer.deallocate()
            audioBuffer = nil
        }
        
        cancellables.removeAll()
        
        print("Audio mixer service cleaned up")
    }
}
