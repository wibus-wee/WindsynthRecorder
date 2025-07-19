//
//  WindsynthRecorderApp.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

@main
struct WindsynthRecorderApp: App {

    init() {
        // 启动实时音频处理
        Task { @MainActor in
            RealtimeAudioManager.shared.start()
        }
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
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
        }
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
