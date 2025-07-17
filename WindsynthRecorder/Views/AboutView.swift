import SwiftUI

// MARK: - Info Row
struct InfoRow: View {
    let label: String
    let value: String
    
    var body: some View {
        HStack {
            Text(label)
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 100, alignment: .leading)
            
            Text(value)
                .font(.system(size: 11))
                .foregroundStyle(.primary)
                .lineLimit(1)
                .truncationMode(.middle)
            
            Spacer()
        }
    }
}

// MARK: - About Window View (for standalone window)
struct AboutWindowView: View {
    // 应用信息
    private let appName = "WindsynthRecorder"
    private let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.0"
    private let buildNumber = Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "1"

    // 系统信息
    private var systemVersion: String {
        let version = ProcessInfo.processInfo.operatingSystemVersion
        return "macOS \(version.majorVersion).\(version.minorVersion).\(version.patchVersion)"
    }

    private var deviceName: String {
        ProcessInfo.processInfo.hostName
    }

    private var bundleIdentifier: String {
        Bundle.main.bundleIdentifier ?? "dev.wibus.WindsynthRecorder"
    }

    private var architecture: String {
        #if arch(arm64)
        return "Apple Silicon"
        #elseif arch(x86_64)
        return "Intel"
        #else
        return "Unknown"
        #endif
    }

    var body: some View {
        VStack(spacing: 0) {
            HStack(spacing: 40) {
                // Left side - App Icon
                VStack {
                    if let appIcon = NSApplication.shared.applicationIconImage {
                        Image(nsImage: appIcon)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 120, height: 120)
                    } else {
                        RoundedRectangle(cornerRadius: 20)
                            .fill(LinearGradient(
                                colors: [.blue.opacity(0.8), .purple.opacity(0.6)],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            ))
                            .frame(width: 120, height: 120)
                            .overlay(
                                Image(systemName: "waveform.circle.fill")
                                    .font(.system(size: 60, weight: .light))
                                    .foregroundStyle(.white)
                            )
                    }

                    Spacer()
                }

                // Right side - App Info
                VStack(alignment: .leading, spacing: 16) {
                    // App Name and Version
                    VStack(alignment: .leading, spacing: 4) {
                        Text(appName)
                            .font(.system(size: 32, weight: .regular))
                            .foregroundStyle(.primary)

                        Text("Version \(version) (\(buildNumber))")
                            .font(.system(size: 14))
                            .foregroundStyle(.secondary)
                    }

                    VStack(spacing: 12) {
                        Text("System Info")
                            .font(.system(size: 14, weight: .semibold))
                            .foregroundStyle(.secondary)
                        
                        VStack(spacing: 6) {
                            InfoRow(label: "System version", value: ProcessInfo.processInfo.operatingSystemVersionString)
                            InfoRow(label: "Device name", value: ProcessInfo.processInfo.hostName)
                            InfoRow(label: "Bundle ID", value: bundleIdentifier)
                        }
                    }
                    .padding(.horizontal, 16)
                    .padding(.vertical, 16)
                    .background(Color.primary.opacity(0.03), in: RoundedRectangle(cornerRadius: 12))

                    Spacer()

                    // Copyright
                    Text("Copyright © 2025 wibus. All rights reserved. WindsynthRecorder and the WindsynthRecorder logo are trademarks of wibus, registered in the China and other countries.")
                        .font(.system(size: 11))
                        .foregroundStyle(.secondary)
                        .lineLimit(nil)
                        .fixedSize(horizontal: false, vertical: true)

                    // Buttons
                    HStack(spacing: 12) {
                        Button("Acknowledgments") {
                            // 可以添加致谢信息
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.regular)

                        Button("License Agreement") {
                            if let url = URL(string: "https://github.com/wibus-wee/WindsynthRecorder/blob/main/LICENSE") {
                                NSWorkspace.shared.open(url)
                            }
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.regular)

                        Spacer()
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }
            .padding(.horizontal, 40)
            .padding(.bottom, 30)
        }
        .frame(width: 650, height: 360)
//        .background(Color(.windowBackgroundColor))
        .padding(.top, 30)
    }
    
}

#Preview("Window View") {
    AboutWindowView()
        .frame(width: 700, height: 440)
}
