import Foundation

class AudioAnalyzer: ObservableObject {
    private let logger = AudioProcessingLogger.shared
    private let ffmpegManager = FFmpegManager.shared
    
    // 分析单个音频文件
    func analyzeFile(at url: URL, completion: @escaping (Result<AudioAnalysisResult, Error>) -> Void) {
        logger.info("开始分析音频", details: "文件: \(url.lastPathComponent)")
        
        guard let ffmpegPath = ffmpegManager.getFFmpegPath() else {
            let error = NSError(domain: "AudioAnalyzer", code: 1, userInfo: [NSLocalizedDescriptionKey: "FFmpeg 不可用"])
            logger.error("FFmpeg 不可用", details: "请在设置中配置 FFmpeg 路径")
            completion(.failure(error))
            return
        }
        
        let process = Process()
        process.executableURL = URL(fileURLWithPath: ffmpegPath)
        process.arguments = [
            "-i", url.path,
            "-af", "loudnorm=I=-16:print_format=json",
            "-f", "null", "-"
        ]
        
        let outputPipe = Pipe()
        process.standardError = outputPipe
        
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                let command = "\(ffmpegPath) " + process.arguments!.joined(separator: " ")
                self.logger.info("开始执行音频分析命令", details: "命令: \(command)")
                
                try process.run()
                self.logger.info("音频分析进程已启动", details: "PID: \(process.processIdentifier)")
                process.waitUntilExit()
                
                let data = outputPipe.fileHandleForReading.readDataToEndOfFile()
                let output = String(data: data, encoding: .utf8) ?? ""
                
                DispatchQueue.main.async {
                    if process.terminationStatus == 0 {
                        self.logger.success("音频分析命令执行成功", details: "命令: \(command)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)")
                        
                        if let result = self.parseAnalysisResult(output: output) {
                            completion(.success(result))
                        } else {
                            let error = NSError(domain: "AudioAnalyzer", code: 2, userInfo: [NSLocalizedDescriptionKey: "无法解析分析结果"])
                            completion(.failure(error))
                        }
                    } else {
                        self.logger.warning("音频分析命令执行失败", details: "命令: \(command)\n退出代码: \(process.terminationStatus)\nPID: \(process.processIdentifier)\n输出: \(output)")
                        let error = NSError(domain: "AudioAnalyzer", code: 3, userInfo: [NSLocalizedDescriptionKey: "分析失败，退出代码: \(process.terminationStatus)"])
                        completion(.failure(error))
                    }
                }
            } catch {
                DispatchQueue.main.async {
                    self.logger.error("音频分析进程启动异常", details: "错误: \(error.localizedDescription)")
                    completion(.failure(error))
                }
            }
        }
    }
    
    // 批量分析音频文件
    func analyzeFiles(_ urls: [URL], progressCallback: @escaping (Int, Int) -> Void, completion: @escaping ([URL: AudioAnalysisResult], [URL: Error]) -> Void) {
        logger.info("开始批量音频分析", details: "文件数量: \(urls.count)")
        
        var results: [URL: AudioAnalysisResult] = [:]
        var errors: [URL: Error] = [:]
        let group = DispatchGroup()
        let queue = DispatchQueue(label: "audio.analysis.queue", qos: .userInitiated)
        
        for (index, url) in urls.enumerated() {
            group.enter()
            
            queue.async {
                self.analyzeFile(at: url) { result in
                    switch result {
                    case .success(let analysisResult):
                        results[url] = analysisResult
                    case .failure(let error):
                        errors[url] = error
                    }
                    
                    DispatchQueue.main.async {
                        progressCallback(index + 1, urls.count)
                    }
                    
                    group.leave()
                }
            }
        }
        
        group.notify(queue: .main) {
            self.logger.info("批量音频分析完成", details: "成功: \(results.count), 失败: \(errors.count)")
            completion(results, errors)
        }
    }
    
    // 解析FFmpeg loudnorm输出
    private func parseAnalysisResult(output: String) -> AudioAnalysisResult? {
        // 查找JSON部分
        let lines = output.components(separatedBy: .newlines)
        var jsonStartIndex = -1
        var jsonEndIndex = -1
        
        for (index, line) in lines.enumerated() {
            if line.trimmingCharacters(in: .whitespaces) == "{" {
                jsonStartIndex = index
            } else if line.trimmingCharacters(in: .whitespaces) == "}" && jsonStartIndex != -1 {
                jsonEndIndex = index
                break
            }
        }
        
        guard jsonStartIndex != -1 && jsonEndIndex != -1 else {
            logger.error("无法找到JSON输出", details: "FFmpeg输出格式不正确")
            return nil
        }
        
        let jsonLines = Array(lines[jsonStartIndex...jsonEndIndex])
        let jsonString = jsonLines.joined(separator: "\n")
        
        guard let jsonData = jsonString.data(using: .utf8) else {
            logger.error("JSON数据转换失败", details: jsonString)
            return nil
        }
        
        do {
            if let json = try JSONSerialization.jsonObject(with: jsonData) as? [String: Any] {
                guard let inputI = json["input_i"] as? String,
                      let inputTP = json["input_tp"] as? String,
                      let targetOffset = json["target_offset"] as? String else {
                    logger.error("JSON解析失败", details: "缺少必要字段")
                    return nil
                }
                
                guard let originalLoudness = Double(inputI),
                      let originalPeak = Double(inputTP),
                      let offset = Double(targetOffset) else {
                    logger.error("数值转换失败", details: "input_i: \(inputI), input_tp: \(inputTP), target_offset: \(targetOffset)")
                    return nil
                }
                
                let targetLoudness = -16.0 // loudnorm默认目标
                let targetPeak = originalPeak + offset
                
                let result = AudioAnalysisResult(
                    originalLoudness: originalLoudness,
                    originalPeak: originalPeak,
                    targetLoudness: targetLoudness,
                    targetPeak: targetPeak
                )
                
                logger.info("音频分析完成", details: "响度: \(originalLoudness) LUFS, 峰值: \(originalPeak) dBTP")
                return result
            }
        } catch {
            logger.error("JSON解析异常", details: error.localizedDescription)
        }
        
        return nil
    }
    
    // 获取音频文件基本信息
    func getAudioFileInfo(at url: URL, completion: @escaping (Result<AudioFileInfo, Error>) -> Void) {
        guard let ffmpegPath = ffmpegManager.getFFmpegPath() else {
            let error = NSError(domain: "AudioAnalyzer", code: 1, userInfo: [NSLocalizedDescriptionKey: "FFmpeg 不可用"])
            completion(.failure(error))
            return
        }
        
        let process = Process()
        process.executableURL = URL(fileURLWithPath: ffmpegPath)
        process.arguments = [
            "-i", url.path,
            "-f", "null", "-"
        ]
        
        let outputPipe = Pipe()
        process.standardError = outputPipe
        
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                try process.run()
                process.waitUntilExit()
                
                let data = outputPipe.fileHandleForReading.readDataToEndOfFile()
                let output = String(data: data, encoding: .utf8) ?? ""
                
                DispatchQueue.main.async {
                    if let info = self.parseAudioFileInfo(output: output, url: url) {
                        completion(.success(info))
                    } else {
                        let error = NSError(domain: "AudioAnalyzer", code: 4, userInfo: [NSLocalizedDescriptionKey: "无法解析音频文件信息"])
                        completion(.failure(error))
                    }
                }
            } catch {
                DispatchQueue.main.async {
                    completion(.failure(error))
                }
            }
        }
    }
    
    // 解析音频文件信息
    private func parseAudioFileInfo(output: String, url: URL) -> AudioFileInfo? {
        // 这里可以解析FFmpeg输出来获取音频文件的详细信息
        // 如采样率、比特率、时长等
        // 简化实现，返回基本信息
        
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            let fileSize = attributes[.size] as? UInt64 ?? 0
            
            return AudioFileInfo(
                url: url,
                fileName: url.lastPathComponent,
                fileSize: fileSize,
                duration: 0, // 需要从FFmpeg输出解析
                sampleRate: 0, // 需要从FFmpeg输出解析
                bitRate: 0, // 需要从FFmpeg输出解析
                channels: 0 // 需要从FFmpeg输出解析
            )
        } catch {
            return nil
        }
    }
}

// 音频文件信息结构
struct AudioFileInfo {
    let url: URL
    let fileName: String
    let fileSize: UInt64
    let duration: TimeInterval
    let sampleRate: Int
    let bitRate: Int
    let channels: Int
    
    var fileSizeMB: Double {
        Double(fileSize) / (1024 * 1024)
    }
    
    var durationString: String {
        let minutes = Int(duration) / 60
        let seconds = Int(duration) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }
}
