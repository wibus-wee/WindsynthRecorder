//
//  EngineContext.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  引擎共享上下文 - 管理所有管理器之间的共享依赖
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <atomic>
#include <mutex>

// AudioGraph 组件
#include "AudioGraph/Core/GraphAudioProcessor.hpp"
#include "AudioGraph/Management/GraphManager.hpp"
#include "AudioGraph/Plugins/PluginManager.hpp"
#include "AudioGraph/Plugins/ModernPluginLoader.hpp"
#include "AudioGraph/Management/AudioIOManager.hpp"
#include "AudioGraph/Management/PresetManager.hpp"

namespace WindsynthVST::Engine::Core {

/**
 * 引擎状态枚举
 */
enum class EngineState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Error
};

/**
 * 引擎配置结构
 */
struct EngineConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
    int numInputChannels = 0;
    int numOutputChannels = 2;
    bool enableRealtimeProcessing = true;
    std::string audioDeviceName;
};

/**
 * 引擎共享上下文
 * 
 * 这个类管理所有管理器之间的共享依赖，包括：
 * - AudioGraph 核心组件
 * - 状态管理
 * - 配置信息
 * - 线程安全的访问控制
 */
class EngineContext {
public:
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    EngineContext();
    ~EngineContext();
    
    //==============================================================================
    // 核心组件访问
    //==============================================================================
    
    std::shared_ptr<AudioGraph::GraphAudioProcessor> getGraphProcessor() const {
        return graphProcessor;
    }
    
    std::shared_ptr<AudioGraph::GraphManager> getGraphManager() const {
        return graphManager;
    }
    
    std::shared_ptr<AudioGraph::PluginManager> getPluginManager() const {
        return pluginManager;
    }
    
    std::shared_ptr<AudioGraph::ModernPluginLoader> getPluginLoader() const {
        return pluginLoader;
    }
    
    std::shared_ptr<AudioGraph::AudioIOManager> getIOManager() const {
        return ioManager;
    }
    
    std::shared_ptr<AudioGraph::PresetManager> getPresetManager() const {
        return presetManager;
    }
    
    //==============================================================================
    // 状态管理
    //==============================================================================
    
    EngineState getState() const {
        return currentState.load();
    }
    
    void setState(EngineState newState) {
        currentState.store(newState);
    }
    
    bool isRunning() const {
        return currentState.load() == EngineState::Running;
    }
    
    //==============================================================================
    // 配置管理
    //==============================================================================
    
    const EngineConfig& getConfig() const {
        std::lock_guard<std::mutex> lock(configMutex);
        return currentConfig;
    }
    
    void setConfig(const EngineConfig& config) {
        std::lock_guard<std::mutex> lock(configMutex);
        currentConfig = config;
    }
    
    //==============================================================================
    // 音频格式管理
    //==============================================================================
    
    std::shared_ptr<juce::AudioFormatManager> getFormatManager() const {
        return formatManager;
    }
    
    //==============================================================================
    // 初始化和清理
    //==============================================================================
    
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized; }

private:
    //==============================================================================
    // 核心组件（共享指针管理）
    //==============================================================================
    
    std::shared_ptr<AudioGraph::GraphAudioProcessor> graphProcessor;
    std::shared_ptr<AudioGraph::GraphManager> graphManager;
    std::shared_ptr<AudioGraph::PluginManager> pluginManager;
    std::shared_ptr<AudioGraph::ModernPluginLoader> pluginLoader;
    std::shared_ptr<AudioGraph::AudioIOManager> ioManager;
    std::shared_ptr<AudioGraph::PresetManager> presetManager;
    
    //==============================================================================
    // 音频格式管理
    //==============================================================================
    
    std::shared_ptr<juce::AudioFormatManager> formatManager;
    
    //==============================================================================
    // 状态管理
    //==============================================================================
    
    std::atomic<EngineState> currentState{EngineState::Stopped};
    EngineConfig currentConfig;
    mutable std::mutex configMutex;
    
    //==============================================================================
    // 初始化状态
    //==============================================================================
    
    std::atomic<bool> initialized{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineContext)
};

} // namespace WindsynthVST::Engine::Core
