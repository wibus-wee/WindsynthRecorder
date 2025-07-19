//
//  NativeWaveformView.swift
//  WindsynthRecorder
//
//  高性能原生波形显示器 - 使用 NSView + Core Graphics
//

import SwiftUI
import AppKit
import AVFoundation

/// SwiftUI 包装器
struct NativeWaveformView: NSViewRepresentable {
    let audioData: [Float]
    let duration: TimeInterval
    let currentTime: TimeInterval
    let onSeek: (TimeInterval) -> Void
    
    func makeNSView(context: Context) -> WaveformNSView {
        let view = WaveformNSView()
        view.onSeek = onSeek
        return view
    }
    
    func updateNSView(_ nsView: WaveformNSView, context: Context) {
        nsView.updateWaveform(
            audioData: audioData,
            duration: duration,
            currentTime: currentTime
        )
    }
}

/// 高性能原生波形视图
class WaveformNSView: NSView {
    
    // MARK: - Properties
    
    var onSeek: ((TimeInterval) -> Void)?
    
    private var audioData: [Float] = []
    private var duration: TimeInterval = 0
    private var currentTime: TimeInterval = 0
    
    // 缩放和滚动状态
    private var zoomLevel: CGFloat = 1.0
    private var horizontalOffset: CGFloat = 0.0
    private var verticalZoom: CGFloat = 1.0
    
    // 交互状态
    private var isDragging = false
    private var lastMouseLocation: NSPoint = .zero
    
    // 性能优化标记
    private var lastDrawTime: CFTimeInterval = 0
    
    // MARK: - Initialization
    
    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        setupView()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupView()
    }
    
    private func setupView() {
        wantsLayer = true
        layer?.backgroundColor = NSColor.black.withAlphaComponent(0.8).cgColor
        layer?.cornerRadius = 8
        layer?.borderWidth = 1
        layer?.borderColor = NSColor.gray.withAlphaComponent(0.3).cgColor
    }
    
    // MARK: - Data Updates
    
    func updateWaveform(audioData: [Float], duration: TimeInterval, currentTime: TimeInterval) {
        let needsRedraw = self.audioData != audioData || 
                         self.duration != duration || 
                         abs(self.currentTime - currentTime) > 0.1
        
        self.audioData = audioData
        self.duration = duration
        self.currentTime = currentTime
        
        if needsRedraw {
            needsDisplay = true
        }
    }
    
    // MARK: - Drawing
    
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        
        guard let context = NSGraphicsContext.current?.cgContext else { return }
        
        // 清除背景
        context.clear(bounds)
        
        if audioData.isEmpty {
            drawPlaceholder(in: context)
        } else {
            drawTimeGrid(in: context)
            drawWaveform(in: context)
            drawPlaybackLine(in: context)
        }
        
        drawZoomInfo(in: context)
    }
    
    private func drawPlaceholder(in context: CGContext) {
        let text = "No Audio Loaded"
        let attributes: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: 16),
            .foregroundColor: NSColor.gray
        ]
        
        let attributedString = NSAttributedString(string: text, attributes: attributes)
        let textSize = attributedString.size()
        let textRect = NSRect(
            x: (bounds.width - textSize.width) / 2,
            y: (bounds.height - textSize.height) / 2,
            width: textSize.width,
            height: textSize.height
        )
        
        attributedString.draw(in: textRect)
    }
    
    private func drawTimeGrid(in context: CGContext) {
        context.setStrokeColor(NSColor.gray.withAlphaComponent(0.2).cgColor)
        context.setLineWidth(1)
        
        let intervals = max(10, Int(10 * zoomLevel))
        let intervalWidth = (bounds.width * zoomLevel) / CGFloat(intervals)
        
        for i in 0...intervals {
            let x = CGFloat(i) * intervalWidth + horizontalOffset
            if x >= 0 && x <= bounds.width {
                context.move(to: CGPoint(x: x, y: 0))
                context.addLine(to: CGPoint(x: x, y: bounds.height))
                context.strokePath()
            }
        }
        
        // 中心线
        context.setStrokeColor(NSColor.gray.withAlphaComponent(0.3).cgColor)
        let centerY = bounds.height / 2
        context.move(to: CGPoint(x: 0, y: centerY))
        context.addLine(to: CGPoint(x: bounds.width, y: centerY))
        context.strokePath()
    }
    
    private func drawWaveform(in context: CGContext) {
        guard !audioData.isEmpty else { return }

        let centerY = bounds.height / 2
        let waveformHeight = bounds.height * 0.8 * verticalZoom

        // 重新设计缩放逻辑
        // 1. 计算每个屏幕像素对应多少音频样本
        let totalSamples = audioData.count
        let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
        let actualSamplesPerPixel = baseSamplesPerPixel / Double(zoomLevel)

        // 2. 计算当前视图的起始样本位置
        let viewStartSample = Int(-horizontalOffset * actualSamplesPerPixel)
        let clampedStartSample = max(0, min(totalSamples, viewStartSample))

        // 创建路径点数组
        var topPoints: [CGPoint] = []
        var bottomPoints: [CGPoint] = []

        // 3. 遍历屏幕上的每个像素
        for screenX in 0..<Int(bounds.width) {
            // 计算这个屏幕像素对应的音频样本索引
            let sampleIndex = clampedStartSample + Int(Double(screenX) * actualSamplesPerPixel)

            if sampleIndex >= 0 && sampleIndex < totalSamples {
                // 限制振幅范围到 [-1, 1]
                let rawAmplitude = CGFloat(audioData[sampleIndex])
                let clampedAmplitude = max(-1.0, min(1.0, rawAmplitude))

                // 计算波形高度（保持正负符号）
                let waveHeight = clampedAmplitude * waveformHeight * 0.5

                // 计算屏幕坐标
                let topY = max(0, min(bounds.height, centerY - waveHeight))
                let bottomY = max(0, min(bounds.height, centerY + waveHeight))

                topPoints.append(CGPoint(x: CGFloat(screenX), y: topY))
                bottomPoints.append(CGPoint(x: CGFloat(screenX), y: bottomY))
            }
        }

        // 绘制填充区域
        if !topPoints.isEmpty && !bottomPoints.isEmpty {
            context.setFillColor(NSColor.blue.withAlphaComponent(0.6).cgColor)

            context.beginPath()
            context.move(to: topPoints[0])

            // 绘制上边缘
            for point in topPoints.dropFirst() {
                context.addLine(to: point)
            }

            // 绘制下边缘（反向）
            for point in bottomPoints.reversed() {
                context.addLine(to: point)
            }

            context.closePath()
            context.fillPath()

            // 绘制轮廓
            context.setStrokeColor(NSColor.blue.cgColor)
            context.setLineWidth(1)

            // 上轮廓
            context.beginPath()
            context.move(to: topPoints[0])
            for point in topPoints.dropFirst() {
                context.addLine(to: point)
            }
            context.strokePath()

            // 下轮廓
            context.beginPath()
            context.move(to: bottomPoints[0])
            for point in bottomPoints.dropFirst() {
                context.addLine(to: point)
            }
            context.strokePath()
        }
    }

    
    private func drawPlaybackLine(in context: CGContext) {
        guard duration > 0 else { return }
        
        let progress = currentTime / duration
        let zoomedWidth = bounds.width * zoomLevel
        let lineX = progress * zoomedWidth + horizontalOffset
        
        if lineX >= 0 && lineX <= bounds.width {
            context.setStrokeColor(NSColor.white.cgColor)
            context.setLineWidth(2)
            context.move(to: CGPoint(x: lineX, y: 0))
            context.addLine(to: CGPoint(x: lineX, y: bounds.height))
            context.strokePath()
            
            // 添加阴影效果
            context.setShadow(offset: CGSize(width: 1, height: 1), blur: 2, color: NSColor.black.cgColor)
        }
    }
    
    private func drawZoomInfo(in context: CGContext) {
        // 计算音频数据的统计信息
        let maxAmplitude = audioData.isEmpty ? 0 : audioData.map { abs($0) }.max() ?? 0
        let avgAmplitude = audioData.isEmpty ? 0 : audioData.map { abs($0) }.reduce(0, +) / Float(audioData.count)

        // 计算有效宽度（防止除零和无穷大）
        let totalSamples = audioData.count
        var effectiveWidth: CGFloat = 0

        if totalSamples > 0 && bounds.width > 0 && zoomLevel > 0 {
            let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
            let actualSamplesPerPixel = baseSamplesPerPixel / Double(zoomLevel)
            if actualSamplesPerPixel > 0 && actualSamplesPerPixel.isFinite {
                effectiveWidth = CGFloat(totalSamples) / CGFloat(actualSamplesPerPixel)
            }
        }

        let zoomText = "H: \(Int(zoomLevel * 100))% V: \(Int(verticalZoom * 100))%"
        let amplitudeText = "Max: \(String(format: "%.3f", maxAmplitude)) Avg: \(String(format: "%.3f", avgAmplitude))"
        let widthText = effectiveWidth.isFinite && !effectiveWidth.isNaN ?
            "Width: \(Int(effectiveWidth))px (min: 100px)" :
            "Width: N/A (min: 100px)"

        let attributes: [NSAttributedString.Key: Any] = [
            .font: NSFont.monospacedSystemFont(ofSize: 10, weight: .regular),
            .foregroundColor: NSColor.white.withAlphaComponent(0.7)
        ]

        // 绘制缩放信息
        let zoomString = NSAttributedString(string: zoomText, attributes: attributes)
        let zoomRect = NSRect(x: 10, y: bounds.height - 25, width: 150, height: 15)
        zoomString.draw(in: zoomRect)

        // 绘制振幅信息
        let amplitudeString = NSAttributedString(string: amplitudeText, attributes: attributes)
        let amplitudeRect = NSRect(x: 10, y: bounds.height - 45, width: 200, height: 15)
        amplitudeString.draw(in: amplitudeRect)

        // 绘制宽度信息
        let widthString = NSAttributedString(string: widthText, attributes: attributes)
        let widthRect = NSRect(x: 10, y: bounds.height - 65, width: 200, height: 15)
        widthString.draw(in: widthRect)
    }

    // MARK: - Mouse Events

    override func mouseDown(with event: NSEvent) {
        let location = convert(event.locationInWindow, from: nil)
        handleSeek(at: location)
        isDragging = true
        lastMouseLocation = location
    }

    override func mouseDragged(with event: NSEvent) {
        let location = convert(event.locationInWindow, from: nil)

        if isDragging {
            handleSeek(at: location)
        }

        lastMouseLocation = location
    }

    override func mouseUp(with event: NSEvent) {
        isDragging = false
    }

    private func handleSeek(at location: NSPoint) {
        guard duration > 0 else { return }

        let adjustedX = (location.x - horizontalOffset) / zoomLevel
        let progress = adjustedX / bounds.width
        let seekTime = duration * Double(max(0, min(1, progress)))

        onSeek?(seekTime)
    }

    // MARK: - Scroll Events

    override func scrollWheel(with event: NSEvent) {
        let scrollX = event.scrollingDeltaX
        let scrollY = event.scrollingDeltaY

        if event.modifierFlags.contains(.command) {
            // Command + 滚轮 = 水平缩放（以播放条为中心，限制最小宽度）
            let zoomFactor = 1.0 + (scrollY * 0.01)
            let oldZoom = zoomLevel
            let newZoom = max(0.1, min(10.0, zoomLevel * zoomFactor))

            // 检查缩放后的有效宽度是否小于100（防止除零和无穷大）
            let totalSamples = audioData.count
            var allowZoom = false

            if totalSamples > 0 && bounds.width > 0 && newZoom > 0 {
                let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
                let newSamplesPerPixel = baseSamplesPerPixel / Double(newZoom)
                if newSamplesPerPixel > 0 && newSamplesPerPixel.isFinite {
                    let effectiveWidth = CGFloat(totalSamples) / CGFloat(newSamplesPerPixel)
                    allowZoom = effectiveWidth.isFinite && !effectiveWidth.isNaN && effectiveWidth >= 100
                }
            }

            if newZoom != oldZoom && allowZoom {
                // 计算播放条在屏幕上的位置
                let playbackProgress = duration > 0 ? currentTime / duration : 0
                let playbackScreenX = CGFloat(playbackProgress) * bounds.width

                // 计算缩放前播放条对应的音频位置
                let totalSamples = audioData.count
                let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
                let oldSamplesPerPixel = baseSamplesPerPixel / Double(oldZoom)
                let oldStartSample = Int(-horizontalOffset * oldSamplesPerPixel)
                let playbackSample = oldStartSample + Int(Double(playbackScreenX) * oldSamplesPerPixel)

                // 更新缩放级别
                zoomLevel = newZoom

                // 计算新的采样率和偏移，保持播放条位置不变
                let newSamplesPerPixel = baseSamplesPerPixel / Double(newZoom)
                let newStartSample = playbackSample - Int(Double(playbackScreenX) * newSamplesPerPixel)
                horizontalOffset = -CGFloat(newStartSample) / CGFloat(newSamplesPerPixel)

                // 限制偏移范围
                let maxOffset = max(0, CGFloat(totalSamples) / CGFloat(newSamplesPerPixel) - bounds.width)
                horizontalOffset = max(-maxOffset, min(0, horizontalOffset))

                needsDisplay = true
            }

        } else if event.modifierFlags.contains(.option) {
            // Option + 滚轮 = 垂直缩放
            let zoomFactor = 1.0 + (scrollY * 0.01)
            let newVerticalZoom = max(0.1, min(5.0, verticalZoom * zoomFactor))
            verticalZoom = newVerticalZoom
            needsDisplay = true

        } else {
            // 普通滚轮 = 水平滚动
            let scrollSensitivity: CGFloat = 2.0
            let deltaX = scrollX * scrollSensitivity

            // 计算滚动限制
            let totalSamples = audioData.count
            let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
            let actualSamplesPerPixel = baseSamplesPerPixel / Double(zoomLevel)
            let maxOffset = max(0, CGFloat(totalSamples) / CGFloat(actualSamplesPerPixel) - bounds.width)

            horizontalOffset = max(-maxOffset, min(0, horizontalOffset + deltaX))
            needsDisplay = true
        }
    }

    // MARK: - Magnification (Trackpad Pinch)

    override func magnify(with event: NSEvent) {
        let oldZoom = zoomLevel
        let newZoom = max(0.1, min(10.0, zoomLevel * (1.0 + event.magnification)))

        // 检查缩放后的有效宽度是否小于100（防止除零和无穷大）
        let totalSamples = audioData.count
        var allowZoom = false

        if totalSamples > 0 && bounds.width > 0 && newZoom > 0 {
            let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
            let newSamplesPerPixel = baseSamplesPerPixel / Double(newZoom)
            if newSamplesPerPixel > 0 && newSamplesPerPixel.isFinite {
                let effectiveWidth = CGFloat(totalSamples) / CGFloat(newSamplesPerPixel)
                allowZoom = effectiveWidth.isFinite && !effectiveWidth.isNaN && effectiveWidth >= 100
            }
        }

        if newZoom != oldZoom && allowZoom {
            // 以播放条为中心进行缩放
            let playbackProgress = duration > 0 ? currentTime / duration : 0
            let playbackScreenX = CGFloat(playbackProgress) * bounds.width

            // 计算缩放前播放条对应的音频位置
            let totalSamples = audioData.count
            let baseSamplesPerPixel = Double(totalSamples) / Double(bounds.width)
            let oldSamplesPerPixel = baseSamplesPerPixel / Double(oldZoom)
            let oldStartSample = Int(-horizontalOffset * oldSamplesPerPixel)
            let playbackSample = oldStartSample + Int(Double(playbackScreenX) * oldSamplesPerPixel)

            // 更新缩放级别
            zoomLevel = newZoom

            // 计算新的采样率和偏移，保持播放条位置不变
            let newSamplesPerPixel = baseSamplesPerPixel / Double(newZoom)
            let newStartSample = playbackSample - Int(Double(playbackScreenX) * newSamplesPerPixel)
            horizontalOffset = -CGFloat(newStartSample) / CGFloat(newSamplesPerPixel)

            // 限制偏移范围
            let maxOffset = max(0, CGFloat(totalSamples) / CGFloat(newSamplesPerPixel) - bounds.width)
            horizontalOffset = max(-maxOffset, min(0, horizontalOffset))

            needsDisplay = true
        }
    }

    // MARK: - Responder Chain

    override var acceptsFirstResponder: Bool {
        return true
    }

    override func acceptsFirstMouse(for event: NSEvent?) -> Bool {
        return true
    }

    // MARK: - Key Events (Optional)

    override func keyDown(with event: NSEvent) {
        switch event.keyCode {
        case 49: // Space bar
            // 可以添加播放/暂停功能
            break
        case 123: // Left arrow
            horizontalOffset = min(0, horizontalOffset + 20)
            needsDisplay = true
        case 124: // Right arrow
            let maxOffset = max(0, (zoomLevel - 1) * bounds.width)
            horizontalOffset = max(-maxOffset, horizontalOffset - 20)
            needsDisplay = true
        default:
            super.keyDown(with: event)
        }
    }
}

// MARK: - 波形数据生成器

/// 波形数据提取错误
enum WaveformError: LocalizedError {
    case cannotReadAudioFile(url: URL)
    case cannotCreateBuffer
    case cannotReadAudioData(error: Error)

    var errorDescription: String? {
        switch self {
        case .cannotReadAudioFile(let url):
            return "无法读取音频文件: \(url.lastPathComponent)"
        case .cannotCreateBuffer:
            return "无法创建音频缓冲区"
        case .cannotReadAudioData(let error):
            return "读取音频数据失败: \(error.localizedDescription)"
        }
    }
}

/// 真实音频波形数据提取器
struct WaveformDataGenerator {

    /// 异步从音频文件提取真实波形数据
    static func generateFromAudioFileAsync(
        url: URL,
        targetSamples: Int = 1000,
        completion: @escaping (Result<[Float], WaveformError>) -> Void
    ) {
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                let waveformData = try generateFromAudioFileSync(url: url, targetSamples: targetSamples)
                DispatchQueue.main.async {
                    completion(.success(waveformData))
                }
            } catch let error as WaveformError {
                DispatchQueue.main.async {
                    completion(.failure(error))
                }
            } catch {
                DispatchQueue.main.async {
                    completion(.failure(.cannotReadAudioData(error: error)))
                }
            }
        }
    }

    /// 同步从音频文件提取真实波形数据（内部使用）
    private static func generateFromAudioFileSync(url: URL, targetSamples: Int = 1000) throws -> [Float] {
        guard let audioFile = try? AVAudioFile(forReading: url) else {
            throw WaveformError.cannotReadAudioFile(url: url)
        }

        let format = audioFile.processingFormat
        let frameCount = AVAudioFrameCount(audioFile.length)

        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCount) else {
            throw WaveformError.cannotCreateBuffer
        }

        do {
            try audioFile.read(into: buffer)
            return extractWaveformData(from: buffer, targetSamples: targetSamples)
        } catch {
            throw WaveformError.cannotReadAudioData(error: error)
        }
    }

    /// 从音频缓冲区提取波形数据
    private static func extractWaveformData(from buffer: AVAudioPCMBuffer, targetSamples: Int) -> [Float] {
        guard let channelData = buffer.floatChannelData?[0] else {
            return []
        }

        let frameLength = Int(buffer.frameLength)
        let samplesPerBin = max(1, frameLength / targetSamples)
        var waveformData: [Float] = []

        // 对音频数据进行下采样和峰值检测
        for i in 0..<targetSamples {
            let startIndex = i * samplesPerBin
            let endIndex = min(startIndex + samplesPerBin, frameLength)

            var maxAmplitude: Float = 0

            // 在每个采样窗口中找到最大振幅
            for j in startIndex..<endIndex {
                let amplitude = abs(channelData[j])
                maxAmplitude = max(maxAmplitude, amplitude)
            }

            waveformData.append(maxAmplitude)
        }

        return waveformData
    }
}
