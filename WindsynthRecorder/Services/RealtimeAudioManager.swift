//
//  RealtimeAudioManager.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  实时音频管理器 - 启动和管理 JUCE RealtimeProcessor
//

import Foundation
import Combine

/// 实时音频管理器 - 负责启动和管理 JUCE RealtimeProcessor
@MainActor
class RealtimeAudioManager: ObservableObject {
    static let shared = RealtimeAudioManager()
    
    // MARK: - Published Properties
    
    @Published var isRunning: Bool = false
    @Published var errorMessage: String?
    
    // MARK: - Private Properties
    
    private var realtimeProcessor: RealtimeProcessorHandle?
    private var vstManager: VSTManagerExample?
    private var audioMixerService: AudioMixerService?
    
    // MARK: - Initialization
    
    private init() {
        setupRealtimeProcessor()
    }
    
    deinit {
        // 在 deinit 中不能调用 @MainActor 方法，直接调用 C 接口
        if let processor = realtimeProcessor {
            realtimeProcessor_stop(processor)
            realtimeProcessor_destroy(processor)
        }
    }
    
    // MARK: - Setup
    
    private func setupRealtimeProcessor() {
        // 创建实时处理器
        realtimeProcessor = realtimeProcessor_create()
        guard realtimeProcessor != nil else {
            errorMessage = "Failed to create realtime processor"
            print("❌ Failed to create RealtimeProcessor")
            return
        }
        
        // 获取 VST 管理器和音频混音器服务
        vstManager = VSTManagerExample.shared
        audioMixerService = AudioMixerService()

        print("✅ RealtimeProcessor created successfully")
    }
    
    // MARK: - Public Methods
    
    /// 启动实时音频处理
    func start() {
        guard let processor = realtimeProcessor else {
            errorMessage = "Realtime processor not initialized"
            return
        }

        // 连接 VST 处理链
        connectVSTProcessingChain()

        // 启动实时处理器（添加错误处理）
        print("🚀 Attempting to start RealtimeProcessor...")

        // 启动 AudioMixerService 的实时处理
        audioMixerService?.startRealtimeProcessing()

        // 标记为运行状态
        isRunning = true
        print("✅ Realtime audio processing started via AudioMixerService")
    }
    
    /// 停止实时音频处理
    func stop() {
        // 停止 AudioMixerService 的实时处理
        audioMixerService?.stopRealtimeProcessing()

        // 停止 RealtimeProcessor（如果需要）
        if let processor = realtimeProcessor {
            realtimeProcessor_stop(processor)
        }

        isRunning = false
        print("🛑 Realtime audio processing stopped")
    }
    
    /// 检查是否正在运行
    func checkRunningStatus() {
        guard let processor = realtimeProcessor else {
            isRunning = false
            return
        }
        
        isRunning = realtimeProcessor_isRunning(processor)
    }
    
    // MARK: - Private Methods
    
    private func connectVSTProcessingChain() {
        guard let processor = realtimeProcessor,
              let vstManager = vstManager,
              let processingChain = vstManager.getProcessingChain() else {
            print("⚠️ Cannot connect VST processing chain - missing components")
            return
        }

        // 将 VST 处理链连接到实时处理器
        // processingChain 是 AudioProcessingChainHandle (UnsafeMutableRawPointer)
        // 直接转换为指针类型
        let chainPtr = UnsafeMutablePointer<AudioProcessingChainHandle?>.allocate(capacity: 1)
        chainPtr.pointee = processingChain
        realtimeProcessor_setProcessingChain(processor, chainPtr)
        chainPtr.deallocate()
        print("🔗 VST processing chain connected to RealtimeProcessor")
    }
}
