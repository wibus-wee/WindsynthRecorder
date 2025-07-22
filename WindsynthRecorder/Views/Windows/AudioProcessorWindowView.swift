//
//  AudioProcessorWindowView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// 音频批处理器独立窗口视图
struct AudioProcessorWindowView: View {
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.dismiss) private var dismiss
    @State private var isPresented = true
    
    var body: some View {
        AudioProcessorView(isPresented: $isPresented)
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("关闭") {
                        windowManager.destroyWindow(withId: WindowManager.WindowConfig.audioProcessor.id)
                        dismiss()
                    }
                    .keyboardShortcut("w", modifiers: .command)
                }
                
                ToolbarItem(placement: .navigation) {
                    Button(action: {
                        // 可以添加音频处理器特定的操作
                    }) {
                        Image(systemName: "waveform.path")
                    }
                    .help("音频批处理")
                }
            }
            .navigationTitle("音频批处理器")
            .frame(
                minWidth: WindowManager.WindowConfig.audioProcessor.minSize.width,
                minHeight: WindowManager.WindowConfig.audioProcessor.minSize.height
            )
            .onAppear {
                windowManager.isAudioProcessorOpen = true
            }
            .onDisappear {
                windowManager.isAudioProcessorOpen = false
            }
            .onChange(of: isPresented) { newValue in
                if !newValue {
                    windowManager.destroyWindow(withId: WindowManager.WindowConfig.audioProcessor.id)
                    dismiss()
                }
            }
    }
}

#Preview {
    AudioProcessorWindowView()
        .environmentObject(WindowManager.shared)
}
