//
//  AudioChainManager.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频链管理服务 - 使用 JUCE 静态库 C 接口
//

import Foundation
import Combine

/// 音频链配置
struct AudioChainConfig {
    var sampleRate: Double = 44100.0
    var samplesPerBlock: Int = 512
    var numInputChannels: Int = 2
    var numOutputChannels: Int = 2
    var enableMidi: Bool = true
}

/// 性能统计
struct PerformanceStats {
    var cpuUsage: Double = 0.0
    var memoryUsage: Double = 0.0
    var latency: Double = 0.0
    var droppedSamples: Int = 0
}

/// 音频链管理服务
/// 管理VST插件链的创建、配置和处理
@MainActor
class AudioChainManager: ObservableObject {
    static let shared = AudioChainManager()
    
    // MARK: - Published Properties
    
    @Published var isEnabled: Bool = false
    @Published var chainConfig = AudioChainConfig()
    @Published var performanceStats = PerformanceStats()
    @Published var errorMessage: String?
    @Published var loadedPlugins: [String] = []
    
    // MARK: - Private Properties
    
    private var processingChain: AudioProcessingChainHandle?
    private var vstManager: VSTManagerExample?
    
    // MARK: - Initialization
    
    private init() {
        setupAudioChain()
    }
    
    deinit {
        if let chain = processingChain {
            audioProcessingChain_destroy(chain)
            processingChain = nil
        }
    }
    
    // MARK: - Setup and Cleanup
    
    private func setupAudioChain() {
        // 创建音频处理链
        processingChain = audioProcessingChain_create()
        guard processingChain != nil else {
            errorMessage = "Failed to create audio processing chain"
            return
        }
        
        // 创建 VST 管理器
        vstManager = VSTManagerExample.shared
        
        // 配置默认设置
        configureChain(with: chainConfig)
        
        print("Audio chain manager initialized successfully")
    }
    
    private func cleanup() {
        if let chain = processingChain {
            audioProcessingChain_destroy(chain)
            processingChain = nil
        }
    }
    
    // MARK: - Configuration
    
    func configureChain(with config: AudioChainConfig) {
        guard let chain = processingChain else {
            errorMessage = "Audio processing chain not initialized"
            return
        }
        
        var cConfig = ProcessingChainConfig_C()
        cConfig.sampleRate = config.sampleRate
        cConfig.samplesPerBlock = Int32(config.samplesPerBlock)
        cConfig.numInputChannels = Int32(config.numInputChannels)
        cConfig.numOutputChannels = Int32(config.numOutputChannels)
        cConfig.enableMidi = config.enableMidi
        
        audioProcessingChain_configure(chain, &cConfig)
        audioProcessingChain_prepareToPlay(chain, config.sampleRate, Int32(config.samplesPerBlock))
        
        self.chainConfig = config
        
        print("Audio chain configured: \(config.sampleRate)Hz, \(config.samplesPerBlock) samples")
    }
    
    func updateSampleRate(_ sampleRate: Double) {
        chainConfig.sampleRate = sampleRate
        configureChain(with: chainConfig)
    }
    
    func updateBufferSize(_ bufferSize: Int) {
        chainConfig.samplesPerBlock = bufferSize
        configureChain(with: chainConfig)
    }
    
    // MARK: - Plugin Management
    
    func addPlugin(named pluginName: String) -> Bool {
        guard let _ = processingChain,
              let manager = vstManager else {
            errorMessage = "Audio chain or VST manager not initialized"
            return false
        }
        
        let success = manager.loadPlugin(named: pluginName)
        if success {
            loadedPlugins.append(pluginName)
            print("Added plugin to chain: \(pluginName)")
        } else {
            errorMessage = "Failed to add plugin: \(pluginName)"
        }
        
        return success
    }
    
    func removePlugin(at index: Int) -> Bool {
        guard let chain = processingChain,
              index >= 0 && index < loadedPlugins.count else {
            return false
        }
        
        let success = audioProcessingChain_removePlugin(chain, Int32(index))
        if success {
            let removedPlugin = loadedPlugins.remove(at: index)
            print("Removed plugin from chain: \(removedPlugin)")
        }
        
        return success
    }
    
    func clearAllPlugins() {
        guard let chain = processingChain else { return }
        
        audioProcessingChain_clearPlugins(chain)
        loadedPlugins.removeAll()
        print("Cleared all plugins from chain")
    }
    
    func movePlugin(from sourceIndex: Int, to destinationIndex: Int) -> Bool {
        guard sourceIndex >= 0 && sourceIndex < loadedPlugins.count,
              destinationIndex >= 0 && destinationIndex < loadedPlugins.count,
              sourceIndex != destinationIndex else {
            return false
        }
        
        // 移动插件名称
        let plugin = loadedPlugins.remove(at: sourceIndex)
        loadedPlugins.insert(plugin, at: destinationIndex)
        
        // TODO: 实现 C 接口中的插件重排序功能
        print("Moved plugin from \(sourceIndex) to \(destinationIndex)")
        
        return true
    }
    
    // MARK: - Chain Control
    
    func enableChain() {
        isEnabled = true
        print("Audio chain enabled")
    }
    
    func disableChain() {
        isEnabled = false
        print("Audio chain disabled")
    }
    
    func resetChain() {
        clearAllPlugins()
        configureChain(with: AudioChainConfig()) // 重置为默认配置
        performanceStats = PerformanceStats() // 重置统计
        errorMessage = nil
        print("Audio chain reset")
    }
    
    // MARK: - Performance Monitoring
    
    func updatePerformanceStats() {
        // TODO: 从 C 接口获取实际的性能数据
        // 这里先使用模拟数据
        performanceStats = PerformanceStats(
            cpuUsage: Double.random(in: 0.1...0.3),
            memoryUsage: Double.random(in: 0.2...0.4),
            latency: Double(chainConfig.samplesPerBlock) / chainConfig.sampleRate * 1000, // ms
            droppedSamples: 0
        )
    }
    
    // MARK: - Utility Methods
    
    func getChainInfo() -> [String: Any] {
        return [
            "isEnabled": isEnabled,
            "sampleRate": chainConfig.sampleRate,
            "bufferSize": chainConfig.samplesPerBlock,
            "numChannels": chainConfig.numInputChannels,
            "loadedPlugins": loadedPlugins.count,
            "cpuUsage": performanceStats.cpuUsage,
            "latency": performanceStats.latency
        ]
    }
    
    func validateConfiguration() -> Bool {
        guard processingChain != nil else {
            errorMessage = "Processing chain not initialized"
            return false
        }
        
        guard chainConfig.sampleRate > 0 && chainConfig.samplesPerBlock > 0 else {
            errorMessage = "Invalid audio configuration"
            return false
        }
        
        return true
    }
}
