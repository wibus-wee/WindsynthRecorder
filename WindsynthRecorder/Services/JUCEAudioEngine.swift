import Foundation
import Combine
import AVFoundation

/// JUCE 音频引擎服务 - 使用 JUCE 进行音频播放和 VST 处理
class JUCEAudioEngine: NSObject, ObservableObject {
    
    // MARK: - Published Properties
    
    @Published var playbackState: PlaybackState = .stopped
    @Published var currentTime: TimeInterval = 0
    @Published var duration: TimeInterval = 0
    @Published var isVSTProcessingEnabled: Bool = false
    @Published var outputLevel: Float = 0.0
    @Published var currentFileName: String = ""
    @Published var errorMessage: String?
    
    // MARK: - Private Properties
    
    private var realtimeProcessor: RealtimeProcessorHandle?
    private var audioProcessingChain: AudioProcessingChainHandle?
    private var vstManager: VSTManagerExample
    private var config: AudioMixerConfig
    
    // 文件播放相关
    private var audioFile: AVAudioFile?
    private var audioFileReader: AudioFileReaderHandle?
    private var transportSource: AudioTransportSourceHandle?
    
    // 定时器和监控
    private var playbackTimer: Timer?
    private var levelTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    // 音频格式信息
    private var sampleRate: Double = 44100.0
    private var numChannels: Int = 2
    private var bufferSize: Int = 512
    
    // MARK: - Initialization
    
    override init() {
        self.vstManager = VSTManagerExample.shared
        self.config = AudioMixerConfig()
        
        super.init()
        
        setupJUCEAudioEngine()
        setupObservers()
    }
    
    deinit {
        cleanup()
    }
    
    // MARK: - Setup Methods
    
    private func setupJUCEAudioEngine() {
        // 创建实时处理器
        realtimeProcessor = realtimeProcessor_create()
        guard realtimeProcessor != nil else {
            errorMessage = "Failed to create JUCE realtime processor"
            return
        }
        
        // 使用 VSTManagerExample 的处理链，而不是创建新的
        audioProcessingChain = vstManager.getProcessingChain()
        guard let chain = audioProcessingChain else {
            errorMessage = "Failed to get processing chain from VST manager"
            return
        }

        // 让实时处理器使用这个处理链
        realtimeProcessor_setProcessingChain(realtimeProcessor!, chain)

        // 配置实时处理器
        configureRealtimeProcessor()
        
        print("✅ JUCE Audio Engine initialized successfully")
    }
    
    private func configureRealtimeProcessor() {
        guard let processor = realtimeProcessor else { return }
        
        var config = RealtimeProcessorConfig_C(
            sampleRate: sampleRate,
            bufferSize: Int32(bufferSize),
            numInputChannels: 0, // 不需要输入通道，避免请求麦克风权限
            numOutputChannels: 2, // 始终使用立体声，兼容大多数 VST 插件
            enableMonitoring: true,
            enableRecording: false,
            monitoringGain: 1.0,
            latencyCompensationSamples: 0
        )
        
        realtimeProcessor_configure(processor, &config)
        
        print("🔧 JUCE Realtime Processor configured: \(sampleRate)Hz, \(bufferSize) samples, \(numChannels) channels")
    }
    
    private func setupObservers() {
        // 监听 VST 管理器状态变化
        vstManager.$loadedPlugins
            .sink { [weak self] plugins in
                print("🔍 VST plugins changed: \(plugins.count) plugins loaded: \(plugins)")
                // 如果有插件加载，自动启用 VST 处理
                if !plugins.isEmpty && !(self?.isVSTProcessingEnabled ?? false) {
                    self?.isVSTProcessingEnabled = true
                    print("🎛️ Auto-enabled VST processing due to loaded plugins")
                }
                self?.updateVSTProcessingState()
            }
            .store(in: &cancellables)
    }
    
    // MARK: - Public Methods
    
    /// 加载音频文件
    func loadAudioFile(from url: URL) {
        do {
            // 使用 AVAudioFile 读取文件信息
            let file = try AVAudioFile(forReading: url)
            audioFile = file
            
            // 更新文件信息
            duration = Double(file.length) / file.fileFormat.sampleRate
            currentFileName = url.lastPathComponent
            currentTime = 0
            
            // 获取音频格式信息
            sampleRate = file.fileFormat.sampleRate
            numChannels = Int(file.fileFormat.channelCount)
            
            // 创建 JUCE 音频文件读取器
            guard let pathCString = url.path.cString(using: .utf8) else {
                errorMessage = "Failed to convert file path to C string"
                return
            }
            audioFileReader = audioFileReader_create(pathCString)
            
            guard audioFileReader != nil else {
                errorMessage = "Failed to create JUCE audio file reader"
                return
            }
            
            // 创建传输源
            transportSource = audioTransportSource_create(audioFileReader)
            guard let transport = transportSource else {
                errorMessage = "Failed to create JUCE transport source"
                return
            }

            // 准备音频传输源
            audioTransportSource_prepareToPlay(transport, Int32(bufferSize), sampleRate)
            
            // 重新配置音频引擎以匹配文件格式
            reconfigureForAudioFile()
            
            playbackState = .stopped
            
            print("✅ Audio file loaded: \(url.lastPathComponent), duration: \(duration)s, format: \(sampleRate)Hz, \(numChannels) channels")
            
        } catch {
            errorMessage = "加载音频文件失败: \(error.localizedDescription)"
            print("❌ Failed to load audio file: \(error)")
        }
    }
    
    private func reconfigureForAudioFile() {
        guard let processor = realtimeProcessor else { return }
        
        // 停止当前处理
        if realtimeProcessor_isRunning(processor) {
            realtimeProcessor_stop(processor)
        }
        
        // 重新配置处理器
        configureRealtimeProcessor()
        
        // 配置音频处理链
        if let chain = audioProcessingChain {
            var chainConfig = ProcessingChainConfig_C()
            chainConfig.sampleRate = sampleRate
            chainConfig.samplesPerBlock = Int32(bufferSize)
            chainConfig.numInputChannels = 2  // 始终使用立体声
            chainConfig.numOutputChannels = 2 // 始终使用立体声
            chainConfig.enableMidi = true

            audioProcessingChain_configure(chain, &chainConfig)

            // 准备处理链以便插件能正确初始化
            audioProcessingChain_prepareToPlay(chain, sampleRate, Int32(bufferSize))
            print("🔧 Audio processing chain prepared: \(sampleRate)Hz, \(bufferSize) samples")
        }
        
        // 配置 VST 管理器
        vstManager.configureAudioProcessing(
            sampleRate: sampleRate,
            samplesPerBlock: bufferSize,
            numChannels: numChannels
        )
        
        print("🔄 Audio engine reconfigured for file format")
    }
    
    /// 开始播放
    func play() {
        guard let processor = realtimeProcessor,
              let transport = transportSource,
              playbackState != .playing else { return }
        
        // 初始化音频设备
        if !realtimeProcessor_initialize(processor) {
            errorMessage = "Failed to initialize audio device"
            return
        }

        // 连接音频传输源到实时处理器
        realtimeProcessor_setAudioTransportSource(processor, transport)

        // 先启动 AudioTransportSource
        if playbackState == .paused {
            audioTransportSource_start(transport)
        } else {
            // 从头开始播放
            audioTransportSource_setPosition(transport, 0.0)
            audioTransportSource_start(transport)
        }

        // 然后启动实时处理器（这样音频回调开始时 AudioTransportSource 已经在播放）
        if !realtimeProcessor_start(processor) {
            errorMessage = "Failed to start realtime processor"
            return
        }

        playbackState = .playing
        startTimers()

        print("✅ JUCE playback started")
    }
    
    /// 暂停播放
    func pause() {
        guard let transport = transportSource, playbackState == .playing else { return }
        
        audioTransportSource_stop(transport)
        playbackState = .paused
        stopTimers()
        
        print("⏸️ JUCE playback paused")
    }
    
    /// 停止播放
    func stop() {
        guard let processor = realtimeProcessor,
              let transport = transportSource else { return }
        
        // 停止传输源
        audioTransportSource_stop(transport)
        audioTransportSource_setPosition(transport, 0.0)

        // 断开音频传输源连接
        realtimeProcessor_clearAudioTransportSource(processor)

        // 停止实时处理器
        realtimeProcessor_stop(processor)
        
        playbackState = .stopped
        currentTime = 0
        stopTimers()
        
        print("⏹️ JUCE playback stopped")
    }
    
    /// 跳转到指定时间
    func seek(to time: TimeInterval) {
        guard let transport = transportSource else { return }

        audioTransportSource_setPosition(transport, time)
        currentTime = time

        print("⏭️ JUCE seek to: \(time)s")
    }

    // MARK: - VST Processing Methods

    /// 启用/禁用 VST 处理
    func setVSTProcessingEnabled(_ enabled: Bool) {
        isVSTProcessingEnabled = enabled
        updateVSTProcessingState()
    }

    private func updateVSTProcessingState() {
        guard let chain = audioProcessingChain else { return }

        // 直接从处理链获取插件数量
        let numPlugins = audioProcessingChain_getNumPlugins(chain)
        let hasPlugins = numPlugins > 0

        // 启用/禁用音频处理链
        audioProcessingChain_setEnabled(chain, isVSTProcessingEnabled && hasPlugins)

        print("🎛️ VST processing \(isVSTProcessingEnabled ? "enabled" : "disabled"), chain plugins: \(numPlugins), swift plugins: \(vstManager.loadedPlugins.count)")
    }

    /// 获取 VST 管理器
    func getVSTManager() -> VSTManagerExample {
        return vstManager
    }

    /// 启动实时音频处理（不需要播放文件）
    func startRealtimeProcessing() {
        guard let processor = realtimeProcessor else { return }

        // 初始化音频设备
        if !realtimeProcessor_initialize(processor) {
            errorMessage = "Failed to initialize audio device for realtime processing"
            return
        }

        // 启动实时处理器
        if !realtimeProcessor_start(processor) {
            errorMessage = "Failed to start realtime processing"
            return
        }

        print("✅ JUCE realtime processing started")
    }

    /// 停止实时音频处理
    func stopRealtimeProcessing() {
        guard let processor = realtimeProcessor else { return }

        if realtimeProcessor_isRunning(processor) && playbackState == .stopped {
            realtimeProcessor_stop(processor)
            print("🛑 JUCE realtime processing stopped")
        }
    }

    // MARK: - Timer Methods

    private func startTimers() {
        // 播放进度定时器
        playbackTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            self?.updatePlaybackTime()
        }

        // 音频电平定时器
        levelTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            self?.updateAudioLevel()
        }
    }

    private func stopTimers() {
        playbackTimer?.invalidate()
        playbackTimer = nil

        levelTimer?.invalidate()
        levelTimer = nil
    }

    private func updatePlaybackTime() {
        guard let transport = transportSource else { return }

        let position = audioTransportSource_getCurrentPosition(transport)
        currentTime = position

        // 检查是否播放结束
        if position >= duration && playbackState == .playing {
            DispatchQueue.main.async { [weak self] in
                self?.stop()
            }
        }
    }

    private func updateAudioLevel() {
        guard let processor = realtimeProcessor else { return }

        // 获取输出电平
        let level = realtimeProcessor_getOutputLevel(processor)
        outputLevel = Float(level)
    }

    // MARK: - Cleanup

    private func cleanup() {
        stop()
        stopTimers()

        // 清理 JUCE 资源
        if let transport = transportSource {
            audioTransportSource_destroy(transport)
        }

        if let reader = audioFileReader {
            audioFileReader_destroy(reader)
        }

        if let processor = realtimeProcessor {
            realtimeProcessor_destroy(processor)
        }

        if let chain = audioProcessingChain {
            audioProcessingChain_destroy(chain)
        }

        cancellables.removeAll()

        print("🗑️ JUCE Audio Engine cleaned up")
    }
}

// MARK: - JUCE Bridge Functions

// 这些函数需要在 VSTBridge.h 中声明并在 VSTBridge.mm 中实现

// Realtime Processor
@_silgen_name("realtimeProcessor_create")
func realtimeProcessor_create() -> RealtimeProcessorHandle?

@_silgen_name("realtimeProcessor_destroy")
func realtimeProcessor_destroy(_ handle: RealtimeProcessorHandle)

@_silgen_name("realtimeProcessor_configure")
func realtimeProcessor_configure(_ handle: RealtimeProcessorHandle, _ config: UnsafePointer<RealtimeProcessorConfig_C>)

@_silgen_name("realtimeProcessor_initialize")
func realtimeProcessor_initialize(_ handle: RealtimeProcessorHandle) -> Bool

@_silgen_name("realtimeProcessor_start")
func realtimeProcessor_start(_ handle: RealtimeProcessorHandle) -> Bool

@_silgen_name("realtimeProcessor_stop")
func realtimeProcessor_stop(_ handle: RealtimeProcessorHandle)

@_silgen_name("realtimeProcessor_isRunning")
func realtimeProcessor_isRunning(_ handle: RealtimeProcessorHandle) -> Bool

@_silgen_name("realtimeProcessor_setProcessingChain")
func realtimeProcessor_setProcessingChain(_ handle: RealtimeProcessorHandle, _ chain: AudioProcessingChainHandle)

@_silgen_name("realtimeProcessor_getOutputLevel")
func realtimeProcessor_getOutputLevel(_ handle: RealtimeProcessorHandle) -> Double

@_silgen_name("realtimeProcessor_setAudioTransportSource")
func realtimeProcessor_setAudioTransportSource(_ handle: RealtimeProcessorHandle, _ transportHandle: AudioTransportSourceHandle)

@_silgen_name("realtimeProcessor_clearAudioTransportSource")
func realtimeProcessor_clearAudioTransportSource(_ handle: RealtimeProcessorHandle)

// Audio File Reader
@_silgen_name("audioFileReader_create")
func audioFileReader_create(_ filePath: UnsafePointer<CChar>) -> AudioFileReaderHandle?

@_silgen_name("audioFileReader_destroy")
func audioFileReader_destroy(_ handle: AudioFileReaderHandle)

// Audio Transport Source
@_silgen_name("audioTransportSource_create")
func audioTransportSource_create(_ reader: AudioFileReaderHandle?) -> AudioTransportSourceHandle?

@_silgen_name("audioTransportSource_destroy")
func audioTransportSource_destroy(_ handle: AudioTransportSourceHandle)

@_silgen_name("audioTransportSource_prepareToPlay")
func audioTransportSource_prepareToPlay(_ handle: AudioTransportSourceHandle, _ samplesPerBlock: Int32, _ sampleRate: Double)

@_silgen_name("audioTransportSource_start")
func audioTransportSource_start(_ handle: AudioTransportSourceHandle)

@_silgen_name("audioTransportSource_stop")
func audioTransportSource_stop(_ handle: AudioTransportSourceHandle)

@_silgen_name("audioTransportSource_setPosition")
func audioTransportSource_setPosition(_ handle: AudioTransportSourceHandle, _ position: Double)

@_silgen_name("audioTransportSource_getCurrentPosition")
func audioTransportSource_getCurrentPosition(_ handle: AudioTransportSourceHandle) -> Double

@_silgen_name("audioProcessingChain_setEnabled")
func audioProcessingChain_setEnabled(_ handle: AudioProcessingChainHandle, _ enabled: Bool)

@_silgen_name("audioProcessingChain_prepareToPlay")
func audioProcessingChain_prepareToPlay(_ handle: AudioProcessingChainHandle, _ sampleRate: Double, _ samplesPerBlock: Int32)

@_silgen_name("audioProcessingChain_getNumPlugins")
func audioProcessingChain_getNumPlugins(_ handle: AudioProcessingChainHandle) -> Int32

// Handle Types
typealias RealtimeProcessorHandle = OpaquePointer
typealias AudioFileReaderHandle = OpaquePointer
typealias AudioTransportSourceHandle = OpaquePointer

// Configuration Structures
struct RealtimeProcessorConfig_C {
    var sampleRate: Double
    var bufferSize: Int32
    var numInputChannels: Int32
    var numOutputChannels: Int32
    var enableMonitoring: Bool
    var enableRecording: Bool
    var monitoringGain: Double
    var latencyCompensationSamples: Int32
}
