//
//  AudioExportProgressView.swift
//  WindsynthRecorder
//
//  音频导出进度显示界面
//

import SwiftUI

struct AudioExportProgressView: View {
    @ObservedObject var exportService: AudioExportService
    @Binding var isPresented: Bool
    
    @State private var showingCompletedTasks = false
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // 顶部信息
                headerSection
                
                // 总体进度
                overallProgressSection
                
                // 任务列表
                ScrollView {
                    LazyVStack(spacing: 8) {
                        ForEach(filteredTasks, id: \.id) { task in
                            TaskProgressRow(task: task, exportService: exportService)
                        }
                    }
                    .padding(16)
                }
                
                // 底部控制
                bottomControlSection
            }
        }
        .frame(width: 500, height: 600)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
    }
    
    // MARK: - 界面组件
    
    private var headerSection: some View {
        VStack(spacing: 8) {
            HStack {
                Image(systemName: "arrow.down.circle.fill")
                    .font(.title2)
                    .foregroundColor(.blue)
                
                Text("导出进度")
                    .font(.title2)
                    .fontWeight(.semibold)
                
                Spacer()
                
                Button("关闭") {
                    isPresented = false
                }
                .keyboardShortcut(.escape)
                .disabled(exportService.isProcessing)
            }
            
            HStack {
                Text("\(exportService.tasks.count) 个任务")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                if exportService.isProcessing {
                    HStack(spacing: 4) {
                        ProgressView()
                            .scaleEffect(0.7)
                        Text("处理中...")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }
    
    private var overallProgressSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("总体进度")
                    .font(.headline)
                
                Spacer()
                
                Text("\(Int(exportService.overallProgress * 100))%")
                    .font(.headline)
                    .foregroundColor(.blue)
            }
            
            ProgressView(value: exportService.overallProgress)
                .progressViewStyle(LinearProgressViewStyle(tint: .blue))
            
            HStack {
                let completedCount = exportService.tasks.filter { $0.status == .completed }.count
                let failedCount = exportService.tasks.filter { $0.status == .failed }.count
                let totalCount = exportService.tasks.count
                
                Text("完成: \(completedCount)")
                    .font(.caption)
                    .foregroundColor(.green)
                
                Text("失败: \(failedCount)")
                    .font(.caption)
                    .foregroundColor(.red)
                
                Spacer()
                
                Text("总计: \(totalCount)")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(16)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
        .padding(.horizontal, 16)
    }
    
    private var bottomControlSection: some View {
        HStack {
            Toggle("显示已完成", isOn: $showingCompletedTasks)
                .toggleStyle(.switch)
            
            Spacer()
            
            if exportService.isProcessing {
                Button("停止") {
                    exportService.stopProcessing()
                }
                .buttonStyle(.bordered)
                .foregroundColor(.red)
            } else {
                Button("清空列表") {
                    exportService.clearAllTasks()
                }
                .buttonStyle(.bordered)
                .disabled(exportService.tasks.isEmpty)
            }
            
            Button("关闭") {
                isPresented = false
            }
            .buttonStyle(.borderedProminent)
            .disabled(exportService.isProcessing)
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }
    
    // MARK: - 计算属性
    
    private var filteredTasks: [ExportTask] {
        if showingCompletedTasks {
            return exportService.tasks
        } else {
            return exportService.tasks.filter { $0.status != .completed }
        }
    }
}

// MARK: - 任务进度行

struct TaskProgressRow: View {
    let task: ExportTask
    @ObservedObject var exportService: AudioExportService
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                // 状态图标
                statusIcon
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(task.inputURL.lastPathComponent)
                        .font(.system(.body, design: .monospaced))
                        .lineLimit(1)
                    
                    Text("→ \(task.outputURL.lastPathComponent)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                }
                
                Spacer()
                
                // 进度信息
                VStack(alignment: .trailing, spacing: 2) {
                    Text(statusText)
                        .font(.caption)
                        .foregroundColor(statusColor)
                    
                    if task.status == .processing {
                        Text("\(Int(task.progress * 100))%")
                            .font(.caption)
                            .foregroundColor(.blue)
                    }
                }
                
                // 操作按钮
                if task.status == .pending || task.status == .failed {
                    Button(action: {
                        exportService.removeTask(taskId: task.id)
                    }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.red)
                    }
                    .buttonStyle(.plain)
                }
            }
            
            // 进度条
            if task.status == .processing {
                ProgressView(value: task.progress)
                    .progressViewStyle(LinearProgressViewStyle(tint: .blue))
            }
            
            // 错误信息
            if let error = task.error, !error.isEmpty {
                Text(error)
                    .font(.caption)
                    .foregroundColor(.red)
                    .padding(.top, 4)
            }
        }
        .padding(12)
        .background(backgroundColor, in: RoundedRectangle(cornerRadius: 8))
    }
    
    // MARK: - 计算属性
    
    private var statusIcon: some View {
        Group {
            switch task.status {
            case .pending:
                Image(systemName: "clock")
                    .foregroundColor(.orange)
            case .processing:
                ProgressView()
                    .scaleEffect(0.8)
            case .completed:
                Image(systemName: "checkmark.circle.fill")
                    .foregroundColor(.green)
            case .failed:
                Image(systemName: "xmark.circle.fill")
                    .foregroundColor(.red)
            case .cancelled:
                Image(systemName: "minus.circle.fill")
                    .foregroundColor(.gray)
            }
        }
        .frame(width: 20, height: 20)
    }
    
    private var statusText: String {
        switch task.status {
        case .pending: return "等待中"
        case .processing: return "处理中"
        case .completed: return "已完成"
        case .failed: return "失败"
        case .cancelled: return "已取消"
        }
    }
    
    private var statusColor: Color {
        switch task.status {
        case .pending: return .orange
        case .processing: return .blue
        case .completed: return .green
        case .failed: return .red
        case .cancelled: return .gray
        }
    }
    
    private var backgroundColor: Color {
        switch task.status {
        case .pending: return .orange.opacity(0.1)
        case .processing: return .blue.opacity(0.1)
        case .completed: return .green.opacity(0.1)
        case .failed: return .red.opacity(0.1)
        case .cancelled: return .gray.opacity(0.1)
        }
    }
}

#Preview {
    AudioExportProgressView(
        exportService: AudioExportService(),
        isPresented: .constant(true)
    )
}
