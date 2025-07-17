# WindsynthRecorder VST 插件实现技术指南

## 开发环境配置

### 1. JUCE 框架集成

#### 步骤 1：配置 CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.22)
project(WindsynthRecorderVST VERSION 1.0.0)

# 设置 JUCE 路径
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
add_subdirectory(${JUCE_DIR})

# 创建 VST 支持库
add_library(VSTSupport STATIC
    Libraries/VSTSupport/VSTPluginManager.cpp
    Libraries/VSTSupport/AudioProcessingChain.cpp
    Libraries/VSTSupport/RealtimeProcessor.cpp
    Libraries/VSTSupport/OfflineProcessor.cpp
)

# 链接 JUCE 模块
target_link_libraries(VSTSupport
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
)

# 设置编译定义
target_compile_definitions(VSTSupport PUBLIC
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)
```

#### 步骤 2：Xcode 项目配置
1. 在 Xcode 中添加 C++ 编译支持
2. 设置 Header Search Paths 包含 JUCE 目录
3. 添加必要的 macOS 框架：
   - AudioToolbox.framework
   - AudioUnit.framework
   - CoreAudio.framework
   - CoreMIDI.framework

### 2. VST3 SDK 配置

```bash
# 下载 VST3 SDK
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git

# 在 CMakeLists.txt 中设置 VST3 路径
set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk")
juce_set_vst3_sdk_path("${VST3_SDK_ROOT}")
```

## 核心组件实现

### 1. VSTPluginManager (C++)

```cpp
// Libraries/VSTSupport/VSTPluginManager.hpp
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>

class VSTPluginManager {
public:
    struct PluginInfo {
        juce::String name;
        juce::String manufacturer;
        juce::String category;
        juce::String pluginFormatName;
        juce::PluginDescription description;
    };
    
    VSTPluginManager();
    ~VSTPluginManager();
    
    // 插件扫描和管理
    void scanForPlugins();
    std::vector<PluginInfo> getAvailablePlugins() const;
    std::unique_ptr<juce::AudioPluginInstance> loadPlugin(const juce::PluginDescription& desc);
    
    // 插件格式管理
    void addFormat(std::unique_ptr<juce::AudioPluginFormat> format);
    
private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    std::unique_ptr<juce::FileSearchPath> pluginSearchPath;
    
    void initializeFormats();
    void loadPluginList();
    void savePluginList();
};
```

```cpp
// Libraries/VSTSupport/VSTPluginManager.cpp
#include "VSTPluginManager.hpp"

VSTPluginManager::VSTPluginManager() {
    initializeFormats();
    loadPluginList();
}

void VSTPluginManager::initializeFormats() {
    // 添加 VST3 格式支持
    formatManager.addDefaultFormats();
    
    // 设置插件搜索路径
    juce::FileSearchPath searchPath;
    
#if JUCE_MAC
    searchPath.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
    searchPath.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    searchPath.add(juce::File("/System/Library/Audio/Plug-Ins/VST3"));
#elif JUCE_WINDOWS
    searchPath.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    searchPath.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
#endif
    
    pluginSearchPath = std::make_unique<juce::FileSearchPath>(searchPath);
}

void VSTPluginManager::scanForPlugins() {
    juce::PluginDirectoryScanner scanner(knownPluginList, formatManager, 
                                       *pluginSearchPath, true, juce::File());
    
    juce::String pluginBeingScanned;
    while (scanner.scanNextFile(true, pluginBeingScanned)) {
        // 扫描进度回调可以在这里处理
    }
    
    savePluginList();
}

std::unique_ptr<juce::AudioPluginInstance> VSTPluginManager::loadPlugin(
    const juce::PluginDescription& desc) {
    
    juce::String errorMessage;
    auto plugin = formatManager.createPluginInstance(desc, 44100.0, 512, errorMessage);
    
    if (plugin == nullptr) {
        juce::Logger::writeToLog("Failed to load plugin: " + errorMessage);
    }
    
    return plugin;
}
```

### 2. AudioProcessingChain (C++)

```cpp
// Libraries/VSTSupport/AudioProcessingChain.hpp
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>

class AudioProcessingChain {
public:
    AudioProcessingChain();
    ~AudioProcessingChain();
    
    // 插件链管理
    void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
    void removePlugin(int index);
    void movePlugin(int fromIndex, int toIndex);
    void bypassPlugin(int index, bool bypass);
    
    // 音频处理
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // 状态管理
    void getStateInformation(juce::MemoryBlock& destData);
    void setStateInformation(const void* data, int sizeInBytes);
    
    // 插件信息
    int getNumPlugins() const { return static_cast<int>(plugins.size()); }
    juce::AudioPluginInstance* getPlugin(int index);
    
private:
    struct PluginSlot {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        bool bypassed = false;
        juce::AudioBuffer<float> tempBuffer;
    };
    
    std::vector<PluginSlot> plugins;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    void updatePluginChain();
};
```

### 3. Swift 桥接层

```objc
// Libraries/Bridge/VSTBridge.h
#import <Foundation/Foundation.h>

@class VSTPluginInfo;
@class VSTProcessingChain;

@interface VSTBridge : NSObject

+ (instancetype)shared;

// 插件管理
- (void)scanForPlugins;
- (NSArray<VSTPluginInfo *> *)getAvailablePlugins;
- (BOOL)loadPlugin:(VSTPluginInfo *)pluginInfo intoChain:(VSTProcessingChain *)chain;

// 音频处理
- (VSTProcessingChain *)createProcessingChain;
- (void)processAudioBuffer:(float *)buffer 
                    length:(int)length 
                  channels:(int)channels 
                 withChain:(VSTProcessingChain *)chain;

// 参数控制
- (void)setPluginParameter:(int)pluginIndex 
               parameterIndex:(int)parameterIndex 
                        value:(float)value 
                      inChain:(VSTProcessingChain *)chain;

@end

@interface VSTPluginInfo : NSObject
@property (nonatomic, strong) NSString *name;
@property (nonatomic, strong) NSString *manufacturer;
@property (nonatomic, strong) NSString *category;
@property (nonatomic, strong) NSString *pluginFormat;
@end

@interface VSTProcessingChain : NSObject
// 内部持有 C++ AudioProcessingChain 实例
@end
```

```swift
// Libraries/Bridge/VSTBridge.swift
import Foundation

class VSTPluginService: ObservableObject {
    @Published var availablePlugins: [VSTPluginInfo] = []
    @Published var isScanning = false
    
    private let bridge = VSTBridge.shared()
    
    func scanForPlugins() {
        isScanning = true
        
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            self?.bridge.scanForPlugins()
            
            DispatchQueue.main.async {
                self?.availablePlugins = self?.bridge.getAvailablePlugins() ?? []
                self?.isScanning = false
            }
        }
    }
    
    func createProcessingChain() -> VSTProcessingChain? {
        return bridge.createProcessingChain()
    }
    
    func loadPlugin(_ pluginInfo: VSTPluginInfo, into chain: VSTProcessingChain) -> Bool {
        return bridge.loadPlugin(pluginInfo, into: chain)
    }
}
```

### 4. 实时音频处理集成

```swift
// Services/RealtimeAudioService.swift
import Foundation
import AVFoundation

class RealtimeAudioService: ObservableObject {
    @Published var isProcessingEnabled = false
    @Published var cpuUsage: Float = 0.0
    @Published var latency: TimeInterval = 0.0
    
    private var audioEngine: AVAudioEngine?
    private var vstChain: VSTProcessingChain?
    private var vstService = VSTPluginService()
    
    func setupRealtimeProcessing() {
        audioEngine = AVAudioEngine()
        vstChain = vstService.createProcessingChain()
        
        guard let audioEngine = audioEngine,
              let vstChain = vstChain else { return }
        
        let inputNode = audioEngine.inputNode
        let outputNode = audioEngine.outputNode
        
        // 创建自定义音频单元用于 VST 处理
        let vstAudioUnit = createVSTAudioUnit(with: vstChain)
        
        // 连接音频节点
        audioEngine.attach(vstAudioUnit)
        audioEngine.connect(inputNode, to: vstAudioUnit, format: inputNode.outputFormat(forBus: 0))
        audioEngine.connect(vstAudioUnit, to: outputNode, format: outputNode.inputFormat(forBus: 0))
        
        // 启动音频引擎
        do {
            try audioEngine.start()
            isProcessingEnabled = true
        } catch {
            print("Failed to start audio engine: \(error)")
        }
    }
    
    private func createVSTAudioUnit(with chain: VSTProcessingChain) -> AVAudioUnit {
        // 创建自定义 AVAudioUnit 包装 VST 处理链
        // 这里需要实现 AVAudioUnit 子类
        return VSTAudioUnit(processingChain: chain)
    }
}
```

## 用户界面实现

### 1. VST 插件浏览器

```swift
// Views/VSTPluginBrowser.swift
import SwiftUI

struct VSTPluginBrowser: View {
    @StateObject private var vstService = VSTPluginService()
    @State private var searchText = ""
    @State private var selectedCategory = "All"
    
    let categories = ["All", "Effect", "Instrument", "Analyzer", "Mastering"]
    
    var filteredPlugins: [VSTPluginInfo] {
        vstService.availablePlugins.filter { plugin in
            (selectedCategory == "All" || plugin.category == selectedCategory) &&
            (searchText.isEmpty || plugin.name.localizedCaseInsensitiveContains(searchText))
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // 搜索和过滤栏
            HStack {
                TextField("搜索插件...", text: $searchText)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                
                Picker("分类", selection: $selectedCategory) {
                    ForEach(categories, id: \.self) { category in
                        Text(category).tag(category)
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .frame(width: 120)
            }
            .padding()
            
            // 插件列表
            List(filteredPlugins, id: \.name) { plugin in
                VSTPluginRow(plugin: plugin)
            }
            
            // 底部工具栏
            HStack {
                Button("扫描插件") {
                    vstService.scanForPlugins()
                }
                .disabled(vstService.isScanning)
                
                Spacer()
                
                if vstService.isScanning {
                    ProgressView()
                        .scaleEffect(0.8)
                    Text("扫描中...")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            .padding()
        }
        .onAppear {
            if vstService.availablePlugins.isEmpty {
                vstService.scanForPlugins()
            }
        }
    }
}

struct VSTPluginRow: View {
    let plugin: VSTPluginInfo
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(plugin.name)
                    .font(.headline)
                Text(plugin.manufacturer)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            Text(plugin.category)
                .font(.caption)
                .padding(.horizontal, 8)
                .padding(.vertical, 2)
                .background(Color.blue.opacity(0.2))
                .cornerRadius(4)
            
            Button("添加") {
                // 添加插件到处理链
            }
            .buttonStyle(BorderedButtonStyle())
        }
        .padding(.vertical, 4)
    }
}
```

### 2. 音频处理链编辑器

```swift
// Views/AudioChainEditor.swift
import SwiftUI

struct AudioChainEditor: View {
    @StateObject private var chainManager = AudioChainManager()
    @State private var showingPluginBrowser = false
    
    var body: some View {
        VStack(spacing: 0) {
            // 处理链标题栏
            HStack {
                Text("音频处理链")
                    .font(.title2)
                    .fontWeight(.semibold)
                
                Spacer()
                
                Button("添加插件") {
                    showingPluginBrowser = true
                }
                .buttonStyle(BorderedProminentButtonStyle())
            }
            .padding()
            
            // 处理链可视化
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 16) {
                    ForEach(chainManager.plugins.indices, id: \.self) { index in
                        VSTPluginSlot(
                            plugin: chainManager.plugins[index],
                            index: index,
                            onRemove: { chainManager.removePlugin(at: index) },
                            onBypass: { chainManager.toggleBypass(at: index) }
                        )
                    }
                    
                    // 添加插件按钮
                    Button(action: { showingPluginBrowser = true }) {
                        VStack {
                            Image(systemName: "plus.circle.dashed")
                                .font(.system(size: 40))
                                .foregroundColor(.secondary)
                            Text("添加插件")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                    .frame(width: 120, height: 80)
                    .background(Color.gray.opacity(0.1))
                    .cornerRadius(8)
                    .buttonStyle(PlainButtonStyle())
                }
                .padding()
            }
            
            Spacer()
        }
        .sheet(isPresented: $showingPluginBrowser) {
            VSTPluginBrowser()
        }
    }
}

struct VSTPluginSlot: View {
    let plugin: VSTPluginWrapper
    let index: Int
    let onRemove: () -> Void
    let onBypass: () -> Void
    
    var body: some View {
        VStack(spacing: 8) {
            // 插件信息
            VStack(spacing: 4) {
                Text(plugin.name)
                    .font(.headline)
                    .lineLimit(1)
                Text(plugin.manufacturer)
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .lineLimit(1)
            }
            
            // 控制按钮
            HStack(spacing: 8) {
                Button(action: onBypass) {
                    Image(systemName: plugin.isBypassed ? "power" : "power.circle.fill")
                        .foregroundColor(plugin.isBypassed ? .red : .green)
                }
                
                Button("设置") {
                    // 打开插件参数界面
                }
                .font(.caption)
                
                Button(action: onRemove) {
                    Image(systemName: "trash")
                        .foregroundColor(.red)
                }
            }
        }
        .frame(width: 120, height: 80)
        .padding(8)
        .background(plugin.isBypassed ? Color.gray.opacity(0.3) : Color.blue.opacity(0.1))
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(plugin.isBypassed ? Color.gray : Color.blue, lineWidth: 1)
        )
    }
}
```

## 性能优化策略

### 1. 音频线程优化

```cpp
// 在音频回调中避免内存分配
void AudioProcessingChain::processBlock(juce::AudioBuffer<float>& buffer, 
                                      juce::MidiBuffer& midiMessages) {
    // 使用预分配的缓冲区
    for (auto& slot : plugins) {
        if (!slot.bypassed && slot.plugin != nullptr) {
            // 确保临时缓冲区大小正确
            if (slot.tempBuffer.getNumSamples() != buffer.getNumSamples()) {
                slot.tempBuffer.setSize(buffer.getNumChannels(), 
                                      buffer.getNumSamples(), 
                                      false, false, true);
            }
            
            // 复制输入到临时缓冲区
            slot.tempBuffer.makeCopyOf(buffer);
            
            // 处理音频
            slot.plugin->processBlock(slot.tempBuffer, midiMessages);
            
            // 复制处理结果回主缓冲区
            buffer.makeCopyOf(slot.tempBuffer);
        }
    }
}
```

### 2. 内存管理优化

```cpp
class PluginInstancePool {
public:
    std::unique_ptr<juce::AudioPluginInstance> acquireInstance(
        const juce::PluginDescription& desc) {
        
        auto key = desc.createIdentifierString();
        
        if (auto it = pool.find(key); it != pool.end() && !it->second.empty()) {
            auto instance = std::move(it->second.back());
            it->second.pop_back();
            return instance;
        }
        
        // 创建新实例
        return createNewInstance(desc);
    }
    
    void releaseInstance(std::unique_ptr<juce::AudioPluginInstance> instance) {
        if (instance) {
            auto key = instance->getPluginDescription().createIdentifierString();
            pool[key].push_back(std::move(instance));
        }
    }
    
private:
    std::unordered_map<juce::String, 
                      std::vector<std::unique_ptr<juce::AudioPluginInstance>>> pool;
};
```

## 错误处理和稳定性

### 1. 插件崩溃隔离

```cpp
class SafePluginWrapper {
public:
    SafePluginWrapper(std::unique_ptr<juce::AudioPluginInstance> plugin)
        : plugin(std::move(plugin)), isStable(true) {}
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
        if (!isStable) {
            return; // 跳过不稳定的插件
        }
        
        try {
            plugin->processBlock(buffer, midi);
            consecutiveFailures = 0;
        } catch (...) {
            handlePluginFailure();
        }
    }
    
private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    bool isStable;
    int consecutiveFailures = 0;
    static constexpr int maxFailures = 3;
    
    void handlePluginFailure() {
        consecutiveFailures++;
        if (consecutiveFailures >= maxFailures) {
            isStable = false;
            juce::Logger::writeToLog("Plugin marked as unstable: " + 
                                   plugin->getName());
        }
    }
};
```

### 2. 资源监控

```swift
class PerformanceMonitor: ObservableObject {
    @Published var cpuUsage: Float = 0.0
    @Published var memoryUsage: Float = 0.0
    @Published var audioDropouts: Int = 0
    
    private var timer: Timer?
    
    func startMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            self.updateMetrics()
        }
    }
    
    private func updateMetrics() {
        // 获取 CPU 使用率
        cpuUsage = getCurrentCPUUsage()
        
        // 获取内存使用率
        memoryUsage = getCurrentMemoryUsage()
        
        // 检查音频中断
        checkAudioDropouts()
    }
}
```

---

**下一步**：开始实施第一阶段的基础设施搭建，建议先从 JUCE 框架集成和基础插件扫描功能开始。
