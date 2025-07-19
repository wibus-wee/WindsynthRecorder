#include <JuceHeader.h>
#include "../Libraries/VSTSupport/VSTPluginManager.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "WindsynthRecorder VST Test Application" << std::endl;
    std::cout << "=======================================" << std::endl;

    try {
        // 创建VST插件管理器
        std::cout << "创建VST插件管理器..." << std::endl;
        WindsynthVST::VSTPluginManager pluginManager;

        // 设置回调
        pluginManager.setScanProgressCallback([](const std::string& pluginName, float progress) {
            std::cout << "扫描进度: " << (progress * 100.0f) << "% - " << pluginName << std::endl;
        });

        pluginManager.setErrorCallback([](const std::string& error) {
            std::cout << "错误: " << error << std::endl;
        });

        std::cout << "VST插件管理器创建成功!" << std::endl;

        // 开始扫描插件
        std::cout << "\n开始扫描VST插件..." << std::endl;
        pluginManager.scanForPlugins();

        // 等待扫描完成
        while (pluginManager.isScanning()) {
            juce::Thread::sleep(100);
        }

        // 显示扫描结果
        int numPlugins = pluginManager.getNumAvailablePlugins();
        std::cout << "\n扫描完成! 找到 " << numPlugins << " 个插件:" << std::endl;

        std::vector<WindsynthVST::VSTPluginInfo> plugins = pluginManager.getAvailablePlugins();
        for (size_t i = 0; i < plugins.size() && i < 10; ++i) { // 只显示前10个
            const WindsynthVST::VSTPluginInfo& plugin = plugins[i];
            std::cout << (i + 1) << ". " << plugin.name
                     << " (" << plugin.manufacturer << ")"
                     << " - " << plugin.category << std::endl;
        }

        if (plugins.size() > 10) {
            std::cout << "... 还有 " << (plugins.size() - 10) << " 个插件" << std::endl;
        }

        // 专门测试iZotope插件加载
        std::cout << "\n开始测试iZotope插件加载..." << std::endl;

        if (!plugins.empty()) {
            // 寻找iZotope插件进行测试
            WindsynthVST::VSTPluginInfo testPlugin;
            bool foundIzotopePlugin = false;

            // 寻找iZotope插件，优先选择简单的
            for (size_t i = 0; i < plugins.size(); ++i) {
                const auto& plugin = plugins[i];
                if (plugin.manufacturer == "iZotope") {
                    // 优先选择Clipper或Gate等相对简单的插件
                    if (plugin.name.find("Clipper") != std::string::npos ||
                        plugin.name.find("Gate") != std::string::npos ||
                        plugin.name.find("Phase") != std::string::npos) {
                        testPlugin = plugin;
                        foundIzotopePlugin = true;
                        break;
                    }
                    // 如果没找到简单的，记录第一个iZotope插件
                    if (!foundIzotopePlugin) {
                        testPlugin = plugin;
                        foundIzotopePlugin = true;
                    }
                }
            }

            if (!foundIzotopePlugin) {
                std::cout << "未找到iZotope插件，使用第一个插件进行测试" << std::endl;
                testPlugin = plugins[0];
            }

            std::cout << "尝试异步加载iZotope插件: " << testPlugin.name
                     << " (" << testPlugin.pluginFormatName << ")" << std::endl;

            // 使用异步加载，这是处理复杂插件的正确方式
            bool loadingComplete = false;
            std::string loadResult;
            std::unique_ptr<WindsynthVST::VSTPluginInstance> loadedInstance;

            auto startTime = std::chrono::steady_clock::now();

            pluginManager.loadPluginAsync(testPlugin,
                [&](std::unique_ptr<WindsynthVST::VSTPluginInstance> instance, const std::string& error) {
                    auto endTime = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                    std::cout << "异步加载耗时: " << duration.count() << "ms" << std::endl;

                    if (instance) {
                        std::cout << "✅ iZotope插件异步加载成功!" << std::endl;
                        std::cout << "插件名称: " << instance->getName() << std::endl;
                        std::cout << "参数数量: " << instance->getNumParameters() << std::endl;
                        std::cout << "有编辑器: " << (instance->hasEditor() ? "是" : "否") << std::endl;

                        // 测试基本功能
                        if (instance->getNumParameters() > 0) {
                            std::cout << "第一个参数名称: " << instance->getParameterName(0) << std::endl;
                            std::cout << "第一个参数值: " << instance->getParameter(0) << std::endl;
                        }

                        loadedInstance = std::move(instance);
                        loadResult = "成功";
                    } else {
                        std::cout << "❌ iZotope插件异步加载失败: " << error << std::endl;
                        loadResult = "失败: " + error;
                    }

                    loadingComplete = true;
                });

            // 等待异步加载完成
            int waitCount = 0;
            const int maxWaitSeconds = 30; // 增加到30秒，因为iZotope插件可能需要更长时间
            while (!loadingComplete && waitCount < maxWaitSeconds * 10) {
                // 简单等待，异步回调会在后台处理
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                waitCount++;
                if (waitCount % 50 == 0) { // 每5秒报告一次
                    std::cout << "等待iZotope插件加载... (" << waitCount/10 << "秒)" << std::endl;
                }
            }

            if (!loadingComplete) {
                std::cout << "❌ iZotope插件加载超时!" << std::endl;
            } else {
                std::cout << "iZotope插件加载结果: " << loadResult << std::endl;
                if (loadedInstance) {
                    std::cout << "🎉 成功加载iZotope插件，可以进行音频处理!" << std::endl;
                }
            }
        } else {
            std::cout << "没有找到可用插件进行测试" << std::endl;
        }

        std::cout << "\niZotope插件异步加载测试完成!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
