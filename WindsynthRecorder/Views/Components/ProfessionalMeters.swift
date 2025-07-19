//
//  ProfessionalMeters.swift
//  WindsynthRecorder
//
//  专业电平表组件
//

import SwiftUI

/// 专业主电平表
struct ProfessionalMasterMeter: View {
    let leftLevel: Float
    let rightLevel: Float
    
    var body: some View {
        HStack(spacing: 4) {
            VStack(spacing: 2) {
                Text("L")
                    .font(.system(.caption2, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                
                ProfessionalVerticalMeter(level: leftLevel)
                    .frame(width: 12, height: 80)
            }
            
            VStack(spacing: 2) {
                Text("R")
                    .font(.system(.caption2, design: .rounded, weight: .bold))
                    .foregroundColor(.gray)
                
                ProfessionalVerticalMeter(level: rightLevel)
                    .frame(width: 12, height: 80)
            }
        }
    }
}

/// 专业垂直电平表
struct ProfessionalVerticalMeter: View {
    let level: Float
    
    var body: some View {
        GeometryReader { geometry in
            let segments = 20
            let segmentHeight = geometry.size.height / CGFloat(segments)
            let activeSegments = Int(Float(segments) * level)
            
            VStack(spacing: 1) {
                ForEach(0..<segments, id: \.self) { index in
                    let segmentIndex = segments - 1 - index
                    let isActive = segmentIndex < activeSegments
                    
                    Rectangle()
                        .fill(isActive ? segmentColor(for: segmentIndex, total: segments) : Color.black.opacity(0.3))
                        .frame(height: segmentHeight - 1)
                        .cornerRadius(1)
                }
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 2)
                .fill(Color.black.opacity(0.6))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 2)
                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
        )
    }
    
    private func segmentColor(for index: Int, total: Int) -> Color {
        let ratio = Float(index) / Float(total)
        if ratio > 0.85 {
            return .red
        } else if ratio > 0.7 {
            return .orange
        } else if ratio > 0.5 {
            return .yellow
        } else {
            return .green
        }
    }
}

/// 专业进度条
struct ProfessionalProgressBar: View {
    let progress: Double
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // 背景轨道
                RoundedRectangle(cornerRadius: 2)
                    .fill(Color.black.opacity(0.5))
                    .frame(height: 4)
                
                // 进度条
                RoundedRectangle(cornerRadius: 2)
                    .fill(
                        LinearGradient(
                            colors: [Color.blue, Color.cyan],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                    )
                    .frame(width: geometry.size.width * progress, height: 4)
                
                // 播放头
                Circle()
                    .fill(Color.white)
                    .frame(width: 8, height: 8)
                    .offset(x: geometry.size.width * progress - 4)
                    .shadow(color: .black.opacity(0.3), radius: 2)
            }
        }
        .frame(height: 8)
    }
}
