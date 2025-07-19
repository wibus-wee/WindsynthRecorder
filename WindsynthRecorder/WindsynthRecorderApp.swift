//
//  WindsynthRecorderApp.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

@main
struct WindsynthRecorderApp: App {
    @StateObject private var windowManager = WindowManager.shared
    @StateObject private var appState = AppState.shared
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

    init() {
        // VST 音频处理会在需要时自动启动
    }

    var body: some Scene {
        // 启动窗口 - 仅在未初始化时显示
        Window("WindsynthRecorder Startup", id: "startup") {
            if !appState.isInitialized {
                StartupInitializationViewWrapper(onComplete: {
                    // 启动完成回调
                    DispatchQueue.main.async {
                        // 标记应用已初始化
                        appState.markAsInitialized()
                        // 关闭启动窗口
                        if let startupWindow = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == "startup" }) {
                            startupWindow.close()
                        }
                    }
                })
                .environmentObject(windowManager)
                .environmentObject(appState)
                .onAppear {
                    // 隐藏启动窗口的标题栏按钮
                    DispatchQueue.main.async {
                        if let startupWindow = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == "startup" }) {
                            startupWindow.standardWindowButton(.closeButton)?.isHidden = true
                            startupWindow.standardWindowButton(.miniaturizeButton)?.isHidden = true
                            startupWindow.standardWindowButton(.zoomButton)?.isHidden = true
                            startupWindow.titleVisibility = .hidden
                            startupWindow.center()
                            startupWindow.makeKeyAndOrderFront(nil)
                        }
                    }
                }
            } else {
                // 已初始化时显示空视图，窗口不会显示
                EmptyView()
            }
        }
        .windowStyle(.hiddenTitleBar)
        .windowResizability(.contentSize)
        .defaultSize(width: 520, height: 300)
        .defaultPosition(.center)

        // 主窗口 - 在初始化完成后可用
        Window("WindsynthRecorder", id: "main") {
            ContentView()
                .environmentObject(windowManager)
                .environmentObject(appState)
                .onAppear {
                    // 确保主窗口正确设置
                    DispatchQueue.main.async {
                        if let mainWindow = NSApplication.shared.windows.first(where: { $0.identifier?.rawValue == "main" }) {
                            mainWindow.titleVisibility = .hidden
                            mainWindow.standardWindowButton(.closeButton)?.isHidden = false
                            mainWindow.standardWindowButton(.miniaturizeButton)?.isHidden = false
                            mainWindow.standardWindowButton(.zoomButton)?.isHidden = false
                        }
                    }
                }
        }
        .windowStyle(.hiddenTitleBar)
        .windowResizability(.contentSize)
        .defaultSize(width: 800, height: 600)
        .defaultPosition(.center)
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("关于 WindsynthRecorder") {
                    openAboutWindow()
                }
                .keyboardShortcut("i", modifiers: .command)
            }

            // 窗口管理命令
            CommandGroup(after: .windowArrangement) {
                Divider()

                Button("音频混音台") {
                    windowManager.openAudioMixer()
                }
                .keyboardShortcut("m", modifiers: .command)

                Button("VST 插件管理器") {
                    windowManager.openVSTManager()
                }
                .keyboardShortcut("v", modifiers: .command)

                Button("音频批处理器") {
                    windowManager.openAudioProcessor()
                }
                .keyboardShortcut("p", modifiers: .command)

                Button("设置") {
                    windowManager.openSettings()
                }
                .keyboardShortcut(",", modifiers: .command)

                Button("日志") {
                    windowManager.openLogs()
                }
                .keyboardShortcut("l", modifiers: .command)
            }
        }

        // 音频混音台窗口
        Window(
            WindowManager.WindowConfig.audioMixer.title,
            id: WindowManager.WindowConfig.audioMixer.id
        ) {
            AudioMixerWindowView()
                .environmentObject(windowManager)
                .windowStyle(WindowManager.WindowConfig.audioMixer)
        }
        .windowResizability(.contentSize)
        .defaultSize(WindowManager.WindowConfig.audioMixer.defaultSize)

        // VST 插件管理器窗口
        Window(
            WindowManager.WindowConfig.vstManager.title,
            id: WindowManager.WindowConfig.vstManager.id
        ) {
            VSTManagerWindowView()
                .environmentObject(windowManager)
                .windowStyle(WindowManager.WindowConfig.vstManager)
        }
        .windowResizability(.contentSize)
        .defaultSize(WindowManager.WindowConfig.vstManager.defaultSize)

        // 音频批处理器窗口
        Window(
            WindowManager.WindowConfig.audioProcessor.title,
            id: WindowManager.WindowConfig.audioProcessor.id
        ) {
            AudioProcessorWindowView()
                .environmentObject(windowManager)
                .windowStyle(WindowManager.WindowConfig.audioProcessor)
        }
        .windowResizability(.contentSize)
        .defaultSize(WindowManager.WindowConfig.audioProcessor.defaultSize)

        // 设置窗口
        Window(
            WindowManager.WindowConfig.settings.title,
            id: WindowManager.WindowConfig.settings.id
        ) {
            SettingsWindowView()
                .environmentObject(windowManager)
                .windowStyle(WindowManager.WindowConfig.settings)
        }
        .windowResizability(.contentSize)
        .defaultSize(WindowManager.WindowConfig.settings.defaultSize)

        // 日志窗口
        Window(
            WindowManager.WindowConfig.logs.title,
            id: WindowManager.WindowConfig.logs.id
        ) {
            LogsWindowView()
                .environmentObject(windowManager)
                .windowStyle(WindowManager.WindowConfig.logs)
        }
        .windowResizability(.contentSize)
        .defaultSize(WindowManager.WindowConfig.logs.defaultSize)
    }

    private func openAboutWindow() {
        // 使用NSWindow直接创建About窗口 - 恢复标题栏
        let aboutWindow = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 650, height: 440),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
//        aboutWindow.title = "About WindsynthRecorder"
        aboutWindow.titleVisibility = .hidden
        aboutWindow.titlebarAppearsTransparent = true
        aboutWindow.backgroundColor = NSColor.windowBackgroundColor
        aboutWindow.contentView = NSHostingView(rootView: AboutWindowView())
        aboutWindow.center()
        aboutWindow.makeKeyAndOrderFront(nil)
        aboutWindow.level = .floating
        aboutWindow.isReleasedWhenClosed = false
        aboutWindow.hasShadow = true
    }
}

// MARK: - 启动窗口包装器
struct StartupInitializationViewWrapper: View {
    let onComplete: () -> Void
    @Environment(\.openWindow) private var openWindow
    @EnvironmentObject private var appState: AppState

    var body: some View {
        StartupInitializationView {
            // 启动完成后打开主窗口
            openWindow(id: "main")
            // 然后执行完成回调
            onComplete()
        }
    }
}
