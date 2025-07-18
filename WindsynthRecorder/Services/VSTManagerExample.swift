//
//  VSTManagerExample.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  演示如何在 Swift 中使用 JUCE VST 静态库
//

import Foundation
import Combine

/// VST 插件信息的 Swift 包装
struct VSTPluginInfo {
    let name: String
    let manufacturer: String
    let version: String
    let category: String
    let pluginFormatName: String
    let fileOrIdentifier: String
    let numInputChannels: Int
    let numOutputChannels: Int
    let isInstrument: Bool
    let acceptsMidi: Bool
    let producesMidi: Bool
    
    init(from cInfo: VSTPluginInfo_C) {
        // 使用 withUnsafeBytes 安全地转换字符串
        self.name = withUnsafeBytes(of: cInfo.name) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.manufacturer = withUnsafeBytes(of: cInfo.manufacturer) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.version = withUnsafeBytes(of: cInfo.version) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.category = withUnsafeBytes(of: cInfo.category) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.pluginFormatName = withUnsafeBytes(of: cInfo.pluginFormatName) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.fileOrIdentifier = withUnsafeBytes(of: cInfo.fileOrIdentifier) { bytes in
            String(cString: bytes.bindMemory(to: CChar.self).baseAddress!)
        }
        self.numInputChannels = Int(cInfo.numInputChannels)
        self.numOutputChannels = Int(cInfo.numOutputChannels)
        self.isInstrument = cInfo.isInstrument
        self.acceptsMidi = cInfo.acceptsMidi
        self.producesMidi = cInfo.producesMidi

        // 调试信息
        print("📦 加载插件: \(self.name) by \(self.manufacturer)")
    }
}

/// VST 管理器 - 演示如何使用 JUCE VST 静态库
class VSTManagerExample: ObservableObject {
    @Published var availablePlugins: [VSTPluginInfo] = []
    @Published var loadedPlugins: [String] = []
    @Published var isScanning = false
    @Published var scanProgress: Float = 0.0
    @Published var errorMessage: String?
    
    private var pluginManager: VSTPluginManagerHandle?
    private var processingChain: AudioProcessingChainHandle?
    
    init() {
        setupVSTManager()
    }
    
    deinit {
        cleanup()
    }
    
    // MARK: - 初始化和清理
    
    private func setupVSTManager() {
        // 创建插件管理器
        pluginManager = vstPluginManager_create()
        guard pluginManager != nil else {
            errorMessage = "Failed to create VST plugin manager"
            return
        }
        
        // 创建音频处理链
        processingChain = audioProcessingChain_create()
        guard processingChain != nil else {
            errorMessage = "Failed to create audio processing chain"
            return
        }
        
        // 设置回调
        setupCallbacks()

        print("VST Manager initialized successfully")
    }
    
    private func setupCallbacks() {
        guard let manager = pluginManager else { return }
        
        // 设置扫描进度回调
        let progressCallback: ScanProgressCallback = { pluginName, progress, userData in
            guard let userData = userData else { return }
            let manager = Unmanaged<VSTManagerExample>.fromOpaque(userData).takeUnretainedValue()

            // 安全地复制字符串内容
            var pluginNameString: String? = nil
            if let name = pluginName {
                // 检查指针是否有效，并安全地创建字符串
                let nameStr = String(cString: name)
                pluginNameString = nameStr
            }

            DispatchQueue.main.async {
                manager.scanProgress = progress
                if let nameStr = pluginNameString {
                    print("Scanning: \(nameStr) - \(Int(progress * 100))%")
                }
            }
        }
        
        let selfPtr = Unmanaged.passUnretained(self).toOpaque()
        vstPluginManager_setScanProgressCallback(manager, progressCallback, selfPtr)
        
        // 设置错误回调
        let errorCallback: ErrorCallback = { error, userData in
            guard let userData = userData, let error = error else { return }
            let manager = Unmanaged<VSTManagerExample>.fromOpaque(userData).takeUnretainedValue()

            // 安全地复制错误信息
            let errorString = String(cString: error)

            DispatchQueue.main.async {
                manager.errorMessage = errorString
                print("VST Error: \(errorString)")
            }
        }
        
        vstPluginManager_setErrorCallback(manager, errorCallback, selfPtr)
    }
    
    private func cleanup() {
        if let manager = pluginManager {
            vstPluginManager_destroy(manager)
            pluginManager = nil
        }
        
        if let chain = processingChain {
            audioProcessingChain_destroy(chain)
            processingChain = nil
        }
    }
    
    // MARK: - 插件扫描
    
    func scanForPlugins() {
        guard let manager = pluginManager else {
            errorMessage = "Plugin manager not initialized"
            return
        }

        isScanning = true
        scanProgress = 0.0
        errorMessage = nil
        availablePlugins.removeAll() // 清除旧的插件列表

        print("🔍 开始扫描 VST 插件...")

        // 在后台线程执行扫描
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            vstPluginManager_scanForPlugins(manager)

            DispatchQueue.main.async {
                self?.isScanning = false
                self?.loadAvailablePlugins()
                print("✅ VST 插件扫描完成")
            }
        }
    }
    
    func addPluginSearchPath(_ path: String) {
        guard let manager = pluginManager else { return }
        vstPluginManager_addPluginSearchPath(manager, path)
        print("Added custom VST search path: \(path)")
    }
    
    private func loadAvailablePlugins() {
        guard let manager = pluginManager else {
            print("❌ Plugin manager is nil")
            return
        }

        let count = vstPluginManager_getNumAvailablePlugins(manager)
        print("🔍 尝试加载 \(count) 个插件...")

        var plugins: [VSTPluginInfo] = []

        for i in 0..<count {
            var cInfo = VSTPluginInfo_C()
            if vstPluginManager_getPluginInfo(manager, Int32(i), &cInfo) {
                let pluginInfo = VSTPluginInfo(from: cInfo)
                plugins.append(pluginInfo)
            } else {
                print("❌ 无法获取插件 \(i) 的信息")
            }
        }

        DispatchQueue.main.async { [weak self] in
            self?.availablePlugins = plugins
            print("✅ 成功加载 \(plugins.count) 个插件到 UI")
            print("📊 当前 availablePlugins.count = \(self?.availablePlugins.count ?? 0)")
            print("📊 availablePlugins.isEmpty = \(self?.availablePlugins.isEmpty ?? true)")
        }
    }
    
    // MARK: - 插件加载和管理
    
    func loadPlugin(named name: String) -> Bool {
        guard let manager = pluginManager,
              let chain = processingChain else {
            errorMessage = "Managers not initialized"
            return false
        }
        
        let pluginInstance = vstPluginManager_loadPlugin(manager, name)
        guard let instance = pluginInstance else {
            errorMessage = "Failed to load plugin: \(name)"
            return false
        }
        
        let success = audioProcessingChain_addPlugin(chain, instance)
        if success {
            loadedPlugins.append(name)
            print("Successfully loaded plugin: \(name)")
        } else {
            errorMessage = "Failed to add plugin to processing chain: \(name)"
            vstPluginInstance_destroy(instance)
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
            loadedPlugins.remove(at: index)
            print("Removed plugin at index \(index)")
        }
        
        return success
    }
    
    func clearAllPlugins() {
        guard let chain = processingChain else { return }
        
        audioProcessingChain_clearPlugins(chain)
        loadedPlugins.removeAll()
        print("Cleared all plugins")
    }
    
    // MARK: - 音频处理配置
    
    func configureAudioProcessing(sampleRate: Double, samplesPerBlock: Int, numChannels: Int) {
        guard let chain = processingChain else { return }
        
        var config = ProcessingChainConfig_C()
        config.sampleRate = sampleRate
        config.samplesPerBlock = Int32(samplesPerBlock)
        config.numInputChannels = Int32(numChannels)
        config.numOutputChannels = Int32(numChannels)
        config.enableMidi = true
        
        audioProcessingChain_configure(chain, &config)
        audioProcessingChain_prepareToPlay(chain, sampleRate, Int32(samplesPerBlock))
        
        print("Configured audio processing: \(sampleRate)Hz, \(samplesPerBlock) samples, \(numChannels) channels")
    }

    // MARK: - 音频处理

    func processAudioBuffer(_ buffer: UnsafeMutablePointer<Float>, numSamples: Int, numChannels: Int) -> Bool {
        guard let chain = processingChain else {
            return false
        }

//        return audioProcessingChain_processAudio(chain, buffer, Int32(numSamples), Int32(numChannels))
        return false
    }

    /// 检查是否有已加载的插件可以进行实时处理
    var hasLoadedPlugins: Bool {
        return !loadedPlugins.isEmpty
    }

    /// 获取处理链的状态信息
    var processingChainStatus: String {
        if loadedPlugins.isEmpty {
            return "无插件加载"
        } else {
            return "已加载 \(loadedPlugins.count) 个插件"
        }
    }

    // MARK: - 实用方法
    
    func findPlugin(named name: String) -> VSTPluginInfo? {
        return availablePlugins.first { $0.name.contains(name) }
    }
    
    func getPluginsByCategory(_ category: String) -> [VSTPluginInfo] {
        return availablePlugins.filter { $0.category.contains(category) }
    }
    
    func getInstrumentPlugins() -> [VSTPluginInfo] {
        return availablePlugins.filter { $0.isInstrument }
    }
    
    func getEffectPlugins() -> [VSTPluginInfo] {
        return availablePlugins.filter { !$0.isInstrument }
    }
}
