# JUCE 静态库与 Xcode 项目集成指南

## 🎯 概述

本指南展示如何将 JUCE 音频处理功能作为静态库集成到现有的 Swift GUI 项目中，**无需重写现有的 Swift 界面**。

## ✅ 已完成的工作

1. **CMake 配置**：已修改 `CMakeLists.txt` 生成 `WindsynthVSTCore` 静态库
2. **静态库构建**：已成功生成 `build/lib/Release/libWindsynthVSTCore.a`
3. **C 桥接接口**：已有完整的 C 接口在 `Libraries/Bridge/VSTBridge.h`
4. **桥接头文件**：已更新 `WindsynthRecorder-Bridging-Header.h`

## 🔧 Xcode 项目集成步骤

### 步骤 1：添加静态库到 Xcode 项目

1. 在 Xcode 中打开 `WindsynthRecorder.xcodeproj`
2. 选择项目根节点，然后选择 `WindsynthRecorder` target
3. 在 **Build Phases** 标签页中：
   - 展开 **Link Binary With Libraries**
   - 点击 `+` 按钮
   - 点击 **Add Other...** → **Add Files...**
   - 导航到并添加：`build/lib/Release/libWindsynthVSTCore.a`

### 步骤 2：配置头文件搜索路径

在 **Build Settings** 标签页中：

1. 搜索 **Header Search Paths**
2. 添加以下路径（设为 **recursive**）：
   ```
   $(SRCROOT)/WindsynthRecorder/Libraries/VSTSupport
   $(SRCROOT)/WindsynthRecorder/Libraries/Bridge
   $(SRCROOT)/JUCE/modules
   ```

### 步骤 3：配置库搜索路径

在 **Build Settings** 中：

1. 搜索 **Library Search Paths**
2. 添加：`$(SRCROOT)/build/lib/Release`

### 步骤 4：链接必要的系统框架

确保以下框架已在 **Link Binary With Libraries** 中：

- `AudioUnit.framework`
- `AudioToolbox.framework`
- `CoreAudio.framework`
- `CoreMIDI.framework`
- `Foundation.framework`
- `Accelerate.framework`
- `CoreFoundation.framework`

### 步骤 5：配置编译设置

在 **Build Settings** 中：

1. 搜索 **Other C++ Flags**，添加：
   ```
   -DJUCE_MAC=1
   -DJUCE_PLUGINHOST_VST3=1
   -DJUCE_PLUGINHOST_AU=1
   ```

2. 搜索 **C++ Language Dialect**，设置为：`C++17`

## 🚀 在 Swift 中使用 VST 功能

### 基本用法示例

```swift
import Foundation

class VSTManager: ObservableObject {
    private var pluginManager: OpaquePointer?
    private var processingChain: OpaquePointer?
    
    init() {
        // 创建插件管理器
        pluginManager = vstPluginManager_create()
        
        // 创建音频处理链
        processingChain = audioProcessingChain_create()
        
        // 扫描插件
        vstPluginManager_scanForPlugins(pluginManager)
    }
    
    deinit {
        if let manager = pluginManager {
            vstPluginManager_destroy(manager)
        }
        if let chain = processingChain {
            audioProcessingChain_destroy(chain)
        }
    }
    
    func getAvailablePlugins() -> [VSTPluginInfo_C] {
        guard let manager = pluginManager else { return [] }
        
        let count = vstPluginManager_getNumAvailablePlugins(manager)
        var plugins: [VSTPluginInfo_C] = []
        
        for i in 0..<count {
            var info = VSTPluginInfo_C()
            if vstPluginManager_getPluginInfo(manager, i, &info) {
                plugins.append(info)
            }
        }
        
        return plugins
    }
    
    func loadPlugin(named name: String) -> Bool {
        guard let manager = pluginManager,
              let chain = processingChain else { return false }
        
        let pluginInstance = vstPluginManager_loadPlugin(manager, name)
        if let instance = pluginInstance {
            return audioProcessingChain_addPlugin(chain, instance)
        }
        
        return false
    }
}
```

## 🔄 重新构建静态库

当你修改 C++ 代码后，需要重新构建静态库：

```bash
cd /Users/wibus/dev/WindsynthRecorder
cmake --build build --target WindsynthVSTCore --config Release
```

## 🐛 常见问题解决

### 问题 1：链接错误
**错误**：`Undefined symbols for architecture arm64`

**解决**：
1. 确保静态库路径正确
2. 检查所有必要的框架都已链接
3. 确保 C++ 标准设置为 C++17

### 问题 2：头文件找不到
**错误**：`'VSTBridge.h' file not found`

**解决**：
1. 检查 Header Search Paths 设置
2. 确保路径设置为 recursive
3. 清理并重新构建项目

### 问题 3：运行时崩溃
**错误**：应用启动时崩溃

**解决**：
1. 检查所有 JUCE 相关的编译定义
2. 确保没有 GUI 模块冲突
3. 在调试模式下运行查看具体错误

## 📚 下一步

1. **测试基本功能**：在 Swift 中创建 VSTManager 实例
2. **集成到现有 UI**：将 VST 功能添加到现有的 SwiftUI 界面
3. **实现音频处理**：连接到现有的 AudioRecorder 服务
4. **添加插件管理**：实现插件浏览和参数控制界面

## 🎉 优势

- ✅ **保持现有 Swift GUI**：无需重写任何界面代码
- ✅ **完整 JUCE 功能**：获得完整的 VST3/AU 插件支持
- ✅ **性能优化**：静态库方式，无进程间通信开销
- ✅ **类型安全**：通过 C 接口提供类型安全的 Swift 绑定
- ✅ **易于维护**：清晰的模块分离，C++ 和 Swift 代码独立
