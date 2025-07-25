//
//  RefactoringDemo.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  重构演示 - 展示新架构的使用方式
//

#include "RefactoredWindsynthEngineFacade.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WindsynthVST::Engine;

/**
 * 演示重构后的引擎使用方式
 */
void demonstrateRefactoredEngine() {
    std::cout << "=== WindsynthEngineFacade 重构演示 ===" << std::endl;
    
    // 1. 创建重构后的引擎实例
    auto engine = std::make_unique<RefactoredWindsynthEngineFacade>();
    
    // 2. 设置回调（向后兼容）
    engine->setStateCallback([](Core::EngineState state, const std::string& message) {
        std::cout << "[状态变化] " << static_cast<int>(state) << " - " << message << std::endl;
    });
    
    engine->setErrorCallback([](const std::string& error) {
        std::cout << "[错误] " << error << std::endl;
    });
    
    // 3. 配置引擎
    Core::EngineConfig config;
    config.sampleRate = 44100.0;
    config.bufferSize = 512;
    config.numInputChannels = 0;
    config.numOutputChannels = 2;
    
    // 4. 初始化引擎
    std::cout << "\n--- 初始化引擎 ---" << std::endl;
    if (engine->initialize(config)) {
        std::cout << "引擎初始化成功" << std::endl;
    } else {
        std::cout << "引擎初始化失败" << std::endl;
        return;
    }
    
    // 5. 启动引擎
    std::cout << "\n--- 启动引擎 ---" << std::endl;
    if (engine->start()) {
        std::cout << "引擎启动成功" << std::endl;
    } else {
        std::cout << "引擎启动失败" << std::endl;
        return;
    }
    
    // 6. 演示模块化访问
    std::cout << "\n--- 演示模块化访问 ---" << std::endl;
    
    // 直接访问生命周期管理器
    auto lifecycleManager = engine->getLifecycleManager();
    if (lifecycleManager) {
        std::cout << "生命周期管理器可用，当前状态: " 
                  << static_cast<int>(lifecycleManager->getState()) << std::endl;
    }
    
    // 直接访问音频文件管理器
    auto audioFileManager = engine->getAudioFileManager();
    if (audioFileManager) {
        std::cout << "音频文件管理器可用，有文件: " 
                  << (audioFileManager->hasAudioFile() ? "是" : "否") << std::endl;
    }
    
    // 直接访问参数控制器
    auto parameterController = engine->getParameterController();
    if (parameterController) {
        std::cout << "参数控制器可用" << std::endl;
    }
    
    // 7. 演示插件管理
    std::cout << "\n--- 演示插件管理 ---" << std::endl;
    auto availablePlugins = engine->getAvailablePlugins();
    std::cout << "可用插件数量: " << availablePlugins.size() << std::endl;
    
    // 8. 演示配置更新
    std::cout << "\n--- 演示配置更新 ---" << std::endl;
    const auto& currentConfig = engine->getConfiguration();
    std::cout << "当前采样率: " << currentConfig.sampleRate << std::endl;
    
    Core::EngineConfig newConfig = currentConfig;
    newConfig.sampleRate = 48000.0;
    
    if (engine->updateConfiguration(newConfig)) {
        std::cout << "配置更新成功，新采样率: " << engine->getConfiguration().sampleRate << std::endl;
    }
    
    // 9. 关闭引擎
    std::cout << "\n--- 关闭引擎 ---" << std::endl;
    engine->shutdown();
    std::cout << "引擎已关闭" << std::endl;
    
    std::cout << "\n=== 重构演示完成 ===" << std::endl;
}

/**
 * 演示观察者模式的使用
 */
void demonstrateObserverPattern() {
    std::cout << "\n=== 观察者模式演示 ===" << std::endl;
    
    // 创建引擎和获取通知器
    auto engine = std::make_unique<RefactoredWindsynthEngineFacade>();
    auto context = engine->getContext();
    
    if (!context) {
        std::cout << "无法获取引擎上下文" << std::endl;
        return;
    }
    
    // 这里可以演示如何添加自定义观察者
    // 但由于当前实现中通知器不直接暴露，我们使用回调方式
    
    engine->setStateCallback([](Core::EngineState state, const std::string& message) {
        std::cout << "[观察者] 状态变化: " << static_cast<int>(state) << " - " << message << std::endl;
    });
    
    // 触发一些状态变化
    Core::EngineConfig config;
    engine->initialize(config);
    engine->start();
    engine->stop();
    engine->shutdown();
    
    std::cout << "=== 观察者模式演示完成 ===" << std::endl;
}

/**
 * 演示单一职责原则的好处
 */
void demonstrateSingleResponsibility() {
    std::cout << "\n=== 单一职责原则演示 ===" << std::endl;
    
    auto engine = std::make_unique<RefactoredWindsynthEngineFacade>();
    
    // 每个管理器都有明确的职责边界
    std::cout << "1. 生命周期管理器 - 只负责引擎的启动/停止" << std::endl;
    std::cout << "2. 音频文件管理器 - 只负责音频文件的加载/播放" << std::endl;
    std::cout << "3. 参数控制器 - 只负责插件参数的控制" << std::endl;
    std::cout << "4. 门面类 - 只负责协调各个管理器" << std::endl;
    
    // 这样的设计使得：
    std::cout << "\n优势:" << std::endl;
    std::cout << "- 每个类的职责明确，易于理解和维护" << std::endl;
    std::cout << "- 可以独立测试每个管理器" << std::endl;
    std::cout << "- 修改一个功能不会影响其他功能" << std::endl;
    std::cout << "- 可以轻松扩展新功能" << std::endl;
    
    std::cout << "=== 单一职责原则演示完成 ===" << std::endl;
}

/**
 * 主函数 - 运行所有演示
 */
int main() {
    try {
        demonstrateRefactoredEngine();
        demonstrateObserverPattern();
        demonstrateSingleResponsibility();
        
        std::cout << "\n所有演示完成！" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "演示过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
}
