//
//  VSTProcessorView.swift
//  WindsynthRecorder
//
//  Created by wibus on 2025/2/21.
//  专业VST插件管理器 - 参考Logic Pro X设计
//

import SwiftUI

/// VST 插件管理器 - 专业版本
struct VSTProcessorView: View {
    @StateObject private var vstManager = VSTManagerExample.shared
    @State private var selectedCategory: String = "全部"
    @State private var searchText: String = ""
    @State private var showingOnlyEnabled: Bool = false
    @State private var sortBy: SortOption = .name

    enum SortOption: String, CaseIterable {
        case name = "名称"
        case manufacturer = "制造商"
        case category = "类别"
        case format = "格式"
    }

    var body: some View {
        VStack(spacing: 0) {
            // 工具栏
            toolbarSection

            Divider()

            // 主要内容区域
            HStack(spacing: 0) {
                // 左侧：类别和过滤器
                sidebarSection
                    .frame(width: 200)

                Divider()

                // 右侧：插件列表
                mainContentSection
            }

            Divider()

            // 底部状态栏
            statusBarSection
        }
        .navigationTitle("插件管理器")
        .onAppear {
            // 视图出现时自动获取插件列表
            DispatchQueue.main.async {
                // 如果还没有扫描过插件，自动开始扫描
                if vstManager.availablePlugins.isEmpty && !vstManager.isScanning {
                    print("🔍 自动扫描 VST 插件...")
                    vstManager.scanForPlugins()
                } else {
                    // 如果已有插件列表，只刷新UI
                    vstManager.objectWillChange.send()
                }
            }
        }
    }

    // MARK: - 视图组件

    private var toolbarSection: some View {
        HStack(spacing: 12) {
            // 扫描按钮
            Button(action: {
                vstManager.scanForPlugins()
            }) {
                HStack(spacing: 4) {
                    Image(systemName: vstManager.isScanning ? "arrow.clockwise" : "magnifyingglass")
                        .rotationEffect(.degrees(vstManager.isScanning ? 360 : 0))
                        .animation(vstManager.isScanning ? .linear(duration: 1).repeatForever(autoreverses: false) : .default, value: vstManager.isScanning)
                    Text(vstManager.isScanning ? "扫描中..." : "扫描插件")
                }
            }
            .disabled(vstManager.isScanning)
            .buttonStyle(.bordered)

            Divider()
                .frame(height: 20)

            // 搜索框
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundStyle(.secondary)
                TextField("搜索插件...", text: $searchText)
                    .textFieldStyle(.plain)
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundStyle(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(Color(.controlBackgroundColor), in: RoundedRectangle(cornerRadius: 6))
            .frame(width: 200)

            Spacer()

            // 排序选项
            Picker("排序", selection: $sortBy) {
                ForEach(SortOption.allCases, id: \.self) { option in
                    Text(option.rawValue).tag(option)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 80)

            // 仅显示已启用
            Toggle("仅已启用", isOn: $showingOnlyEnabled)
                .toggleStyle(.checkbox)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .background(Color(.controlBackgroundColor))
    }

    private var sidebarSection: some View {
        VStack(alignment: .leading, spacing: 0) {
            // 类别标题
            Text("类别")
                .font(.headline)
                .padding(.horizontal, 12)
                .padding(.top, 12)
                .padding(.bottom, 8)

            // 类别列表
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 2) {
                    CategoryRowView(
                        name: "全部",
                        count: vstManager.availablePlugins.count,
                        isSelected: selectedCategory == "全部"
                    ) {
                        selectedCategory = "全部"
                    }

                    ForEach(availableCategories, id: \.self) { category in
                        CategoryRowView(
                            name: category,
                            count: pluginsInCategory(category).count,
                            isSelected: selectedCategory == category
                        ) {
                            selectedCategory = category
                        }
                    }
                }
                .padding(.horizontal, 8)
            }

            Spacer()

            // 制造商过滤器
            if !availableManufacturers.isEmpty {
                Divider()
                    .padding(.horizontal, 12)

                Text("制造商")
                    .font(.headline)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)

                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 2) {
                        ForEach(availableManufacturers.prefix(10), id: \.self) { manufacturer in
                            ManufacturerRowView(
                                name: manufacturer,
                                count: pluginsByManufacturer(manufacturer).count
                            )
                        }
                    }
                    .padding(.horizontal, 8)
                }
                .frame(maxHeight: 150)
            }
        }
        .background(Color(.controlBackgroundColor))
    }

    private var mainContentSection: some View {
        VStack(spacing: 0) {
            // 插件列表头部
            pluginListHeader

            Divider()

            // 插件列表
            if filteredPlugins.isEmpty {
                emptyStateView
            } else {
                pluginTableView
            }
        }
    }

    private var pluginListHeader: some View {
        HStack(spacing: 0) {
            Text("名称")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 200, alignment: .leading)

            Text("制造商")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 120, alignment: .leading)

            Text("类别")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 100, alignment: .leading)

            Text("格式")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 80, alignment: .leading)

            Text("版本")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 60, alignment: .leading)

            Text("状态")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 80, alignment: .leading)

            Spacer()
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .background(Color(.controlBackgroundColor))
    }

    private var pluginTableView: some View {
        ScrollView {
            LazyVStack(spacing: 0) {
                ForEach(Array(filteredPlugins.enumerated()), id: \.element.fileOrIdentifier) { index, plugin in
                    ProfessionalPluginRowView(
                        plugin: plugin,
                        vstManager: vstManager,
                        isEven: index % 2 == 0
                    )
                }
            }
        }
    }

    private var emptyStateView: some View {
        VStack(spacing: 16) {
            Image(systemName: "music.note.list")
                .font(.system(size: 48))
                .foregroundStyle(.secondary)

            Text(vstManager.availablePlugins.isEmpty ? "暂无插件" : "无匹配结果")
                .font(.headline)
                .foregroundStyle(.secondary)

            Text(vstManager.availablePlugins.isEmpty ?
                 "点击\"扫描插件\"来查找系统中的VST插件" :
                 "尝试调整搜索条件或选择其他类别")
                .font(.subheadline)
                .foregroundStyle(.tertiary)
                .multilineTextAlignment(.center)

            if vstManager.availablePlugins.isEmpty {
                Button("扫描插件") {
                    vstManager.scanForPlugins()
                }
                .buttonStyle(.borderedProminent)
                .disabled(vstManager.isScanning)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding(40)
    }

    private var statusBarSection: some View {
        HStack {
            HStack(spacing: 16) {
                Text("总计: \(vstManager.availablePlugins.count) 个插件")
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)

                Text("已启用: \(enabledPluginsCount) 个")
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)

                Text("已加载: \(vstManager.loadedPlugins.count) 个")
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)
            }

            Spacer()

            if vstManager.isScanning {
                HStack(spacing: 8) {
                    ProgressView()
                        .progressViewStyle(CircularProgressViewStyle())
                        .scaleEffect(0.7)

                    Text("扫描进度: \(Int(vstManager.scanProgress * 100))%")
                        .font(.system(size: 11))
                        .foregroundStyle(.secondary)
                }
            } else if let error = vstManager.errorMessage {
                Text("错误: \(error)")
                    .font(.system(size: 11))
                    .foregroundStyle(.red)
            } else {
                Text("就绪")
                    .font(.system(size: 11))
                    .foregroundStyle(.green)
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .background(Color(.controlBackgroundColor))
    }

    // MARK: - 计算属性

    private var filteredPlugins: [VSTPluginInfo] {
        var plugins = vstManager.availablePlugins

        // 按类别过滤
        if selectedCategory != "全部" {
            plugins = plugins.filter { $0.category == selectedCategory }
        }

        // 按搜索文本过滤
        if !searchText.isEmpty {
            plugins = plugins.filter { plugin in
                plugin.name.localizedCaseInsensitiveContains(searchText) ||
                plugin.manufacturer.localizedCaseInsensitiveContains(searchText)
            }
        }

        // 按启用状态过滤
        if showingOnlyEnabled {
            plugins = plugins.filter { plugin in
                vstManager.loadedPlugins.contains(plugin.fileOrIdentifier)
            }
        }

        // 排序
        switch sortBy {
        case .name:
            plugins.sort { $0.name.localizedCompare($1.name) == .orderedAscending }
        case .manufacturer:
            plugins.sort { $0.manufacturer.localizedCompare($1.manufacturer) == .orderedAscending }
        case .category:
            plugins.sort { $0.category.localizedCompare($1.category) == .orderedAscending }
        case .format:
            plugins.sort { $0.pluginFormatName.localizedCompare($1.pluginFormatName) == .orderedAscending }
        }

        return plugins
    }

    private var availableCategories: [String] {
        let categories = Set(vstManager.availablePlugins.map { $0.category })
        return Array(categories).sorted()
    }

    private var availableManufacturers: [String] {
        let manufacturers = Set(vstManager.availablePlugins.map { $0.manufacturer })
        return Array(manufacturers).sorted()
    }

    private var enabledPluginsCount: Int {
        vstManager.availablePlugins.filter { plugin in
            vstManager.loadedPlugins.contains(plugin.fileOrIdentifier)
        }.count
    }

    // MARK: - 辅助方法

    private func pluginsInCategory(_ category: String) -> [VSTPluginInfo] {
        vstManager.availablePlugins.filter { $0.category == category }
    }

    private func pluginsByManufacturer(_ manufacturer: String) -> [VSTPluginInfo] {
        vstManager.availablePlugins.filter { $0.manufacturer == manufacturer }
    }
}

// MARK: - 类别行视图
struct CategoryRowView: View {
    let name: String
    let count: Int
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        HStack {
            Text(name)
                .font(.system(size: 13, weight: isSelected ? .medium : .regular))
                .foregroundStyle(isSelected ? .primary : .secondary)

            Spacer()

            Text("\(count)")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.tertiary)
                .padding(.horizontal, 6)
                .padding(.vertical, 2)
                .background(Color(.quaternaryLabelColor), in: RoundedRectangle(cornerRadius: 8))
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 6)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(isSelected ? Color.accentColor.opacity(0.15) : Color.clear, in: RoundedRectangle(cornerRadius: 6))
        .contentShape(Rectangle()) // 让整个区域都可以点击
        .onTapGesture {
            action()
        }
    }
}

// MARK: - 制造商行视图
struct ManufacturerRowView: View {
    let name: String
    let count: Int

    var body: some View {
        HStack {
            Text(name)
                .font(.system(size: 12))
                .foregroundStyle(.secondary)
                .lineLimit(1)

            Spacer()

            Text("\(count)")
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(.tertiary)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 2)
    }
}

// MARK: - 专业插件行视图
struct ProfessionalPluginRowView: View {
    let plugin: VSTPluginInfo
    @ObservedObject var vstManager: VSTManagerExample
    let isEven: Bool
    @State private var isHovered = false

    var body: some View {
        HStack(spacing: 0) {
            // 启用状态指示器
            Circle()
                .fill(isPluginLoaded ? Color.green : Color.clear)
                .frame(width: 8, height: 8)
                .padding(.trailing, 8)

            // 插件名称
            Text(plugin.name)
                .font(.system(size: 13, weight: .medium))
                .foregroundStyle(.primary)
                .frame(width: 180, alignment: .leading)
                .lineLimit(1)
                .truncationMode(.tail)

            // 制造商
            Text(plugin.manufacturer)
                .font(.system(size: 12))
                .foregroundStyle(.secondary)
                .frame(width: 120, alignment: .leading)
                .lineLimit(1)

            // 类别
            Text(plugin.category)
                .font(.system(size: 12))
                .foregroundStyle(.secondary)
                .frame(width: 100, alignment: .leading)
                .lineLimit(1)

            // 格式
            Text(plugin.pluginFormatName)
                .font(.system(size: 12, weight: .medium))
                .foregroundStyle(.tertiary)
                .frame(width: 80, alignment: .leading)

            // 版本
            Text(plugin.version)
                .font(.system(size: 11))
                .foregroundStyle(.tertiary)
                .frame(width: 60, alignment: .leading)

            // 状态和操作 - 固定宽度避免布局挤压
            HStack(spacing: 4) {
                // 状态文本区域 - 固定宽度
                Group {
                    if isPluginLoaded {
                        Text("已启用")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundStyle(.green)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(Color.green.opacity(0.1), in: RoundedRectangle(cornerRadius: 4))
                    } else {
                        Text("未启用")
                            .font(.system(size: 10))
                            .foregroundStyle(.secondary)
                    }
                }
                .frame(width: 50, alignment: .leading)

                // 操作按钮区域 - 固定宽度
                HStack(spacing: 2) {
                    if isHovered {
                        if !isPluginLoaded {
                            Button(action: loadPlugin) {
                                Image(systemName: "plus.circle")
                                    .font(.system(size: 12))
                                    .foregroundStyle(.blue)
                            }
                            .buttonStyle(.plain)
                            .help("启用插件")
                        } else {
                            Button(action: showPluginUI) {
                                Image(systemName: "slider.horizontal.3")
                                    .font(.system(size: 12))
                                    .foregroundStyle(.orange)
                            }
                            .buttonStyle(.plain)
                            .help("打开插件界面")

                            Button(action: unloadPlugin) {
                                Image(systemName: "minus.circle")
                                    .font(.system(size: 12))
                                    .foregroundStyle(.red)
                            }
                            .buttonStyle(.plain)
                            .help("禁用插件")
                        }
                    }
                }
                .frame(width: 60, alignment: .leading) // 固定宽度防止布局变化
            }
            .frame(width: 120, alignment: .leading) // 整个状态区域固定宽度

            Spacer()
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 6)
        .background(isEven ? Color(.controlBackgroundColor) : Color.clear)
        .onHover { hovering in
            withAnimation(.easeInOut(duration: 0.2)) {
                isHovered = hovering
            }
        }
    }

    private var isPluginLoaded: Bool {
        vstManager.loadedPlugins.contains(plugin.fileOrIdentifier)
    }

    private func loadPlugin() {
        let success = vstManager.loadPlugin(named: plugin.fileOrIdentifier)
        if success {
            print("✅ 成功启用插件: \(plugin.name)")
        } else {
            print("❌ 启用插件失败: \(plugin.name)")
        }
    }

    private func unloadPlugin() {
        let success = vstManager.unloadPlugin(identifier: plugin.fileOrIdentifier)
        if success {
            print("✅ 成功禁用插件: \(plugin.name)")
        } else {
            print("❌ 禁用插件失败: \(plugin.name)")
        }
    }

    private func showPluginUI() {
        if vstManager.hasPluginEditor(identifier: plugin.fileOrIdentifier) {
            vstManager.showPluginEditor(identifier: plugin.fileOrIdentifier)
            print("🎛️ 打开插件界面: \(plugin.name)")
        } else {
            print("⚠️ 插件没有界面: \(plugin.name)")
        }
    }
}

#Preview {
    VSTProcessorView()
}
