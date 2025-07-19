//
//  VSTPluginSlot.swift
//  WindsynthRecorder
//
//  VST 插件槽组件
//

import SwiftUI

/// 专业插件槽
struct ProfessionalPluginSlot: View {
    let pluginName: String
    let identifier: String
    let vstManager: VSTManagerExample
    let onParametersPressed: () -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            // 插件图标
            RoundedRectangle(cornerRadius: 4)
                .fill(Color.blue.opacity(0.2))
                .frame(width: 32, height: 32)
                .overlay(
                    Image(systemName: "music.note")
                        .foregroundColor(.blue)
                )
            
            // 插件信息
            VStack(alignment: .leading, spacing: 2) {
                Text(pluginName)
                    .font(.system(.caption, design: .rounded, weight: .medium))
                    .foregroundColor(.white)
                    .lineLimit(1)
                
                Text("VST3")
                    .font(.system(.caption2, design: .rounded))
                    .foregroundColor(.gray)
            }
            
            Spacer()
            
            // 控制按钮
            HStack(spacing: 6) {
                Button(action: {
                    vstManager.showPluginEditor(identifier: identifier)
                }) {
                    Image(systemName: "slider.horizontal.3")
                        .font(.caption)
                        .foregroundColor(.blue)
                        .frame(width: 24, height: 24)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(Color.blue.opacity(0.1))
                        )
                        .overlay(
                            RoundedRectangle(cornerRadius: 4)
                                .stroke(Color.blue.opacity(0.3), lineWidth: 1)
                        )
                }
                .buttonStyle(.plain)
                
                Button(action: onParametersPressed) {
                    Image(systemName: "gear")
                        .font(.caption)
                        .foregroundColor(.gray)
                        .frame(width: 24, height: 24)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(Color.gray.opacity(0.1))
                        )
                        .overlay(
                            RoundedRectangle(cornerRadius: 4)
                                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                        )
                }
                .buttonStyle(.plain)
                
                Button(action: {
                    _ = vstManager.unloadPlugin(identifier: identifier)
                }) {
                    Image(systemName: "xmark")
                        .font(.caption)
                        .foregroundColor(.red)
                        .frame(width: 24, height: 24)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(Color.red.opacity(0.1))
                        )
                        .overlay(
                            RoundedRectangle(cornerRadius: 4)
                                .stroke(Color.red.opacity(0.3), lineWidth: 1)
                        )
                }
                .buttonStyle(.plain)
            }
        }
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.2))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(Color.gray.opacity(0.2), lineWidth: 1)
        )
    }
}
