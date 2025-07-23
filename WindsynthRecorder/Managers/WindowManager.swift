//
//  WindowManager.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI
import Combine

/// 应用状态管理器 - 管理应用的初始化状态
@MainActor
class AppState: ObservableObject {
    static let shared = AppState()

    /// 应用是否已完成初始化（每次启动重置，不持久化）
    @Published var isInitialized = false

    private init() {}

    /// 标记应用已完成初始化
    func markAsInitialized() {
        isInitialized = true
    }

    /// 重置初始化状态（用于测试或特殊情况）
    func resetInitializationState() {
        isInitialized = false
    }
}

/// 窗口管理器 - 统一管理应用中的所有窗口状态
@MainActor
class WindowManager: ObservableObject {
    static let shared = WindowManager()
    
    // MARK: - 窗口状态跟踪
    @Published var isAudioMixerOpen = false
    @Published var isVSTManagerOpen = false
    @Published var isAudioProcessorOpen = false
    @Published var isSettingsOpen = false
    @Published var isLogsOpen = false
    
    // MARK: - 窗口配置
    struct WindowConfig {
        let id: String
        let title: String
        let defaultSize: CGSize
        let minSize: CGSize
        let resizable: Bool
        let level: NSWindow.Level
        
        static let audioMixer = WindowConfig(
            id: "audio-mixer",
            title: "音频混音台",
            defaultSize: CGSize(width: 1000, height: 700),
            minSize: CGSize(width: 800, height: 600),
            resizable: true,
            level: .normal
        )
        
        static let vstManager = WindowConfig(
            id: "vst-manager",
            title: "VST 插件管理器",
            defaultSize: CGSize(width: 1000, height: 700),
            minSize: CGSize(width: 900, height: 600),
            resizable: true,
            level: .normal
        )
        
        static let audioProcessor = WindowConfig(
            id: "audio-processor",
            title: "音频批处理器",
            defaultSize: CGSize(width: 800, height: 600),
            minSize: CGSize(width: 600, height: 500),
            resizable: true,
            level: .normal
        )
        
        static let settings = WindowConfig(
            id: "settings",
            title: "设置",
            defaultSize: CGSize(width: 600, height: 500),
            minSize: CGSize(width: 500, height: 400),
            resizable: true,
            level: .normal
        )
        
        static let logs = WindowConfig(
            id: "logs",
            title: "音频处理日志",
            defaultSize: CGSize(width: 800, height: 600),
            minSize: CGSize(width: 600, height: 400),
            resizable: true,
            level: .normal
        )
    }
    
    private init() {}
    
    // MARK: - 窗口控制方法

    /// 打开主窗口
    func openMainWindow() {
        // 通过 NSApplication 查找并激活主窗口
        DispatchQueue.main.async {
            // 查找主窗口
            if let mainWindow = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == "main" }) {
                mainWindow.makeKeyAndOrderFront(nil)
                NSApplication.shared.activate(ignoringOtherApps: true)
            } else {
                // 如果主窗口不存在，尝试通过 SwiftUI 的 openWindow 来创建
                // 这种情况可能发生在应用刚启动或所有窗口都被关闭后
                print("Main window not found - will be created by SwiftUI Scene")
                NSApplication.shared.activate(ignoringOtherApps: true)
            }
        }
    }

    /// 检查主窗口是否存在
    var isMainWindowOpen: Bool {
        return NSApplication.shared.windows.contains { $0.identifier?.rawValue == "main" }
    }

    /// 打开音频混音台窗口
    func openAudioMixer() {
        isAudioMixerOpen = true
    }
    
    /// 关闭音频混音台窗口
    func closeAudioMixer() {
        isAudioMixerOpen = false
    }
    
    /// 打开VST管理器窗口
    func openVSTManager() {
        isVSTManagerOpen = true
    }
    
    /// 关闭VST管理器窗口
    func closeVSTManager() {
        isVSTManagerOpen = false
    }
    
    /// 打开音频处理器窗口
    func openAudioProcessor() {
        isAudioProcessorOpen = true
    }
    
    /// 关闭音频处理器窗口
    func closeAudioProcessor() {
        isAudioProcessorOpen = false
    }
    
    /// 打开设置窗口
    func openSettings() {
        isSettingsOpen = true
    }
    
    /// 关闭设置窗口
    func closeSettings() {
        isSettingsOpen = false
    }
    
    /// 打开日志窗口
    func openLogs() {
        isLogsOpen = true
    }
    
    /// 关闭日志窗口
    func closeLogs() {
        isLogsOpen = false
    }
    
    /// 关闭所有工具窗口
    func closeAllToolWindows() {
        isAudioMixerOpen = false
        isVSTManagerOpen = false
        isAudioProcessorOpen = false
        isSettingsOpen = false
        isLogsOpen = false
    }
    
    /// 获取当前打开的窗口数量
    var openWindowCount: Int {
        var count = 1 // 主窗口始终打开
        if isAudioMixerOpen { count += 1 }
        if isVSTManagerOpen { count += 1 }
        if isAudioProcessorOpen { count += 1 }
        if isSettingsOpen { count += 1 }
        if isLogsOpen { count += 1 }
        return count
    }
    
    /// 检查是否有工具窗口打开
    var hasToolWindowsOpen: Bool {
        return isAudioMixerOpen || isVSTManagerOpen || isAudioProcessorOpen || isSettingsOpen || isLogsOpen
    }

    /// 根据窗口 ID 关闭对应的窗口
    func closeWindow(withId windowId: String) {
        switch windowId {
        case WindowConfig.audioMixer.id:
            closeAudioMixer()
        case WindowConfig.vstManager.id:
            closeVSTManager()
        case WindowConfig.audioProcessor.id:
            closeAudioProcessor()
        case WindowConfig.settings.id:
            closeSettings()
        case WindowConfig.logs.id:
            closeLogs()
        default:
            print("Unknown window ID: \(windowId)")
        }
    }

    /// 强制关闭并销毁指定的窗口
    func destroyWindow(withId windowId: String) {
        // 首先更新状态
        closeWindow(withId: windowId)

        // 然后尝试从系统中移除窗口
        DispatchQueue.main.async {
            if let window = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == windowId }) {
                window.close()
                print("Window with ID '\(windowId)' has been destroyed")
            }
        }
    }
}

// MARK: - 窗口样式修饰符
struct WindowStyleModifier: ViewModifier {
    let config: WindowManager.WindowConfig
    @Environment(\.dismiss) private var dismiss

    func body(content: Content) -> some View {
        content
            .frame(
                minWidth: config.minSize.width,
                minHeight: config.minSize.height
            )
            .navigationTitle(config.title)
    }

    /// 关闭并销毁当前窗口
    private func closeWindow() {
        let windowManager = WindowManager.shared

        // 使用通用的窗口销毁方法
        windowManager.destroyWindow(withId: config.id)

        // 销毁 SwiftUI 视图
        dismiss()
    }
}

extension View {
    func windowStyle(_ config: WindowManager.WindowConfig) -> some View {
        modifier(WindowStyleModifier(config: config))
    }
}
