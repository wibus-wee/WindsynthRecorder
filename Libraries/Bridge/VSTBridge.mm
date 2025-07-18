#include "VSTBridge.h"
#include "../VSTSupport/VSTPluginManager.hpp"
#include "../VSTSupport/AudioProcessingChain.hpp"
#include <memory>
#include <string>
#include <cstring>

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
    ScanProgressCallback scanProgressCallback = nullptr;
    ErrorCallback errorCallback = nullptr;
    void* scanProgressUserData = nullptr;
    void* errorUserData = nullptr;
};

struct VSTPluginInstanceHandle {
    std::unique_ptr<VSTPluginInstance> instance;
    juce::DocumentWindow* editorWindow = nullptr;
};

struct AudioProcessingChainHandle {
    std::unique_ptr<AudioProcessingChain> chain;
    ErrorCallback errorCallback = nullptr;
    void* errorUserData = nullptr;
};

// 辅助函数
static void copyString(const std::string& src, char* dest, size_t maxLength) {
    if (dest && maxLength > 0) {
        strncpy(dest, src.c_str(), maxLength - 1);
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
        auto handle = new VSTPluginManagerHandle();
        handle->manager = std::make_unique<VSTPluginManager>();
        
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
        std::string paramName = handle->instance->getParameterName(index);
        copyString(paramName, name, maxLength);
        return true;
    }
    return false;
}

bool vstPluginInstance_getParameterText(VSTPluginInstanceHandle* handle, int index, char* text, int maxLength) {
    if (handle && handle->instance && text && maxLength > 0) {
        std::string paramText = handle->instance->getParameterText(index);
        copyString(paramText, text, maxLength);
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
        // 创建编辑器窗口
        auto* editor = handle->instance->createEditor();
        if (editor) {
            // 创建一个简单的窗口来承载编辑器
            auto window = std::make_unique<juce::DocumentWindow>(
                handle->instance->getName() + " Editor",
                juce::Colours::lightgrey,
                juce::DocumentWindow::allButtons
            );

            window->setContentOwned(editor, true);
            window->setResizable(editor->isResizable(), false);
            window->centreWithSize(editor->getWidth(), editor->getHeight());
            window->setVisible(true);

            // 存储窗口引用（简化实现，实际应该管理窗口生命周期）
            handle->editorWindow = window.release();
        }
    }
}

void vstPluginInstance_hideEditor(VSTPluginInstanceHandle* handle) {
    if (handle && handle->editorWindow) {
        delete handle->editorWindow;
        handle->editorWindow = nullptr;
    }
}

// ============================================================================
// AudioProcessingChain C接口实现
// ============================================================================

AudioProcessingChainHandle* audioProcessingChain_create(void) {
    try {
        auto handle = new AudioProcessingChainHandle();
        handle->chain = std::make_unique<AudioProcessingChain>();

        // 设置错误回调
        handle->chain->setErrorCallback([handle](const std::string& error) {
            if (handle->errorCallback) {
                handle->errorCallback(error.c_str(), handle->errorUserData);
            }
        });

        return handle;
    } catch (...) {
        return nullptr;
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
        const auto& cppConfig = handle->chain->getConfig();
        config->sampleRate = cppConfig.sampleRate;
        config->samplesPerBlock = cppConfig.samplesPerBlock;
        config->numInputChannels = cppConfig.numInputChannels;
        config->numOutputChannels = cppConfig.numOutputChannels;
        config->enableMidi = cppConfig.enableMidi;
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
    if (!handle || !handle->chain || !audioBuffer) {
        return;
    }

    try {
        // 创建JUCE音频缓冲区
        juce::AudioBuffer<float> buffer(audioBuffer, numChannels, numSamples);

        // 创建MIDI缓冲区
        juce::MidiBuffer midiBuffer;
        if (midiData && midiDataSize > 0) {
            // TODO: 实现MIDI数据解析
        }

        handle->chain->processBlock(buffer, midiBuffer);
    } catch (...) {
        // 忽略异常
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
        std::unique_ptr<VSTPluginInstance> pluginPtr = std::move(plugin->instance);
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: Plugin ownership transferred, calling chain->addPlugin");

        bool success = handle->chain->addPlugin(std::move(pluginPtr));
        NSLog(@"[VSTBridge] audioProcessingChain_addPlugin: chain->addPlugin returned: %s", success ? "true" : "false");

        if (success) {
            // 清空插件句柄中的实例指针，因为所有权已经转移
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
        // 获取指定索引的节点
        auto* node = handle->chain->getNode(index);
        if (!node) {
            NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Node at index %d not found", index);
            return false;
        }

        // 获取插件实例
        auto* plugin = node->getPlugin();
        if (!plugin) {
            NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Plugin at index %d not found", index);
            return false;
        }

        // 检查插件是否有编辑器
        if (!plugin->hasEditor()) {
            NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Plugin at index %d has no editor", index);
            return false;
        }

        // 创建编辑器
        auto* editor = plugin->createEditor();
        if (!editor) {
            NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Failed to create editor for plugin at index %d", index);
            return false;
        }

        // 创建窗口来承载编辑器
        auto window = std::make_unique<juce::DocumentWindow>(
            plugin->getName() + " Editor",
            juce::Colours::lightgrey,
            juce::DocumentWindow::allButtons
        );

        window->setContentOwned(editor, true);
        window->setResizable(editor->isResizable(), false);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);

        // TODO: 需要管理窗口的生命周期，这里暂时泄漏窗口
        window.release();

        NSLog(@"[VSTBridge] audioProcessingChain_showPluginEditor: Successfully opened editor for plugin at index %d", index);
        return true;

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
    // TODO: 实现编辑器窗口的隐藏逻辑
    // 需要维护一个窗口映射表来管理编辑器窗口
}

bool audioProcessingChain_hasPluginEditor(AudioProcessingChainHandle* handle, int index) {
    if (!handle || !handle->chain) {
        return false;
    }

    try {
        auto* node = handle->chain->getNode(index);
        if (!node) return false;

        auto* plugin = node->getPlugin();
        return plugin ? plugin->hasEditor() : false;
    } catch (...) {
        return false;
    }
}
