//
//  VSTProcessorView.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  临时 VST 处理器视图
//

import SwiftUI

/// VST 处理器视图 - 临时实现
struct VSTProcessorView: View {
    @StateObject private var vstManager = VSTManagerExample()
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        VStack(spacing: 20) {
            headerSection
            statusSection
            scanSection
            pluginSection
            Spacer()
            footerSection
        }
        .padding()
        .navigationTitle("VST 处理器")
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button("关闭") {
                    dismiss()
                }
            }
        }
        .onAppear {
            // VST 管理器会在初始化时自动设置
        }
    }

    // MARK: - 视图组件

    private var headerSection: some View {
        Text("VST 插件处理器")
            .font(.largeTitle)
            .fontWeight(.bold)
    }

    private var statusSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("状态: VST 功能正常")
                    .foregroundColor(.green)
                Spacer()
                if vstManager.isScanning {
                    HStack(spacing: 4) {
                        ProgressView()
                            .progressViewStyle(CircularProgressViewStyle())
                            .scaleEffect(0.6)
                        Text("扫描中...")
                            .font(.caption)
                            .foregroundColor(.orange)
                    }
                }
            }

            if let error = vstManager.errorMessage {
                Text("错误: \(error)")
                    .foregroundColor(.red)
                    .font(.caption)
            }

            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("可用插件: \(vstManager.availablePlugins.count)")
                        .font(.system(size: 13, weight: .medium))
                    Text("已加载插件: \(vstManager.loadedPlugins.count)")
                        .font(.system(size: 13, weight: .medium))
                }

                Spacer()

                if !vstManager.loadedPlugins.isEmpty {
                    VStack(alignment: .trailing, spacing: 2) {
                        Text("处理链状态")
                            .font(.caption)
                            .foregroundColor(.secondary)
                        Text("✓ 已激活")
                            .font(.caption)
                            .foregroundColor(.green)
                    }
                }
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
    }

    private var scanSection: some View {
        VStack {
            Button("扫描 VST 插件") {
                vstManager.scanForPlugins()
            }
            .disabled(vstManager.isScanning)

            if vstManager.isScanning {
                ProgressView("扫描中...")
                    .progressViewStyle(CircularProgressViewStyle())
            }
        }
    }

    @ViewBuilder
    private var pluginSection: some View {
        if !vstManager.availablePlugins.isEmpty {
            pluginListView
        } else {
            emptyPluginView
        }
    }

    private var pluginListView: some View {
        ScrollView {
            LazyVStack(alignment: .leading, spacing: 8) {
                ForEach(vstManager.availablePlugins, id: \.name) { plugin in
                    PluginRowView(plugin: plugin, vstManager: vstManager)
                }
            }
            .padding(.horizontal)
        }
        .frame(maxHeight: 300)
    }

    private var emptyPluginView: some View {
        Text("暂无可用插件")
            .foregroundColor(.secondary)
            .italic()
    }

    private var footerSection: some View {
        Text("VST 功能正在开发中，使用 JUCE 静态库集成")
            .font(.caption)
            .foregroundColor(.secondary)
            .multilineTextAlignment(.center)
    }
}

// MARK: - 插件行视图
struct PluginRowView: View {
    let plugin: VSTPluginInfo
    @ObservedObject var vstManager: VSTManagerExample

    var body: some View {
        HStack(spacing: 12) {
            VStack(alignment: .leading, spacing: 4) {
                Text(plugin.name)
                    .font(.headline)
                    .foregroundColor(.primary)

                Text(plugin.manufacturer)
                    .font(.caption)
                    .foregroundColor(.secondary)

                Text("\(plugin.category) • \(plugin.pluginFormatName)")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }

            Spacer()

            VStack(spacing: 4) {
                Button(action: {
                    loadPlugin()
                }) {
                    HStack(spacing: 4) {
                        Image(systemName: "plus.circle.fill")
                            .font(.system(size: 12))
                        Text("加载")
                            .font(.system(size: 11, weight: .medium))
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(Color.blue.opacity(0.1))
                    .foregroundColor(.blue)
                    .cornerRadius(6)
                }
                .buttonStyle(.plain)
                .disabled(isPluginLoaded)

                if isPluginLoaded {
                    Text("已加载")
                        .font(.system(size: 10))
                        .foregroundColor(.green)
                }
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
    }

    private var isPluginLoaded: Bool {
        vstManager.loadedPlugins.contains(plugin.name)
    }

    private func loadPlugin() {
        let success = vstManager.loadPlugin(named: plugin.name)
        if success {
            print("✅ 成功加载插件: \(plugin.name)")
        } else {
            print("❌ 加载插件失败: \(plugin.name)")
        }
    }
}

#Preview {
    VSTProcessorView()
}
