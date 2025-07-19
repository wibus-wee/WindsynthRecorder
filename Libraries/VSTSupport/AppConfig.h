//
//  AppConfig.h
//  WindsynthRecorder VST Support
//
//  JUCE 应用配置文件
//

#pragma once

// 定义 JUCE 应用配置
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

// 平台定义
#if defined(__APPLE__)
    #define JUCE_MAC 1
    #define JUCE_IOS 0
#elif defined(_WIN32)
    #define JUCE_WINDOWS 1
#elif defined(__linux__)
    #define JUCE_LINUX 1
#endif

// 核心模块配置
#define JUCE_MODULE_AVAILABLE_juce_core 1
#define JUCE_MODULE_AVAILABLE_juce_events 1
#define JUCE_MODULE_AVAILABLE_juce_data_structures 1

// 音频模块配置
#define JUCE_MODULE_AVAILABLE_juce_audio_basics 1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices 1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats 1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors 1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils 1

// GUI 模块配置
#define JUCE_MODULE_AVAILABLE_juce_graphics 1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics 1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra 1

// DSP 模块配置
#define JUCE_MODULE_AVAILABLE_juce_dsp 1

// 音频插件配置
#define JUCE_PLUGINHOST_VST3 1
#define JUCE_PLUGINHOST_AU 0  // 禁用 AudioUnit 避免 GUI 冲突

// 禁用不需要的功能
#define JUCE_USE_CURL 0
#define JUCE_WEB_BROWSER 0
#define JUCE_USE_CAMERA 0

// 音频设备配置
#define JUCE_ASIO 0
#define JUCE_WASAPI 1
#define JUCE_DIRECTSOUND 1
#define JUCE_ALSA 1
#define JUCE_JACK 0
#define JUCE_BELA 0
#define JUCE_USE_ANDROID_OBOE 0
#define JUCE_USE_ANDROID_OPENSLES 0

// 音频格式配置
#define JUCE_USE_FLAC 1
#define JUCE_USE_OGGVORBIS 1
#define JUCE_USE_MP3AUDIOFORMAT 1
#define JUCE_USE_LAME_AUDIO_FORMAT 0
#define JUCE_USE_WINDOWS_MEDIA_FORMAT 1

// VST 配置
#define JUCE_PLUGINHOST_VST 0  // 禁用 VST2
#define JUCE_PLUGINHOST_VST3 1 // 启用 VST3
#define JUCE_PLUGINHOST_AU 0   // 禁用 AudioUnit 避免 GUI 冲突

// 其他配置
#define JUCE_DISPLAY_SPLASH_SCREEN 0
#define JUCE_REPORT_APP_USAGE 0
#define JUCE_USE_DARK_SPLASH_SCREEN 1

// 调试配置
#ifdef DEBUG
    #define JUCE_DEBUG 1
    #define JUCE_LOG_ASSERTIONS 1
#else
    #define JUCE_DEBUG 0
    #define JUCE_LOG_ASSERTIONS 0
#endif

// 内存管理
#define JUCE_CHECK_MEMORY_LEAKS 1
#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES 0

// 字符串编码
#define JUCE_STRING_UTF_TYPE 8

// 线程配置
#define JUCE_USE_WINRT_MIDI 0

// 网络配置
#define JUCE_USE_CURL 0
#define JUCE_LOAD_CURL_SYMBOLS_LAZILY 0

// OpenGL 配置
#define JUCE_OPENGL 0
#define JUCE_USE_OPENGL_SHADERS 0

// 应用程序配置
#define JucePlugin_Build_VST 0
#define JucePlugin_Build_VST3 0
#define JucePlugin_Build_AU 0
#define JucePlugin_Build_AUv3 0
#define JucePlugin_Build_RTAS 0
#define JucePlugin_Build_AAX 0
#define JucePlugin_Build_Standalone 1
#define JucePlugin_Build_Unity 0
