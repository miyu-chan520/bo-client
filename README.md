# bo-client — Game Instrumentation & Systemless Overlay Service

[![Build Magisk Module](https://github.com/miyu-chan520/bo-client/actions/workflows/build.yml/badge.svg)](https://github.com/miyu-chan520/bo-client/actions/workflows/build.yml)
![Platform](https://img.shields.io/badge/Platform-Android%20arm64--v8a-brightgreen)
![Target](https://img.shields.io/badge/Target-Free%20Fire%20(com.dts.freefireth)-blue)
![Architecture](https://img.shields.io/badge/Architecture-Magisk%20%2F%20KernelSU%20%2F%20APatch-orange)

High-performance, root-level game instrumentation and graphical overlay engine tailored for Unity IL2CPP games on Android (arm64-v8a). Built with native C++17, ImGui, SurfaceFlinger native surface rendering, and direct kernel syscalls for maximum stealth and zero overhead.

---

## Key Highlights

- **Zero Network Auth & Keyless**: Completely decoupled from external licensing and network verification servers. No libcurl or OpenSSL bloat; zero outbound network sockets.
- **Hardware-Accelerated Overlay**: Direct rendering via `ANativeWindowCreator` (EGL / GLES3) with private SurfaceFlinger buffer composition and touch event pass-through.
- **Direct Kernel Memory Access**: Memory operations leverage direct assembly syscalls (`svc #0` with `SYS_vm_read` / #270) bypassing libc-level userspace hooks.
- **Process Camouflage**: Runtime process title masquerading via `prctl(PR_SET_NAME, "surfaceflinger")` and sanitized `/proc/self/cmdline`.
- **Systemless Magisk Module**: Packaged with a late-start background watchdog service (`service.sh`) that detects game launch, attaches silently, and terminates on exit.

---

## Architecture & Module Structure

```
bo-client/
├── .github/workflows/
│   └── build.yml               # Automated Android NDK CI pipeline
├── includes/
│   ├── draw/                   # ImGui, touch handling, and GL rendering backends
│   ├── font/                   # High-DPI embedded typography assets
│   ├── internal/               # Native surface and window creation primitives
│   └── protect/                # Compile-time string and memory obfuscation (oxorany)
├── jni/
│   ├── Android.mk              # NDK build specifications and compilation flags
│   └── Application.mk          # Toolchain targeting arm64-v8a & C++17
├── magisk_module/              # Flashable Magisk/KernelSU module structure
│   ├── module.prop             # Module metadata
│   ├── customize.sh            # Root installer and permission configurations
│   ├── service.sh              # Background automated watchdog daemon
│   └── system/bin/             # Target deployment path for native binary
├── src/
│   ├── func/                   # ESP rendering routines (box, bones, tags)
│   ├── game/                   # Memory models, player structures, and IL2CPP offsets
│   ├── other/                  # Direct syscall memory engine (rpm/wpm)
│   ├── ui/                     # ImGui menu layout, dashboard, and widgets
│   ├── bypass.cpp              # Anti-detection hooks via ShadowHook
│   ├── globals.h               # Shared runtime state definitions
│   └── main.cpp                # Core entry point and lifecycle orchestrator
└── README.md
```

---

## Target Game & Offsets Specification

Current offsets are verified against **Free Fire OB55 / 1.132.1** (`com.dts.freefireth`):

| Struct / Function | Address / RVA | Target Context |
|---|---|---|
| `GameFacade` | `0xA72E4E0` | Static class pointer cell |
| `Player.TakeDamage` (9 params) | `0x6F91500` | Damage processing hook |
| `Player.TakeDamage` (4 params) | `0x6F913B4` | Direct impact hook |
| `Player.GetHeadTF` | `0x6FAE37C` | Head bone transform resolver |
| `Player.get_HeadCollider` | `0x6F2ACB0` | Head collider accessor |
| `Player.GetHeuristicTarget` | `0x6F8E764` | Target heuristic computation |
| `Player.StartFiring` | `0x6F988AC` | Weapon discharge trigger |
| `Match_PlayerDict` | `0xC0` | Player registry table |
| `Match_LocalPlayer` | `0xD8` | Local entity instance |

---

## Installation & Deployment

### Option 1: Magisk / KernelSU / APatch (Recommended)
1. Download `FF_Overlay_Magisk_Module_v2.0.zip` from [Releases](https://github.com/miyu-chan520/bo-client/releases) or the CI Artifacts.
2. Open **Magisk**, **KernelSU**, or **APatch** application.
3. Select **Modules** -> **Install from storage** -> select the `.zip`.
4. Reboot device. The background watchdog (`service.sh`) will automatically activate the overlay whenever `com.dts.freefireth` is launched.

### Option 2: Manual Terminal Execution (Root)
Ensure your device has root privileges:
```bash
# Push binary to device
adb push libs/arm64-v8a/surfaceflinger_helper /data/local/tmp/
adb shell

# Execute with root shell
su
chmod 755 /data/local/tmp/surfaceflinger_helper
/data/local/tmp/surfaceflinger_helper
```

---

## Building from Source

### Prerequisites
- Android NDK (r25c or higher recommended)
- Git & Make

### Local Build (Linux / macOS / WSL)
```bash
# Clone repository
git clone https://github.com/miyu-chan520/bo-client.git
cd bo-client

# Compile arm64 binary
ndk-build -C . NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk -j$(nproc)

# Strip symbols
$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-all libs/arm64-v8a/surfaceflinger_helper

# Package into flashable Magisk module
mkdir -p magisk_module/system/bin
cp libs/arm64-v8a/surfaceflinger_helper magisk_module/system/bin/
cd magisk_module && zip -r9 ../bo-client-magisk.zip ./* && cd ..
```

### Automated CI
Pushes to `main` or `master` automatically trigger GitHub Actions to compile the project and generate downloadable flashable zips under the **Actions** tab.

---

## Disclaimer

This software is developed strictly for educational, security research, and performance instrumentation purposes on hardware and environments owned by the operator.
