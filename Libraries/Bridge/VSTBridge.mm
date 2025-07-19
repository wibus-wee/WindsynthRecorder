#include "VSTBridge.h"
#include "../VSTSupport/JuceIncludes.h"
#include "../VSTSupport/VSTPluginManager.hpp"
#include "../VSTSupport/AudioProcessingChain.hpp"
#include "../VSTSupport/RealtimeProcessor.hpp"
#include <memory>
#include <string>
#include <cstring>

// 使用 WindsynthVST 命名空间
using namespace WindsynthVST;

// 避免与 macOS 系统头文件的命名冲突
#define Point CarbonPoint
#define Component CarbonComponent
#import <Foundation/Foundation.h>
#undef Point
#undef Component

using namespace WindsynthVST;

// 内部结构体定义
struct VSTPluginManagerHandle {
    std::unique_ptr<VSTPluginManager> manager;
    ScanProgressCallback scanProgressCallback;
    ErrorCallback errorCallback;
    void* scanProgressUserData;
    void* errorUserData;

    VSTPluginManagerHandle() : scanProgressCallback(nullptr), errorCallback(nullptr),
                               scanProgressUserData(nullptr), errorUserData(nullptr) {}
};

struct VSTPluginInstanceHandle {
    std::unique_ptr<VSTPluginInstance> instance;
    void* editorWindow; // 简化为void*避免JUCE依赖

    VSTPluginInstanceHandle() : editorWindow(0) {}
};

struct AudioProcessingChainHandle {
    std::unique_ptr<AudioProcessingChain> chain;
    ErrorCallback errorCallback;
    void* errorUserData;

    AudioProcessingChainHandle() : errorCallback(nullptr), errorUserData(nullptr) {}
};

// VSTAudioUnit 已移除，不再需要此结构体

// 辅助函数
static void copyString(const juce::String& src, char* dest, size_t maxLength) {
    if (dest && maxLength > 0) {
        strncpy(dest, src.toUTF8(), maxLength - 1);
        dest[maxLength - 1] = '\0';
    }
}

static void convertPluginInfo(const VSTPluginInfo& src, VSTPluginInfo_C* dest) {
    if (!dest) return;
    
    copyString(src.name, dest->name, sizeof(dest->name));
    copyString(src.manufacturer, dest->manufacturer, sizeof(dest->manufacturer));
    copyString(src.version, dest->version, sizeof(dest->version));
    copyString(src.category, dest->category, sizeof(dest->category));
    copyString(src.pluginFormatName, dest->pluginFormatName, sizeof(dest->pluginFormatName));
    copyString(src.fileOrIdentifier, dest->fileOrIdentifier, sizeof(dest->fileOrIdentifier));
    
    dest->numInputChannels = src.numInputChannels;
    dest->numOutputChannels = src.numOutputChannels;
    dest->isInstrument = src.isInstrument;
    dest->acceptsMidi = src.acceptsMidi;
    dest->producesMidi = src.producesMidi;
}

// ============================================================================
// VSTPluginManager C接口实现
// ============================================================================

VSTPluginManagerHandle* vstPluginManager_create(void) {
    try {
        VSTPluginManagerHandle* handle = new VSTPluginManagerHandle();
        handle->manager.reset(new VSTPluginManager());
        
        // 设置回调 - 使用同步调用确保字符串生命周期
        handle->manager->setScanProgressCallback([handle](const std::string& pluginName, float progress) {
            if (handle->scanProgressCallback) {
                // 直接在当前线程中调用，确保字符串在调用期间有效
                const char* namePtr = pluginName.c_str();
                handle->scanProgressCallback(namePtr, progress, handle->scanProgressUserData);
            }
        });

        handle->manager->setErrorCallback([handle](const std::string& error) {
            if (handle->errorCallback) {
                // 直接在当前线程中调用，确保字符串在调用期间有效
                const char* errorPtr = error.c_str();
                handle->errorCallback(errorPtr, handle->errorUserData);
            }
        });
        
        return handle;
    } catch (...) {
        return nullptr;
    }
}

void vstPluginManager_destroy(VSTPluginManagerHandle* handle) {
    delete handle;
}

void vstPluginManager_scanForPlugins(VSTPluginManagerHandle* handle) {
    if (handle && handle->manager) {
        handle->manager->scanForPlugins();
    }
}

void vstPluginManager_scanDirectory(VSTPluginManagerHandle* handle, const char* directoryPath) {
    if (handle && handle->manager && directoryPath) {
        handle->manager->scanDirectory(directoryPath);
    }
}

void vstPluginManager_addPluginSearchPath(VSTPluginManagerHandle* handle, const char* path) {
    if (handle && handle->manager && path) {
        handle->manager->addPluginSearchPath(path);
    }
}

int vstPluginManager_getNumAvailablePlugins(VSTPluginManagerHandle* handle) {
    if (handle && handle->manager) {
        return handle->manager->getNumAvailablePlugins();
    }
    return 0;
}

bool vstPluginManager_getPluginInfo(VSTPluginManagerHandle* handle, int index, VSTPluginInfo_C* info) {
    if (!handle || !handle->manager || !info || index < 0) {
        return false;
    }
    
    try {
        auto plugins = handle->manager->getAvailablePlugins();
        if (index >= static_cast<int>(plugins.size())) {
            return false;
        }
        
        convertPluginInfo(plugins[index], info);
        return true;
    } catch (...) {
        return false;
    }
}

int vstPluginManager_findPluginByName(VSTPluginManagerHandle* handle, const char* name) {
    if (!handle || !handle->manager || !name) {
        return -1;
    }
    
    try {
        auto plugins = handle->manager->getAvailablePlugins();
        for (int i = 0; i < static_cast<int>(plugins.size()); ++i) {
            if (plugins[i].name == name) {
                return i;
            }
        }
    } catch (...) {
        // 忽略异常
    }
    
    return -1;
}

VSTPluginInstanceHandle* vstPluginManager_loadPlugin(VSTPluginManagerHandle* handle, const char* identifier) {
    if (!handle || !handle->manager || !identifier) {
        NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Invalid parameters - handle=%p, manager=%p, identifier=%s",
              handle, handle ? handle->manager.get() : nullptr, identifier ? identifier : "null");
        return nullptr;
    }

    NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Loading plugin with identifier: %s", identifier);

    try {
        auto instance = handle->manager->loadPlugin(identifier);
        if (instance) {
            NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Plugin instance created successfully");
            auto instanceHandle = new VSTPluginInstanceHandle();
            instanceHandle->instance = std::move(instance);
            NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Plugin handle created, returning success");
            return instanceHandle;
        } else {
            NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Failed to create plugin instance");
        }
    } catch (const std::exception& e) {
        NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Exception caught: %s", e.what());
    } catch (...) {
        NSLog(@"[VSTBridge] vstPluginManager_loadPlugin: Unknown exception caught");
    }

    return nullptr;
}

VSTPluginInstanceHandle* vstPluginManager_loadPluginByIndex(VSTPluginManagerHandle* handle, int index) {
    if (!handle || !handle->manager || index < 0) {
        return nullptr;
    }
    
    try {
        auto plugins = handle->manager->getAvailablePlugins();
        if (index >= static_cast<int>(plugins.size())) {
            return nullptr;
        }
        
        auto instance = handle->manager->loadPlugin(plugins[index]);
        if (instance) {
            auto instanceHandle = new VSTPluginInstanceHandle();
            instanceHandle->instance = std::move(instance);
            return instanceHandle;
        }
    } catch (...) {
        // 忽略异常
    }
    
    return nullptr;
}

bool vstPluginManager_isScanning(VSTPluginManagerHandle* handle) {
    if (handle && handle->manager) {
        return handle->manager->isScanning();
    }
    return false;
}

void vstPluginManager_setScanProgressCallback(VSTPluginManagerHandle* handle, 
                                            ScanProgressCallback callback, void* userData) {
    if (handle) {
        handle->scanProgressCallback = callback;
        handle->scanProgressUserData = userData;
    }
}

void vstPluginManager_setErrorCallback(VSTPluginManagerHandle* handle, 
                                     ErrorCallback callback, void* userData) {
    if (handle) {
        handle->errorCallback = callback;
        handle->errorUserData = userData;
    }
}

// ============================================================================
// VSTPluginInstance C接口实现
// ============================================================================

void vstPluginInstance_destroy(VSTPluginInstanceHandle* handle) {
    delete handle;
}

bool vstPluginInstance_isValid(VSTPluginInstanceHandle* handle) {
    return handle && handle->instance && handle->instance->isValid();
}

const char* vstPluginInstance_getName(VSTPluginInstanceHandle* handle) {
    if (handle && handle->instance) {
        return handle->instance->getName().c_str();
    }
    return nullptr;
}

void vstPluginInstance_prepareToPlay(VSTPluginInstanceHandle* handle, double sampleRate, int samplesPerBlock) {
    if (handle && handle->instance) {
        handle->instance->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void vstPluginInstance_processBlock(VSTPluginInstanceHandle* handle, 
                                  float** audioBuffer, int numChannels, int numSamples,
                                  uint8_t* midiData, int midiDataSize) {
    if (!handle || !handle->instance || !audioBuffer) {
        return;
    }
    
    try {
        // 创建JUCE音频缓冲区
        juce::AudioBuffer<float> buffer(audioBuffer, numChannels, numSamples);
        
        // 创建MIDI缓冲区
        juce::MidiBuffer midiBuffer;
        if (midiData && midiDataSize > 0) {
            // 这里需要解析MIDI数据，暂时留空
            // TODO: 实现MIDI数据解析
        }
        
        handle->instance->processBlock(buffer, midiBuffer);
    } catch (...) {
        // 忽略异常
    }
}

void vstPluginInstance_releaseResources(VSTPluginInstanceHandle* handle) {
    if (handle && handle->instance) {
        handle->instance->releaseResources();
    }
}

int vstPluginInstance_getNumParameters(VSTPluginInstanceHandle* handle) {
    if (handle && handle->instance) {
        return handle->instance->getNumParameters();
    }
    return 0;
}

float vstPluginInstance_getParameter(VSTPluginInstanceHandle* handle, int index) {
    if (handle && handle->instance) {
        return handle->instance->getParameter(index);
    }
    return 0.0f;
}

void vstPluginInstance_setParameter(VSTPluginInstanceHandle* handle, int index, float value) {
    if (handle && handle->instance) {
        handle->instance->setParameter(index, value);
    }
}

bool vstPluginInstance_getParameterName(VSTPluginInstanceHandle* handle, int index, char* name, int maxLength) {
    if (handle && handle->instance && name && maxLength > 0) {
        // 暂时返回空字符串，避免编译错误
        strncpy(name, "Parameter", maxLength - 1);
        name[maxLength - 1] = '\0';
        return true;
    }
    return false;
}

bool vstPluginInstance_getParameterText(VSTPluginInstanceHandle* handle, int index, char* text, int maxLength) {
    if (handle && handle->instance && text && maxLength > 0) {
        // 暂时返回空字符串，避免编译错误
        strncpy(text, "0.0", maxLength - 1);
        text[maxLength - 1] = '\0';
        return true;
    }
    return false;
}

bool vstPluginInstance_hasEditor(VSTPluginInstanceHandle* handle) {
    if (handle && handle->instance) {
        return handle->instance->hasEditor();
    }
    return false;
}

void vstPluginInstance_showEditor(VSTPluginInstanceHandle* handle) {
    if (handle && handle->instance && handle->instance->hasEditor()) {
        // 暂时简化实现，避免编译错误
        NSLog(@"[VSTBridge] vstPluginInstance_showEditor: Editor functionality temporarily disabled");
    }
}

void vstPluginInstance_hideEditor(VSTPluginInstanceHandle* handle) {
    if (handle && handle->editorWindow) {
        // 暂时简化实现
        handle->editorWindow = 0;
    }
}

// ============================================================================
// AudioProcessingChain C接口实现
// ============================================================================

AudioProcessingChainHandle* audioProcessingChain_create(void) {
    try {
        AudioProcessingChainHandle* handle = new AudioProcessingChainHandle();
        handle->chain.reset(new AudioProcessingChain());
        return handle;
    } catch (...) {
        return 0;
    }
}

void audioProcessingChain_destroy(AudioProcessingChainHandle* handle) {
    delete handle;
}

void audioProcessingChain_configure(AudioProcessingChainHandle* handle, const ProcessingChainConfig_C* config) {
    if (handle && handle->chain && config) {
        ProcessingChainConfig cppConfig;
        cppConfig.sampleRate = config->sampleRate;
        cppConfig.samplesPerBlock = config->samplesPerBlock;
        cppConfig.numInputChannels = config->numInputChannels;
        cppConfig.numOutputChannels = config->numOutputChannels;
        cppConfig.enableMidi = config->enableMidi;

        handle->chain->configure(cppConfig);
    }
}

void audioProcessingChain_getConfig(AudioProcessingChainHandle* handle, ProcessingChainConfig_C* config) {
    if (handle && handle->chain && config) {
        // 暂时简化实现
        config->sampleRate = 44100.0;
        config->samplesPerBlock = 512;
        config->numInputChannels = 2;
        config->numOutputChannels = 2;
        config->enableMidi = false;
    }
}

void audioProcessingChain_prepareToPlay(AudioProcessingChainHandle* handle, double sampleRate, int samplesPerBlock) {
    if (handle && handle->chain) {
        handle->chain->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void audioProcessingChain_processBlock(AudioProcessingChainHandle* handle,
                                     float** audioBuffer, int numChannels, int numSamples,
                                     uint8_t* midiData, int midiDataSize) {
    if (!handle || !handle->chain || !audioBuffer || !audioBuffer[0]) {
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Invalid parameters - handle=%p, chain=%p, buffer=%p",
              handle, handle ? handle->chain.get() : nullptr, audioBuffer);
        return;
    }

    // 检查处理链是否有插件
    if (handle->chain->getNumPlugins() == 0) {
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: No plugins in chain, skipping processing");
        return; // 没有插件，直接返回
    }

    try {
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Processing %d samples, %d channels", numSamples, numChannels);

        // 验证参数范围
        if (numSamples <= 0 || numSamples > 8192 || numChannels <= 0 || numChannels > 8) {
            NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Invalid parameters - samples=%d, channels=%d", numSamples, numChannels);
            return;
        }

        // 调用真正的 VST 处理链
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Processing %d samples, %d channels (calling real VST processing)", numSamples, numChannels);

        // 创建 JUCE AudioBuffer 从 float** 数据
        juce::AudioBuffer<float> juceBuffer(audioBuffer, numChannels, numSamples);

        // 创建空的 MIDI 缓冲区
        juce::MidiBuffer midiBuffer;

        // 调用 AudioProcessingChain 的 processBlock 方法
        handle->chain->processBlock(juceBuffer, midiBuffer);

        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Real VST processing completed successfully");

    } catch (const std::exception& e) {
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Exception caught: %s", e.what());
    } catch (...) {
        NSLog(@"[VSTBridge] audioProcessingChain_processBlock: Unknown exception caught");
    }
}

void audioProcessingChain_releaseResources(AudioProcessingChainHandle* handle) {
    if (handle && handle->chain) {
        handle->chain->releaseResources();
    }
}

bool audioProcessingChain_addPlugin(AudioProcessingChainHandle* handle, VSTPluginInstanceHandle* plugin) {
    NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Starting plugin addition");

    if (!handle) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: handle is null");
        return false;
    }
    if (!handle->chain) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: handle->chain is null");
        return false;
    }
    if (!plugin) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: plugin is null");
        return false;
    }
    if (!plugin->instance) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: plugin->instance is null");
        return false;
    }

    NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: All parameters valid, attempting to add plugin");

    try {
        // 转移插件实例的所有权到处理链
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Attempting to add plugin to chain");

        // 直接转移插件实例的所有权到处理链
        bool success = handle->chain->addPlugin(std::move(plugin->instance));

        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: chain->addPlugin returned: %s", success ? "true" : "false");

        if (success) {
            // 插件实例的所有权已经转移到处理链，清空句柄中的指针
            plugin->instance.reset();
            NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Plugin successfully added to chain");
        } else {
            NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Failed to add plugin to chain");
        }

        return success;
    } catch (const std::exception& e) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Exception caught: %s", e.what());
        return false;
    } catch (...) {
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Unknown exception caught");
        return false;
    }
}

int audioProcessingChain_getNumPlugins(AudioProcessingChainHandle* handle) {
    if (handle && handle->chain) {
        return handle->chain->getNumPlugins();
    }
    return 0;
}

void audioProcessingChain_setPluginBypassed(AudioProcessingChainHandle* handle, int index, bool bypassed) {
    if (handle && handle->chain) {
        handle->chain->setPluginBypassed(index, bypassed);
    }
}

bool audioProcessingChain_isPluginBypassed(AudioProcessingChainHandle* handle, int index) {
    if (handle && handle->chain) {
        return handle->chain->isPluginBypassed(index);
    }
    return false;
}

void audioProcessingChain_setEnabled(AudioProcessingChainHandle* handle, bool enabled) {
    if (handle && handle->chain) {
        handle->chain->setEnabled(enabled);
    }
}

bool audioProcessingChain_isEnabled(AudioProcessingChainHandle* handle) {
    if (handle && handle->chain) {
        return handle->chain->isEnabled();
    }
    return false;
}

void audioProcessingChain_setErrorCallback(AudioProcessingChainHandle* handle,
                                         ErrorCallback callback, void* userData) {
    if (handle) {
        handle->errorCallback = callback;
        handle->errorUserData = userData;
    }
}

bool audioProcessingChain_removePlugin(AudioProcessingChainHandle* handle, int index) {
    if (handle && handle->chain) {
        return handle->chain->removePlugin(index);
    }
    return false;
}

void audioProcessingChain_clearPlugins(AudioProcessingChainHandle* handle) {
    if (handle && handle->chain) {
        handle->chain->clearPlugins();
    }
}

// ============================================================================
// AudioProcessingChain 插件编辑器管理
// ============================================================================

bool audioProcessingChain_showPluginEditor(AudioProcessingChainHandle* handle, int index) {
    if (!handle || !handle->chain) {
        NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Invalid handle");
        return false;
    }

    NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Attempting to show editor for plugin at index %d", index);

    try {
        return handle->chain->showPluginEditor(index);
    } catch (const std::exception& e) {
        NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Exception: %s", e.what());
        return false;
    } catch (...) {
        NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Unknown exception");
        return false;
    }
}

void audioProcessingChain_hidePluginEditor(AudioProcessingChainHandle* handle, int index) {
    if (!handle || !handle->chain) {
        return;
    }

    NSLog(@"[VSTBridge] audioProcessingChain_hidePluginEditor: Hiding editor for plugin at index %d", index);
    handle->chain->hidePluginEditor(index);
}

bool audioProcessingChain_hasPluginEditor(AudioProcessingChainHandle* handle, int index) {
    if (!handle || !handle->chain) {
        return false;
    }

    try {
        return handle->chain->hasPluginEditor(index);
    } catch (...) {
        return false;
    }
}

// ============================================================================
// RealtimeProcessor 简化接口实现
// ============================================================================

struct RealtimeProcessorHandle_Internal {
    std::unique_ptr<RealtimeProcessor> processor;
    bool initialized;

    RealtimeProcessorHandle_Internal() : initialized(false) {
        try {
            processor.reset(new RealtimeProcessor());
            initialized = true;
        } catch (const std::exception& e) {
            NSLog(@"[VSTBridge] Failed to create RealtimeProcessor: %s", e.what());
            processor.reset();
            initialized = false;
        } catch (...) {
            NSLog(@"[VSTBridge] Failed to create RealtimeProcessor: unknown error");
            processor.reset();
            initialized = false;
        }
    }
};

RealtimeProcessorHandle realtimeProcessor_create() {
    try {
        auto handle = new RealtimeProcessorHandle_Internal();
        return reinterpret_cast<RealtimeProcessorHandle>(handle);
    } catch (...) {
        return nullptr;
    }
}

void realtimeProcessor_destroy(RealtimeProcessorHandle handle) {
    if (handle) {
        auto internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
        delete internalHandle;
    }
}

void realtimeProcessor_configure(RealtimeProcessorHandle handle, const RealtimeProcessorConfig_C* config) {
    if (!handle || !config) return;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return;

    try {
        // 转换 C 配置结构到 C++ 配置
        WindsynthVST::RealtimeProcessorConfig cppConfig;
        cppConfig.sampleRate = config->sampleRate;
        cppConfig.bufferSize = static_cast<int>(config->bufferSize);
        cppConfig.numInputChannels = static_cast<int>(config->numInputChannels);
        cppConfig.numOutputChannels = static_cast<int>(config->numOutputChannels);
        cppConfig.enableMonitoring = config->enableMonitoring;
        cppConfig.enableRecording = config->enableRecording;
        cppConfig.monitoringGain = config->monitoringGain;
        cppConfig.latencyCompensationSamples = static_cast<int>(config->latencyCompensationSamples);

        // 配置处理器
        internalHandle->processor->configure(cppConfig);

    } catch (...) {
        // 忽略异常
    }
}

void realtimeProcessor_getConfig(RealtimeProcessorHandle handle, RealtimeProcessorConfig_C* config) {
    if (!handle || !config) return;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return;

    try {
        auto cppConfig = internalHandle->processor->getConfig();

        config->sampleRate = cppConfig.sampleRate;
        config->bufferSize = static_cast<int32_t>(cppConfig.bufferSize);
        config->numInputChannels = static_cast<int32_t>(cppConfig.numInputChannels);
        config->numOutputChannels = static_cast<int32_t>(cppConfig.numOutputChannels);
        config->enableMonitoring = cppConfig.enableMonitoring;
        config->enableRecording = cppConfig.enableRecording;
        config->monitoringGain = cppConfig.monitoringGain;
        config->latencyCompensationSamples = static_cast<int32_t>(cppConfig.latencyCompensationSamples);

    } catch (...) {
        // 忽略异常
    }
}

bool realtimeProcessor_start(RealtimeProcessorHandle handle) {
    if (handle) {
        RealtimeProcessorHandle_Internal* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
        if (internalHandle->initialized && internalHandle->processor) {
            try {
                NSLog(@"[VSTBridge] Starting RealtimeProcessor...");
                bool result = internalHandle->processor->start();
                NSLog(@"[VSTBridge] RealtimeProcessor start result: %s", result ? "success" : "failed");
                return result;
            } catch (const std::exception& e) {
                NSLog(@"[VSTBridge] RealtimeProcessor start failed with exception: %s", e.what());
                return false;
            } catch (...) {
                NSLog(@"[VSTBridge] RealtimeProcessor start failed with unknown exception");
                return false;
            }
        } else {
            NSLog(@"[VSTBridge] RealtimeProcessor not initialized or null");
        }
    }
    return false;
}

void realtimeProcessor_stop(RealtimeProcessorHandle handle) {
    if (handle) {
        auto internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
        if (internalHandle->processor) {
            try {
                internalHandle->processor->stop();
            } catch (...) {
                // 忽略异常
            }
        }
    }
}

bool realtimeProcessor_isRunning(RealtimeProcessorHandle handle) {
    if (handle) {
        auto internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
        if (internalHandle->processor) {
            try {
                return internalHandle->processor->isRunning();
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

void realtimeProcessor_setProcessingChain(RealtimeProcessorHandle handle, AudioProcessingChainHandle* chainHandle) {
    if (handle && chainHandle) {
        RealtimeProcessorHandle_Internal* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
        if (internalHandle->processor && chainHandle->chain) {
            try {
                // 将 unique_ptr 转换为 shared_ptr（不转移所有权）
                // 使用空删除器，因为我们不想删除对象
                struct NullDeleter { void operator()(AudioProcessingChain*) {} };
                std::shared_ptr<AudioProcessingChain> sharedChain(chainHandle->chain.get(), NullDeleter());
                internalHandle->processor->setProcessingChain(sharedChain);
            } catch (...) {
                // 忽略异常
            }
        }
    }
}

// ============================================================================
// JUCE 音频引擎桥接实现
// ============================================================================

// 音频文件读取器句柄结构
struct AudioFileReaderHandle_Internal {
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::unique_ptr<juce::AudioFormatManager> formatManager;
    juce::File file;

    AudioFileReaderHandle_Internal() {
        formatManager = std::make_unique<juce::AudioFormatManager>();
        formatManager->registerBasicFormats();
    }
};

// 音频传输源句柄结构
struct AudioTransportSourceHandle_Internal {
    std::unique_ptr<juce::AudioTransportSource> transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::TimeSliceThread> backgroundThread;

    AudioTransportSourceHandle_Internal() {
        transportSource = std::make_unique<juce::AudioTransportSource>();
        backgroundThread = std::make_unique<juce::TimeSliceThread>("Audio File Reader Thread");
        backgroundThread->startThread();
    }

    ~AudioTransportSourceHandle_Internal() {
        if (backgroundThread) {
            backgroundThread->stopThread(1000);
        }
    }
};

// 音频文件读取器管理
AudioFileReaderHandle audioFileReader_create(const char* filePath) {
    if (!filePath) {
        NSLog(@"[VSTBridge] audioFileReader_create: filePath is null");
        return nullptr;
    }

    try {
        NSLog(@"[VSTBridge] audioFileReader_create: Attempting to create reader for file: %s", filePath);

        auto handle = new AudioFileReaderHandle_Internal();

        // 使用 NSString 来正确处理 UTF-8 编码
        NSString* nsPath = [NSString stringWithUTF8String:filePath];
        juce::String jucePath = juce::String::fromUTF8([nsPath UTF8String]);
        handle->file = juce::File(jucePath);

        // 详细的文件检查
        NSLog(@"[VSTBridge] File path: %s", filePath);
        NSLog(@"[VSTBridge] JUCE File path: %s", handle->file.getFullPathName().toRawUTF8());
        NSLog(@"[VSTBridge] File exists (JUCE): %s", handle->file.exists() ? "YES" : "NO");
        NSLog(@"[VSTBridge] File exists as file (JUCE): %s", handle->file.existsAsFile() ? "YES" : "NO");
        NSLog(@"[VSTBridge] File size: %lld bytes", handle->file.getSize());

        // 使用 NSFileManager 检查
        BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:nsPath];
        NSLog(@"[VSTBridge] File exists (NSFileManager): %s", fileExists ? "YES" : "NO");

        if (!handle->file.existsAsFile()) {
            NSLog(@"[VSTBridge] audioFileReader_create: File does not exist: %s", filePath);
            delete handle;
            return nullptr;
        }

        NSLog(@"[VSTBridge] audioFileReader_create: File exists, creating reader...");
        handle->reader.reset(handle->formatManager->createReaderFor(handle->file));

        if (!handle->reader) {
            NSLog(@"[VSTBridge] audioFileReader_create: Failed to create reader for file: %s", filePath);
            delete handle;
            return nullptr;
        }

        NSLog(@"[VSTBridge] audioFileReader_create: Successfully created reader for file: %s", filePath);
        return reinterpret_cast<AudioFileReaderHandle>(handle);
    } catch (const std::exception& e) {
        NSLog(@"[VSTBridge] audioFileReader_create: Exception: %s", e.what());
        return nullptr;
    } catch (...) {
        NSLog(@"[VSTBridge] audioFileReader_create: Unknown exception");
        return nullptr;
    }
}

void audioFileReader_destroy(AudioFileReaderHandle handle) {
    if (handle) {
        auto* internalHandle = reinterpret_cast<AudioFileReaderHandle_Internal*>(handle);
        delete internalHandle;
    }
}

double audioFileReader_getLengthInSeconds(AudioFileReaderHandle handle) {
    if (!handle) return 0.0;

    auto* internalHandle = reinterpret_cast<AudioFileReaderHandle_Internal*>(handle);
    if (!internalHandle->reader) return 0.0;

    return static_cast<double>(internalHandle->reader->lengthInSamples) / internalHandle->reader->sampleRate;
}

double audioFileReader_getSampleRate(AudioFileReaderHandle handle) {
    if (!handle) return 0.0;

    auto* internalHandle = reinterpret_cast<AudioFileReaderHandle_Internal*>(handle);
    if (!internalHandle->reader) return 0.0;

    return internalHandle->reader->sampleRate;
}

int audioFileReader_getNumChannels(AudioFileReaderHandle handle) {
    if (!handle) return 0;

    auto* internalHandle = reinterpret_cast<AudioFileReaderHandle_Internal*>(handle);
    if (!internalHandle->reader) return 0;

    return static_cast<int>(internalHandle->reader->numChannels);
}

// 音频传输源管理
AudioTransportSourceHandle audioTransportSource_create(AudioFileReaderHandle reader) {
    if (!reader) {
        return nullptr;
    }

    try {
        auto* readerHandle = reinterpret_cast<AudioFileReaderHandle_Internal*>(reader);
        if (!readerHandle->reader) {
            return nullptr;
        }

        auto handle = new AudioTransportSourceHandle_Internal();

        // 创建 AudioFormatReaderSource
        handle->readerSource = std::make_unique<juce::AudioFormatReaderSource>(
            readerHandle->reader.get(), false  // 不删除 reader，因为它由 AudioFileReaderHandle 管理
        );

        // 设置传输源
        handle->transportSource->setSource(
            handle->readerSource.get(),
            32768,  // 预读缓冲区大小
            handle->backgroundThread.get(),  // 后台线程
            readerHandle->reader->sampleRate  // 采样率
        );

        return reinterpret_cast<AudioTransportSourceHandle>(handle);
    } catch (...) {
        return nullptr;
    }
}

void audioTransportSource_destroy(AudioTransportSourceHandle handle) {
    if (handle) {
        auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);

        // 停止播放
        if (internalHandle->transportSource) {
            internalHandle->transportSource->stop();
            internalHandle->transportSource->setSource(nullptr);
        }

        delete internalHandle;
    }
}

void audioTransportSource_prepareToPlay(AudioTransportSourceHandle handle, int samplesPerBlock, double sampleRate) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (internalHandle->transportSource) {
        internalHandle->transportSource->prepareToPlay(samplesPerBlock, sampleRate);
        std::cout << "[VSTBridge] AudioTransportSource prepared: " << samplesPerBlock << " samples, " << sampleRate << "Hz" << std::endl;
    }
}

void audioTransportSource_start(AudioTransportSourceHandle handle) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (internalHandle->transportSource) {
        internalHandle->transportSource->start();
    }
}

void audioTransportSource_stop(AudioTransportSourceHandle handle) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (internalHandle->transportSource) {
        internalHandle->transportSource->stop();
    }
}

void audioTransportSource_setPosition(AudioTransportSourceHandle handle, double position) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (internalHandle->transportSource) {
        internalHandle->transportSource->setPosition(position);
    }
}

double audioTransportSource_getCurrentPosition(AudioTransportSourceHandle handle) {
    if (!handle) return 0.0;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (!internalHandle->transportSource) return 0.0;

    return internalHandle->transportSource->getCurrentPosition();
}

bool audioTransportSource_isPlaying(AudioTransportSourceHandle handle) {
    if (!handle) return false;

    auto* internalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(handle);
    if (!internalHandle->transportSource) return false;

    return internalHandle->transportSource->isPlaying();
}

// 实时处理器扩展 API
bool realtimeProcessor_initialize(RealtimeProcessorHandle handle) {
    if (!handle) return false;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return false;

    try {
        return internalHandle->processor->initialize();
    } catch (...) {
        return false;
    }
}

double realtimeProcessor_getOutputLevel(RealtimeProcessorHandle handle) {
    if (!handle) return 0.0;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return 0.0;

    try {
        auto stats = internalHandle->processor->getStats();
        return stats.outputLevel;
    } catch (...) {
        return 0.0;
    }
}

void realtimeProcessor_setAudioTransportSource(RealtimeProcessorHandle handle, AudioTransportSourceHandle transportHandle) {
    if (!handle || !transportHandle) return;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    auto* transportInternalHandle = reinterpret_cast<AudioTransportSourceHandle_Internal*>(transportHandle);

    if (internalHandle->processor && transportInternalHandle->transportSource) {
        internalHandle->processor->setAudioTransportSource(transportInternalHandle->transportSource.get());
    }
}

void realtimeProcessor_clearAudioTransportSource(RealtimeProcessorHandle handle) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (internalHandle->processor) {
        internalHandle->processor->clearAudioTransportSource();
    }
}

double realtimeProcessor_getInputLevel(RealtimeProcessorHandle handle) {
    if (!handle) return 0.0;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return 0.0;

    try {
        auto stats = internalHandle->processor->getStats();
        return stats.inputLevel;
    } catch (...) {
        return 0.0;
    }
}

void realtimeProcessor_setAudioCallback(RealtimeProcessorHandle handle, RealtimeAudioCallback callback, void* userData) {
    if (!handle) return;

    auto* internalHandle = reinterpret_cast<RealtimeProcessorHandle_Internal*>(handle);
    if (!internalHandle->processor) return;

    try {
        // 设置音频回调
        internalHandle->processor->setAudioCallback([callback, userData](const juce::AudioBuffer<float>& buffer, bool isInput) {
            if (callback) {
                // 转换 JUCE AudioBuffer 为 float** 格式
                const int numChannels = buffer.getNumChannels();
                const int numSamples = buffer.getNumSamples();

                // 创建临时指针数组
                std::vector<float*> channelPointers(numChannels);
                for (int ch = 0; ch < numChannels; ++ch) {
                    channelPointers[ch] = const_cast<float*>(buffer.getReadPointer(ch));
                }

                callback(channelPointers.data(), numChannels, numSamples, isInput, userData);
            }
        });
    } catch (...) {
        // 忽略异常
    }
}


