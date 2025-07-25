//
//  EngineContext.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎共享上下文实现
//

#include "EngineContext.hpp"
#include <iostream>

namespace WindsynthVST::Engine::Core {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

EngineContext::EngineContext() {
    std::cout << "[EngineContext] 构造函数" << std::endl;
}

EngineContext::~EngineContext() {
    std::cout << "[EngineContext] 析构函数" << std::endl;
    shutdown();
}

//==============================================================================
// 初始化和清理
//==============================================================================

bool EngineContext::initialize() {
    std::cout << "[EngineContext] 初始化共享上下文" << std::endl;
    
    if (initialized.load()) {
        std::cout << "[EngineContext] 上下文已经初始化" << std::endl;
        return true;
    }
    
    try {
        // 创建核心组件
        graphProcessor = std::make_shared<AudioGraph::GraphAudioProcessor>();
        pluginLoader = std::make_shared<AudioGraph::ModernPluginLoader>();
        pluginManager = std::make_shared<AudioGraph::PluginManager>(*graphProcessor, *pluginLoader);
        graphManager = std::make_shared<AudioGraph::GraphManager>(*graphProcessor);
        ioManager = std::make_shared<AudioGraph::AudioIOManager>(*graphProcessor);
        presetManager = std::make_shared<AudioGraph::PresetManager>(*graphProcessor, *pluginManager);
        
        // 创建音频格式管理器
        formatManager = std::make_shared<juce::AudioFormatManager>();
        formatManager->registerBasicFormats();
        
        initialized.store(true);
        std::cout << "[EngineContext] 共享上下文初始化完成" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[EngineContext] 初始化失败: " << e.what() << std::endl;
        return false;
    }
}

void EngineContext::shutdown() {
    if (!initialized.load()) {
        return;
    }
    
    std::cout << "[EngineContext] 关闭共享上下文" << std::endl;
    
    try {
        // 按依赖顺序清理组件
        presetManager.reset();
        ioManager.reset();
        graphManager.reset();
        pluginManager.reset();
        pluginLoader.reset();
        graphProcessor.reset();
        formatManager.reset();
        
        initialized.store(false);
        std::cout << "[EngineContext] 共享上下文已关闭" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[EngineContext] 关闭时出错: " << e.what() << std::endl;
    }
}

} // namespace WindsynthVST::Engine::Core
