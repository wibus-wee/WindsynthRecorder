import Foundation
import AVFAudio
import CoreAudio

class AudioDeviceManager: ObservableObject {
    static let shared = AudioDeviceManager()
    
    @Published var availableInputDevices: [AudioDevice] = []
    @Published var availableOutputDevices: [AudioDevice] = []
    @Published var currentInputDevice: AudioDevice?
    @Published var currentOutputDevice: AudioDevice?
    
    private let srBlueDeviceName = "SR_EWI-0964"
    private let srRecDeviceName = "SR-REC"
    
    struct AudioDevice: Identifiable, Equatable {
        let id: AudioDeviceID
        let name: String
        let isInput: Bool
        let isOutput: Bool
        
        static func == (lhs: AudioDevice, rhs: AudioDevice) -> Bool {
            return lhs.id == rhs.id
        }
    }
    
    private init() {
        refreshDeviceList()
        // 打印所有设备名称以便调试
        // printAllDevices()
    }
    
    private func printAllDevices() {
        print("=== 所有输出设备 ===")
        for device in availableOutputDevices {
            print("输出设备: \(device.name)")
        }
        
        print("\n=== 所有输入设备 ===")
        for device in availableInputDevices {
            print("输入设备: \(device.name)")
        }
        
        print("\n=== 当前设备 ===")
        print("当前输出设备: \(currentOutputDevice?.name ?? "无")")
        print("当前输入设备: \(currentInputDevice?.name ?? "无")")
    }
    
    func refreshDeviceList() {
        var propertySize: UInt32 = 0
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDevices,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        
        var status = AudioObjectGetPropertyDataSize(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            &propertySize)
        
        let deviceCount = Int(propertySize) / MemoryLayout<AudioDeviceID>.size
        var deviceIDs = [AudioDeviceID](repeating: 0, count: deviceCount)
        
        status = AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            &propertySize,
            &deviceIDs)
        
        availableInputDevices.removeAll()
        availableOutputDevices.removeAll()
        
        for deviceID in deviceIDs {
            if let device = getDeviceInfo(deviceID: deviceID) {
                if device.isInput {
                    availableInputDevices.append(device)
                }
                if device.isOutput {
                    availableOutputDevices.append(device)
                }
            }
        }
        
        updateCurrentDevices()
        // 每次刷新后打印设备列表
        // printAllDevices()
    }
    
    private func getDeviceInfo(deviceID: AudioDeviceID) -> AudioDevice? {
        var propertySize = UInt32(256)
        var deviceName = [UInt8](repeating: 0, count: 256)
        
        var property = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyDeviceName,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        
        let status = AudioObjectGetPropertyData(
            deviceID,
            &property,
            0,
            nil,
            &propertySize,
            &deviceName)
        
        if status != kAudioHardwareNoError {
            return nil
        }
        
        let name = String(bytes: deviceName.prefix(while: { $0 != 0 }), encoding: .utf8) ?? ""
        
        // 检查设备是否支持输入
        var inputAddress = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyStreamConfiguration,
            mScope: kAudioDevicePropertyScopeInput,
            mElement: kAudioObjectPropertyElementMain)
        
        // 检查设备是否支持输出
        var outputAddress = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyStreamConfiguration,
            mScope: kAudioDevicePropertyScopeOutput,
            mElement: kAudioObjectPropertyElementMain)
        
        let supportsInput = AudioObjectHasProperty(deviceID, &inputAddress)
        let supportsOutput = AudioObjectHasProperty(deviceID, &outputAddress)
        
        // 如果设备名称是 SR_EWI-0964，强制设置为支持输出
        if name == srBlueDeviceName {
            return AudioDevice(id: deviceID, name: name, isInput: supportsInput, isOutput: true)
        }
        
        return AudioDevice(id: deviceID, name: name, isInput: supportsInput, isOutput: supportsOutput)
    }
    
    private func updateCurrentDevices() {
        if let defaultOutputDevice = getDefaultOutputDevice() {
            currentOutputDevice = availableOutputDevices.first { $0.id == defaultOutputDevice }
        }
        
        if let defaultInputDevice = getDefaultInputDevice() {
            currentInputDevice = availableInputDevices.first { $0.id == defaultInputDevice }
        }
    }
    
    private func getDefaultOutputDevice() -> AudioDeviceID? {
        var deviceID: AudioDeviceID = 0
        var propertySize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        
        let status = AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            &propertySize,
            &deviceID)
        
        return status == kAudioHardwareNoError ? deviceID : nil
    }
    
    private func getDefaultInputDevice() -> AudioDeviceID? {
        var deviceID: AudioDeviceID = 0
        var propertySize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultInputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        
        let status = AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            &propertySize,
            &deviceID)
        
        return status == kAudioHardwareNoError ? deviceID : nil
    }
    
    func setDefaultOutputDevice(deviceID: AudioDeviceID) -> Bool {
        var propertySize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var mutableDeviceID = deviceID
        
        let status = AudioObjectSetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            propertySize,
            &mutableDeviceID)
        
        if status == kAudioHardwareNoError {
            refreshDeviceList()
            return true
        }
        return false
    }
    
    func setDefaultInputDevice(deviceID: AudioDeviceID) -> Bool {
        var propertySize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultInputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var mutableDeviceID = deviceID
        
        let status = AudioObjectSetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            propertySize,
            &mutableDeviceID)
        
        if status == kAudioHardwareNoError {
            refreshDeviceList()
            return true
        }
        return false
    }
    
    func checkAndSetupSRBlueDevice() -> (success: Bool, message: String) {
        AudioProcessingLogger.shared.info("检查 SR Blue 设备", details: "查找设备: \(srBlueDeviceName)")

        // 首先在输入设备中查找 SR_EWI-0964
        if let srBlueDevice = availableInputDevices.first(where: { $0.name == srBlueDeviceName }) {
            AudioProcessingLogger.shared.info("找到 SR Blue 设备", details: "设备ID: \(srBlueDevice.id), 名称: \(srBlueDevice.name)")

            if currentOutputDevice?.id == srBlueDevice.id {
                AudioProcessingLogger.shared.info("SR Blue 设备状态", details: "\(srBlueDevice.name) 已经是当前输出设备")
                return (true, "\(srBlueDevice.name) 已经是当前输出设备")
            }

            AudioProcessingLogger.shared.info("设置 SR Blue 设备", details: "尝试将 \(srBlueDevice.name) 设为默认输出设备")
            if setDefaultOutputDevice(deviceID: srBlueDevice.id) {
                AudioProcessingLogger.shared.success("SR Blue 设备设置成功", details: "已成功切换到 \(srBlueDevice.name)")
                return (true, "已成功切换到 \(srBlueDevice.name)")
            } else {
                AudioProcessingLogger.shared.error("SR Blue 设备设置失败", details: "切换到 \(srBlueDevice.name) 失败，请检查系统设置")
                return (false, "切换到 \(srBlueDevice.name) 失败，请检查系统设置")
            }
        }

        AudioProcessingLogger.shared.warning("SR Blue 设备未找到", details: "未找到 \(srBlueDeviceName)，请检查设备连接状态（蓝牙是否已开启？）")
        return (false, "未找到 \(srBlueDeviceName)，请检查设备连接状态（蓝牙是否已开启？）")
    }
    
    func checkSRRecDevice() -> (available: Bool, message: String) {
        AudioProcessingLogger.shared.info("检查 SR-REC 设备", details: "查找设备: \(srRecDeviceName)")

        if let srRecDevice = availableInputDevices.first(where: { $0.name == srRecDeviceName }) {
            AudioProcessingLogger.shared.success("SR-REC 设备已找到", details: "设备ID: \(srRecDevice.id), 名称: \(srRecDevice.name)")
            return (true, "SR-REC 设备已就绪")
        }

        AudioProcessingLogger.shared.warning("SR-REC 设备未找到", details: "未找到 \(srRecDeviceName)，请检查设备连接状态（音频线是否已连接？）")
        return (false, "未找到 SR-REC 设备，请检查设备连接状态（音频线是否已连接？）")
    }
} 