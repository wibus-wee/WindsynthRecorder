//
//  test_runner_main.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  简单的测试运行器主程序
//

#include <JuceHeader.h>
#include "Libraries/VSTSupport/AudioGraph/Tests/AudioGraphTests.hpp"
#include <iostream>

using namespace WindsynthVST::AudioGraph::Tests;

/**
 * 自定义的测试运行器，重写logMessage方法
 */
class CustomTestRunner : public juce::UnitTestRunner {
public:
    void logMessage(const juce::String& message) override {
        std::cout << message << std::endl;
    }
};

int main() {
    // 初始化JUCE
    juce::initialiseJuce_GUI();

    std::cout << "=== WindsynthVST AudioGraph 单元测试 ===" << std::endl;
    std::cout << "开始运行测试..." << std::endl;

    // 创建自定义测试运行器
    CustomTestRunner testRunner;

    try {
        // 运行所有AudioGraph相关的测试
        testRunner.runTestsInCategory("AudioGraph");

        // 输出结果摘要
        std::cout << "\n=== 测试结果摘要 ===" << std::endl;
        std::cout << "总测试数: " << testRunner.getNumResults() << std::endl;

        int passedTests = 0;
        int failedTests = 0;

        for (int i = 0; i < testRunner.getNumResults(); ++i) {
            auto* result = testRunner.getResult(i);
            if (result) {
                if (result->failures == 0) {
                    passedTests++;
                } else {
                    failedTests++;
                }
            }
        }

        std::cout << "通过: " << passedTests << std::endl;
        std::cout << "失败: " << failedTests << std::endl;

        double successRate = testRunner.getNumResults() > 0 ?
            (double(passedTests) / testRunner.getNumResults() * 100.0) : 0.0;
        std::cout << "成功率: " << std::fixed << std::setprecision(1)
                  << successRate << "%" << std::endl;

        // 清理测试文件
        AudioGraphTestUtils::cleanupTestFiles();

        // 清理JUCE
        juce::shutdownJuce_GUI();

        return failedTests == 0 ? 0 : 1;

    } catch (const std::exception& e) {
        std::cout << "测试运行时发生异常: " << e.what() << std::endl;
        juce::shutdownJuce_GUI();
        return 1;
    }
}
