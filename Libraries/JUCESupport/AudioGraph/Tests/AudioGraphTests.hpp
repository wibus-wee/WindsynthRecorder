//
//  AudioGraphTests.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图架构的单元测试
//

#pragma once

#include <JuceHeader.h>
#include "../Core/GraphAudioProcessor.hpp"
#include "../Plugins/ModernPluginLoader.hpp"
#include "../Plugins/PluginManager.hpp"
#include "../Management/GraphManager.hpp"
#include "../Management/AudioIOManager.hpp"
#include "../Management/PresetManager.hpp"

namespace WindsynthVST::AudioGraph::Tests {

//==============================================================================
// 测试用的简单音频处理器
//==============================================================================

/**
 * 简单的测试音频处理器
 * 用于测试音频图功能，不依赖外部插件
 */
class TestAudioProcessor : public juce::AudioPluginInstance {
public:
    TestAudioProcessor(const juce::String& name = "TestProcessor",
                      int numInputs = 2, int numOutputs = 2, bool acceptsMidi = false)
        : AudioPluginInstance(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::canonicalChannelSet(numInputs), true)
                        .withOutput("Output", juce::AudioChannelSet::canonicalChannelSet(numOutputs), true)),
          processorName(name), acceptsMidiInput(acceptsMidi) {}
    
    const juce::String getName() const override { return processorName; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        processCallCount++;
        // 简单的增益处理
        buffer.applyGain(gain);
    }
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&) override {
        processCallCount++;
        buffer.applyGain(static_cast<double>(gain));
    }
    
    bool acceptsMidi() const override { return acceptsMidiInput; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock& destData) override {
        juce::MemoryOutputStream stream(destData, true);
        stream.writeFloat(gain);
    }
    
    void setStateInformation(const void* data, int sizeInBytes) override {
        juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
        gain = stream.readFloat();
    }

    // AudioPluginInstance 特有的方法
    void fillInPluginDescription(juce::PluginDescription& description) const override {
        description.name = processorName;
        description.descriptiveName = processorName + " Test Plugin";
        description.pluginFormatName = "Internal";
        description.category = "Test";
        description.manufacturerName = "Test Manufacturer";
        description.version = "1.0.0";
        description.fileOrIdentifier = processorName;
        description.lastFileModTime = juce::Time::getCurrentTime();
        description.lastInfoUpdateTime = juce::Time::getCurrentTime();
        description.uniqueId = processorName.hashCode();
        description.isInstrument = false;
        description.numInputChannels = getTotalNumInputChannels();
        description.numOutputChannels = getTotalNumOutputChannels();
    }

    // 测试辅助方法
    void setGain(float newGain) { gain = newGain; }
    float getGain() const { return gain; }
    int getProcessCallCount() const { return processCallCount; }
    void resetProcessCallCount() { processCallCount = 0; }
    
private:
    juce::String processorName;
    bool acceptsMidiInput;
    float gain = 1.0f;
    std::atomic<int> processCallCount{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessor)
};

//==============================================================================
// GraphAudioProcessor 测试
//==============================================================================

class GraphAudioProcessorTests : public juce::UnitTest {
public:
    GraphAudioProcessorTests() : juce::UnitTest("GraphAudioProcessor", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testBasicConstruction();
    void testConfiguration();
    void testPluginManagement();
    void testAudioProcessing();
    void testStateManagement();
    void testPerformanceMonitoring();
};

//==============================================================================
// ModernPluginLoader 测试
//==============================================================================

class ModernPluginLoaderTests : public juce::UnitTest {
public:
    ModernPluginLoaderTests() : juce::UnitTest("ModernPluginLoader", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testFormatInitialization();
    void testPluginScanning();
    void testPluginQuerying();
    void testBlacklistManagement();
    void testCacheManagement();
};

//==============================================================================
// PluginManager 测试
//==============================================================================

class PluginManagerTests : public juce::UnitTest {
public:
    PluginManagerTests() : juce::UnitTest("PluginManager", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testPluginInstanceManagement();
    void testParameterManagement();
    void testPresetManagement();
    void testStateManagement();
    void testPerformanceMonitoring();
};

//==============================================================================
// GraphManager 测试
//==============================================================================

class GraphManagerTests : public juce::UnitTest {
public:
    GraphManagerTests() : juce::UnitTest("GraphManager", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testNodeManagement();
    void testConnectionManagement();
    void testGraphValidation();
    void testGraphAnalysis();
    void testUndoRedo();
    void testBatchOperations();
};

//==============================================================================
// AudioIOManager 测试
//==============================================================================

class AudioIOManagerTests : public juce::UnitTest {
public:
    AudioIOManagerTests() : juce::UnitTest("AudioIOManager", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testIOConfiguration();
    void testChannelMapping();
    void testSmartConnections();
    void testLevelMonitoring();
    void testAudioControls();
};

//==============================================================================
// PresetManager 测试
//==============================================================================

class PresetManagerTests : public juce::UnitTest {
public:
    PresetManagerTests() : juce::UnitTest("PresetManager", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testPresetManagement();
    void testCategoryManagement();
    void testStateCapture();
    void testSearchAndQuery();
    void testFileOperations();
    void testAutoBackup();
};

//==============================================================================
// 集成测试
//==============================================================================

class AudioGraphIntegrationTests : public juce::UnitTest {
public:
    AudioGraphIntegrationTests() : juce::UnitTest("AudioGraph Integration", "AudioGraph") {}
    
    void runTest() override;
    
private:
    void testCompleteWorkflow();
    void testPerformanceComparison();
    void testMemoryUsage();
    void testThreadSafety();
    void testErrorHandling();
};

//==============================================================================
// 测试工具类
//==============================================================================

class AudioGraphTestUtils {
public:
    /**
     * 创建测试音频缓冲区
     */
    static juce::AudioBuffer<float> createTestBuffer(int numChannels, int numSamples, float frequency = 440.0f);
    
    /**
     * 创建测试MIDI缓冲区
     */
    static juce::MidiBuffer createTestMidiBuffer(int numNotes = 1);
    
    /**
     * 比较两个音频缓冲区
     */
    static bool compareBuffers(const juce::AudioBuffer<float>& buffer1, 
                              const juce::AudioBuffer<float>& buffer2, 
                              float tolerance = 0.001f);
    
    /**
     * 验证音频缓冲区不为空
     */
    static bool isBufferSilent(const juce::AudioBuffer<float>& buffer, float threshold = 0.0001f);
    
    /**
     * 创建测试插件描述
     */
    static juce::PluginDescription createTestPluginDescription(const juce::String& name);
    
    /**
     * 测量处理时间
     */
    static double measureProcessingTime(std::function<void()> processFunction);
    
    /**
     * 创建临时测试文件
     */
    static juce::File createTempTestFile(const juce::String& filename);
    
    /**
     * 清理测试文件
     */
    static void cleanupTestFiles();
    
private:
    static juce::Array<juce::File> tempFiles;
};

//==============================================================================
// 性能基准测试
//==============================================================================

class AudioGraphBenchmarks : public juce::UnitTest {
public:
    AudioGraphBenchmarks() : juce::UnitTest("AudioGraph Benchmarks", "Performance") {}
    
    void runTest() override;
    
private:
    void benchmarkBasicProcessing();
    void benchmarkPluginLoading();
    void benchmarkConnectionManagement();
    void benchmarkStateOperations();
    void benchmarkMemoryUsage();
    
    struct BenchmarkResult {
        juce::String testName;
        double averageTimeMs;
        double minTimeMs;
        double maxTimeMs;
        size_t memoryUsageBytes;
    };
    
    void logBenchmarkResult(const BenchmarkResult& result);
};

} // namespace WindsynthVST::AudioGraph::Tests
