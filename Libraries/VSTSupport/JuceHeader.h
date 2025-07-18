/*
    JUCE Header for WindsynthRecorder VST Support

    This file includes all necessary JUCE modules for VST plugin hosting.
*/

#pragma once

// 包含 JUCE 配置
#include "AppConfig.h"

// 包含必要的 JUCE 模块
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>

#if ! DONT_SET_USING_JUCE_NAMESPACE
 // 使用 JUCE 命名空间以简化代码
 using namespace juce;
#endif
