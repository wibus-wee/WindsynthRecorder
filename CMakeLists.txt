cmake_minimum_required(VERSION 3.22)

# 设置项目名称和版本
project(WindsynthRecorderVST VERSION 1.0.0)

# 设置 macOS 部署目标（与 Xcode 项目保持一致）
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

# 设置 macOS 架构为 universal binary（支持 arm64 和 x86_64）
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加JUCE框架
add_subdirectory(JUCE)

# ============================================================================
# 创建 VST 音频处理静态库（用于 Swift 项目集成）
# ============================================================================

# 创建一个临时的控制台应用来生成JUCE头文件
juce_add_console_app(WindsynthVSTCore_Temp
    PRODUCT_NAME "WindsynthVSTCore_Temp"
)

# 添加一个临时的main文件
file(WRITE "${CMAKE_BINARY_DIR}/temp_main.cpp" "int main() { return 0; }")
target_sources(WindsynthVSTCore_Temp PRIVATE "${CMAKE_BINARY_DIR}/temp_main.cpp")

# 链接JUCE模块到临时应用
target_link_libraries(WindsynthVSTCore_Temp PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
)

# 生成JUCE头文件
juce_generate_juce_header(WindsynthVSTCore_Temp)

# 创建静态库目标
add_library(WindsynthVSTCore STATIC
    # 新的AudioGraph架构
    Libraries/JUCESupport/AudioGraph/Core/GraphAudioProcessor.cpp
    Libraries/JUCESupport/AudioGraph/Plugins/ModernPluginLoader.cpp
    Libraries/JUCESupport/AudioGraph/Plugins/PluginManager.cpp
    Libraries/JUCESupport/AudioGraph/Management/GraphManager.cpp
    Libraries/JUCESupport/AudioGraph/Management/AudioIOManager.cpp
    Libraries/JUCESupport/AudioGraph/Management/PresetManager.cpp

    # 新的引擎门面和桥接层
    Libraries/JUCESupport/WindsynthEngineFacade.cpp
    Libraries/JUCESupport/AudioGraphBridge.mm
)

# 确保静态库在临时应用生成头文件后构建
add_dependencies(WindsynthVSTCore WindsynthVSTCore_Temp)

# 设置头文件搜索路径
target_include_directories(WindsynthVSTCore PUBLIC
    Libraries/JUCESupport/AudioGraph
    Libraries/JUCESupport
    $<TARGET_PROPERTY:WindsynthVSTCore_Temp,JUCE_GENERATED_SOURCES_DIRECTORY>
)

# 链接JUCE模块
target_link_libraries(WindsynthVSTCore PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# 设置编译定义
target_compile_definitions(WindsynthVSTCore PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0
    JUCE_MODAL_LOOPS_PERMITTED=0
)

# 为静态库添加 macOS 特定配置
if(APPLE)
    target_link_libraries(WindsynthVSTCore PUBLIC
        "-framework AudioUnit"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework Foundation"
        "-framework Accelerate"
    )
endif()

# 设置静态库输出路径
if(APPLE)
    set_target_properties(WindsynthVSTCore PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
    )
endif()



# ============================================================================
# 安装配置（将静态库和头文件复制到指定位置）
# ============================================================================
if(APPLE)
    # 安装静态库
    install(TARGETS WindsynthVSTCore
        ARCHIVE DESTINATION lib
    )

    # 安装头文件
    install(DIRECTORY Libraries/JUCESupport/
        DESTINATION include/JUCESupport
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()
