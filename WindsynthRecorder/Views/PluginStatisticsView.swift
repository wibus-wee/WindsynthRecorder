//
//  PluginStatisticsView.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  插件统计信息视图 - 使用 JUCE 静态库数据
//

import SwiftUI

/// 插件统计信息
struct PluginStatistics {
    var totalPlugins: Int = 0
    var instrumentPlugins: Int = 0
    var effectPlugins: Int = 0
    var vst3Plugins: Int = 0
    var manufacturers: [String: Int] = [:]
    var categories: [String: Int] = [:]
}

/// 插件统计信息视图
struct PluginStatisticsView: View {
    @ObservedObject private var vstManager = VSTManagerExample.shared
    @Environment(\.dismiss) private var dismiss
    
    private var statistics: PluginStatistics {
        calculateStatistics()
    }
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    // 总体统计
                    overviewSection
                    
                    // 类型分布
                    typeDistributionSection
                    
                    // 制造商分布
                    manufacturerSection
                    
                    // 类别分布
                    categorySection
                }
                .padding()
            }
            .navigationTitle("插件统计")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("关闭") {
                        dismiss()
                    }
                }
                
                ToolbarItem(placement: .secondaryAction) {
                    Button("刷新") {
                        vstManager.scanForPlugins()
                    }
                    .disabled(vstManager.isScanning)
                }
            }
        }
        .onAppear {
            if vstManager.availablePlugins.isEmpty {
                vstManager.scanForPlugins()
            }
        }
    }
    
    // MARK: - View Sections
    
    private var overviewSection: some View {
        VStack(spacing: 16) {
            Text("总体统计")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 16) {
                StatCard(title: "总插件数", value: "\(statistics.totalPlugins)", color: .blue)
                StatCard(title: "乐器插件", value: "\(statistics.instrumentPlugins)", color: .green)
                StatCard(title: "效果插件", value: "\(statistics.effectPlugins)", color: .orange)
                StatCard(title: "VST3 插件", value: "\(statistics.vst3Plugins)", color: .purple)
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(12)
    }
    
    private var typeDistributionSection: some View {
        VStack(spacing: 16) {
            Text("插件类型分布")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            if statistics.totalPlugins > 0 {
                VStack(spacing: 8) {
                    ProgressBar(
                        title: "乐器插件",
                        value: statistics.instrumentPlugins,
                        total: statistics.totalPlugins,
                        color: .green
                    )
                    
                    ProgressBar(
                        title: "效果插件",
                        value: statistics.effectPlugins,
                        total: statistics.totalPlugins,
                        color: .orange
                    )
                }
            } else {
                Text("暂无插件数据")
                    .foregroundColor(.secondary)
                    .italic()
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(12)
    }
    
    private var manufacturerSection: some View {
        VStack(spacing: 16) {
            Text("制造商分布")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            if !statistics.manufacturers.isEmpty {
                VStack(spacing: 8) {
                    ForEach(Array(statistics.manufacturers.sorted(by: { $0.value > $1.value }).prefix(5)), id: \.key) { manufacturer, count in
                        ProgressBar(
                            title: manufacturer,
                            value: count,
                            total: statistics.totalPlugins,
                            color: .blue
                        )
                    }
                }
            } else {
                Text("暂无制造商数据")
                    .foregroundColor(.secondary)
                    .italic()
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(12)
    }
    
    private var categorySection: some View {
        VStack(spacing: 16) {
            Text("类别分布")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            if !statistics.categories.isEmpty {
                VStack(spacing: 8) {
                    ForEach(Array(statistics.categories.sorted(by: { $0.value > $1.value }).prefix(5)), id: \.key) { category, count in
                        ProgressBar(
                            title: category,
                            value: count,
                            total: statistics.totalPlugins,
                            color: .purple
                        )
                    }
                }
            } else {
                Text("暂无类别数据")
                    .foregroundColor(.secondary)
                    .italic()
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(12)
    }
    
    // MARK: - Helper Methods
    
    private func calculateStatistics() -> PluginStatistics {
        let plugins = vstManager.availablePlugins
        
        var stats = PluginStatistics()
        stats.totalPlugins = plugins.count
        
        for plugin in plugins {
            // 统计插件类型
            if plugin.isInstrument {
                stats.instrumentPlugins += 1
            } else {
                stats.effectPlugins += 1
            }
            
            // 统计格式（目前只有 VST3）
            if plugin.pluginFormatName.contains("VST3") {
                stats.vst3Plugins += 1
            }
            
            // 统计制造商
            let manufacturer = plugin.manufacturer.isEmpty ? "Unknown" : plugin.manufacturer
            stats.manufacturers[manufacturer, default: 0] += 1
            
            // 统计类别
            let category = plugin.category.isEmpty ? "Uncategorized" : plugin.category
            stats.categories[category, default: 0] += 1
        }
        
        return stats
    }
}

// MARK: - Supporting Views

struct StatCard: View {
    let title: String
    let value: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 8) {
            Text(value)
                .font(.title2)
                .fontWeight(.bold)
                .foregroundColor(color)
            
            Text(title)
                .font(.caption)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(Color.white)
        .cornerRadius(8)
        .shadow(radius: 2)
    }
}

struct ProgressBar: View {
    let title: String
    let value: Int
    let total: Int
    let color: Color
    
    private var percentage: Double {
        total > 0 ? Double(value) / Double(total) : 0
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(title)
                    .font(.caption)
                    .foregroundColor(.primary)
                
                Spacer()
                
                Text("\(value) (\(Int(percentage * 100))%)")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(Color.gray.opacity(0.3))
                        .frame(height: 6)
                        .cornerRadius(3)
                    
                    Rectangle()
                        .fill(color)
                        .frame(width: geometry.size.width * percentage, height: 6)
                        .cornerRadius(3)
                }
            }
            .frame(height: 6)
        }
    }
}

#Preview {
    PluginStatisticsView()
}
