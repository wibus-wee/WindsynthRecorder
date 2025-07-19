//
//  StartupInitializationView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// 启动初始化窗口 - 类似 Logic Pro 的启动流程
struct StartupInitializationView: View {
    @StateObject private var initializationManager = StartupInitializationManager()
    let onComplete: () -> Void

    var body: some View {
        ZStack {
            // 背景 - 跟随系统颜色方案
            Color(NSColor.windowBackgroundColor)
                .ignoresSafeArea()

            HStack(spacing: 40) {
                // 左侧 - 应用图标（稍微偏上对齐）
                VStack {
                    Spacer()
                        .frame(height: 20) // 顶部较小的固定间距
                    appIconSection
                    Spacer()
                        .frame(minHeight: 40) // 底部最小间距，但可以拉伸
                }

                // 右侧 - 应用信息和初始化内容
                VStack(alignment: .leading, spacing: 0) {
                    // 应用标题和版本 - 与图标顶部大致对齐
                    appInfoSection

                    Spacer()
                        .frame(height: 25) // 中间间距

                    // 初始化内容
                    initializationContent

                    Spacer() // 弹性间距推动版权信息到底部

                    // 版权信息
                    footerSection
                }
                .frame(maxWidth: 300)
            }
            .padding(.trailing, 40) // 左右较小的固定间距
            .padding(.leading, 30)
            .padding(.vertical, 20)
        }
        .frame(width: 580, height: 300)
        .onAppear {
            startInitialization()
        }
    }

    // MARK: - Header Section

    // 左侧应用图标
    private var appIconSection: some View {
        Group {
            if let appIcon = NSApplication.shared.applicationIconImage {
                Image(nsImage: appIcon)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 128, height: 128)
                    .clipShape(RoundedRectangle(cornerRadius: 22))
                    .shadow(color: Color.black.opacity(0.2), radius: 8, x: 0, y: 4)
            } else {
                // 备用图标
                RoundedRectangle(cornerRadius: 22)
                    .fill(LinearGradient(
                        gradient: Gradient(colors: [.blue.opacity(0.8), .purple.opacity(0.8)]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    ))
                    .frame(width: 128, height: 128)
                    .overlay(
                        Image(systemName: "waveform.circle.fill")
                            .font(.system(size: 64))
                            .foregroundStyle(.white)
                    )
                    .shadow(color: Color.black.opacity(0.2), radius: 8, x: 0, y: 4)
            }
        }
    }

    // 右侧应用信息
    private var appInfoSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            // Apple 标志
            Image(systemName: "applelogo")
                .font(.system(size: 28))
                .foregroundStyle(Color(NSColor.secondaryLabelColor))

            // 应用标题
            Text("WindsynthRecorder")
                .font(.system(size: 32, weight: .light, design: .default))
                .foregroundStyle(Color(NSColor.labelColor))

            // 版本信息
            Text("版本 1.0.0")
                .font(.system(size: 14))
                .foregroundStyle(Color(NSColor.secondaryLabelColor))
        }
    }

    // MARK: - Initialization Content

    private var initializationContent: some View {
        VStack(alignment: .leading, spacing: 6) {
            // 当前任务描述
            Text(initializationManager.currentTask)
                .font(.system(size: 11))
                .foregroundStyle(Color(NSColor.secondaryLabelColor))
                .frame(maxWidth: .infinity, alignment: .leading)

            // 详细信息（如果有）
            Text(initializationManager.detailText)
                .font(.system(size: 11))
                .foregroundStyle(Color(NSColor.secondaryLabelColor))
                .frame(maxWidth: .infinity, alignment: .leading)
        }
        .frame(maxWidth: 280)
    }

    // MARK: - Footer Section

    private var footerSection: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text("© 2024-2025 Wibus. 保留一切权利。Wibus、WindsynthRecorder 是 Wibus 在中国及其他国家和地区注册的商标。")
                .font(.system(size: 9))
                .foregroundStyle(Color(NSColor.tertiaryLabelColor))
        }
        .frame(maxWidth: 270)
    }

    // MARK: - Methods

    private func startInitialization() {
        initializationManager.startInitialization { _ in
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                withAnimation(.easeInOut(duration: 0.3)) {
                    onComplete()
                }
            }
        }
    }
}

// MARK: - Initialization Manager

@MainActor
class StartupInitializationManager: ObservableObject {
    @Published var currentTask = "正在启动..."
    @Published var progress: Double = 0.0
    @Published var detailText = ""

    private let vstManager = VSTManagerExample.shared
    private let audioDeviceManager = AudioDeviceManager.shared
    private let ffmpegManager = FFmpegManager.shared
    private let notificationManager = NotificationManager.shared

    func startInitialization(completion: @escaping (Bool) -> Void) {
        Task {
            await performInitializationSteps()
            completion(true)
        }
    }

    private func performInitializationSteps() async {
        // 步骤 1: 记录应用启动信息
        await updateProgress(task: "正在启动应用...", progress: 0.05)
        AudioProcessingLogger.shared.info("应用启动", details: "WindsynthRecorder 已启动\n系统: \(ProcessInfo.processInfo.operatingSystemVersionString)\n设备: \(ProcessInfo.processInfo.hostName)")
        try? await Task.sleep(nanoseconds: 200_000_000) // 0.2秒

        // 步骤 2: 初始化音频系统
        await updateProgress(task: "正在初始化音频系统...", progress: 0.1)
        try? await Task.sleep(nanoseconds: 300_000_000) // 0.3秒

        // 步骤 3: 检查音频设备
        await updateProgress(task: "正在检查音频设备...", progress: 0.15, detail: "刷新设备列表并检查 SR 设备状态")
        await checkDevices()
        try? await Task.sleep(nanoseconds: 300_000_000) // 0.3秒

        // 步骤 4: 检查和配置 FFmpeg
        await updateProgress(task: "正在检查 FFmpeg...", progress: 0.2, detail: "验证音频处理工具")
        await checkAndSetupFFmpeg()
        try? await Task.sleep(nanoseconds: 300_000_000) // 0.3秒

        // 步骤 5: 扫描 VST 插件
        await updateProgress(task: "正在扫描 VST 插件...", progress: 0.35, detail: "搜索系统插件目录")
        await scanVSTPlugins()

        // 步骤 6: 加载插件缓存
        await updateProgress(task: "正在加载插件缓存...", progress: 0.85)
        try? await Task.sleep(nanoseconds: 300_000_000)

        // 步骤 7: 完成初始化
        await updateProgress(task: "正在完成初始化...", progress: 0.95)
        try? await Task.sleep(nanoseconds: 200_000_000)

        // 步骤 8: 准备就绪
        await updateProgress(task: "初始化完成", progress: 1.0, detail: "WindsynthRecorder 已准备就绪")
        try? await Task.sleep(nanoseconds: 10_00_000_000) // sleep for 2 seconds
    }

    private func scanVSTPlugins() async {
        // 设置扫描进度回调
        vstManager.setScanProgressCallback { [weak self] pluginName, progress in
            DispatchQueue.main.async {
                self?.currentTask = "正在扫描 VST 插件..."
                self?.progress = 0.35 + (Double(progress) * 0.5) // 从35%到85%
                self?.detailText = "\(pluginName)"
            }
        }

        // 开始扫描
        vstManager.scanForPlugins()

        // 等待扫描完成
        while vstManager.isScanning {
            try? await Task.sleep(nanoseconds: 100_000_000) // 0.1秒
        }
    }

    private func updateProgress(task: String, progress: Double, detail: String = "") async {
        currentTask = task
        self.progress = progress
        detailText = detail

        // 给UI一些时间更新
        try? await Task.sleep(nanoseconds: 50_000_000) // 0.05秒
    }

    private func updateProgressSync(task: String, progress: Double, detail: String = "") {
        DispatchQueue.main.async { [weak self] in
            self?.currentTask = task
            self?.progress = progress
            self?.detailText = detail
        }
    }

    // MARK: - Device and System Checks

    private func checkDevices() async {
        AudioProcessingLogger.shared.info("开始设备检查", details: "刷新设备列表并检查 SR 设备状态")

        audioDeviceManager.refreshDeviceList()
        let (success, message) = audioDeviceManager.checkAndSetupSRBlueDevice()
        if !success {
            AudioProcessingLogger.shared.warning("SR Blue 设备检查", details: message)
        }

        let (available, recMessage) = audioDeviceManager.checkSRRecDevice()
        if !available {
            AudioProcessingLogger.shared.warning("SR-REC 设备检查", details: recMessage)
        }

        AudioProcessingLogger.shared.info("设备检查完成", details: "SR Blue: \(success ? "成功" : "失败"), SR-REC: \(available ? "可用" : "不可用")")
    }

    private func checkAndSetupFFmpeg() async {
        await updateProgress(task: "正在检查 FFmpeg...", progress: 0.21, detail: "验证音频处理工具配置")

        // 触发 FFmpeg 初始化检查（如果需要的话会自动发现）
        ffmpegManager.initializeIfNeeded()

        // 更新进度和日志
        if ffmpegManager.isFFmpegAvailable {
            await updateProgress(task: "FFmpeg 配置完成", progress: 0.25, detail: "已找到并配置 FFmpeg")
            AudioProcessingLogger.shared.success("FFmpeg 配置成功", details: "路径: \(ffmpegManager.ffmpegPath)\n版本: \(ffmpegManager.ffmpegVersion)")
        } else {
            await updateProgress(task: "FFmpeg 未找到", progress: 0.25, detail: "某些功能可能不可用")
            AudioProcessingLogger.shared.warning("FFmpeg 未找到", details: "某些功能可能不可用，请在设置中手动配置 FFmpeg 路径")
        }
    }
}

// MARK: - Preview

#Preview {
    StartupInitializationView(onComplete: {})
}
