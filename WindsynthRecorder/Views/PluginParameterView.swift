//
//  PluginParameterView.swift
//  WindsynthRecorder
//
//  Created by AI Assistant
//  VST插件参数控制界面
//

import SwiftUI

/// 插件参数控制界面
struct PluginParameterView: View {
    let pluginName: String
    let nodeID: UInt32
    let audioGraphService: AudioGraphService
    
    @Environment(\.dismiss) private var dismiss
    @State private var parameters: [PluginParameter] = []
    @State private var isLoading = true
    @State private var errorMessage: String?
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // 插件信息头部
                headerSection
                
                Divider()
                
                // 参数控制区域
                if isLoading {
                    loadingSection
                } else if parameters.isEmpty {
                    emptyParametersSection
                } else {
                    parametersSection
                }
                
                Spacer()
            }
            .navigationTitle("插件参数")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("完成") {
                        dismiss()
                    }
                }
            }
        }
        .frame(minWidth: 600, minHeight: 500)
        .onAppear {
            loadPluginParameters()
        }
    }
    
    // MARK: - 视图组件
    
    private var headerSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text(pluginName)
                    .font(.title2)
                    .fontWeight(.semibold)
                
                Spacer()
                
                // 插件状态指示器
                HStack(spacing: 4) {
                    Circle()
                        .fill(Color.green)
                        .frame(width: 8, height: 8)
                    
                    Text("已加载")
                        .font(.caption)
                        .foregroundColor(.green)
                }
            }
            
            Text("实时参数控制")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .padding()
        .background(Color(.controlBackgroundColor))
    }
    
    private var loadingSection: some View {
        VStack(spacing: 16) {
            ProgressView()
                .progressViewStyle(CircularProgressViewStyle())
            
            Text("加载插件参数中...")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
    private var emptyParametersSection: some View {
        VStack(spacing: 16) {
            Image(systemName: "slider.horizontal.3")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("该插件暂无可调参数")
                .font(.headline)
                .foregroundColor(.secondary)
            
            Text("或参数加载失败")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
    private var parametersSection: some View {
        ScrollView {
            LazyVStack(spacing: 12) {
                ForEach(parameters, id: \.id) { parameter in
                    ParameterControlView(
                        parameter: parameter,
                        nodeID: nodeID,
                        audioGraphService: audioGraphService,
                        pluginName: pluginName
                    )
                }
            }
            .padding()
        }
    }
    
    // MARK: - 方法
    
    private func loadPluginParameters() {
        isLoading = true
        errorMessage = nil

        DispatchQueue.global(qos: .userInitiated).async {
            // 从AudioGraphService获取真实参数数据
            let paramCount = audioGraphService.getNodeParameterCount(nodeID: nodeID)
            var loadedParameters: [PluginParameter] = []

            for i in 0..<paramCount {
                if let paramInfo = audioGraphService.getNodeParameterInfo(nodeID: nodeID, parameterIndex: i) {
                    let parameter = PluginParameter(
                        id: i,
                        name: paramInfo.name,
                        value: paramInfo.currentValue,
                        minValue: paramInfo.minValue,
                        maxValue: paramInfo.maxValue,
                        defaultValue: paramInfo.defaultValue,
                        unit: paramInfo.units
                    )
                    loadedParameters.append(parameter)
                }
            }

            DispatchQueue.main.async {
                if loadedParameters.isEmpty {
                    // 如果没有参数，使用模拟数据作为后备
                    parameters = createSampleParameters()
                } else {
                    parameters = loadedParameters
                }
                isLoading = false
            }
        }
    }
    
    private func createSampleParameters() -> [PluginParameter] {
        // 创建示例参数（实际实现应该从VST插件获取）
        return [
            PluginParameter(
                id: 0,
                name: "Input Gain",
                value: 0.75,
                minValue: 0.0,
                maxValue: 2.0,
                defaultValue: 1.0,
                unit: "dB"
            ),
            PluginParameter(
                id: 1,
                name: "Low Cut",
                value: 0.2,
                minValue: 0.0,
                maxValue: 1.0,
                defaultValue: 0.0,
                unit: "Hz"
            ),
            PluginParameter(
                id: 2,
                name: "High Cut",
                value: 0.8,
                minValue: 0.0,
                maxValue: 1.0,
                defaultValue: 1.0,
                unit: "Hz"
            ),
            PluginParameter(
                id: 3,
                name: "Compression Ratio",
                value: 0.4,
                minValue: 0.0,
                maxValue: 1.0,
                defaultValue: 0.5,
                unit: ":1"
            )
        ]
    }
}

// MARK: - 插件参数数据模型

/// 插件参数数据模型
struct PluginParameter {
    let id: Int
    let name: String
    var value: Float
    let minValue: Float
    let maxValue: Float
    let defaultValue: Float
    let unit: String
    
    var displayValue: String {
        if unit == "dB" {
            let dbValue = 20 * log10(value)
            return String(format: "%.1f %@", dbValue, unit)
        } else if unit == "Hz" {
            let hzValue = minValue + (maxValue - minValue) * value
            return String(format: "%.0f %@", hzValue, unit)
        } else {
            return String(format: "%.2f %@", value, unit)
        }
    }
}

// MARK: - 参数控制视图

/// 单个参数控制视图
struct ParameterControlView: View {
    @State var parameter: PluginParameter
    let nodeID: UInt32
    let audioGraphService: AudioGraphService
    let pluginName: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // 参数名称和值显示
            HStack {
                Text(parameter.name)
                    .font(.system(.body, weight: .medium))
                
                Spacer()
                
                Text(parameter.displayValue)
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.secondary)
            }
            
            // 参数滑块
            HStack {
                Slider(
                    value: $parameter.value,
                    in: parameter.minValue...parameter.maxValue
                ) { isEditing in
                    if !isEditing {
                        // 滑块编辑结束时更新插件参数
                        updatePluginParameter()
                    }
                }
                .onChange(of: parameter.value) { _ in
                    // 实时更新参数值（可选，可能会导致性能问题）
                    updatePluginParameter()
                }
                
                // 重置按钮
                Button(action: resetParameter) {
                    Image(systemName: "arrow.counterclockwise")
                        .font(.caption)
                }
                .buttonStyle(.bordered)
                .controlSize(.mini)
                .help("重置为默认值")
            }
        }
        .padding()
        .background(Color(.controlBackgroundColor))
        .cornerRadius(8)
    }
    
    private func updatePluginParameter() {
        let success = audioGraphService.setNodeParameter(nodeID: nodeID, parameterIndex: parameter.id, value: parameter.value)
        if success {
            print("✅ 参数更新成功: \(pluginName) - \(parameter.name) = \(parameter.value)")
        } else {
            print("❌ 参数更新失败: \(pluginName) - \(parameter.name) = \(parameter.value)")
        }
    }
    
    private func resetParameter() {
        parameter.value = parameter.defaultValue
        updatePluginParameter()
    }
}

#Preview {
    PluginParameterView(
        pluginName: "示例插件",
        nodeID: 1,
        audioGraphService: AudioGraphService.shared
    )
}
