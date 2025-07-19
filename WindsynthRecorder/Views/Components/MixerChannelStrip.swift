//
//  MixerChannelStrip.swift
//  WindsynthRecorder
//
//  混音台通道条组件
//

import SwiftUI

/// 专业通道条
struct ProfessionalChannelStrip: View {
    let title: String
    @Binding var gain: Float
    let level: Float
    let isMuted: Bool
    let isSolo: Bool
    let color: Color
    
    var body: some View {
        VStack(spacing: 12) {
            // 通道标题
            Text(title)
                .font(.system(.caption, design: .rounded, weight: .bold))
                .foregroundColor(.white)
                .tracking(1)
            
            // 增益旋钮
            ProfessionalKnob(
                value: $gain,
                range: 0...2,
                label: "GAIN"
            )
            
            // 垂直推子
            ProfessionalFader(
                value: $gain,
                range: 0...2,
                color: color
            )
            .frame(height: 200)
            
            // 静音/独奏按钮
            HStack(spacing: 8) {
                ProfessionalChannelButton(
                    label: "M",
                    isActive: isMuted,
                    color: .red
                )
                
                ProfessionalChannelButton(
                    label: "S",
                    isActive: isSolo,
                    color: .yellow
                )
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 16)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.3))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(Color.gray.opacity(0.2), lineWidth: 1)
        )
    }
}

/// 专业推子控件
struct ProfessionalFader: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let color: Color
    
    @State private var isDragging = false
    
    var body: some View {
        GeometryReader { geometry in
            let trackHeight = geometry.size.height - 20
            let knobPosition = CGFloat((value - range.lowerBound) / (range.upperBound - range.lowerBound)) * trackHeight
            
            ZStack(alignment: .bottom) {
                // 推子轨道
                RoundedRectangle(cornerRadius: 3)
                    .fill(Color.black.opacity(0.6))
                    .frame(width: 8)
                    .overlay(
                        RoundedRectangle(cornerRadius: 3)
                            .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    )
                
                // 推子把手
                RoundedRectangle(cornerRadius: 4)
                    .fill(
                        LinearGradient(
                            colors: [
                                Color.white.opacity(0.9),
                                Color.gray.opacity(0.7)
                            ],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                    .frame(width: 20, height: 20)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(Color.black.opacity(0.3), lineWidth: 1)
                    )
                    .shadow(color: .black.opacity(0.3), radius: 2, x: 0, y: 1)
                    .offset(y: -knobPosition)
                    .scaleEffect(isDragging ? 1.1 : 1.0)
                    .animation(.easeInOut(duration: 0.1), value: isDragging)
                    .gesture(
                        DragGesture()
                            .onChanged { gesture in
                                isDragging = true
                                let newPosition = knobPosition - gesture.translation.height
                                let clampedPosition = max(0, min(trackHeight, newPosition))
                                let newValue = range.lowerBound + Float(clampedPosition / trackHeight) * (range.upperBound - range.lowerBound)
                                value = newValue
                            }
                            .onEnded { _ in
                                isDragging = false
                            }
                    )
            }
        }
    }
}

/// 专业通道按钮
struct ProfessionalChannelButton: View {
    let label: String
    let isActive: Bool
    let color: Color
    
    var body: some View {
        Button(action: {}) {
            Text(label)
                .font(.system(.caption, design: .rounded, weight: .bold))
                .foregroundColor(isActive ? .black : .white)
                .frame(width: 20, height: 20)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(isActive ? color : Color.gray.opacity(0.3))
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 3)
                        .stroke(Color.black.opacity(0.3), lineWidth: 1)
                )
        }
        .buttonStyle(.plain)
    }
}
