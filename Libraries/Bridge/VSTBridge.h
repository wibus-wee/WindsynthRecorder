#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct VSTPluginManagerHandle VSTPluginManagerHandle;
typedef struct VSTPluginInstanceHandle VSTPluginInstanceHandle;
typedef struct AudioProcessingChainHandle AudioProcessingChainHandle;

// 插件信息结构（C兼容）
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

// 性能统计结构（C兼容）
typedef struct {
    double averageProcessingTime;
    double peakProcessingTime;
    double cpuUsagePercent;
    int bufferUnderruns;
} PerformanceStats_C;

// 处理链配置结构（C兼容）
typedef struct {
    double sampleRate;
    int samplesPerBlock;
    int numInputChannels;
    int numOutputChannels;
    bool enableMidi;
} ProcessingChainConfig_C;

// 回调函数类型定义
typedef void (*ScanProgressCallback)(const char* pluginName, float progress, void* userData);
typedef void (*ErrorCallback)(const char* error, void* userData);

// ============================================================================
// VSTPluginManager C接口
// ============================================================================

// 创建和销毁
VSTPluginManagerHandle* vstPluginManager_create(void);
void vstPluginManager_destroy(VSTPluginManagerHandle* handle);

// 插件扫描
void vstPluginManager_scanForPlugins(VSTPluginManagerHandle* handle);
void vstPluginManager_scanDirectory(VSTPluginManagerHandle* handle, const char* directoryPath);
void vstPluginManager_addPluginSearchPath(VSTPluginManagerHandle* handle, const char* path);

// 插件信息查询
int vstPluginManager_getNumAvailablePlugins(VSTPluginManagerHandle* handle);
bool vstPluginManager_getPluginInfo(VSTPluginManagerHandle* handle, int index, VSTPluginInfo_C* info);
int vstPluginManager_findPluginByName(VSTPluginManagerHandle* handle, const char* name);

// 插件加载
VSTPluginInstanceHandle* vstPluginManager_loadPlugin(VSTPluginManagerHandle* handle, const char* identifier);
VSTPluginInstanceHandle* vstPluginManager_loadPluginByIndex(VSTPluginManagerHandle* handle, int index);

// 状态查询
bool vstPluginManager_isScanning(VSTPluginManagerHandle* handle);

// 回调设置
void vstPluginManager_setScanProgressCallback(VSTPluginManagerHandle* handle, 
                                            ScanProgressCallback callback, void* userData);
void vstPluginManager_setErrorCallback(VSTPluginManagerHandle* handle, 
                                     ErrorCallback callback, void* userData);

// ============================================================================
// VSTPluginInstance C接口
// ============================================================================

// 销毁
void vstPluginInstance_destroy(VSTPluginInstanceHandle* handle);

// 基本信息
bool vstPluginInstance_isValid(VSTPluginInstanceHandle* handle);
const char* vstPluginInstance_getName(VSTPluginInstanceHandle* handle);

// 音频处理
void vstPluginInstance_prepareToPlay(VSTPluginInstanceHandle* handle, double sampleRate, int samplesPerBlock);
void vstPluginInstance_processBlock(VSTPluginInstanceHandle* handle, 
                                  float** audioBuffer, int numChannels, int numSamples,
                                  uint8_t* midiData, int midiDataSize);
void vstPluginInstance_releaseResources(VSTPluginInstanceHandle* handle);

// 参数控制
int vstPluginInstance_getNumParameters(VSTPluginInstanceHandle* handle);
float vstPluginInstance_getParameter(VSTPluginInstanceHandle* handle, int index);
void vstPluginInstance_setParameter(VSTPluginInstanceHandle* handle, int index, float value);
bool vstPluginInstance_getParameterName(VSTPluginInstanceHandle* handle, int index, char* name, int maxLength);
bool vstPluginInstance_getParameterText(VSTPluginInstanceHandle* handle, int index, char* text, int maxLength);

// 预设管理
int vstPluginInstance_getStateSize(VSTPluginInstanceHandle* handle);
bool vstPluginInstance_getState(VSTPluginInstanceHandle* handle, void* data, int maxSize);
bool vstPluginInstance_setState(VSTPluginInstanceHandle* handle, const void* data, int size);

// 编辑器
bool vstPluginInstance_hasEditor(VSTPluginInstanceHandle* handle);
void vstPluginInstance_showEditor(VSTPluginInstanceHandle* handle);
void vstPluginInstance_hideEditor(VSTPluginInstanceHandle* handle);

// ============================================================================
// AudioProcessingChain C接口
// ============================================================================

// 创建和销毁
AudioProcessingChainHandle* audioProcessingChain_create(void);
void audioProcessingChain_destroy(AudioProcessingChainHandle* handle);

// 配置
void audioProcessingChain_configure(AudioProcessingChainHandle* handle, const ProcessingChainConfig_C* config);
void audioProcessingChain_getConfig(AudioProcessingChainHandle* handle, ProcessingChainConfig_C* config);

// 音频处理生命周期
void audioProcessingChain_prepareToPlay(AudioProcessingChainHandle* handle, double sampleRate, int samplesPerBlock);
void audioProcessingChain_processBlock(AudioProcessingChainHandle* handle, 
                                     float** audioBuffer, int numChannels, int numSamples,
                                     uint8_t* midiData, int midiDataSize);
void audioProcessingChain_releaseResources(AudioProcessingChainHandle* handle);

// 插件管理
bool audioProcessingChain_addPlugin(AudioProcessingChainHandle* handle, VSTPluginInstanceHandle* plugin);
bool audioProcessingChain_insertPlugin(AudioProcessingChainHandle* handle, int index, VSTPluginInstanceHandle* plugin);
bool audioProcessingChain_removePlugin(AudioProcessingChainHandle* handle, int index);
bool audioProcessingChain_movePlugin(AudioProcessingChainHandle* handle, int fromIndex, int toIndex);
void audioProcessingChain_clearPlugins(AudioProcessingChainHandle* handle);

// 插件访问
int audioProcessingChain_getNumPlugins(AudioProcessingChainHandle* handle);
int audioProcessingChain_findPluginIndex(AudioProcessingChainHandle* handle, const char* pluginName);

// 旁路控制
void audioProcessingChain_setPluginBypassed(AudioProcessingChainHandle* handle, int index, bool bypassed);
bool audioProcessingChain_isPluginBypassed(AudioProcessingChainHandle* handle, int index);

// 全局控制
void audioProcessingChain_setEnabled(AudioProcessingChainHandle* handle, bool enabled);
bool audioProcessingChain_isEnabled(AudioProcessingChainHandle* handle);
void audioProcessingChain_setMasterBypass(AudioProcessingChainHandle* handle, bool bypass);
bool audioProcessingChain_isMasterBypassed(AudioProcessingChainHandle* handle);

// 性能监控
void audioProcessingChain_getPerformanceStats(AudioProcessingChainHandle* handle, PerformanceStats_C* stats);
void audioProcessingChain_resetPerformanceStats(AudioProcessingChainHandle* handle);

// 插件编辑器管理
bool audioProcessingChain_showPluginEditor(AudioProcessingChainHandle* handle, int index);
void audioProcessingChain_hidePluginEditor(AudioProcessingChainHandle* handle, int index);
bool audioProcessingChain_hasPluginEditor(AudioProcessingChainHandle* handle, int index);

// ============================================================================
// VSTAudioUnit C接口
// ============================================================================

typedef struct VSTAudioUnitHandle VSTAudioUnitHandle;

// 创建和销毁
VSTAudioUnitHandle* vstAudioUnit_create(void);
void vstAudioUnit_destroy(VSTAudioUnitHandle* handle);

// 初始化和配置
bool vstAudioUnit_initialize(VSTAudioUnitHandle* handle, double sampleRate, unsigned int framesPerSlice);
void vstAudioUnit_uninitialize(VSTAudioUnitHandle* handle);

// 音频处理
int vstAudioUnit_processAudio(VSTAudioUnitHandle* handle, float** audioData, unsigned int numChannels, unsigned int numFrames);

// 处理链管理
void vstAudioUnit_setProcessingChain(VSTAudioUnitHandle* handle, AudioProcessingChainHandle* chain);

// 状态管理
bool vstAudioUnit_isInitialized(VSTAudioUnitHandle* handle);
bool vstAudioUnit_isEnabled(VSTAudioUnitHandle* handle);
void vstAudioUnit_setEnabled(VSTAudioUnitHandle* handle, bool enabled);

// 延迟补偿
int audioProcessingChain_getTotalLatency(AudioProcessingChainHandle* handle);
void audioProcessingChain_setLatencyCompensation(AudioProcessingChainHandle* handle, bool enable);
bool audioProcessingChain_isLatencyCompensationEnabled(AudioProcessingChainHandle* handle);

// 回调设置
void audioProcessingChain_setErrorCallback(AudioProcessingChainHandle* handle,
                                         ErrorCallback callback, void* userData);

// ============================================================================
// 离线音频处理器 API
// ============================================================================

// 离线处理器句柄
typedef struct OfflineProcessorHandle OfflineProcessorHandle;

// 离线处理配置
typedef struct {
    double sampleRate;
    int bufferSize;
    int numChannels;
    bool normalizeOutput;
    double outputGain;
    bool enableDithering;
    int outputBitDepth;
} OfflineProcessingConfig_C;

// 处理任务状态
typedef enum {
    TASK_STATUS_PENDING = 0,
    TASK_STATUS_PROCESSING = 1,
    TASK_STATUS_COMPLETED = 2,
    TASK_STATUS_FAILED = 3,
    TASK_STATUS_CANCELLED = 4
} TaskStatus_C;

// 实时处理器类型定义
typedef void* RealtimeProcessorHandle;

// 实时处理器配置
typedef struct {
    double sampleRate;
    int bufferSize;
    int numInputChannels;
    int numOutputChannels;
    bool enableMonitoring;
    bool enableRecording;
    double monitoringGain;
    int latencyCompensationSamples;
} RealtimeProcessorConfig_C;

// 实时处理器统计信息
typedef struct {
    double inputLevel;
    double outputLevel;
    double averageLatency;
    double peakLatency;
    double cpuUsage;
} RealtimeStats_C;

// 音频路由模式
typedef enum {
    AudioRouting_DirectMonitoring = 0,
    AudioRouting_ProcessedMonitoring = 1,
    AudioRouting_SplitMonitoring = 2
} AudioRouting_C;

// 回调函数类型
typedef void (*RealtimeErrorCallback)(const char* error, void* userData);
typedef void (*RealtimeLevelCallback)(double inputLevel, double outputLevel, void* userData);
typedef void (*RealtimeAudioCallback)(float** audioData, int numChannels, int numSamples, bool isInput, void* userData);

// 实时处理器管理
RealtimeProcessorHandle realtimeProcessor_create();
void realtimeProcessor_destroy(RealtimeProcessorHandle handle);

// 配置
void realtimeProcessor_configure(RealtimeProcessorHandle handle, const RealtimeProcessorConfig_C* config);
void realtimeProcessor_getConfig(RealtimeProcessorHandle handle, RealtimeProcessorConfig_C* config);

// 音频设备管理
bool realtimeProcessor_initialize(RealtimeProcessorHandle handle);
bool realtimeProcessor_start(RealtimeProcessorHandle handle);
void realtimeProcessor_stop(RealtimeProcessorHandle handle);
bool realtimeProcessor_isRunning(RealtimeProcessorHandle handle);

// 插件链管理

// 监听控制
void realtimeProcessor_setMonitoringEnabled(RealtimeProcessorHandle handle, bool enabled);
bool realtimeProcessor_isMonitoringEnabled(RealtimeProcessorHandle handle);
void realtimeProcessor_setMonitoringGain(RealtimeProcessorHandle handle, double gain);
double realtimeProcessor_getMonitoringGain(RealtimeProcessorHandle handle);
void realtimeProcessor_setAudioRouting(RealtimeProcessorHandle handle, AudioRouting_C routing);
AudioRouting_C realtimeProcessor_getAudioRouting(RealtimeProcessorHandle handle);

// 录音控制
bool realtimeProcessor_startRecording(RealtimeProcessorHandle handle, const char* outputFilePath);
void realtimeProcessor_stopRecording(RealtimeProcessorHandle handle);
bool realtimeProcessor_isRecording(RealtimeProcessorHandle handle);

// 统计信息
void realtimeProcessor_getStats(RealtimeProcessorHandle handle, RealtimeStats_C* stats);
void realtimeProcessor_resetStats(RealtimeProcessorHandle handle);

// 回调设置
void realtimeProcessor_setErrorCallback(RealtimeProcessorHandle handle, RealtimeErrorCallback callback, void* userData);
void realtimeProcessor_setLevelCallback(RealtimeProcessorHandle handle, RealtimeLevelCallback callback, void* userData);
void realtimeProcessor_setAudioCallback(RealtimeProcessorHandle handle, RealtimeAudioCallback callback, void* userData);

// ============================================================================
// RealtimeProcessor 简化接口（最小化桥接）
// ============================================================================

// 实时处理器句柄
typedef void* RealtimeProcessorHandle;

// 简化的实时处理器管理
RealtimeProcessorHandle realtimeProcessor_create();
void realtimeProcessor_destroy(RealtimeProcessorHandle handle);
bool realtimeProcessor_start(RealtimeProcessorHandle handle);
void realtimeProcessor_stop(RealtimeProcessorHandle handle);
bool realtimeProcessor_isRunning(RealtimeProcessorHandle handle);
void realtimeProcessor_setProcessingChain(RealtimeProcessorHandle handle, AudioProcessingChainHandle* chainHandle);

// 处理进度回调
typedef void (*ProcessingProgressCallback)(const char* taskId, double progress, void* userData);
typedef void (*ProcessingCompletionCallback)(const char* taskId, bool success, const char* error, void* userData);

// 离线处理器管理
OfflineProcessorHandle* offlineProcessor_create();
void offlineProcessor_destroy(OfflineProcessorHandle* handle);

// 任务管理
const char* offlineProcessor_addTask(OfflineProcessorHandle* handle,
                                   const char* inputFilePath,
                                   const char* outputFilePath,
                                   const OfflineProcessingConfig_C* config,
                                   AudioProcessingChainHandle* processingChain);

bool offlineProcessor_removeTask(OfflineProcessorHandle* handle, const char* taskId);
void offlineProcessor_clearTasks(OfflineProcessorHandle* handle);

// 处理控制
void offlineProcessor_startProcessing(OfflineProcessorHandle* handle);
void offlineProcessor_stopProcessing(OfflineProcessorHandle* handle);
bool offlineProcessor_isProcessing(OfflineProcessorHandle* handle);

// 任务查询
TaskStatus_C offlineProcessor_getTaskStatus(OfflineProcessorHandle* handle, const char* taskId);
double offlineProcessor_getTaskProgress(OfflineProcessorHandle* handle, const char* taskId);
double offlineProcessor_getOverallProgress(OfflineProcessorHandle* handle);

// 回调设置
void offlineProcessor_setProgressCallback(OfflineProcessorHandle* handle,
                                        ProcessingProgressCallback callback, void* userData);
void offlineProcessor_setCompletionCallback(OfflineProcessorHandle* handle,
                                          ProcessingCompletionCallback callback, void* userData);

// ============================================================================
// JUCE 音频引擎桥接 API
// ============================================================================

// 音频文件读取器句柄
typedef void* AudioFileReaderHandle;

// 音频传输源句柄
typedef void* AudioTransportSourceHandle;

// 音频文件读取器管理
AudioFileReaderHandle audioFileReader_create(const char* filePath);
void audioFileReader_destroy(AudioFileReaderHandle handle);
double audioFileReader_getLengthInSeconds(AudioFileReaderHandle handle);
double audioFileReader_getSampleRate(AudioFileReaderHandle handle);
int audioFileReader_getNumChannels(AudioFileReaderHandle handle);

// 音频传输源管理
AudioTransportSourceHandle audioTransportSource_create(AudioFileReaderHandle reader);
void audioTransportSource_destroy(AudioTransportSourceHandle handle);
void audioTransportSource_prepareToPlay(AudioTransportSourceHandle handle, int samplesPerBlock, double sampleRate);
void audioTransportSource_start(AudioTransportSourceHandle handle);
void audioTransportSource_stop(AudioTransportSourceHandle handle);
void audioTransportSource_setPosition(AudioTransportSourceHandle handle, double position);
double audioTransportSource_getCurrentPosition(AudioTransportSourceHandle handle);
bool audioTransportSource_isPlaying(AudioTransportSourceHandle handle);

// 实时处理器扩展 API
bool realtimeProcessor_initialize(RealtimeProcessorHandle handle);
double realtimeProcessor_getOutputLevel(RealtimeProcessorHandle handle);
double realtimeProcessor_getInputLevel(RealtimeProcessorHandle handle);
void realtimeProcessor_setAudioCallback(RealtimeProcessorHandle handle, RealtimeAudioCallback callback, void* userData);
void realtimeProcessor_setAudioTransportSource(RealtimeProcessorHandle handle, AudioTransportSourceHandle transportHandle);
void realtimeProcessor_clearAudioTransportSource(RealtimeProcessorHandle handle);

// 音频处理链扩展 API
void audioProcessingChain_setEnabled(AudioProcessingChainHandle* handle, bool enabled);

#ifdef __cplusplus
}
#endif
