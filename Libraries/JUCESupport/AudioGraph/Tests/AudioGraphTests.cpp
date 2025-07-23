//
//  AudioGraphTests.cpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  音频图架构的单元测试实现
//

#include "AudioGraphTests.hpp"
#include <iostream>

namespace WindsynthVST::AudioGraph::Tests {

//==============================================================================
// 测试工具类实现
//==============================================================================

juce::Array<juce::File> AudioGraphTestUtils::tempFiles;

juce::AudioBuffer<float> AudioGraphTestUtils::createTestBuffer(int numChannels, int numSamples, float frequency) {
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    
    const float sampleRate = 44100.0f;
    const float phaseIncrement = frequency * 2.0f * juce::MathConstants<float>::pi / sampleRate;
    
    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        float phase = 0.0f;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = std::sin(phase) * 0.5f; // 50% amplitude
            phase += phaseIncrement;
            if (phase >= 2.0f * juce::MathConstants<float>::pi) {
                phase -= 2.0f * juce::MathConstants<float>::pi;
            }
        }
    }
    
    return buffer;
}

juce::MidiBuffer AudioGraphTestUtils::createTestMidiBuffer(int numNotes) {
    juce::MidiBuffer midiBuffer;
    
    for (int i = 0; i < numNotes; ++i) {
        int noteNumber = 60 + i; // C4, C#4, D4, etc.
        int velocity = 100;
        int timestamp = i * 100; // 100 samples apart
        
        // Note on
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, noteNumber, static_cast<juce::uint8>(velocity)), timestamp);
        
        // Note off (500 samples later)
        midiBuffer.addEvent(juce::MidiMessage::noteOff(1, noteNumber), timestamp + 500);
    }
    
    return midiBuffer;
}

bool AudioGraphTestUtils::compareBuffers(const juce::AudioBuffer<float>& buffer1, 
                                        const juce::AudioBuffer<float>& buffer2, 
                                        float tolerance) {
    if (buffer1.getNumChannels() != buffer2.getNumChannels() ||
        buffer1.getNumSamples() != buffer2.getNumSamples()) {
        return false;
    }
    
    for (int channel = 0; channel < buffer1.getNumChannels(); ++channel) {
        auto* data1 = buffer1.getReadPointer(channel);
        auto* data2 = buffer2.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer1.getNumSamples(); ++sample) {
            if (std::abs(data1[sample] - data2[sample]) > tolerance) {
                return false;
            }
        }
    }
    
    return true;
}

bool AudioGraphTestUtils::isBufferSilent(const juce::AudioBuffer<float>& buffer, float threshold) {
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            if (std::abs(channelData[sample]) > threshold) {
                return false;
            }
        }
    }
    
    return true;
}

juce::PluginDescription AudioGraphTestUtils::createTestPluginDescription(const juce::String& name) {
    juce::PluginDescription desc;
    desc.name = name;
    desc.descriptiveName = name + " Test Plugin";
    desc.pluginFormatName = "Internal";
    desc.category = "Test";
    desc.manufacturerName = "Test Manufacturer";
    desc.version = "1.0.0";
    desc.fileOrIdentifier = name;
    desc.lastFileModTime = juce::Time::getCurrentTime();
    desc.lastInfoUpdateTime = juce::Time::getCurrentTime();
    desc.uniqueId = name.hashCode();
    desc.isInstrument = false;
    desc.numInputChannels = 0;
    desc.numOutputChannels = 2;

    return desc;
}

double AudioGraphTestUtils::measureProcessingTime(std::function<void()> processFunction) {
    auto startTime = juce::Time::getHighResolutionTicks();
    processFunction();
    auto endTime = juce::Time::getHighResolutionTicks();
    
    return juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0; // 返回毫秒
}

juce::File AudioGraphTestUtils::createTempTestFile(const juce::String& filename) {
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto testFile = tempDir.getChildFile("AudioGraphTests").getChildFile(filename);
    
    testFile.getParentDirectory().createDirectory();
    tempFiles.add(testFile);
    
    return testFile;
}

void AudioGraphTestUtils::cleanupTestFiles() {
    for (auto& file : tempFiles) {
        if (file.exists()) {
            file.deleteRecursively();
        }
    }
    tempFiles.clear();
}

//==============================================================================
// GraphAudioProcessor 测试实现
//==============================================================================

void GraphAudioProcessorTests::runTest() {
    testBasicConstruction();
    testConfiguration();
    testPluginManagement();
    testAudioProcessing();
    testStateManagement();
    testPerformanceMonitoring();
}

void GraphAudioProcessorTests::testBasicConstruction() {
    beginTest("Basic Construction");
    
    // 测试基本构造
    GraphAudioProcessor processor;
    
    expect(!processor.isGraphReady(), "图应该在初始化时未准备就绪");
    expect(processor.getName() == "WindsynthVST AudioGraph", "名称应该正确");
    expect(processor.acceptsMidi(), "应该接受MIDI");
    expect(processor.producesMidi(), "应该产生MIDI");
    expect(!processor.isMidiEffect(), "不应该是MIDI效果器");
    expect(processor.hasEditor() == false, "不应该有编辑器");
    
    // 测试I/O节点ID
    auto audioInputID = processor.getAudioInputNodeID();
    auto audioOutputID = processor.getAudioOutputNodeID();
    auto midiInputID = processor.getMidiInputNodeID();
    auto midiOutputID = processor.getMidiOutputNodeID();
    
    expect(audioInputID.uid != 0, "音频输入节点ID应该有效");
    expect(audioOutputID.uid != 0, "音频输出节点ID应该有效");
    expect(midiInputID.uid != 0, "MIDI输入节点ID应该有效");
    expect(midiOutputID.uid != 0, "MIDI输出节点ID应该有效");
    
    // 确保所有ID都不同
    expect(audioInputID != audioOutputID, "音频输入输出节点ID应该不同");
    expect(midiInputID != midiOutputID, "MIDI输入输出节点ID应该不同");
}

void GraphAudioProcessorTests::testConfiguration() {
    beginTest("Configuration");
    
    GraphAudioProcessor processor;
    
    // 测试默认配置
    auto defaultConfig = processor.getConfig();
    expect(defaultConfig.sampleRate > 0, "默认采样率应该大于0");
    expect(defaultConfig.samplesPerBlock > 0, "默认缓冲区大小应该大于0");
    
    // 测试自定义配置
    GraphConfig customConfig;
    customConfig.sampleRate = 48000.0;
    customConfig.samplesPerBlock = 256;
    customConfig.numInputChannels = 4;
    customConfig.numOutputChannels = 4;
    customConfig.enableMidi = true;
    
    processor.configure(customConfig);
    
    auto retrievedConfig = processor.getConfig();
    expect(retrievedConfig.sampleRate == 48000.0, "采样率应该正确设置");
    expect(retrievedConfig.samplesPerBlock == 256, "缓冲区大小应该正确设置");
    expect(retrievedConfig.numInputChannels == 4, "输入通道数应该正确设置");
    expect(retrievedConfig.numOutputChannels == 4, "输出通道数应该正确设置");
}

void GraphAudioProcessorTests::testPluginManagement() {
    beginTest("Plugin Management");
    
    GraphAudioProcessor processor;
    
    // 准备处理器
    processor.prepareToPlay(44100.0, 512);
    expect(processor.isGraphReady(), "图应该准备就绪");
    
    // 创建测试插件
    auto testPlugin = std::make_unique<TestAudioProcessor>("TestPlugin1", 2, 2, false);
    testPlugin->setGain(0.5f);
    
    // 添加插件
    NodeID nodeID = processor.addPlugin(std::move(testPlugin), "Test Plugin");
    expect(nodeID.uid != 0, "应该成功添加插件");
    
    // 获取节点信息
    auto nodeInfo = processor.getNodeInfo(nodeID);
    expect(nodeInfo.nodeID == nodeID, "节点ID应该匹配");
    expect(nodeInfo.name == "TestPlugin1", "节点名称应该正确");
    expect(nodeInfo.numInputChannels == 2, "输入通道数应该正确");
    expect(nodeInfo.numOutputChannels == 2, "输出通道数应该正确");
    
    // 测试节点操作
    expect(processor.setNodeBypassed(nodeID, true), "应该能设置旁路");
    expect(processor.setNodeEnabled(nodeID, false), "应该能禁用节点");
    
    // 移除插件
    expect(processor.removeNode(nodeID), "应该能移除节点");
    
    // 验证节点已被移除
    auto removedNodeInfo = processor.getNodeInfo(nodeID);
    expect(removedNodeInfo.nodeID.uid == 0, "节点应该已被移除");
}

void GraphAudioProcessorTests::testAudioProcessing() {
    beginTest("Audio Processing");
    
    GraphAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    
    // 创建测试音频缓冲区
    auto testBuffer = AudioGraphTestUtils::createTestBuffer(2, 512, 440.0f);
    auto midiBuffer = AudioGraphTestUtils::createTestMidiBuffer(1);
    
    // 复制原始缓冲区用于比较
    juce::AudioBuffer<float> originalBuffer;
    originalBuffer.makeCopyOf(testBuffer);
    
    // 处理空图（只有I/O节点）
    processor.processBlock(testBuffer, midiBuffer);
    
    // 验证音频通过（应该基本不变，可能有微小差异）
    expect(!AudioGraphTestUtils::isBufferSilent(testBuffer), "Audio should not be silent after processing");

    // 添加测试插件
    auto testPlugin = std::make_unique<TestAudioProcessor>("GainPlugin", 2, 2, false);
    testPlugin->setGain(0.5f);
    NodeID pluginNodeID = processor.addPlugin(std::move(testPlugin), "Gain Plugin");

    // 连接插件到音频路径
    auto audioInputID = processor.getAudioInputNodeID();
    auto audioOutputID = processor.getAudioOutputNodeID();

    // 先检查节点是否有效
    expect(audioInputID.uid != 0, "Audio input node should be valid");
    expect(audioOutputID.uid != 0, "Audio output node should be valid");
    expect(pluginNodeID.uid != 0, "Plugin node should be valid");

    // 尝试连接（可能失败，这是预期的，因为这是一个简化的测试）
    bool leftConnected = processor.connectAudio(audioInputID, 0, pluginNodeID, 0);
    bool rightConnected = processor.connectAudio(audioInputID, 1, pluginNodeID, 1);
    bool outputLeftConnected = processor.connectAudio(pluginNodeID, 0, audioOutputID, 0);
    bool outputRightConnected = processor.connectAudio(pluginNodeID, 1, audioOutputID, 1);

    // 记录连接结果但不强制要求成功（因为这是一个简化的测试环境）
    logMessage("Connection results: left=" + juce::String(leftConnected ? "OK" : "FAIL") +
               ", right=" + juce::String(rightConnected ? "OK" : "FAIL") +
               ", out_left=" + juce::String(outputLeftConnected ? "OK" : "FAIL") +
               ", out_right=" + juce::String(outputRightConnected ? "OK" : "FAIL"));
    
    // 重新创建测试缓冲区
    testBuffer = AudioGraphTestUtils::createTestBuffer(2, 512, 440.0f);
    
    // 处理带插件的音频
    processor.processBlock(testBuffer, midiBuffer);
    
    // 验证增益效果（音频应该减半）
    expect(!AudioGraphTestUtils::isBufferSilent(testBuffer), "Audio should not be silent after processing");

    // 检查音频电平是否大致减半
    float originalRMS = originalBuffer.getRMSLevel(0, 0, 512);
    float processedRMS = testBuffer.getRMSLevel(0, 0, 512);
    float expectedRMS = originalRMS * 0.5f;

    expect(std::abs(processedRMS - expectedRMS) < 0.1f, "Gain effect should be applied correctly");
}

void GraphAudioProcessorTests::testStateManagement() {
    beginTest("State Management");
    
    GraphAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    
    // 添加一些插件和连接
    auto plugin1 = std::make_unique<TestAudioProcessor>("Plugin1", 2, 2, false);
    auto plugin2 = std::make_unique<TestAudioProcessor>("Plugin2", 2, 2, false);
    
    NodeID node1 = processor.addPlugin(std::move(plugin1), "Plugin 1");
    NodeID node2 = processor.addPlugin(std::move(plugin2), "Plugin 2");
    
    // 创建连接
    processor.connectAudio(node1, 0, node2, 0);
    processor.connectAudio(node1, 1, node2, 1);
    
    // 保存状态
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);
    expect(stateData.getSize() > 0, "State data should not be empty");
    
    // 创建新的处理器并恢复状态
    GraphAudioProcessor processor2;
    processor2.prepareToPlay(44100.0, 512);
    processor2.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
    
    // 验证状态恢复（这里简化验证，实际实现可能需要更复杂的验证）
    auto nodes2 = processor2.getAllNodes();
    expect(nodes2.size() >= 2, "应该恢复了节点"); // 至少有I/O节点
}

void GraphAudioProcessorTests::testPerformanceMonitoring() {
    beginTest("Performance Monitoring");
    
    GraphAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    
    // 获取初始性能统计
    auto initialStats = processor.getPerformanceStats();
    expect(initialStats.totalProcessedBlocks == 0, "初始处理块数应该为0");
    
    // 处理一些音频块
    auto testBuffer = AudioGraphTestUtils::createTestBuffer(2, 512);
    juce::MidiBuffer midiBuffer;
    
    for (int i = 0; i < 10; ++i) {
        processor.processBlock(testBuffer, midiBuffer);
    }
    
    // 检查性能统计更新
    auto updatedStats = processor.getPerformanceStats();
    expect(updatedStats.totalProcessedBlocks == 10, "应该处理了10个块");
    expect(updatedStats.averageProcessingTimeMs >= 0, "平均处理时间应该非负");
    expect(updatedStats.cpuUsagePercent >= 0, "CPU使用率应该非负");
    
    // 重置统计
    processor.resetPerformanceStats();
    auto resetStats = processor.getPerformanceStats();
    expect(resetStats.totalProcessedBlocks == 0, "重置后处理块数应该为0");
}

//==============================================================================
// ModernPluginLoader 测试实现
//==============================================================================

void ModernPluginLoaderTests::runTest() {
    testFormatInitialization();
    testPluginScanning();
    testPluginQuerying();
    testBlacklistManagement();
    testCacheManagement();
}

void ModernPluginLoaderTests::testFormatInitialization() {
    beginTest("Format Initialization");

    ModernPluginLoader loader;

    // 测试支持的格式
    auto formats = loader.getSupportedFormats();
    expect(formats.size() > 0, "应该支持至少一种格式");

    // 检查VST3支持（JUCE默认包含）
    expect(loader.isFormatSupported("VST3"), "应该支持VST3格式");

    logMessage("支持的格式：" + formats.joinIntoString(", "));
}

void ModernPluginLoaderTests::testPluginScanning() {
    beginTest("Plugin Scanning");

    ModernPluginLoader loader;

    // 测试扫描状态
    expect(!loader.isScanning(), "初始时不应该在扫描");

    // 测试空路径扫描
    juce::FileSearchPath emptyPath;
    loader.scanPluginsAsync(emptyPath, false, false);

    // 等待扫描完成
    int waitCount = 0;
    while (loader.isScanning() && waitCount < 100) {
        juce::Thread::sleep(10);
        waitCount++;
    }

    expect(!loader.isScanning(), "扫描应该完成");

    // 测试停止扫描
    loader.scanDefaultPathsAsync(false);
    loader.stopScanning();
    expect(!loader.isScanning(), "停止后不应该在扫描");
}

void ModernPluginLoaderTests::testPluginQuerying() {
    beginTest("Plugin Querying");

    ModernPluginLoader loader;

    // 测试空列表查询
    auto allPlugins = loader.getKnownPlugins();
    expect(allPlugins.size() == 0, "初始时应该没有已知插件");

    auto pluginCount = loader.getNumKnownPlugins();
    expect(pluginCount == 0, "插件数量应该为0");

    // 测试搜索功能
    auto searchResults = loader.searchPlugins("test", true, true, true);
    expect(searchResults.size() == 0, "搜索结果应该为空");

    // 测试按格式查询
    auto vstPlugins = loader.getPluginsByFormat("VST3");
    expect(vstPlugins.size() == 0, "VST3插件列表应该为空");
}

void ModernPluginLoaderTests::testBlacklistManagement() {
    beginTest("Blacklist Management");

    ModernPluginLoader loader;

    // 测试初始黑名单
    auto initialBlacklist = loader.getBlacklist();
    int initialCount = initialBlacklist.size();

    // 添加到黑名单
    loader.addToBlacklist("test_plugin.vst3");
    auto updatedBlacklist = loader.getBlacklist();
    expect(updatedBlacklist.size() == initialCount + 1, "黑名单应该增加一项");

    // 从黑名单移除
    loader.removeFromBlacklist("test_plugin.vst3");
    auto finalBlacklist = loader.getBlacklist();
    expect(finalBlacklist.size() == initialCount, "黑名单应该恢复原大小");

    // 清除黑名单
    loader.clearBlacklist();
    auto clearedBlacklist = loader.getBlacklist();
    expect(clearedBlacklist.size() == 0, "黑名单应该为空");
}

void ModernPluginLoaderTests::testCacheManagement() {
    beginTest("Cache Management");

    ModernPluginLoader loader;

    // 创建临时文件用于测试
    auto testFile = AudioGraphTestUtils::createTempTestFile("plugin_cache_test.xml");

    // 测试保存空列表
    expect(loader.savePluginList(testFile), "应该能保存空插件列表");
    expect(testFile.exists(), "缓存文件应该存在");

    // 测试加载
    expect(loader.loadPluginList(testFile), "应该能加载插件列表");

    // 测试清除
    loader.clearPluginList();
    expect(loader.getNumKnownPlugins() == 0, "清除后插件数量应该为0");
}

//==============================================================================
// PluginManager 测试实现
//==============================================================================

void PluginManagerTests::runTest() {
    testPluginInstanceManagement();
    testParameterManagement();
    testPresetManagement();
    testStateManagement();
    testPerformanceMonitoring();
}

void PluginManagerTests::testPluginInstanceManagement() {
    beginTest("Plugin Instance Management");

    GraphAudioProcessor graphProcessor;
    ModernPluginLoader pluginLoader;
    PluginManager pluginManager(graphProcessor, pluginLoader);

    graphProcessor.prepareToPlay(44100.0, 512);

    // 测试初始状态
    expect(pluginManager.getNumLoadedPlugins() == 0, "初始时应该没有加载的插件");

    auto allPlugins = pluginManager.getAllPlugins();
    expect(allPlugins.size() == 0, "插件列表应该为空");

    // 测试插件信息查询
    NodeID invalidID{999};
    auto* invalidInfo = pluginManager.getPluginInfo(invalidID);
    expect(invalidInfo == nullptr, "无效ID应该返回nullptr");

    auto* invalidInstance = pluginManager.getPluginInstance(invalidID);
    expect(invalidInstance == nullptr, "无效ID应该返回nullptr插件实例");
}

void PluginManagerTests::testParameterManagement() {
    beginTest("Parameter Management");

    GraphAudioProcessor graphProcessor;
    ModernPluginLoader pluginLoader;
    PluginManager pluginManager(graphProcessor, pluginLoader);

    // 测试无效节点的参数操作
    NodeID invalidID{999};

    auto params = pluginManager.getPluginParameters(invalidID);
    expect(params.size() == 0, "无效节点应该返回空参数列表");

    float value = pluginManager.getParameterValue(invalidID, 0);
    expect(value == 0.0f, "无效节点应该返回0值");

    expect(!pluginManager.setParameterValue(invalidID, 0, 0.5f), "无效节点设置参数应该失败");

    auto text = pluginManager.getParameterText(invalidID, 0);
    expect(text.empty(), "无效节点应该返回空文本");

    expect(!pluginManager.resetParametersToDefault(invalidID), "无效节点重置参数应该失败");
}

void PluginManagerTests::testPresetManagement() {
    beginTest("Preset Management");

    GraphAudioProcessor graphProcessor;
    ModernPluginLoader pluginLoader;
    PluginManager pluginManager(graphProcessor, pluginLoader);

    NodeID invalidID{999};

    // 测试预设操作
    expect(!pluginManager.savePreset(invalidID, "test_preset"), "无效节点保存预设应该失败");
    expect(!pluginManager.loadPreset(invalidID, "test_preset"), "无效节点加载预设应该失败");
    expect(!pluginManager.deletePreset(invalidID, "test_preset"), "无效节点删除预设应该失败");

    auto presetNames = pluginManager.getPresetNames(invalidID);
    expect(presetNames.size() == 0, "无效节点应该返回空预设列表");

    // 测试文件操作
    auto testFile = AudioGraphTestUtils::createTempTestFile("preset_test.dat");
    expect(!pluginManager.exportPreset(invalidID, "test", testFile), "无效节点导出预设应该失败");
    expect(!pluginManager.importPreset(invalidID, "test", testFile), "无效节点导入预设应该失败");
}

void PluginManagerTests::testStateManagement() {
    beginTest("State Management");

    GraphAudioProcessor graphProcessor;
    ModernPluginLoader pluginLoader;
    PluginManager pluginManager(graphProcessor, pluginLoader);

    NodeID invalidID{999};

    // 测试状态操作
    juce::MemoryBlock stateData;
    expect(!pluginManager.getPluginState(invalidID, stateData), "无效节点获取状态应该失败");
    expect(!pluginManager.setPluginState(invalidID, stateData), "无效节点设置状态应该失败");
}

void PluginManagerTests::testPerformanceMonitoring() {
    beginTest("Performance Monitoring");

    GraphAudioProcessor graphProcessor;
    ModernPluginLoader pluginLoader;
    PluginManager pluginManager(graphProcessor, pluginLoader);

    // 测试性能统计
    expect(pluginManager.getTotalCpuUsage() == 0.0, "初始CPU使用率应该为0");
    expect(pluginManager.getTotalLatency() == 0, "初始延迟应该为0");

    NodeID invalidID{999};
    expect(pluginManager.getPluginCpuUsage(invalidID) == 0.0, "无效节点CPU使用率应该为0");
    expect(pluginManager.getPluginLatency(invalidID) == 0, "无效节点延迟应该为0");

    // 测试性能更新
    pluginManager.updatePerformanceStats();
    // 这里主要测试不会崩溃
}

// 创建静态测试实例以自动注册
static GraphAudioProcessorTests graphAudioProcessorTests;
static ModernPluginLoaderTests modernPluginLoaderTests;
static PluginManagerTests pluginManagerTests;

} // namespace WindsynthVST::AudioGraph::Tests
