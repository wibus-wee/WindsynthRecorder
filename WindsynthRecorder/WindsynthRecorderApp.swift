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

    init() {
        // VST 音频处理会在需要时自动启动
    }

    var body: some Scene {
        // 主窗口 - 录音控制界面
        WindowGroup {
            ContentView()
                .environmentObject(windowManager)
        }
        .windowStyle(.hiddenTitleBar)
        .windowResizability(.contentSize)
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
