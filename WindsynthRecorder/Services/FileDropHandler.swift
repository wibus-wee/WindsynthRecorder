import Foundation
import SwiftUI
import UniformTypeIdentifiers

class FileDropHandler: ObservableObject {
    @Published var isDragTargeted = false
    private let logger = AudioProcessingLogger.shared
    
    // 支持的音频文件扩展名
    private let audioExtensions = ["mp3", "wav", "aiff", "aif", "m4a", "flac", "ogg", "wma", "aac", "mp4", "3gp", "amr"]
    
    // 文件大小限制（MB）
    private let maxFileSizeMB: Double = 500
    
    // 处理拖放文件的主要方法
    func handleDrop(providers: [NSItemProvider], completion: @escaping ([URL]) -> Void) -> Bool {
        logger.info("检测到文件拖放", details: "处理 \(providers.count) 个项目")
        
        var droppedURLs: [URL] = []
        let group = DispatchGroup()
        
        for provider in providers {
            group.enter()
            
            let typeIdentifiers = provider.registeredTypeIdentifiers
            DispatchQueue.main.async {
                self.logger.info("处理拖放项目", details: "可用类型: \(typeIdentifiers)")
            }
            
            // 首先尝试作为文件URL处理
            if provider.hasItemConformingToTypeIdentifier(UTType.fileURL.identifier) {
                provider.loadItem(forTypeIdentifier: UTType.fileURL.identifier, options: nil) { (item, error) in
                    defer { group.leave() }
                    
                    if let error = error {
                        DispatchQueue.main.async {
                            self.logger.error("文件URL加载失败", details: error.localizedDescription)
                        }
                        return
                    }
                    
                    if let validURL = self.handleURLItem(item) {
                        droppedURLs.append(validURL)
                    }
                }
            }
            // 尝试处理音频类型
            else if let audioType = typeIdentifiers.first(where: { identifier in
                identifier.hasPrefix("public.") && (
                    identifier.contains("mp3") ||
                    identifier.contains("wav") ||
                    identifier.contains("aiff") ||
                    identifier.contains("m4a") ||
                    identifier.contains("flac") ||
                    identifier.contains("audio")
                )
            }) {
                DispatchQueue.main.async {
                    self.logger.info("检测到音频类型", details: "类型: \(audioType)")
                }
                
                provider.loadItem(forTypeIdentifier: audioType, options: nil) { (item, error) in
                    defer { group.leave() }
                    
                    if let error = error {
                        DispatchQueue.main.async {
                            self.logger.error("音频文件加载失败", details: "类型: \(audioType), 错误: \(error.localizedDescription)")
                        }
                        return
                    }
                    
                    if let validURL = self.handleAudioItem(item, audioType: audioType) {
                        droppedURLs.append(validURL)
                    }
                }
            }
            // 尝试处理通用文件类型
            else if provider.hasItemConformingToTypeIdentifier(UTType.item.identifier) {
                provider.loadItem(forTypeIdentifier: UTType.item.identifier, options: nil) { (item, error) in
                    defer { group.leave() }
                    
                    if let error = error {
                        DispatchQueue.main.async {
                            self.logger.error("通用项目加载失败", details: error.localizedDescription)
                        }
                        return
                    }
                    
                    if let validURL = self.handleURLItem(item) {
                        droppedURLs.append(validURL)
                    }
                }
            }
            else {
                // 不支持的类型
                DispatchQueue.main.async {
                    self.logger.warning("不支持的拖放类型", details: "提供者类型: \(typeIdentifiers)")
                }
                group.leave()
            }
        }
        
        group.notify(queue: .main) {
            if !droppedURLs.isEmpty {
                self.logger.success("拖放文件处理完成", details: "成功添加 \(droppedURLs.count) 个音频文件")
                completion(droppedURLs)
            } else {
                self.logger.warning("拖放处理完成", details: "未找到有效的音频文件")
                completion([])
            }
        }
        
        return !providers.isEmpty
    }
    
    // 处理URL类型的拖放项目
    private func handleURLItem(_ item: Any?) -> URL? {
        var url: URL?
        
        // 尝试多种方式解析URL
        if let urlItem = item as? URL {
            url = urlItem
            DispatchQueue.main.async {
                self.logger.info("URL方式解析成功", details: "路径: \(urlItem.path)")
            }
        } else if let data = item as? Data {
            if let urlFromData = URL(dataRepresentation: data, relativeTo: nil) {
                url = urlFromData
                DispatchQueue.main.async {
                    self.logger.info("Data方式解析成功", details: "路径: \(urlFromData.path)")
                }
            } else if let urlString = String(data: data, encoding: .utf8),
                      let urlFromString = URL(string: urlString) {
                url = urlFromString
                DispatchQueue.main.async {
                    self.logger.info("String方式解析成功", details: "路径: \(urlFromString.path)")
                }
            }
        }
        
        guard let finalURL = url else {
            DispatchQueue.main.async {
                self.logger.warning("无法解析拖放的URL", details: "项目类型: \(type(of: item))")
            }
            return nil
        }
        
        return validateAudioFile(finalURL)
    }
    
    // 处理音频类型的拖放项目
    private func handleAudioItem(_ item: Any?, audioType: String) -> URL? {
        DispatchQueue.main.async {
            self.logger.info("处理音频项目", details: "类型: \(audioType), 项目类型: \(type(of: item))")
        }
        
        // 音频文件通常也会包含URL信息
        if let url = item as? URL {
            DispatchQueue.main.async {
                self.logger.info("音频文件URL解析成功", details: "路径: \(url.path)")
            }
            return validateAudioFile(url)
        } else if let data = item as? Data {
            // 尝试从数据中提取URL
            if let urlFromData = URL(dataRepresentation: data, relativeTo: nil) {
                DispatchQueue.main.async {
                    self.logger.info("从音频数据解析URL成功", details: "路径: \(urlFromData.path)")
                }
                return validateAudioFile(urlFromData)
            } else {
                DispatchQueue.main.async {
                    self.logger.warning("无法从音频数据解析URL", details: "数据大小: \(data.count) 字节")
                }
                return nil
            }
        } else {
            DispatchQueue.main.async {
                self.logger.warning("不支持的音频项目类型", details: "项目类型: \(type(of: item))")
            }
            return nil
        }
    }
    
    // 验证音频文件
    private func validateAudioFile(_ url: URL) -> URL? {
        // 检查文件是否存在
        guard FileManager.default.fileExists(atPath: url.path) else {
            DispatchQueue.main.async {
                self.logger.warning("文件不存在", details: "路径: \(url.path)")
            }
            return nil
        }
        
        // 检查文件是否为音频文件
        let fileExtension = url.pathExtension.lowercased()
        
        if audioExtensions.contains(fileExtension) {
            // 检查文件大小（避免过大的文件）
            do {
                let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
                let fileSize = attributes[.size] as? UInt64 ?? 0
                let fileSizeMB = Double(fileSize) / (1024 * 1024)
                
                if fileSizeMB > maxFileSizeMB {
                    DispatchQueue.main.async {
                        self.logger.warning("文件过大", details: "文件: \(url.lastPathComponent), 大小: \(String(format: "%.1f", fileSizeMB)) MB")
                    }
                    return nil
                }
                
                DispatchQueue.main.async {
                    self.logger.info("识别音频文件", details: "文件: \(url.lastPathComponent)\n格式: \(fileExtension)\n大小: \(String(format: "%.1f", fileSizeMB)) MB")
                }
                return url
            } catch {
                DispatchQueue.main.async {
                    self.logger.warning("无法获取文件信息", details: "文件: \(url.lastPathComponent), 错误: \(error.localizedDescription)")
                }
                return nil
            }
        } else {
            DispatchQueue.main.async {
                self.logger.warning("跳过非音频文件", details: "文件: \(url.lastPathComponent), 格式: \(fileExtension.isEmpty ? "无扩展名" : fileExtension)")
            }
            return nil
        }
    }
}
