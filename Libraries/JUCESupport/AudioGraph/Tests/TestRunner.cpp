//
//  TestRunner.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图测试运行器
//

#include "AudioGraphTests.hpp"
#include <iostream>

using namespace WindsynthVST::AudioGraph::Tests;

/**
 * 简单的测试运行器
 * 运行所有注册的JUCE单元测试
 */
class AudioGraphTestRunner {
public:
    static int runAllTests() {
        std::cout << "=== WindsynthVST AudioGraph 单元测试 ===" << std::endl;
        std::cout << "开始运行测试..." << std::endl;
        
        // 获取所有注册的测试
        auto& testRunner = juce::UnitTestRunner::getInstance();
        
        // 设置测试结果监听器
        TestResultListener listener;
        testRunner.addListener(&listener);
        
        // 运行所有AudioGraph相关的测试
        testRunner.runTestsInCategory("AudioGraph");
        
        // 移除监听器
        testRunner.removeListener(&listener);
        
        // 输出结果摘要
        std::cout << "\n=== 测试结果摘要 ===" << std::endl;
        std::cout << "总测试数: " << listener.getTotalTests() << std::endl;
        std::cout << "通过: " << listener.getPassedTests() << std::endl;
        std::cout << "失败: " << listener.getFailedTests() << std::endl;
        std::cout << "成功率: " << std::fixed << std::setprecision(1) 
                  << listener.getSuccessRate() << "%" << std::endl;
        
        if (listener.getFailedTests() > 0) {
            std::cout << "\n失败的测试:" << std::endl;
            for (const auto& failure : listener.getFailures()) {
                std::cout << "- " << failure << std::endl;
            }
        }
        
        // 清理测试文件
        AudioGraphTestUtils::cleanupTestFiles();
        
        return listener.getFailedTests() == 0 ? 0 : 1;
    }
    
    static int runSpecificTest(const std::string& testName) {
        std::cout << "=== 运行特定测试: " << testName << " ===" << std::endl;
        
        auto& testRunner = juce::UnitTestRunner::getInstance();
        TestResultListener listener;
        testRunner.addListener(&listener);
        
        // 查找并运行特定测试
        bool testFound = false;
        for (auto* test : juce::UnitTest::getAllTests()) {
            if (test->getName().toStdString() == testName) {
                testRunner.runTest(test);
                testFound = true;
                break;
            }
        }
        
        testRunner.removeListener(&listener);
        
        if (!testFound) {
            std::cout << "错误: 找不到测试 '" << testName << "'" << std::endl;
            std::cout << "可用的测试:" << std::endl;
            for (auto* test : juce::UnitTest::getAllTests()) {
                if (test->getCategory() == "AudioGraph") {
                    std::cout << "- " << test->getName() << std::endl;
                }
            }
            return 1;
        }
        
        std::cout << "测试完成: " << (listener.getFailedTests() == 0 ? "通过" : "失败") << std::endl;
        
        AudioGraphTestUtils::cleanupTestFiles();
        
        return listener.getFailedTests() == 0 ? 0 : 1;
    }

private:
    /**
     * 测试结果监听器
     */
    class TestResultListener : public juce::UnitTestRunner::TestResultListener {
    public:
        void testStarted(const juce::String& testName) override {
            currentTestName = testName;
            std::cout << "运行测试: " << testName << "..." << std::endl;
        }
        
        void testFinished(const juce::String& testName, bool passed) override {
            totalTests++;
            if (passed) {
                passedTests++;
                std::cout << "✓ " << testName << " 通过" << std::endl;
            } else {
                failedTests++;
                failures.push_back(testName.toStdString());
                std::cout << "✗ " << testName << " 失败" << std::endl;
            }
        }
        
        void logMessage(const juce::String& message) override {
            std::cout << "  " << message << std::endl;
        }
        
        int getTotalTests() const { return totalTests; }
        int getPassedTests() const { return passedTests; }
        int getFailedTests() const { return failedTests; }
        double getSuccessRate() const { 
            return totalTests > 0 ? (double(passedTests) / totalTests * 100.0) : 0.0; 
        }
        const std::vector<std::string>& getFailures() const { return failures; }
        
    private:
        juce::String currentTestName;
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        std::vector<std::string> failures;
    };
};

/**
 * 性能基准测试运行器
 */
class BenchmarkRunner {
public:
    static void runBenchmarks() {
        std::cout << "\n=== 性能基准测试 ===" << std::endl;
        
        // 运行基本处理性能测试
        benchmarkBasicProcessing();
        
        // 运行内存使用测试
        benchmarkMemoryUsage();
        
        // 运行连接管理性能测试
        benchmarkConnectionManagement();
    }

private:
    static void benchmarkBasicProcessing() {
        std::cout << "\n--- 基本音频处理性能 ---" << std::endl;
        
        GraphAudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);
        
        auto testBuffer = AudioGraphTestUtils::createTestBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        // 预热
        for (int i = 0; i < 10; ++i) {
            processor.processBlock(testBuffer, midiBuffer);
        }
        
        // 测量处理时间
        const int numIterations = 1000;
        auto startTime = juce::Time::getHighResolutionTicks();
        
        for (int i = 0; i < numIterations; ++i) {
            processor.processBlock(testBuffer, midiBuffer);
        }
        
        auto endTime = juce::Time::getHighResolutionTicks();
        double totalTimeMs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
        double averageTimeMs = totalTimeMs / numIterations;
        
        std::cout << "处理 " << numIterations << " 个音频块:" << std::endl;
        std::cout << "总时间: " << std::fixed << std::setprecision(2) << totalTimeMs << " ms" << std::endl;
        std::cout << "平均时间: " << std::fixed << std::setprecision(4) << averageTimeMs << " ms/块" << std::endl;
        std::cout << "实时性能: " << std::fixed << std::setprecision(1) 
                  << (11.6 / averageTimeMs) << "x 实时" << std::endl; // 512 samples @ 44.1kHz = 11.6ms
    }
    
    static void benchmarkMemoryUsage() {
        std::cout << "\n--- 内存使用测试 ---" << std::endl;
        
        // 这里可以添加内存使用测量
        // 由于JUCE没有内置的内存测量工具，这里简化处理
        std::cout << "内存使用测试需要平台特定的实现" << std::endl;
    }
    
    static void benchmarkConnectionManagement() {
        std::cout << "\n--- 连接管理性能 ---" << std::endl;
        
        GraphAudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);
        
        // 添加多个测试插件
        std::vector<NodeID> nodeIDs;
        const int numPlugins = 10;
        
        auto startTime = juce::Time::getHighResolutionTicks();
        
        for (int i = 0; i < numPlugins; ++i) {
            auto plugin = std::make_unique<TestAudioProcessor>("Plugin" + juce::String(i), 2, 2);
            NodeID nodeID = processor.addPlugin(std::move(plugin));
            nodeIDs.push_back(nodeID);
        }
        
        auto endTime = juce::Time::getHighResolutionTicks();
        double addTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
        
        std::cout << "添加 " << numPlugins << " 个插件: " 
                  << std::fixed << std::setprecision(2) << addTime << " ms" << std::endl;
        
        // 测试连接创建
        startTime = juce::Time::getHighResolutionTicks();
        
        for (size_t i = 0; i < nodeIDs.size() - 1; ++i) {
            processor.connectAudio(nodeIDs[i], 0, nodeIDs[i + 1], 0);
            processor.connectAudio(nodeIDs[i], 1, nodeIDs[i + 1], 1);
        }
        
        endTime = juce::Time::getHighResolutionTicks();
        double connectTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
        
        std::cout << "创建 " << (numPlugins - 1) * 2 << " 个连接: " 
                  << std::fixed << std::setprecision(2) << connectTime << " ms" << std::endl;
    }
};

/**
 * 主函数 - 可以作为独立的测试程序运行
 */
int main(int argc, char* argv[]) {
    // 初始化JUCE
    juce::initialiseJuce_GUI();
    
    int result = 0;
    
    try {
        if (argc > 1) {
            std::string command = argv[1];
            
            if (command == "--test" && argc > 2) {
                // 运行特定测试
                result = AudioGraphTestRunner::runSpecificTest(argv[2]);
            } else if (command == "--benchmark") {
                // 运行性能基准测试
                BenchmarkRunner::runBenchmarks();
            } else if (command == "--help") {
                std::cout << "用法:" << std::endl;
                std::cout << "  " << argv[0] << "                    运行所有测试" << std::endl;
                std::cout << "  " << argv[0] << " --test <name>      运行特定测试" << std::endl;
                std::cout << "  " << argv[0] << " --benchmark        运行性能基准测试" << std::endl;
                std::cout << "  " << argv[0] << " --help             显示帮助信息" << std::endl;
            } else {
                std::cout << "未知命令: " << command << std::endl;
                std::cout << "使用 --help 查看可用命令" << std::endl;
                result = 1;
            }
        } else {
            // 默认运行所有测试
            result = AudioGraphTestRunner::runAllTests();
            
            // 如果测试通过，也运行基准测试
            if (result == 0) {
                BenchmarkRunner::runBenchmarks();
            }
        }
    } catch (const std::exception& e) {
        std::cout << "测试运行时发生异常: " << e.what() << std::endl;
        result = 1;
    }
    
    // 清理JUCE
    juce::shutdownJuce_GUI();
    
    return result;
}
