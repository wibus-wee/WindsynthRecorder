//
//  AudioMixerWindowView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//

import SwiftUI

/// 音频混音台独立窗口视图
struct AudioMixerWindowView: View {
    @EnvironmentObject private var windowManager: WindowManager
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        AudioMixerView()
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("关闭") {
                        windowManager.closeAudioMixer()
                        dismiss()
                    }
                    .keyboardShortcut("w", modifiers: .command)
                }
                
                ToolbarItem(placement: .navigation) {
                    Button(action: {
                        // 可以添加混音台特定的操作
                    }) {
                        Image(systemName: "slider.horizontal.3")
                    }
                    .help("混音台控制")
                }
            }
            .navigationTitle("音频混音台")
            .frame(
                minWidth: WindowManager.WindowConfig.audioMixer.minSize.width,
                minHeight: WindowManager.WindowConfig.audioMixer.minSize.height
            )
            .onAppear {
                windowManager.isAudioMixerOpen = true
            }
            .onDisappear {
                windowManager.isAudioMixerOpen = false
            }
    }
}

#Preview {
    AudioMixerWindowView()
        .environmentObject(WindowManager.shared)
}
