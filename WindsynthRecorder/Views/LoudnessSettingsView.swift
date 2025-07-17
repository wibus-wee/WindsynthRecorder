import SwiftUI

struct LoudnessSettingsView: View {
    @Binding var settings: LoudnessSettings
    @Binding var isPresented: Bool
    
    var body: some View {
        VStack(spacing: 20) {
            Text("响度最大化设置")
                .font(.headline)
                .padding(.top)
            
            ScrollView {
                VStack(spacing: 16) {
                    // 预设选择
                    presetSection
                    
                    Divider()
                    
                    // 高级设置
                    advancedSection
                }
                .padding()
            }
            
            // 底部按钮
            HStack {
                Button("重置为默认") {
                    settings = LoudnessSettings()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("取消") {
                    isPresented = false
                }
                .buttonStyle(.bordered)
                
                Button("确定") {
                    isPresented = false
                }
                .buttonStyle(.borderedProminent)
            }
            .padding()
        }
        .frame(width: 500, height: 600)
    }
    
    private var presetSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("预设模式")
                .font(.subheadline)
                .fontWeight(.semibold)
            
            VStack(spacing: 8) {
                ForEach(LoudnessPreset.allCases, id: \.self) { preset in
                    HStack {
                        Button(action: {
                            settings.applyPreset(preset)
                        }) {
                            HStack {
                                Image(systemName: settings.currentPreset == preset ? "checkmark.circle.fill" : "circle")
                                    .foregroundColor(settings.currentPreset == preset ? .blue : .gray)
                                
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(preset.displayName)
                                        .fontWeight(.medium)
                                    Text(preset.description)
                                        .font(.caption)
                                        .foregroundColor(.secondary)
                                }
                                
                                Spacer()
                            }
                            .padding(.vertical, 4)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            .padding(.leading, 8)
        }
    }
    
    private var advancedSection: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("高级设置")
                .font(.subheadline)
                .fontWeight(.semibold)
            
            // 峰值设置
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("目标峰值")
                    Spacer()
                    Text("\(settings.peak, specifier: "%.2f")")
                        .foregroundColor(.secondary)
                }

                Slider(value: $settings.peak, in: 0.5...1.0, step: 0.05)

                Text("设置音频的目标峰值水平。数值越高，音频越响亮。")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            // 最大增益
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("最大增益")
                    Spacer()
                    Text("\(settings.maxGain, specifier: "%.0f")x")
                        .foregroundColor(.secondary)
                }

                Slider(value: $settings.maxGain, in: 1...50, step: 1)

                Text("限制音频增益的最大倍数，防止过度放大造成失真。")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            // 滤波器大小
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("滤波器大小")
                    Spacer()
                    Text("\(settings.gaussSize)")
                        .foregroundColor(.secondary)
                }

                Slider(value: Binding(
                    get: { Double(settings.gaussSize) },
                    set: { settings.gaussSize = Int($0) }
                ), in: 3...301, step: 2)

                Text("控制响度分析的精度。较小值响应更快，较大值更平滑。")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            // 压缩因子
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("压缩因子")
                    Spacer()
                    Text("\(settings.compress, specifier: "%.1f")")
                        .foregroundColor(.secondary)
                }

                Slider(value: $settings.compress, in: 0.0...30.0, step: 0.5)

                Text("控制动态范围压缩的强度。0为禁用，数值越高压缩越强。")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            // DC校正
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Toggle(isOn: $settings.enableDCCorrection) {
                        VStack(alignment: .leading, spacing: 1) {
                            Text("DC偏移校正")
                                .font(.system(size: 13, weight: .medium))
                            Text("移除音频中的直流偏移，改善音质")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                    .toggleStyle(.switch)

                    Spacer()
                }
            }
        }
    }
}

// 响度设置数据模型
struct LoudnessSettings {
    var peak: Double = 0.95         // 目标峰值 (0-1.0)
    var maxGain: Double = 10.0      // 最大增益 (1-100)
    var gaussSize: Int = 31         // 滤波器大小 (3-301)
    var compress: Double = 0.0      // 压缩因子 (0-30)
    var enableDCCorrection: Bool = false  // DC校正
    var currentPreset: LoudnessPreset = .balanced

    // 生成 ffmpeg 参数字符串
    func toDynaudnormString() -> String {
        var params = [
            "p=\(peak)",           // peak value
            "m=\(maxGain)",        // max amplification
            "g=\(gaussSize)"       // filter size (gauss size)
        ]

        // 添加压缩因子（如果大于0）
        if compress > 0 {
            params.append("s=\(compress)")  // compress factor
        }

        // 添加DC校正（如果启用）
        if enableDCCorrection {
            params.append("c=true")  // DC correction
        }

        return "dynaudnorm=" + params.joined(separator: ":")
    }
    
    // 应用预设
    mutating func applyPreset(_ preset: LoudnessPreset) {
        currentPreset = preset
        switch preset {
        case .gentle:
            peak = 0.80  // 降低峰值，减少噪声
            maxGain = 3.0  // 进一步降低最大增益
            gaussSize = 31
            compress = 0.0
            enableDCCorrection = false
        case .balanced:
            peak = 0.90  // 稍微降低峰值
            maxGain = 7.0  // 降低最大增益
            gaussSize = 31
            compress = 0.0
            enableDCCorrection = false
        case .aggressive:
            peak = 0.95
            maxGain = 20.0
            gaussSize = 15
            compress = 5.0
            enableDCCorrection = true
        case .custom:
            break // 保持当前设置
        }
    }
}

// 预设模式
enum LoudnessPreset: CaseIterable {
    case gentle
    case balanced
    case aggressive
    case custom
    
    var displayName: String {
        switch self {
        case .gentle: return "轻柔模式"
        case .balanced: return "平衡模式"
        case .aggressive: return "强化模式"
        case .custom: return "自定义"
        }
    }
    
    var description: String {
        switch self {
        case .gentle: return "温和的响度提升，保持音频自然感"
        case .balanced: return "适中的响度增强，适合大多数情况"
        case .aggressive: return "强力的响度最大化，获得最大音量"
        case .custom: return "手动调整各项参数"
        }
    }
}

#Preview {
    LoudnessSettingsView(
        settings: .constant(LoudnessSettings()),
        isPresented: .constant(true)
    )
}
