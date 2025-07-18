#include <JuceHeader.h>
#include "../Libraries/VSTSupport/VSTPluginManager.hpp"
#include <iostream>
#include <string>

using namespace WindsynthVST;

//==============================================================================
class IzotopePluginTestApp : public juce::JUCEApplicationBase
{
public:
    //==============================================================================
    IzotopePluginTestApp() {}

    const juce::String getApplicationName() override       { return "IzotopePluginTest"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& /*commandLine*/) override
    {
        std::cout << "iZotope Plugin Test Application" << std::endl;
        std::cout << "===============================" << std::endl;

        // 启动测试
        startTest();
    }

    void shutdown() override
    {
        // 清理资源
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void suspended() override
    {
        // 应用程序被挂起时调用
    }

    void resumed() override
    {
        // 应用程序恢复时调用
    }

    void unhandledException(const std::exception* e, const juce::String& sourceFilename, int lineNumber) override
    {
        std::cout << "未处理的异常: " << (e ? e->what() : "未知错误")
                  << " 在 " << sourceFilename << ":" << lineNumber << std::endl;
        quit();
    }

    //==============================================================================
    void anotherInstanceStarted (const juce::String& /*commandLine*/) override
    {
        // 当另一个实例启动时调用
    }

private:
    std::unique_ptr<VSTPluginManager> pluginManager;
    bool testCompleted = false;

    void startTest()
    {
        try {
            // 创建VST插件管理器
            std::cout << "创建VST插件管理器..." << std::endl;
            pluginManager = std::make_unique<VSTPluginManager>();

            // 设置回调
            pluginManager->setScanProgressCallback([](const std::string& pluginName, float progress) {
                std::cout << "扫描进度: " << (progress * 100.0f) << "% - " << pluginName << std::endl;
            });

            pluginManager->setErrorCallback([](const std::string& error) {
                std::cout << "错误: " << error << std::endl;
            });

            std::cout << "VST插件管理器创建成功!" << std::endl;

            // 开始扫描插件
            std::cout << "\n开始扫描VST插件..." << std::endl;
            pluginManager->scanForPlugins();

            // 等待扫描完成
            juce::Timer::callAfterDelay(1000, [this]() {
                checkScanProgress();
            });

        } catch (const std::exception& e) {
            std::cout << "❌ 初始化失败: " << e.what() << std::endl;
            quit();
        }
    }

    void checkScanProgress()
    {
        if (pluginManager->isScanning()) {
            // 继续等待
            juce::Timer::callAfterDelay(1000, [this]() {
                checkScanProgress();
            });
        } else {
            // 扫描完成，开始测试
            testIzotopePlugins();
        }
    }

    void testIzotopePlugins()
    {
        std::cout << "\n扫描完成!" << std::endl;

        auto plugins = pluginManager->getAvailablePlugins();
        std::cout << "找到 " << plugins.size() << " 个插件:" << std::endl;

        // 显示前10个插件
        for (size_t i = 0; i < plugins.size() && i < 10; ++i) {
            const auto& plugin = plugins[i];
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
            VSTPluginInfo testPlugin;
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
            auto startTime = juce::Time::getMillisecondCounter();
            
            pluginManager->loadPluginAsync(testPlugin, 
                [this, startTime](std::unique_ptr<VSTPluginInstance> instance, const std::string& error) {
                    auto endTime = juce::Time::getMillisecondCounter();
                    std::cout << "异步加载耗时: " << (endTime - startTime) << "ms" << std::endl;
                    
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
                        
                        std::cout << "🎉 成功加载iZotope插件，可以进行音频处理!" << std::endl;
                    } else {
                        std::cout << "❌ iZotope插件异步加载失败: " << error << std::endl;
                    }
                    
                    // 测试完成
                    completeTest();
                });
            
            // 设置超时
            juce::Timer::callAfterDelay(30000, [this]() {
                if (!testCompleted) {
                    std::cout << "❌ iZotope插件加载超时!" << std::endl;
                    completeTest();
                }
            });

        } else {
            std::cout << "没有找到可用插件进行测试" << std::endl;
            completeTest();
        }
    }

    void completeTest()
    {
        if (testCompleted) return;
        testCompleted = true;
        
        std::cout << "\niZotope插件异步加载测试完成!" << std::endl;
        
        // 延迟退出，让用户看到结果
        juce::Timer::callAfterDelay(2000, []() {
            juce::JUCEApplicationBase::quit();
        });
    }
};

//==============================================================================
// 这个宏生成应用程序的main()函数
START_JUCE_APPLICATION (IzotopePluginTestApp)
