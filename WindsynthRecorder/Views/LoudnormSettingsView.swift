import SwiftUI

struct LoudnormSettingsView: View {
    @Binding var settings: LoudnormSettings
    @Binding var isPresented: Bool
    
    var body: some View {
        VStack(spacing: 20) {
            Text("EBU R128 标准化设置")
                .font(.headline)
                .padding(.top)
            
            ScrollView {
                VStack(spacing: 16) {
                    // 预设选择
                    presetSection
                    
                    Divider()
                    
                    // 高级设置
                    advancedSection
                    
                    Divider()
                    
                    // 参数说明
                    infoSection
                }
                .padding()
            }
            
            // 底部按钮
            HStack {
                Button("重置为默认") {
                    settings = LoudnormSettings()
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
        .frame(width: 520, height: 650)
    }
    
    private var presetSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("预设模式")
                .font(.subheadline)
                .fontWeight(.semibold)
            
            VStack(spacing: 8) {
                ForEach(LoudnormPreset.allCases, id: \.self) { preset in
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
            
            // 目标响度 (I)
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("目标响度 (I)")
                    Spacer()
                    Text("\(settings.integratedLoudness, specifier: "%.1f") LUFS")
                        .foregroundColor(settings.isIntegratedLoudnessValid ? .secondary : .red)
                }

                Slider(value: $settings.integratedLoudness, in: -70.0...(-5.0), step: 0.5)

                if !settings.isIntegratedLoudnessValid {
                    Text("⚠️ 目标响度过高可能导致音频失真")
                        .font(.caption)
                        .foregroundColor(.red)
                } else {
                    Text("设置音频的目标响度水平。-16 LUFS 适合大多数内容。")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }

            // 响度范围 (LRA)
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("响度范围 (LRA)")
                    Spacer()
                    Text("\(settings.loudnessRange, specifier: "%.1f") LU")
                        .foregroundColor(settings.isLoudnessRangeValid ? .secondary : .orange)
                }

                Slider(value: $settings.loudnessRange, in: 1.0...50.0, step: 0.5)

                if !settings.isLoudnessRangeValid {
                    Text("⚠️ 响度范围过小可能导致动态压缩过度")
                        .font(.caption)
                        .foregroundColor(.orange)
                } else {
                    Text("控制音频的动态范围。较大值保持更多动态，较小值更一致。")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }

            // 真实峰值 (TP)
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("真实峰值 (TP)")
                    Spacer()
                    Text("\(settings.truePeak, specifier: "%.1f") dBFS")
                        .foregroundColor(settings.isTruePeakValid ? .secondary : .red)
                }

                Slider(value: $settings.truePeak, in: -9.0...0.0, step: 0.1)

                if !settings.isTruePeakValid {
                    Text("⚠️ 峰值限制过高可能导致削波失真")
                        .font(.caption)
                        .foregroundColor(.red)
                } else {
                    Text("防止音频峰值超过指定水平。-3 dBFS 是安全的选择。")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private var infoSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("参数说明")
                .font(.subheadline)
                .fontWeight(.semibold)
            
            VStack(alignment: .leading, spacing: 6) {
                Text("• **目标响度 (I)**: 音频的整体响度目标，单位为 LUFS")
                Text("• **响度范围 (LRA)**: 允许的动态范围，单位为 LU")  
                Text("• **真实峰值 (TP)**: 峰值限制，防止削波失真，单位为 dBFS")
                Text("• **EBU R128**: 欧洲广播联盟制定的响度标准")
            }
            .font(.caption)
            .foregroundColor(.secondary)
        }
    }
}

// Loudnorm 设置数据模型
struct LoudnormSettings {
    var integratedLoudness: Double = -16.0    // 目标响度 (-70 to -5 LUFS)
    var loudnessRange: Double = 11.0          // 响度范围 (1 to 50 LU)
    var truePeak: Double = -3.0               // 真实峰值 (-9 to 0 dBFS)
    var currentPreset: LoudnormPreset = .balanced
    
    // 参数验证
    var isIntegratedLoudnessValid: Bool {
        integratedLoudness <= -10.0  // 警告阈值
    }
    
    var isLoudnessRangeValid: Bool {
        loudnessRange >= 5.0  // 警告阈值
    }
    
    var isTruePeakValid: Bool {
        truePeak <= -1.0  // 警告阈值
    }
    
    // 生成 ffmpeg 参数字符串
    func toLoudnormString() -> String {
        return "loudnorm=I=\(integratedLoudness):LRA=\(loudnessRange):TP=\(truePeak)"
    }
    
    // 应用预设
    mutating func applyPreset(_ preset: LoudnormPreset) {
        currentPreset = preset
        switch preset {
        case .gentle:
            integratedLoudness = -18.0
            loudnessRange = 15.0
            truePeak = -4.0
        case .balanced:
            integratedLoudness = -16.0
            loudnessRange = 11.0
            truePeak = -3.0
        case .aggressive:
            integratedLoudness = -14.0
            loudnessRange = 7.0
            truePeak = -2.0
        case .broadcast:
            integratedLoudness = -23.0
            loudnessRange = 18.0
            truePeak = -2.0
        case .custom:
            break // 保持当前设置
        }
    }
}

// Loudnorm 预设模式
enum LoudnormPreset: CaseIterable {
    case gentle
    case balanced
    case aggressive
    case broadcast
    case custom
    
    var displayName: String {
        switch self {
        case .gentle: return "轻柔模式"
        case .balanced: return "平衡模式"
        case .aggressive: return "强化模式"
        case .broadcast: return "广播标准"
        case .custom: return "自定义"
        }
    }
    
    var description: String {
        switch self {
        case .gentle: return "温和的响度标准化，保持更多动态范围"
        case .balanced: return "适中的响度处理，适合大多数内容"
        case .aggressive: return "强力的响度标准化，获得一致的音量"
        case .broadcast: return "符合广播标准的 -23 LUFS 设置"
        case .custom: return "手动调整各项参数"
        }
    }
}

#Preview {
    LoudnormSettingsView(
        settings: .constant(LoudnormSettings()),
        isPresented: .constant(true)
    )
}
