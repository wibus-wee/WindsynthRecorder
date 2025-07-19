import Foundation
import SwiftUI

class FFmpegManager: ObservableObject {
    static let shared = FFmpegManager()
    
    @Published var ffmpegPath: String = ""
    @Published var isFFmpegAvailable: Bool = false
    @Published var ffmpegVersion: String = ""
    
    private let userDefaults = UserDefaults.standard
    private let ffmpegPathKey = "FFmpegPath"
    
    // 常见的 FFmpeg 安装路径
    private let commonPaths = [
        "/opt/homebrew/bin/ffmpeg",           // Apple Silicon Homebrew
        "/usr/local/bin/ffmpeg",              // Intel Homebrew
        "/usr/bin/ffmpeg",                    // 系统安装
        "/usr/local/Cellar/ffmpeg/*/bin/ffmpeg", // Homebrew Cellar (需要通配符处理)
        "/Applications/ffmpeg",               // 手动安装
        "~/bin/ffmpeg",                       // 用户本地安装
        "/snap/bin/ffmpeg"                    // Snap 安装 (Linux)
    ]
    
    private init() {
        loadSavedPath()
        // 不在初始化时自动发现，改为在 StartupInitializationView 中统一处理
    }
    
    // MARK: - Public Methods

    func initializeIfNeeded() {
        // 首先验证已保存的路径（如果有的话）
        if !ffmpegPath.isEmpty && validateFFmpegPath(ffmpegPath) {
            isFFmpegAvailable = true
            updateFFmpegVersion()
            return
        }

        // 如果没有保存的路径或路径无效，则自动发现
        autoDiscoverFFmpeg()
    }

    func validateFFmpegPath(_ path: String) -> Bool {
        let expandedPath = NSString(string: path).expandingTildeInPath
        let fileManager = FileManager.default
        
        // 检查文件是否存在且可执行
        var isDirectory: ObjCBool = false
        guard fileManager.fileExists(atPath: expandedPath, isDirectory: &isDirectory),
              !isDirectory.boolValue,
              fileManager.isExecutableFile(atPath: expandedPath) else {
            return false
        }
        
        // 尝试执行 ffmpeg -version 来验证
        return testFFmpegExecution(expandedPath)
    }
    
    func setFFmpegPath(_ path: String) {
        let expandedPath = NSString(string: path).expandingTildeInPath
        
        if validateFFmpegPath(expandedPath) {
            ffmpegPath = expandedPath
            isFFmpegAvailable = true
            saveFFmpegPath()
            updateFFmpegVersion()
            
            AudioProcessingLogger.shared.success("FFmpeg 路径设置成功", details: "路径: \(expandedPath)")
        } else {
            AudioProcessingLogger.shared.error("无效的 FFmpeg 路径", details: "路径: \(expandedPath)")
        }
    }
    
    func autoDiscoverFFmpeg() {
        AudioProcessingLogger.shared.info("开始自动探索 FFmpeg")
        
        // 首先尝试 which 命令
        if let whichPath = findFFmpegWithWhich() {
            setFFmpegPath(whichPath)
            return
        }
        
        // 然后尝试常见路径
        for path in commonPaths {
            let expandedPath = NSString(string: path).expandingTildeInPath
            
            // 处理通配符路径
            if path.contains("*") {
                if let resolvedPath = resolveWildcardPath(path) {
                    if validateFFmpegPath(resolvedPath) {
                        setFFmpegPath(resolvedPath)
                        return
                    }
                }
            } else {
                if validateFFmpegPath(expandedPath) {
                    setFFmpegPath(expandedPath)
                    return
                }
            }
        }
        
        // 如果都没找到
        isFFmpegAvailable = false
        ffmpegPath = ""
        ffmpegVersion = ""
        AudioProcessingLogger.shared.warning("未找到 FFmpeg", details: "请手动设置 FFmpeg 路径")
    }
    
    func getFFmpegPath() -> String? {
        return isFFmpegAvailable ? ffmpegPath : nil
    }
    
    // MARK: - Private Methods
    
    private func loadSavedPath() {
        if let savedPath = userDefaults.string(forKey: ffmpegPathKey) {
            ffmpegPath = savedPath
            // 不在加载时验证，延迟到 initializeIfNeeded() 中进行
            isFFmpegAvailable = false
        }
    }
    
    private func saveFFmpegPath() {
        userDefaults.set(ffmpegPath, forKey: ffmpegPathKey)
    }
    
    private func findFFmpegWithWhich() -> String? {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/which")
        process.arguments = ["ffmpeg"]

        let pipe = Pipe()
        process.standardOutput = pipe

        let command = "which ffmpeg"
        AudioProcessingLogger.shared.info("执行命令", details: command)

        do {
            try process.run()
            process.waitUntilExit()

            if process.terminationStatus == 0 {
                let data = pipe.fileHandleForReading.readDataToEndOfFile()
                if let output = String(data: data, encoding: .utf8)?.trimmingCharacters(in: .whitespacesAndNewlines),
                   !output.isEmpty {
                    AudioProcessingLogger.shared.success("命令执行成功", details: "命令: \(command)\n退出代码: \(process.terminationStatus)\n输出: \(output)")
                    return output
                }
            } else {
                AudioProcessingLogger.shared.warning("命令执行失败", details: "命令: \(command)\n退出代码: \(process.terminationStatus)")
            }
        } catch {
            AudioProcessingLogger.shared.error("命令执行异常", details: "命令: \(command)\n错误: \(error.localizedDescription)")
        }

        return nil
    }
    
    private func resolveWildcardPath(_ path: String) -> String? {
        let components = path.components(separatedBy: "/")
        var currentPath = ""
        
        for component in components {
            if component.contains("*") {
                // 处理通配符组件
                let parentPath = currentPath.isEmpty ? "/" : currentPath
                
                do {
                    let contents = try FileManager.default.contentsOfDirectory(atPath: parentPath)
                    let pattern = component.replacingOccurrences(of: "*", with: ".*")
                    let regex = try NSRegularExpression(pattern: "^" + pattern + "$")
                    
                    for item in contents {
                        let range = NSRange(location: 0, length: item.utf16.count)
                        if regex.firstMatch(in: item, options: [], range: range) != nil {
                            currentPath = parentPath + "/" + item
                            break
                        }
                    }
                } catch {
                    return nil
                }
            } else {
                currentPath += "/" + component
            }
        }
        
        return currentPath.isEmpty ? nil : currentPath
    }
    
    private func testFFmpegExecution(_ path: String) -> Bool {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: path)
        process.arguments = ["-version"]

        let pipe = Pipe()
        process.standardOutput = pipe
        process.standardError = pipe

        let command = "\(path) -version"
        AudioProcessingLogger.shared.info("测试 FFmpeg 可执行性", details: "命令: \(command)")

        do {
            try process.run()
            process.waitUntilExit()

            let data = pipe.fileHandleForReading.readDataToEndOfFile()
            let output = String(data: data, encoding: .utf8) ?? ""

            if process.terminationStatus == 0 {
                AudioProcessingLogger.shared.success("FFmpeg 测试成功", details: "命令: \(command)\n退出代码: \(process.terminationStatus)")
                return true
            } else {
                AudioProcessingLogger.shared.warning("FFmpeg 测试失败", details: "命令: \(command)\n退出代码: \(process.terminationStatus)\n输出: \(output)")
                return false
            }
        } catch {
            AudioProcessingLogger.shared.error("FFmpeg 测试异常", details: "命令: \(command)\n错误: \(error.localizedDescription)")
            return false
        }
    }
    
    private func updateFFmpegVersion() {
        guard !ffmpegPath.isEmpty else { return }

        let process = Process()
        process.executableURL = URL(fileURLWithPath: ffmpegPath)
        process.arguments = ["-version"]

        let pipe = Pipe()
        process.standardOutput = pipe

        let command = "\(ffmpegPath) -version"
        AudioProcessingLogger.shared.info("获取 FFmpeg 版本信息", details: "命令: \(command)")

        do {
            try process.run()
            process.waitUntilExit()

            if process.terminationStatus == 0 {
                let data = pipe.fileHandleForReading.readDataToEndOfFile()
                if let output = String(data: data, encoding: .utf8) {
                    // 提取版本信息（第一行）
                    let lines = output.components(separatedBy: .newlines)
                    if let firstLine = lines.first {
                        ffmpegVersion = firstLine
                        AudioProcessingLogger.shared.success("FFmpeg 版本信息获取成功", details: "命令: \(command)\n版本: \(firstLine)")
                    }
                }
            } else {
                ffmpegVersion = "版本信息获取失败"
                AudioProcessingLogger.shared.warning("FFmpeg 版本信息获取失败", details: "命令: \(command)\n退出代码: \(process.terminationStatus)")
            }
        } catch {
            ffmpegVersion = "版本信息获取失败"
            AudioProcessingLogger.shared.error("FFmpeg 版本信息获取异常", details: "命令: \(command)\n错误: \(error.localizedDescription)")
        }
    }
}

// MARK: - FFmpeg Settings View
struct FFmpegSettingsView: View {
    @ObservedObject var ffmpegManager = FFmpegManager.shared
    @Binding var isPresented: Bool
    @State private var customPath: String = ""
    @State private var showingFilePicker = false
    
    var body: some View {
        VStack(spacing: 20) {
            // Header
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("FFmpeg 设置")
                        .font(.system(size: 16, weight: .semibold))
                    
                    Text("配置 FFmpeg 可执行文件路径")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }
                
                Spacer()
                
                Button("关闭") {
                    isPresented = false
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
            }
            .padding()
            .background(Color.primary.opacity(0.03))
            
            Divider()
            
            VStack(spacing: 16) {
                // 当前状态
                VStack(alignment: .leading, spacing: 8) {
                    Text("当前状态")
                        .font(.system(size: 14, weight: .semibold))
                    
                    HStack {
                        Image(systemName: ffmpegManager.isFFmpegAvailable ? "checkmark.circle.fill" : "xmark.circle.fill")
                            .foregroundStyle(ffmpegManager.isFFmpegAvailable ? .green : .red)
                        
                        Text(ffmpegManager.isFFmpegAvailable ? "FFmpeg 可用" : "FFmpeg 不可用")
                            .font(.system(size: 13, weight: .medium))
                        
                        Spacer()
                    }
                    
                    if ffmpegManager.isFFmpegAvailable {
                        VStack(alignment: .leading, spacing: 4) {
                            Text("路径: \(ffmpegManager.ffmpegPath)")
                                .font(.system(size: 11, design: .monospaced))
                                .foregroundStyle(.secondary)
                            
                            if !ffmpegManager.ffmpegVersion.isEmpty {
                                Text(ffmpegManager.ffmpegVersion)
                                    .font(.system(size: 11, design: .monospaced))
                                    .foregroundStyle(.secondary)
                                    .lineLimit(1)
                            }
                        }
                    }
                }
                .padding(12)
                .background(Color.primary.opacity(0.02), in: RoundedRectangle(cornerRadius: 8))
                
                // 自动探索
                VStack(alignment: .leading, spacing: 8) {
                    Text("自动探索")
                        .font(.system(size: 14, weight: .semibold))
                    
                    Button("重新扫描系统") {
                        ffmpegManager.autoDiscoverFFmpeg()
                    }
                    .buttonStyle(.bordered)
                    .frame(maxWidth: .infinity)
                }
                
                // 手动设置
                VStack(alignment: .leading, spacing: 8) {
                    Text("手动设置")
                        .font(.system(size: 14, weight: .semibold))
                    
                    HStack {
                        TextField("输入 FFmpeg 路径", text: $customPath)
                            .textFieldStyle(.roundedBorder)
                        
                        Button("浏览") {
                            showingFilePicker = true
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.small)
                    }
                    
                    HStack {
                        Button("应用") {
                            if !customPath.isEmpty {
                                ffmpegManager.setFFmpegPath(customPath)
                            }
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(customPath.isEmpty)
                        
                        Button("重置") {
                            customPath = ""
                        }
                        .buttonStyle(.bordered)
                        
                        Spacer()
                    }
                }
            }
            .padding(16)
            
            Spacer()
        }
        .padding()
        .frame(width: 500, height: 500)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
        .fileImporter(
            isPresented: $showingFilePicker,
            allowedContentTypes: [.unixExecutable, .executable],
            allowsMultipleSelection: false
        ) { result in
            switch result {
            case .success(let files):
                if let file = files.first {
                    customPath = file.path
                }
            case .failure(let error):
                AudioProcessingLogger.shared.error("文件选择失败", details: error.localizedDescription)
            }
        }
        .onAppear {
            customPath = ffmpegManager.ffmpegPath
        }
    }
}

#Preview {
    FFmpegSettingsView(isPresented: .constant(true))
}
