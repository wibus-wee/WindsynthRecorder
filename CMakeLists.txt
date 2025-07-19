cmake_minimum_required(VERSION 3.22)

# 设置项目名称和版本
project(WindsynthRecorderVST VERSION 1.0.0)

# 设置 macOS 部署目标（与 Xcode 项目保持一致）
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加JUCE框架
add_subdirectory(JUCE)

# ============================================================================
# 创建 VST 音频处理静态库（用于 Swift 项目集成）
# ============================================================================
# 创建一个控制台应用，然后将其转换为静态库
juce_add_console_app(WindsynthVSTCore_App
    PRODUCT_NAME "WindsynthVSTCore"
)

# 添加源文件
target_sources(WindsynthVSTCore_App PRIVATE
    Libraries/VSTSupport/VSTPluginManager.cpp
    Libraries/VSTSupport/AudioProcessingChain.cpp
    Libraries/VSTSupport/RealtimeProcessor.cpp
    Libraries/VSTSupport/OfflineProcessor.cpp
    Libraries/Bridge/VSTBridge.mm
    Tests/VSTLibraryMain.cpp
)

# 设置头文件搜索路径
target_include_directories(WindsynthVSTCore_App PUBLIC
    Libraries/VSTSupport
    Libraries/Bridge
)

# 链接JUCE模块（注意：不包含GUI模块，避免与Swift GUI冲突）
target_link_libraries(WindsynthVSTCore_App PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    # 注意：不链接 juce_graphics 和 juce_gui_basics 避免GUI冲突
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# 设置编译定义
target_compile_definitions(WindsynthVSTCore_App PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0  # 完全禁用 AudioUnit 插件格式
    JUCE_MODAL_LOOPS_PERMITTED=0  # 重要：禁用模态循环，避免与Swift GUI冲突
)

# 设置macOS特定配置
if(APPLE)
    target_compile_definitions(WindsynthVSTCore_App PUBLIC
        JUCE_MAC=1
    )

    # 链接macOS框架
    target_link_libraries(WindsynthVSTCore_App PUBLIC
        "-framework AudioUnit"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework Foundation"
        "-framework Accelerate"
        "-lc++"  # 链接 C++ 标准库
    )
endif()

# 生成JUCE头文件
juce_generate_juce_header(WindsynthVSTCore_App)

# 创建真正的静态库目标
add_library(WindsynthVSTCore STATIC
    Libraries/VSTSupport/VSTPluginManager.cpp
    Libraries/VSTSupport/AudioProcessingChain.cpp
    Libraries/VSTSupport/RealtimeProcessor.cpp
    Libraries/VSTSupport/OfflineProcessor.cpp
    Libraries/Bridge/VSTBridge.mm
)

# 从 JUCE 应用目标复制配置到静态库
target_include_directories(WindsynthVSTCore PUBLIC
    $<TARGET_PROPERTY:WindsynthVSTCore_App,INCLUDE_DIRECTORIES>
)

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

target_compile_definitions(WindsynthVSTCore PUBLIC
    $<TARGET_PROPERTY:WindsynthVSTCore_App,COMPILE_DEFINITIONS>
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
        "-lc++"  # 链接 C++ 标准库
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
# 测试应用程序（可选，用于调试）
# ============================================================================
# 创建VST测试应用程序
juce_add_console_app(VSTTest
    PRODUCT_NAME "VST Test Application")

# 添加源文件
target_sources(VSTTest PRIVATE
    Tests/VSTTest.cpp
)

# 链接我们的静态库
target_link_libraries(VSTTest PRIVATE
    WindsynthVSTCore
)

# iZotope插件测试程序（带完整JUCE应用框架）
juce_add_gui_app(IzotopePluginTest
    PRODUCT_NAME "iZotope Plugin Test")

# 添加源文件
target_sources(IzotopePluginTest PRIVATE
    Tests/IzotopePluginTest.cpp
)

# 链接我们的静态库和GUI模块
target_link_libraries(IzotopePluginTest PRIVATE
    WindsynthVSTCore
    juce::juce_graphics
    juce::juce_gui_basics
)

# 设置macOS特定配置
if(APPLE)
    # 链接额外的GUI框架
    target_link_libraries(IzotopePluginTest PRIVATE
        "-framework Cocoa"
        "-framework Carbon"
        "-framework QuartzCore"
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
    install(DIRECTORY Libraries/VSTSupport/
        DESTINATION include/VSTSupport
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )

    install(DIRECTORY Libraries/Bridge/
        DESTINATION include/Bridge
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )

    # 创建一个脚本来帮助集成到Xcode项目
    # configure_file(
    #     "${CMAKE_SOURCE_DIR}/scripts/integrate_to_xcode.sh.in"
    #     "${CMAKE_BINARY_DIR}/integrate_to_xcode.sh"
    #     @ONLY
    # )
endif()
