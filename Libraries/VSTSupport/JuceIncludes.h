//
//  JuceIncludes.h
//  WindsynthRecorder VST Support
//
//  自定义 JUCE 头文件包含
//

#ifndef JuceIncludes_h
#define JuceIncludes_h

// 包含 JUCE 配置
#include "AppConfig.h"

// 核心 JUCE 模块
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_events/juce_events.h"
#include "../../JUCE/modules/juce_data_structures/juce_data_structures.h"

// 音频相关模块
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../../JUCE/modules/juce_audio_formats/juce_audio_formats.h"
#include "../../JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "../../JUCE/modules/juce_audio_utils/juce_audio_utils.h"

// GUI 模块（如果需要插件编辑器）
#include "../../JUCE/modules/juce_graphics/juce_graphics.h"
#include "../../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../../JUCE/modules/juce_gui_extra/juce_gui_extra.h"

// DSP 模块
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"

// 使用 JUCE 命名空间
using namespace juce;

#endif /* JuceIncludes_h */
