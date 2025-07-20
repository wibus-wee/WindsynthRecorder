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

    @State private var isEnabled: Bool = true
    
    var body: some View {
        HStack(spacing: 12) {
            // 拖拽指示器
            Image(systemName: "line.3.horizontal")
                .font(.caption)
                .foregroundColor(.gray.opacity(0.6))
                .frame(width: 16)

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
                // 开关按钮
                Button(action: {
                    isEnabled.toggle()
                    _ = vstManager.setPluginEnabled(identifier: identifier, enabled: isEnabled)
                }) {
                    Image(systemName: isEnabled ? "power.circle.fill" : "power.circle")
                        .font(.caption)
                        .foregroundColor(isEnabled ? .green : .gray)
                        .frame(width: 24, height: 24)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill((isEnabled ? Color.green : Color.gray).opacity(0.1))
                        )
                        .overlay(
                            RoundedRectangle(cornerRadius: 4)
                                .stroke((isEnabled ? Color.green : Color.gray).opacity(0.3), lineWidth: 1)
                        )
                }
                .buttonStyle(.plain)
                .help(isEnabled ? "禁用插件" : "启用插件")

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
                .help("打开插件编辑器")

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
                .help("插件参数设置")
                
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
                .fill(Color.black.opacity(isEnabled ? 0.2 : 0.4))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke((isEnabled ? Color.gray : Color.red).opacity(0.2), lineWidth: 1)
        )
        .opacity(isEnabled ? 1.0 : 0.6)
        .animation(.easeInOut(duration: 0.2), value: isEnabled)
        .onAppear {
            // 初始化插件状态
            isEnabled = vstManager.isPluginEnabled(identifier: identifier)
        }
    }
}
