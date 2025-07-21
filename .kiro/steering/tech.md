# Technology Stack & Build System

## Primary Technologies

### Frontend
- **SwiftUI**: Modern declarative UI framework for macOS
- **Swift 5.0+**: Primary application language
- **Combine**: Reactive programming for state management

### Audio Processing
- **JUCE 8.0.8**: C++ audio framework for VST hosting and audio processing
- **AVFoundation**: macOS native audio framework
- **Core Audio**: Low-level audio system integration

### Build System
- **Xcode**: Primary IDE and build system
- **CMake**: C++ library build system for JUCE components
- **Swift Package Manager**: Dependency management (where applicable)

## Architecture Pattern

**Hybrid Swift/C++ Architecture**:
- Swift handles UI, application logic, and system integration
- C++ (JUCE) handles audio processing, VST hosting, and real-time audio
- C bridge layer (`VSTBridge.h/.mm`) connects Swift and C++

## Key Libraries & Frameworks

### Audio Processing
- `juce_audio_basics`: Core audio data structures
- `juce_audio_devices`: Audio device management
- `juce_audio_processors`: VST plugin hosting
- `juce_dsp`: Digital signal processing
- `AudioUnit.framework`: macOS audio units
- `CoreAudio.framework`: Low-level audio
- `CoreMIDI.framework`: MIDI support

### System Integration
- `Foundation.framework`: Core system services
- `Accelerate.framework`: High-performance computing

## Build Commands

### Initial Setup
```bash
# Initialize git submodules (JUCE)
git submodule update --init --recursive

# Build C++ static library
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target WindsynthVSTCore --config Release
```

### Development Workflow
```bash
# Rebuild C++ library after changes
make clean && make

# Build and run application
xcodebuild -project WindsynthRecorder.xcodeproj -scheme WindsynthRecorder build
# Or use Xcode GUI: Cmd+R
```

### Testing
```bash
# Run C++ tests
cd build && ctest

# Run Swift tests through Xcode
xcodebuild test -project WindsynthRecorder.xcodeproj -scheme WindsynthRecorder
```

## Configuration Requirements

### Xcode Project Settings
- **C++ Language Dialect**: C++17
- **Header Search Paths**: 
  - `$(SRCROOT)/Libraries/VSTSupport`
  - `$(SRCROOT)/Libraries/Bridge`
  - `$(SRCROOT)/JUCE/modules`
- **Library Search Paths**: `$(SRCROOT)/build/lib/Release`
- **Bridging Header**: `WindsynthRecorder-Bridging-Header.h`

### System Requirements
- **macOS**: 13.0+ (Ventura)
- **Xcode**: 14.0+
- **CMake**: 3.22+
- **Architecture**: Universal (Intel x64 + Apple Silicon ARM64)

## Development Notes

- Always rebuild C++ library before Xcode builds when C++ code changes
- VST plugins must be installed in standard macOS locations
- Audio processing requires proper entitlements for microphone access
- Use `@_silgen_name` for Swift-C++ function bridging