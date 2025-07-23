//
//  ModernPluginLoader.hpp
//  WindsynthRecorder
//
//  Created by AI Assistant
//  现代插件加载器，基于JUCE最佳实践
//

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <atomic>
#include <mutex>

namespace WindsynthVST::AudioGraph {

/**
 * 现代插件加载器 - 基于JUCE最佳实践
 *
 * 参考JUCE AudioPluginHost实现，提供企业级插件扫描和加载功能：
 * - 使用PluginDirectoryScanner进行高效扫描
 * - Dead Man's Pedal崩溃保护机制
 * - 多线程并行扫描优化
 * - VST3快速扫描支持
 * - 子进程隔离扫描（可选）
 * - 智能缓存和增量扫描
 */
class ModernPluginLoader {
public:
    //==============================================================================
    // 类型定义
    //==============================================================================
    
    /**
     * 插件加载完成回调
     * @param instance 成功时返回插件实例，失败时为nullptr
     * @param error 错误信息，成功时为空
     */
    using PluginLoadCallback = std::function<void(std::unique_ptr<juce::AudioPluginInstance> instance, 
                                                 const juce::String& error)>;
    
    /**
     * 扫描进度回调
     * @param progress 进度（0.0-1.0）
     * @param currentFile 当前正在扫描的文件
     */
    using ScanProgressCallback = std::function<void(float progress, const juce::String& currentFile)>;
    
    /**
     * 扫描完成回调
     * @param foundPlugins 找到的插件数量
     */
    using ScanCompleteCallback = std::function<void(int foundPlugins)>;
    
    //==============================================================================
    // 构造函数和析构函数
    //==============================================================================
    
    /**
     * 构造函数
     */
    ModernPluginLoader();
    
    /**
     * 析构函数
     */
    ~ModernPluginLoader();
    
    //==============================================================================
    // 插件格式管理
    //==============================================================================
    
    /**
     * 初始化插件格式（VST2/VST3/AU等）
     * @param enableVST2 是否启用VST2支持
     * @param enableVST3 是否启用VST3支持
     * @param enableAU 是否启用AU支持（仅macOS）
     */
    void initializeFormats(bool enableVST2 = true, bool enableVST3 = true, bool enableAU = true);
    
    /**
     * 获取支持的插件格式列表
     * @return 格式名称列表
     */
    juce::StringArray getSupportedFormats() const;
    
    /**
     * 检查是否支持指定格式
     * @param formatName 格式名称
     * @return 支持返回true
     */
    bool isFormatSupported(const juce::String& formatName) const;
    
    //==============================================================================
    // 插件扫描 - 基于JUCE最佳实践（简化版）
    //==============================================================================

    /**
     * 扫描默认路径（主要方法）
     * @param rescanExisting 是否重新扫描已知插件
     * @param numThreads 扫描线程数（0=自动检测）
     */
    void scanDefaultPathsAsync(bool rescanExisting = false, int numThreads = 0);

    /**
     * 扫描单个文件或目录（用于特殊情况）
     * @param fileOrDirectory 文件或目录路径
     * @param rescanExisting 是否重新扫描已知插件
     */
    void scanFileAsync(const juce::File& fileOrDirectory, bool rescanExisting = false);

    /**
     * 停止当前扫描
     */
    void stopScanning();

    /**
     * 检查是否正在扫描
     * @return 正在扫描返回true
     */
    bool isScanning() const;
    
    //==============================================================================
    // 插件查询
    //==============================================================================
    
    /**
     * 获取所有已知插件
     * @return 插件描述列表
     */
    juce::Array<juce::PluginDescription> getKnownPlugins() const;
    
    /**
     * 按类别获取插件
     * @param category 类别名称
     * @return 匹配的插件描述列表
     */
    juce::Array<juce::PluginDescription> getPluginsByCategory(const juce::String& category) const;
    
    /**
     * 按制造商获取插件
     * @param manufacturer 制造商名称
     * @return 匹配的插件描述列表
     */
    juce::Array<juce::PluginDescription> getPluginsByManufacturer(const juce::String& manufacturer) const;
    
    /**
     * 按格式获取插件
     * @param formatName 格式名称
     * @return 匹配的插件描述列表
     */
    juce::Array<juce::PluginDescription> getPluginsByFormat(const juce::String& formatName) const;
    
    /**
     * 搜索插件
     * @param searchText 搜索文本
     * @param searchInName 是否在名称中搜索
     * @param searchInManufacturer 是否在制造商中搜索
     * @param searchInCategory 是否在类别中搜索
     * @return 匹配的插件描述列表
     */
    juce::Array<juce::PluginDescription> searchPlugins(const juce::String& searchText,
                                                       bool searchInName = true,
                                                       bool searchInManufacturer = true,
                                                       bool searchInCategory = true) const;
    
    /**
     * 根据文件路径查找插件
     * @param fileOrIdentifier 文件路径或标识符
     * @return 匹配的插件描述，未找到时返回nullptr
     */
    const juce::PluginDescription* findPluginByFile(const juce::String& fileOrIdentifier) const;
    
    //==============================================================================
    // 插件加载
    //==============================================================================
    
    /**
     * 异步加载插件
     * @param description 插件描述
     * @param sampleRate 采样率
     * @param bufferSize 缓冲区大小
     * @param callback 加载完成回调
     */
    void loadPluginAsync(const juce::PluginDescription& description,
                        double sampleRate,
                        int bufferSize,
                        PluginLoadCallback callback);
    
    /**
     * 同步加载插件（可能阻塞）
     * @param description 插件描述
     * @param sampleRate 采样率
     * @param bufferSize 缓冲区大小
     * @param errorMessage 错误信息输出
     * @return 插件实例，失败时返回nullptr
     */
    std::unique_ptr<juce::AudioPluginInstance> loadPluginSync(const juce::PluginDescription& description,
                                                             double sampleRate,
                                                             int bufferSize,
                                                             juce::String& errorMessage);
    
    /**
     * 检查插件是否仍然存在
     * @param description 插件描述
     * @return 存在返回true
     */
    bool doesPluginStillExist(const juce::PluginDescription& description) const;
    
    //==============================================================================
    // 崩溃保护和黑名单管理 - Dead Man's Pedal
    //==============================================================================

    /**
     * 设置Dead Man's Pedal文件路径
     * @param file 崩溃检测文件路径
     */
    void setDeadMansPedalFile(const juce::File& file);

    /**
     * 获取Dead Man's Pedal文件
     * @return 崩溃检测文件
     */
    juce::File getDeadMansPedalFile() const;

    /**
     * 添加到黑名单
     * @param pluginId 插件ID
     */
    void addToBlacklist(const juce::String& pluginId);

    /**
     * 从黑名单移除
     * @param pluginId 插件ID
     */
    void removeFromBlacklist(const juce::String& pluginId);

    /**
     * 清除黑名单
     */
    void clearBlacklist();

    /**
     * 获取黑名单
     * @return 黑名单文件列表
     */
    const juce::StringArray& getBlacklist() const;
    
    //==============================================================================
    // 缓存管理
    //==============================================================================
    
    /**
     * 保存插件列表到文件
     * @param file 保存文件
     * @return 成功返回true
     */
    bool savePluginList(const juce::File& file) const;
    
    /**
     * 从文件加载插件列表
     * @param file 加载文件
     * @return 成功返回true
     */
    bool loadPluginList(const juce::File& file);
    
    /**
     * 清除插件列表
     */
    void clearPluginList();
    
    //==============================================================================
    // 回调设置
    //==============================================================================
    
    /**
     * 设置扫描进度回调
     */
    void setScanProgressCallback(ScanProgressCallback callback);
    
    /**
     * 设置扫描完成回调
     */
    void setScanCompleteCallback(ScanCompleteCallback callback);
    
    //==============================================================================
    // 统计信息
    //==============================================================================
    
    /**
     * 获取已知插件数量
     * @return 插件数量
     */
    int getNumKnownPlugins() const;
    
    /**
     * 获取按格式分组的插件数量
     * @return 格式名称到数量的映射
     */
    std::map<juce::String, int> getPluginCountByFormat() const;

private:
    //==============================================================================
    // 内部成员变量
    //==============================================================================

    // JUCE核心组件
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    // 扫描器和线程管理
    std::unique_ptr<juce::PluginDirectoryScanner> currentScanner;
    std::unique_ptr<juce::ThreadPool> scanningThreadPool;
    std::atomic<bool> scanning{false};
    std::atomic<bool> shouldStopScanning{false};

    // Dead Man's Pedal崩溃保护
    juce::File deadMansPedalFile;

    // 回调函数
    ScanProgressCallback progressCallback;
    ScanCompleteCallback completeCallback;

    // 线程安全
    mutable std::mutex listMutex;
    mutable std::mutex scannerMutex;

    //==============================================================================
    // 内部扫描作业类
    //==============================================================================

    class ScanJob;
    friend class ScanJob;

    //==============================================================================
    // 内部方法
    //==============================================================================

    void performScanWithDirectoryScanner(juce::AudioPluginFormat& format,
                                        const juce::FileSearchPath& paths,
                                        bool recursive,
                                        bool rescanExisting,
                                        int numThreads);

    void performLegacyScan(const juce::FileSearchPath& paths, bool recursive, bool rescanExisting);
    void notifyProgress(float progress, const juce::String& currentFile);
    void notifyComplete(int foundPlugins);

    // 默认搜索路径
    juce::FileSearchPath getDefaultSearchPaths() const;

    // 获取推荐的线程数
    int getRecommendedThreadCount() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernPluginLoader)
};

} // namespace WindsynthVST::AudioGraph
