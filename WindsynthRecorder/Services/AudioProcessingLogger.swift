import Foundation
import SwiftUI

class AudioProcessingLogger: ObservableObject {
    static let shared = AudioProcessingLogger()
    
    @Published var logs: [LogEntry] = []
    @Published var isShowingLogs = false
    
    struct LogEntry: Identifiable {
        let id = UUID()
        let timestamp: Date
        let level: LogLevel
        let message: String
        let details: String?
        
        enum LogLevel {
            case info
            case warning
            case error
            case success
            
            var color: Color {
                switch self {
                case .info: return .blue
                case .warning: return .orange
                case .error: return .red
                case .success: return .green
                }
            }
            
            var icon: String {
                switch self {
                case .info: return "info.circle"
                case .warning: return "exclamationmark.triangle"
                case .error: return "xmark.circle"
                case .success: return "checkmark.circle"
                }
            }
            
            var title: String {
                switch self {
                case .info: return "信息"
                case .warning: return "警告"
                case .error: return "错误"
                case .success: return "成功"
                }
            }
        }
    }
    
    private init() {}
    
    func log(_ level: LogEntry.LogLevel, message: String, details: String? = nil) {
        DispatchQueue.main.async {
            let entry = LogEntry(
                timestamp: Date(),
                level: level,
                message: message,
                details: details
            )
            self.logs.insert(entry, at: 0) // 最新的在顶部
            
            // 保持最多100条日志
            if self.logs.count > 100 {
                self.logs.removeLast()
            }
            
            // 打印到控制台（开发时使用）
            print("[\(level.title)] \(message)")
            if let details = details {
                print("详细信息: \(details)")
            }
        }
    }
    
    func info(_ message: String, details: String? = nil) {
        log(.info, message: message, details: details)
    }
    
    func warning(_ message: String, details: String? = nil) {
        log(.warning, message: message, details: details)
    }
    
    func error(_ message: String, details: String? = nil) {
        log(.error, message: message, details: details)
    }
    
    func success(_ message: String, details: String? = nil) {
        log(.success, message: message, details: details)
    }
    
    func clearLogs() {
        DispatchQueue.main.async {
            self.logs.removeAll()
        }
    }
}

// MARK: - Log Viewer
struct AudioProcessingLogView: View {
    @ObservedObject var logger = AudioProcessingLogger.shared
    @Binding var isPresented: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("音频处理日志")
                        .font(.system(size: 16, weight: .semibold))
                    
                    Text("查看详细的处理过程和错误信息")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }
                
                Spacer()
                
                Button("清空日志") {
                    logger.clearLogs()
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
                
                Button("关闭") {
                    isPresented = false
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
            }
            .padding(16)
            .background(Color.primary.opacity(0.03))
            
            Divider()
            
            // Logs List
            if logger.logs.isEmpty {
                VStack(spacing: 12) {
                    Image(systemName: "doc.text")
                        .font(.system(size: 32, weight: .light))
                        .foregroundStyle(.secondary)
                    
                    Text("暂无日志")
                        .font(.system(size: 14, weight: .medium))
                        .foregroundStyle(.secondary)
                    
                    Text("音频处理过程中的信息将显示在这里")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                ScrollView {
                    LazyVStack(spacing: 8) {
                        ForEach(logger.logs) { entry in
                            LogEntryView(entry: entry)
                        }
                    }
                    .padding(16)
                }
            }
        }
        .frame(width: 600, height: 500)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
    }
}

struct LogEntryView: View {
    let entry: AudioProcessingLogger.LogEntry
    @State private var isExpanded = false
    
    private var timeFormatter: DateFormatter {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        return formatter
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 8) {
                Image(systemName: entry.level.icon)
                    .font(.system(size: 12, weight: .medium))
                    .foregroundStyle(entry.level.color)
                    .frame(width: 16)
                
                Text(entry.message)
                    .font(.system(size: 13, weight: .medium))
                    .lineLimit(isExpanded ? nil : 2)
                
                Spacer()
                
                Text(timeFormatter.string(from: entry.timestamp))
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(.secondary)
                
                if entry.details != nil {
                    Button(action: {
                        withAnimation(.easeInOut(duration: 0.2)) {
                            isExpanded.toggle()
                        }
                    }) {
                        Image(systemName: isExpanded ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundStyle(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            
            if isExpanded, let details = entry.details {
                Text(details)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(.secondary)
                    .padding(.leading, 24)
                    .padding(.top, 4)
                    .textSelection(.enabled)
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(entry.level.color.opacity(0.05), in: RoundedRectangle(cornerRadius: 6))
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(entry.level.color.opacity(0.2), lineWidth: 1)
        )
        .onTapGesture {
            if entry.details != nil {
                withAnimation(.easeInOut(duration: 0.2)) {
                    isExpanded.toggle()
                }
            }
        }
    }
}

#Preview {
    AudioProcessingLogView(isPresented: .constant(true))
}
