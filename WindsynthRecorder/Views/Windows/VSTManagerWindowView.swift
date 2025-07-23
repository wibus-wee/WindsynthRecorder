//
//  VSTManagerWindowView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// VST 插件管理器独立窗口视图
struct VSTManagerWindowView: View {
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VSTProcessorView()
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("关闭") {
                        windowManager.destroyWindow(withId: WindowManager.WindowConfig.vstManager.id)
                        dismiss()
                    }
                    .keyboardShortcut("w", modifiers: .command)
                }
            }
            .navigationTitle("VST 插件管理器")
            .frame(
                minWidth: WindowManager.WindowConfig.vstManager.minSize.width,
                minHeight: WindowManager.WindowConfig.vstManager.minSize.height
            )
            .onAppear {
                windowManager.isVSTManagerOpen = true
            }
            .onDisappear {
                windowManager.isVSTManagerOpen = false
            }
    }
}

#Preview {
    VSTManagerWindowView()
        .environmentObject(WindowManager.shared)
}
