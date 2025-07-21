# Project Structure & Organization

## Root Directory Layout

```
WindsynthRecorder/
├── JUCE/                           # Git submodule - JUCE framework
├── Libraries/                      # C++ audio processing libraries
├── WindsynthRecorder/              # Swift application source
├── WindsynthRecorder.xcodeproj/    # Xcode project files
├── Tests/                          # C++ test suite
├── build/                          # CMake build output
├── CMakeLists.txt                  # C++ build configuration
└── Documentation/                  # Project documentation
```

## Swift Application Structure (`WindsynthRecorder/`)

### Core Application
- `WindsynthRecorderApp.swift` - Main app entry point with multi-window setup
- `Info.plist` - App configuration and permissions
- `WindsynthRecorder.entitlements` - Security entitlements
- `WindsynthRecorder-Bridging-Header.h` - Swift-C++ bridge header

### Architecture Layers

#### Managers (`Managers/`)
- `AppDelegate.swift` - Application lifecycle management
- `WindowManager.swift` - Multi-window state management

#### Services (`Services/`)
- `VSTManagerExample.swift` - VST plugin management (singleton)
- `JUCEAudioEngine.swift` - Core audio processing engine
- `AudioMixerService.swift` - Real-time audio mixing
- `AudioRecorder.swift` - Audio recording functionality
- `AudioProcessor.swift` - Audio file processing
- `FFmpegManager.swift` - Audio format conversion
- `NotificationManager.swift` - System notifications

#### Views (`Views/`)
- `ContentView.swift` - Main application interface
- `VSTProcessorView.swift` - VST plugin browser and management
- `AudioMixerView.swift` - Professional mixing console
- `PluginParameterView.swift` - Plugin parameter controls
- `AudioProcessorView.swift` - Batch audio processing
- `StartupInitializationView.swift` - App initialization screen

#### Components (`Views/Components/`)
- Reusable UI components and custom controls

#### Windows (`Views/Windows/`)
- Dedicated window view controllers for multi-window interface

## C++ Libraries Structure (`Libraries/`)

### VST Support (`Libraries/VSTSupport/`)
Core JUCE-based audio processing library:
- `VSTPluginManager.hpp/.cpp` - VST plugin discovery and loading
- `AudioProcessingChain.hpp/.cpp` - Plugin chain management
- `RealtimeProcessor.hpp/.cpp` - Real-time audio processing
- `OfflineProcessor.hpp/.cpp` - Batch audio processing
- `AppConfig.h` - JUCE configuration

### Bridge Layer (`Libraries/Bridge/`)
Swift-C++ interoperability:
- `VSTBridge.h` - C interface declarations
- `VSTBridge.mm` - Objective-C++ implementation

## Build System Organization

### CMake Configuration
- `CMakeLists.txt` - Main build configuration
- `build/` - Generated build files and static library output
- `Makefile` - Convenience wrapper for CMake commands

### Xcode Integration
- Static library linking: `build/lib/libWindsynthVSTCore.a`
- Header search paths configured for C++ libraries
- Framework dependencies managed in project settings

## Naming Conventions

### Swift Code
- **Classes**: PascalCase (`VSTManagerExample`, `AudioMixerService`)
- **Properties**: camelCase (`isVSTProcessingEnabled`, `availablePlugins`)
- **Methods**: camelCase (`loadPlugin(named:)`, `scanForPlugins()`)
- **Files**: PascalCase matching primary class name

### C++ Code
- **Classes**: PascalCase (`VSTPluginManager`, `AudioProcessingChain`)
- **Methods**: camelCase (`loadPlugin`, `processBlock`)
- **Files**: PascalCase matching class name
- **C Interface**: snake_case (`vstPluginManager_create`)

### File Organization
- **Services**: Suffix with `Service` or `Manager`
- **Views**: Suffix with `View`
- **Models**: Plain descriptive names
- **Extensions**: `+Extension.swift` format

## Key Architectural Patterns

### Singleton Pattern
- `VSTManagerExample.shared` - Global VST state management
- `WindowManager.shared` - Window state coordination

### Observer Pattern
- `@Published` properties for reactive UI updates
- Combine publishers for cross-service communication

### Bridge Pattern
- C interface layer isolates Swift from C++ implementation details
- Opaque pointers manage C++ object lifetimes from Swift

### Multi-Window Pattern
- Dedicated window managers and view controllers
- Shared state through environment objects

## Development Guidelines

### File Placement
- New Swift services go in `Services/`
- New UI components go in `Views/Components/`
- C++ audio code goes in `Libraries/VSTSupport/`
- Bridge functions go in `Libraries/Bridge/`

### Dependency Flow
- UI depends on Services
- Services depend on Bridge layer
- Bridge layer depends on C++ libraries
- No circular dependencies between layers

### State Management
- Use `@StateObject` for service ownership
- Use `@ObservedObject` for service injection
- Use `@EnvironmentObject` for shared app state