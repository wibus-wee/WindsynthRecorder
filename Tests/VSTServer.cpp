#include <JuceHeader.h>
#include "../Libraries/VSTSupport/VSTPluginManager.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>

using namespace WindsynthVST;

/**
 * VST 服务器程序
 * 作为独立进程运行，通过标准输入/输出与主应用通信
 */
class VSTServer {
private:
    std::unique_ptr<VSTPluginManager> pluginManager;
    bool running = false;
    
public:
    VSTServer() {
        pluginManager = std::make_unique<VSTPluginManager>();
        
        // 设置回调
        pluginManager->setScanProgressCallback([](const std::string& pluginName, float progress) {
            std::cout << "SCAN_PROGRESS:" << progress << ":" << pluginName << std::endl;
        });
        
        pluginManager->setErrorCallback([](const std::string& error) {
            std::cout << "ERROR:" << error << std::endl;
        });
    }
    
    void run() {
        running = true;
        std::cout << "VST_SERVER_READY" << std::endl;
        
        std::string line;
        while (running && std::getline(std::cin, line)) {
            processCommand(line);
        }
    }
    
    void processCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "SCAN") {
            handleScanCommand();
        } else if (cmd == "LIST") {
            handleListCommand();
        } else if (cmd == "LOAD") {
            std::string identifier;
            iss >> identifier;
            handleLoadCommand(identifier);
        } else if (cmd == "QUIT") {
            running = false;
            std::cout << "QUIT_OK" << std::endl;
        } else {
            std::cout << "ERROR:Unknown command: " << cmd << std::endl;
        }
    }
    
    void handleScanCommand() {
        std::cout << "SCAN_START" << std::endl;
        pluginManager->scanForPlugins();
        
        // 等待扫描完成
        while (pluginManager->isScanning()) {
            juce::Thread::sleep(100);
        }
        
        std::cout << "SCAN_COMPLETE:" << pluginManager->getAvailablePlugins().size() << std::endl;
    }
    
    void handleListCommand() {
        const auto& plugins = pluginManager->getAvailablePlugins();
        std::cout << "PLUGIN_LIST_START:" << plugins.size() << std::endl;
        
        for (size_t i = 0; i < plugins.size(); ++i) {
            const auto& plugin = plugins[i];
            std::cout << "PLUGIN:" << i << ":" << plugin.name << ":" 
                      << plugin.manufacturer << ":" << plugin.category << ":"
                      << plugin.fileOrIdentifier << std::endl;
        }
        
        std::cout << "PLUGIN_LIST_END" << std::endl;
    }
    
    void handleLoadCommand(const std::string& identifier) {
        std::cout << "LOAD_START:" << identifier << std::endl;
        
        pluginManager->loadPluginAsync(identifier, 
            [identifier](std::unique_ptr<VSTPluginInstance> instance, const std::string& error) {
                if (instance) {
                    std::cout << "LOAD_SUCCESS:" << identifier << ":" 
                              << instance->getName() << ":" 
                              << instance->getNumParameters() << ":"
                              << (instance->hasEditor() ? "1" : "0") << std::endl;
                } else {
                    std::cout << "LOAD_ERROR:" << identifier << ":" << error << std::endl;
                }
            });
    }
};

int main() {
    // 初始化 JUCE
    juce::initialiseJuce_GUI();
    
    try {
        VSTServer server;
        server.run();
    } catch (const std::exception& e) {
        std::cout << "FATAL_ERROR:" << e.what() << std::endl;
        return 1;
    }
    
    // 清理 JUCE
    juce::shutdownJuce_GUI();
    
    return 0;
}
