//
//  LogsWindowView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// 日志独立窗口视图
struct LogsWindowView: View {
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.dismiss) private var dismiss
    @State private var isPresented = true
    
    var body: some View {
        AudioProcessingLogView(isPresented: $isPresented)
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("关闭") {
                        windowManager.closeLogs()
                        dismiss()
                    }
                    .keyboardShortcut("w", modifiers: .command)
                }
                
                ToolbarItem(placement: .navigation) {
                    Button(action: {
                        // 可以添加日志特定的操作
                    }) {
                        Image(systemName: "doc.text")
                    }
                    .help("音频处理日志")
                }
            }
            .navigationTitle("音频处理日志")
            .frame(
                minWidth: WindowManager.WindowConfig.logs.minSize.width,
                minHeight: WindowManager.WindowConfig.logs.minSize.height
            )
            .onAppear {
                windowManager.isLogsOpen = true
            }
            .onDisappear {
                windowManager.isLogsOpen = false
            }
            .onChange(of: isPresented) { newValue in
                if !newValue {
                    windowManager.closeLogs()
                    dismiss()
                }
            }
    }
}

#Preview {
    LogsWindowView()
        .environmentObject(WindowManager.shared)
}
