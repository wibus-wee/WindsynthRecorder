import Foundation

// 音频处理配置
struct AudioProcessingConfig {
    let enableLoudnessNormalization: Bool
    let enableNoiseReduction: Bool
    let enableEqualizer: Bool
    let outputFormat: AudioOutputFormat
    let outputQuality: AudioOutputQuality
    
    // 生成FFmpeg滤镜链
    func generateFilterChain() -> [String] {
        var filters: [String] = []
        
        if enableLoudnessNormalization {
            filters.append("loudnorm=I=-16:LRA=7:TP=-1")
        }
        
        if enableNoiseReduction {
            filters.append("afftdn=nf=-25")
        }
        
        if enableEqualizer {
            // 示例均衡器设置
            filters.append("equalizer=f=1000:width_type=h:width=200:g=2")
        }
        
        return filters
    }
}

// 音频输出格式
enum AudioOutputFormat: String, CaseIterable {
    case mp3 = "MP3"
    case wav = "WAV"
    case aiff = "AIFF"
    case m4a = "M4A"
    case flac = "FLAC"
    
    var fileExtension: String {
        switch self {
        case .mp3: return "mp3"
        case .wav: return "wav"
        case .aiff: return "aiff"
        case .m4a: return "m4a"
        case .flac: return "flac"
        }
    }
    
    var codecName: String {
        switch self {
        case .mp3: return "libmp3lame"
        case .wav: return "pcm_s16le"
        case .aiff: return "pcm_s16be"
        case .m4a: return "aac"
        case .flac: return "flac"
        }
    }
}

// 音频输出质量
enum AudioOutputQuality: String, CaseIterable {
    case low = "低质量"
    case medium = "中等质量"
    case high = "高质量"
    case lossless = "无损质量"
    
    var ffmpegParams: [String] {
        switch self {
        case .low:
            return ["-q:a", "9"]
        case .medium:
            return ["-q:a", "5"]
        case .high:
            return ["-q:a", "2"]
        case .lossless:
            return ["-q:a", "0"]
        }
    }
}

class AudioProcessor: ObservableObject {
    private let logger = AudioProcessingLogger.shared
    private let ffmpegManager = FFmpegManager.shared
    
    // 处理单个音频文件
    func processFile(
        inputURL: URL,
        outputURL: URL,
        config: AudioProcessingConfig,
        progressCallback: @escaping (Double) -> Void,
        completion: @escaping (Result<URL, Error>) -> Void
    ) {
        guard let ffmpegPath = ffmpegManager.getFFmpegPath() else {
            let error = NSError(domain: "AudioProcessor", code: 1, userInfo: [NSLocalizedDescriptionKey: "FFmpeg 不可用"])
            logger.error("FFmpeg 不可用", details: "请在设置中配置 FFmpeg 路径")
            completion(.failure(error))
            return
        }
        
        logger.info("开始处理音频文件", details: "输入: \(inputURL.lastPathComponent)\n输出: \(outputURL.lastPathComponent)")
        
        let process = Process()
        process.executableURL = URL(fileURLWithPath: ffmpegPath)
        
        var arguments = [
            "-i", inputURL.path,
            "-y" // 覆盖输出文件
        ]
        
        // 添加音频滤镜
        let filters = config.generateFilterChain()
        if !filters.isEmpty {
            let filterChain = filters.joined(separator: ",")
            arguments.append(contentsOf: ["-af", filterChain])
            logger.info("应用音频滤镜", details: filterChain)
        }
        
        // 添加编码参数
        arguments.append(contentsOf: ["-acodec", config.outputFormat.codecName])
        arguments.append(contentsOf: config.outputQuality.ffmpegParams)
        arguments.append(outputURL.path)
        
        process.arguments = arguments
        
        let fullCommand = "ffmpeg " + arguments.joined(separator: " ")
        logger.info("执行音频处理命令", details: fullCommand)
        
        // 设置管道来捕获错误输出
        let errorPipe = Pipe()
        process.standardError = errorPipe
        
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                try process.run()
                self.logger.info("音频处理进程已启动", details: "PID: \(process.processIdentifier)")
                
                // 模拟进度更新（实际应用中可以解析FFmpeg输出来获取真实进度）
                DispatchQueue.main.async {
                    progressCallback(0.5)
                }
                
                process.waitUntilExit()
                
                // 读取错误输出
                let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
                let errorOutput = String(data: errorData, encoding: .utf8) ?? ""
                
                DispatchQueue.main.async {
                    progressCallback(1.0)
                    
                    if process.terminationStatus == 0 {
                        self.logger.success("音频处理完成", details: "命令: \(fullCommand)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)\n输出文件: \(outputURL.lastPathComponent)")
                        completion(.success(outputURL))
                    } else {
                        self.logger.error("音频处理失败", details: "命令: \(fullCommand)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)\nFFmpeg 错误输出:\n\(errorOutput)")
                        let error = NSError(domain: "AudioProcessor", code: 2, userInfo: [NSLocalizedDescriptionKey: "处理失败，退出代码: \(process.terminationStatus)"])
                        completion(.failure(error))
                    }
                }
            } catch {
                DispatchQueue.main.async {
                    self.logger.error("音频处理进程启动异常", details: "命令: \(fullCommand)\n错误: \(error.localizedDescription)")
                    completion(.failure(error))
                }
            }
        }
    }
    
    // 批量处理音频文件
    func processFiles(
        inputURLs: [URL],
        outputDirectory: URL,
        config: AudioProcessingConfig,
        fileProgressCallback: @escaping (Int, Double) -> Void,
        overallProgressCallback: @escaping (Double) -> Void,
        completion: @escaping ([URL], [Error]) -> Void
    ) {
        logger.info("开始批量音频处理", details: "文件数量: \(inputURLs.count)")
        
        var processedURLs: [URL] = []
        var errors: [Error] = []
        var currentIndex = 0
        
        func processNext() {
            guard currentIndex < inputURLs.count else {
                // 所有文件处理完成
                DispatchQueue.main.async {
                    overallProgressCallback(1.0)
                    self.logger.info("批量处理完成", details: "成功: \(processedURLs.count), 失败: \(errors.count)")
                    completion(processedURLs, errors)
                }
                return
            }
            
            let inputURL = inputURLs[currentIndex]
            let outputFileName = "\(inputURL.deletingPathExtension().lastPathComponent)_processed.\(config.outputFormat.fileExtension)"
            let outputURL = outputDirectory.appendingPathComponent(outputFileName)
            
            processFile(
                inputURL: inputURL,
                outputURL: outputURL,
                config: config,
                progressCallback: { progress in
                    fileProgressCallback(currentIndex, progress)
                },
                completion: { result in
                    switch result {
                    case .success(let url):
                        processedURLs.append(url)
                    case .failure(let error):
                        errors.append(error)
                    }
                    
                    currentIndex += 1
                    let overallProgress = Double(currentIndex) / Double(inputURLs.count)
                    overallProgressCallback(overallProgress)
                    
                    // 处理下一个文件
                    processNext()
                }
            )
        }
        
        // 开始处理第一个文件
        processNext()
    }
    
    // 取消当前处理（如果需要的话）
    func cancelProcessing() {
        // 这里可以实现取消逻辑
        logger.info("取消音频处理", details: "用户请求取消")
    }
    
    // 验证输出目录
    func validateOutputDirectory(_ url: URL) -> Bool {
        var isDirectory: ObjCBool = false
        let exists = FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory)
        
        if !exists {
            do {
                try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
                logger.info("创建输出目录", details: url.path)
                return true
            } catch {
                logger.error("无法创建输出目录", details: "路径: \(url.path), 错误: \(error.localizedDescription)")
                return false
            }
        }
        
        return isDirectory.boolValue
    }
    
    // 生成唯一的输出文件名
    func generateUniqueOutputFileName(for inputURL: URL, in directory: URL, format: AudioOutputFormat) -> URL {
        let baseName = inputURL.deletingPathExtension().lastPathComponent
        let fileExtension = format.fileExtension
        var fileName = "\(baseName)_processed.\(fileExtension)"
        var outputURL = directory.appendingPathComponent(fileName)
        var counter = 1

        while FileManager.default.fileExists(atPath: outputURL.path) {
            fileName = "\(baseName)_processed_\(counter).\(fileExtension)"
            outputURL = directory.appendingPathComponent(fileName)
            counter += 1
        }

        return outputURL
    }
}
