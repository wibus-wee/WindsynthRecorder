//
//  SettingsWindowView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// 设置独立窗口视图
struct SettingsWindowView: View {
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(spacing: 20) {
            // 设置内容将在这里实现
            Text("应用设置")
                .font(.largeTitle)
                .fontWeight(.bold)
            
            Text("设置界面正在开发中...")
                .foregroundStyle(.secondary)
            
            Spacer()
        }
        .padding(20)
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button("关闭") {
                    windowManager.destroyWindow(withId: WindowManager.WindowConfig.settings.id)
                    dismiss()
                }
                .keyboardShortcut("w", modifiers: .command)
            }
            
            ToolbarItem(placement: .navigation) {
                Button(action: {
                    // 可以添加设置特定的操作
                }) {
                    Image(systemName: "gear")
                }
                .help("应用设置")
            }
        }
        .navigationTitle("设置")
        .frame(
            minWidth: WindowManager.WindowConfig.settings.minSize.width,
            minHeight: WindowManager.WindowConfig.settings.minSize.height
        )
        .onAppear {
            windowManager.isSettingsOpen = true
        }
        .onDisappear {
            windowManager.isSettingsOpen = false
        }
    }
}

#Preview {
    SettingsWindowView()
        .environmentObject(WindowManager.shared)
}
