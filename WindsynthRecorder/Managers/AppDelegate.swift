//
//  AppDelegate.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI
import Cocoa

/// 应用委托 - 处理应用级别的事件
@MainActor
class AppDelegate: NSObject, NSApplicationDelegate, ObservableObject {
    
    /// 处理 Dock 图标点击事件
    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        let appState = AppState.shared
        let windowManager = WindowManager.shared

        // 如果没有可见窗口
        if !flag {
            if appState.isInitialized {
                // 已初始化，打开主窗口
                // 如果主窗口不存在，需要通过 SwiftUI 的环境来创建
                if windowManager.isMainWindowOpen {
                    windowManager.openMainWindow()
                } else {
                    // 主窗口不存在，需要重新创建
                    // 这种情况下，我们激活应用，让 SwiftUI 处理窗口创建
                    NSApplication.shared.activate(ignoringOtherApps: true)
                    // 延迟一点再尝试打开主窗口
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                        windowManager.openMainWindow()
                    }
                }
            } else {
                // 未初始化，需要显示启动窗口
                // 由于启动窗口是条件性渲染的，我们需要确保 SwiftUI 重新评估
                NSApplication.shared.activate(ignoringOtherApps: true)
            }
        } else {
            // 有可见窗口时，激活应用并将主窗口置前
            if appState.isInitialized && windowManager.isMainWindowOpen {
                windowManager.openMainWindow()
            }
            NSApplication.shared.activate(ignoringOtherApps: true)
        }

        return true
    }
    
    /// 打开启动窗口
    private func openStartupWindow() {
        // 查找启动窗口
        if let startupWindow = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == "startup" }) {
            startupWindow.makeKeyAndOrderFront(nil)
            NSApplication.shared.activate(ignoringOtherApps: true)
        } else {
            // 如果启动窗口不存在，这通常意味着需要重新创建
            // 在 SwiftUI 中，这会通过 Scene 的条件渲染来处理
            print("Startup window not found - this should be handled by SwiftUI Scene")
        }
    }
    
    /// 应用完成启动
    func applicationDidFinishLaunching(_ notification: Notification) {
        // 禁用窗口状态恢复
        UserDefaults.standard.register(defaults: ["NSQuitAlwaysKeepsWindows": false])

        // 确保应用状态正确初始化
        AppState.shared.resetInitializationState()
    }

    /// 禁用窗口状态恢复
    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return false
    }
    
    /// 应用是否应该终止
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        print("应用请求退出 - 开始清理音频资源")

        // 停止并完全清理音频引擎以避免崩溃
        let audioService = AudioGraphService.shared
        if audioService.isRunning {
            print("停止音频引擎...")
            audioService.stop()
        }

        // 强制调用清理方法来释放所有资源
        print("强制清理音频引擎资源...")
        audioService.forceCleanup()

        // 给音频线程充足时间来完全停止
        Thread.sleep(forTimeInterval: 0.3)

        // 清理窗口资源
        WindowManager.shared.closeAllToolWindows()

        print("音频资源清理完成 - 允许应用退出")
        return .terminateNow
    }

    /// 应用即将终止
    func applicationWillTerminate(_ notification: Notification) {
        print("应用即将终止 - 执行最终清理")
        // 这里不再需要额外的清理，因为在 applicationShouldTerminate 中已经完成
    }

    /// 应用是否应该在最后一个窗口关闭时终止
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        // 对于音频应用，通常不应该在关闭最后一个窗口时退出
        // 用户可能只是想隐藏窗口，稍后通过 Dock 重新打开
        return false
    }
}
