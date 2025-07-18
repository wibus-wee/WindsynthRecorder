//
//  WindsynthRecorder-Bridging-Header.h
//  WindsynthRecorder
//
//  简化的桥接头文件，只包含 C 接口声明
//

#ifndef WindsynthRecorder_Bridging_Header_h
#define WindsynthRecorder_Bridging_Header_h

#include <stdbool.h>

// 基本类型定义
typedef void* VSTPluginManagerHandle;
typedef void* AudioProcessingChainHandle;
typedef void* VSTPluginInstanceHandle;

// 插件信息结构
typedef struct {
    char name[256];
    char manufacturer[256];
    char version[64];
    char category[128];
    char pluginFormatName[64];
    char fileOrIdentifier[512];
    int numInputChannels;
    int numOutputChannels;
    bool isInstrument;
    bool acceptsMidi;
    bool producesMidi;
} VSTPluginInfo_C;

// 处理链配置
typedef struct {
    double sampleRate;
    int samplesPerBlock;
    int numInputChannels;
    int numOutputChannels;
    bool enableMidi;
} ProcessingChainConfig_C;

// 回调函数类型
typedef void (*ScanProgressCallback)(const char* pluginName, float progress, void* userData);
typedef void (*ErrorCallback)(const char* error, void* userData);

// 核心 API 声明
#ifdef __cplusplus
extern "C" {
#endif

// 插件管理器
VSTPluginManagerHandle vstPluginManager_create(void);
void vstPluginManager_destroy(VSTPluginManagerHandle handle);
void vstPluginManager_scanForPlugins(VSTPluginManagerHandle handle);
void vstPluginManager_addPluginSearchPath(VSTPluginManagerHandle handle, const char* path);
int vstPluginManager_getNumAvailablePlugins(VSTPluginManagerHandle handle);
bool vstPluginManager_getPluginInfo(VSTPluginManagerHandle handle, int index, VSTPluginInfo_C* info);
VSTPluginInstanceHandle vstPluginManager_loadPlugin(VSTPluginManagerHandle handle, const char* identifier);
void vstPluginManager_setScanProgressCallback(VSTPluginManagerHandle handle, ScanProgressCallback callback, void* userData);
void vstPluginManager_setErrorCallback(VSTPluginManagerHandle handle, ErrorCallback callback, void* userData);

// 音频处理链
AudioProcessingChainHandle audioProcessingChain_create(void);
void audioProcessingChain_destroy(AudioProcessingChainHandle handle);
void audioProcessingChain_configure(AudioProcessingChainHandle handle, const ProcessingChainConfig_C* config);
void audioProcessingChain_prepareToPlay(AudioProcessingChainHandle handle, double sampleRate, int samplesPerBlock);
bool audioProcessingChain_addPlugin(AudioProcessingChainHandle handle, VSTPluginInstanceHandle plugin);
bool audioProcessingChain_removePlugin(AudioProcessingChainHandle handle, int index);
void audioProcessingChain_clearPlugins(AudioProcessingChainHandle handle);

// 插件实例
void vstPluginInstance_destroy(VSTPluginInstanceHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* WindsynthRecorder_Bridging_Header_h */
