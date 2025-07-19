//
//  ProfessionalControls.swift
//  WindsynthRecorder
//
//  专业音频控件组件
//

import SwiftUI

/// 专业状态 LED 指示器
struct ProfessionalStatusLED: View {
    let label: String
    let isActive: Bool
    let color: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Circle()
                .fill(isActive ? color : Color.gray.opacity(0.3))
                .frame(width: 8, height: 8)
                .overlay(
                    Circle()
                        .stroke(Color.white.opacity(0.3), lineWidth: 1)
                )
                .shadow(color: isActive ? color : .clear, radius: 4)
            
            Text(label)
                .font(.system(.caption2, design: .rounded, weight: .bold))
                .foregroundColor(isActive ? color : .gray)
                .tracking(0.5)
        }
    }
}

/// 专业传输控制按钮
struct ProfessionalTransportButton: View {
    let icon: String
    let action: () -> Void
    let isEnabled: Bool
    let color: Color
    var isLarge: Bool = false
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: isLarge ? 20 : 16, weight: .medium))
                .foregroundColor(isEnabled ? .white : .gray)
                .frame(width: isLarge ? 50 : 40, height: isLarge ? 50 : 40)
                .background(
                    Circle()
                        .fill(
                            LinearGradient(
                                colors: isEnabled ? [
                                    color.opacity(0.8),
                                    color.opacity(0.6)
                                ] : [
                                    Color.gray.opacity(0.3),
                                    Color.gray.opacity(0.2)
                                ],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                        .overlay(
                            Circle()
                                .stroke(
                                    LinearGradient(
                                        colors: [
                                            Color.white.opacity(0.3),
                                            Color.clear
                                        ],
                                        startPoint: .topLeading,
                                        endPoint: .bottomTrailing
                                    ),
                                    lineWidth: 1
                                )
                        )
                        .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
                )
        }
        .buttonStyle(.plain)
        .disabled(!isEnabled)
        .scaleEffect(isEnabled ? 1.0 : 0.95)
        .animation(.easeInOut(duration: 0.1), value: isEnabled)
    }
}

/// 专业旋钮控件
struct ProfessionalKnob: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let label: String
    
    @State private var isDragging = false
    @State private var lastDragValue: CGFloat = 0
    
    var body: some View {
        VStack(spacing: 6) {
            ZStack {
                // 外圈
                Circle()
                    .stroke(Color.gray.opacity(0.3), lineWidth: 2)
                    .frame(width: 40, height: 40)
                
                // 内圈
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [
                                Color.gray.opacity(0.8),
                                Color.gray.opacity(0.4)
                            ],
                            center: .topLeading,
                            startRadius: 5,
                            endRadius: 20
                        )
                    )
                    .frame(width: 36, height: 36)
                
                // 指示器
                Rectangle()
                    .fill(Color.white)
                    .frame(width: 2, height: 8)
                    .offset(y: -12)
                    .rotationEffect(.degrees(Double((value - range.lowerBound) / (range.upperBound - range.lowerBound)) * 270 - 135))
                
                // 中心点
                Circle()
                    .fill(Color.black.opacity(0.6))
                    .frame(width: 4, height: 4)
            }
            .scaleEffect(isDragging ? 1.05 : 1.0)
            .animation(.easeInOut(duration: 0.1), value: isDragging)
            .gesture(
                DragGesture()
                    .onChanged { gesture in
                        if !isDragging {
                            isDragging = true
                            lastDragValue = gesture.translation.height
                        }
                        
                        let delta = Float(lastDragValue - gesture.translation.height) * 0.01
                        let newValue = value + delta * (range.upperBound - range.lowerBound)
                        value = max(range.lowerBound, min(range.upperBound, newValue))
                        lastDragValue = gesture.translation.height
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )
            
            Text(label)
                .font(.system(.caption2, design: .rounded, weight: .medium))
                .foregroundColor(.gray)
                .tracking(0.5)
            
            Text(String(format: "%.1f", value))
                .font(.system(.caption2, design: .monospaced))
                .foregroundColor(.white)
        }
    }
}

/// 专业开关控件
struct ProfessionalToggle: View {
    @Binding var isOn: Bool
    
    var body: some View {
        Button(action: { isOn.toggle() }) {
            RoundedRectangle(cornerRadius: 4)
                .fill(isOn ? Color.green : Color.gray.opacity(0.3))
                .frame(width: 30, height: 16)
                .overlay(
                    Circle()
                        .fill(Color.white)
                        .frame(width: 12, height: 12)
                        .offset(x: isOn ? 7 : -7)
                        .animation(.easeInOut(duration: 0.2), value: isOn)
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(Color.black.opacity(0.2), lineWidth: 1)
                )
        }
        .buttonStyle(.plain)
    }
}
