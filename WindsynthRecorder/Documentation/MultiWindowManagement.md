# WindsynthRecorder 多窗口管理架构

## 🎯 设计目标

将 WindsynthRecorder 从单窗口 + Sheet 模式升级为专业的多窗口音频应用，提供更好的工作流体验。

## 🏗️ 架构概览

### 窗口类型设计

```
WindsynthRecorder App
├── 主窗口 (WindowGroup) - 录音控制界面
├── 混音台窗口 (Window) - 音频混音和实时处理
├── VST管理器窗口 (Window) - 插件管理和参数调节
├── 音频处理器窗口 (Window) - 批量音频处理
├── 设置窗口 (Window) - 应用配置
└── 日志窗口 (Window) - 音频处理日志
```

### 核心组件

1. **WindowManager** - 统一的窗口状态管理器
2. **Window Scene** - 每个工具窗口的独立场景
3. **WindowConfig** - 窗口配置和样式定义
4. **Environment Integration** - SwiftUI 环境值集成

## 🔧 实现细节

### 1. WindowManager 设计

```swift
@MainActor
class WindowManager: ObservableObject {
    static let shared = WindowManager()
    
    // 窗口状态跟踪
    @Published var isAudioMixerOpen = false
    @Published var isVSTManagerOpen = false
    @Published var isAudioProcessorOpen = false
    
    // 窗口配置
    struct WindowConfig {
        let id: String
        let title: String
        let defaultSize: CGSize
        let minSize: CGSize
        let resizable: Bool
        let level: NSWindow.Level
    }
}
```

### 2. App Scene 结构

```swift
@main
struct WindsynthRecorderApp: App {
    @StateObject private var windowManager = WindowManager.shared
    
    var body: some Scene {
        // 主窗口
        WindowGroup {
            ContentView()
                .environmentObject(windowManager)
        }
        
        // 工具窗口
        Window("音频混音台", id: "audio-mixer") {
            AudioMixerWindowView()
                .environmentObject(windowManager)
        }
        
        // ... 其他窗口
    }
}
```

### 3. 窗口控制

```swift
// 在 ContentView 中
@EnvironmentObject private var windowManager: WindowManager
@Environment(\.openWindow) private var openWindow

// 打开窗口
Button("打开混音台") {
    openWindow(id: WindowManager.WindowConfig.audioMixer.id)
}
```

## 🎨 用户体验优势

### 相比 Sheet 的改进

| 特性 | Sheet 模式 | 独立窗口模式 |
|------|------------|--------------|
| 多任务操作 | ❌ 模态阻塞 | ✅ 非模态并行 |
| 多显示器支持 | ❌ 受限 | ✅ 自由布局 |
| 窗口大小 | ❌ 受父窗口限制 | ✅ 独立调整 |
| 专业工作流 | ❌ 业余感 | ✅ 专业音频软件标准 |
| 状态保持 | ❌ 关闭即丢失 | ✅ 窗口状态持久化 |

### 工作流场景

1. **录音 + 实时监听**
   - 主窗口：录音控制
   - 混音台窗口：实时音频监听和调节

2. **批量处理 + VST 调试**
   - 音频处理器窗口：批量文件处理
   - VST管理器窗口：插件参数调试

3. **多显示器工作站**
   - 主显示器：录音界面
   - 副显示器：混音台 + VST 管理器

## 🔑 最佳实践

### 1. 窗口生命周期管理

```swift
// 窗口打开时
.onAppear {
    windowManager.isAudioMixerOpen = true
}

// 窗口关闭时
.onDisappear {
    windowManager.isAudioMixerOpen = false
}
```

### 2. 键盘快捷键

```swift
.commands {
    CommandGroup(after: .windowArrangement) {
        Button("音频混音台") {
            windowManager.openAudioMixer()
        }
        .keyboardShortcut("m", modifiers: .command)
    }
}
```

### 3. 窗口样式统一

```swift
struct WindowStyleModifier: ViewModifier {
    let config: WindowManager.WindowConfig
    
    func body(content: Content) -> some View {
        content
            .frame(
                minWidth: config.minSize.width,
                minHeight: config.minSize.height
            )
            .navigationTitle(config.title)
    }
}
```

## 🚀 未来扩展

### 可能的新窗口类型

1. **频谱分析器窗口** - 实时音频频谱显示
2. **MIDI 控制器窗口** - MIDI 设备管理
3. **录音历史窗口** - 录音文件管理和预览
4. **插件商店窗口** - VST 插件下载和管理

### 高级功能

1. **窗口布局保存** - 保存和恢复窗口位置
2. **工作区模式** - 预设的窗口布局组合
3. **窗口分组** - 相关窗口的联动操作
4. **浮动面板** - 小型工具面板支持

## 📝 迁移指南

### 从 Sheet 到 Window 的迁移步骤

1. **创建 WindowManager** - 统一窗口状态管理
2. **定义 Window Scene** - 在 App.swift 中添加窗口定义
3. **创建窗口包装视图** - 为每个工具创建独立窗口视图
4. **更新按钮操作** - 将 `showingXXX = true` 改为 `openWindow(id:)`
5. **移除 Sheet 修饰符** - 清理不再需要的 `.sheet()` 调用
6. **测试窗口交互** - 确保窗口间的数据同步正常

这种架构为 WindsynthRecorder 提供了专业音频软件级别的多窗口体验，大大提升了用户的工作效率和使用体验。
