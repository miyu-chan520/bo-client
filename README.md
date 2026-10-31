# Bo-Client Zygisk VIP (Sakura Mods Edition)

> **⚠️ EDUCATIONAL RESEARCH & SECURITY ANALYSIS DISCLAIMER**  
> This project, code, and accompanying reverse engineering research are strictly intended for **academic, security analysis, and educational study** of Android runtime environments, Zygote process specialization, GLES3 graphics pipeline hooking, and memory reconstruction.  
> All analysis on target binaries (`P2077kng`, `PS FFH4X RiverMod`, `libanogs.so`, `libil2cpp.so`) was conducted for educational understanding of binary protection mechanisms, anti-cheat detection surfaces, and memory layout architectures.  
> This software is not intended for unfair gameplay or deployment on live public game servers. The author and contributors assume no liability for misuse.

---

## 🌸 Overview

**Bo-Client Zygisk VIP** is an internal Android Zygisk module engineered for **Free Fire & Free Fire MAX (armeabi-v7a & arm64-v8a)**.  
It combines the memory exploitation mechanisms analyzed from **P2077kng**, the movement and physics overrides from **PS Mods (RiverMod)**, and renders an internal **Sakura-themed ImGui HUD** directly inside the target application's EGL render loop without external overlay windows.

---

## 🔬 Reverse Engineering & Binary Analysis

### 1. P2077kng Dissection (`libmain.so` & `libRootMagisk.so`)
- **Anti-Cheat Neutralization**: P20 relies on Dobby inline hooks against `libanogs.so` exported symbols and internal scan offsets to silence anti-debugging, memory scanner threads, and mono reflection checks.
- **AutoKill & UnderKill Mechanics**: AutoKill forces damage redirection via `Player.TakeDamage` parameters:
  - `hitGroup = 1` (Forced Headshot).
  - `damage = 999` (One-hit elimination).
  - **UnderKill**: Sub-terrain coordinate offsetting where `hitLocation.y` is depressed by `-12.0f` to `-25.0f` with an upward normal vector `(0, 1, 0)`. This causes bullet rays to pierce upward through gloo walls, obstacles, and terrain.
- **Ghost Mode (`Ghostt`)**: Decouples the client player's simulation position from the network synchronization packet. The character model remains frozen at `ghost_origin` for other network clients while the local view and weapon origins freely translate.
- **Speed Timer (`SpeedTime` / `pkmods.speedtime`)**: Invokes Unity engine's native `UnityEngine.Time::set_timeScale(float value)` to accelerate local animation ticks and client physics calculations.
- **AutoTeleMark (`posTeleMark_x, posTeleMark_y`)**: Queries `Transform::set_position` on the local player node, instantly repositioning coordinates to marked map targets or `1.5m` behind selected hostile targets.

### 2. PS Mods / RiverMod Dissection (`libRIVERXE.so`, `libcrashlytics.so`, `Loader.java`)
- **Fly Hack (`FlyHigh`, `FlySpeed`, `FlyCarroConfig`)**: Overrides character controller physics by writing `Player_IsGrounded = false`, zeroing out gravitational velocity vectors `velocity.y = 0.0f`, and applying manual vertical propulsion increments directly to `Player_HeadTF` / `Transform_set_position`.
- **Sakura UI Architecture**: Extracted from `Loader.java` and `StringPool.java` (NPStringFog3 decrypted):
  - Base container: `#B30E0E0E` (70% alpha dark glass).
  - Sakura accents: `#FF4B8B` / `#FF758C` (soft glowing cherry blossom pink) and `#9B59B6` (violet trim).
  - Floating petal badge with quick toggle and expandable tabbed panel.

---

## 🛡️ Triple-Layer Bypass Architecture

1. **Layer 1: `libanogs.so` Anti-Cheat Neutralization**
   - Inline function hooks via **ShadowHook** in unique mode.
   - Neutralizes: Anti-Frida (`0x3516DC`), Anti-Debug (`0x365954`), Mono integrity checks (`0x307EF0`, `0x308B2C`, `0x318EF0`), Cheat DLL detectors (`0x30A108`, `0x319CE4`, `0x2D7A78`), and background scanner threads (`0x419E3C`, `0x419EE4`, `0x39FF60`, `0x39FF78`).
2. **Layer 2: Kernel VFS & `/proc` Filter**
   - Intercepts `libc.so::fopen` and `openat`.
   - Sanitizes `/proc/self/maps` by dynamically stripping entries matching `zygisk`, `bo_client`, `libshadowhook`, `magisk`, and `modules`.
   - Clokes `/proc/self/status` by spoofing `TracerPid: 0`.
3. **Layer 3: Memory Watchdog Neutralization**
   - Prevents memory integrity watchdog threads from detecting code patches.

---

## 🎮 Feature Matrix

| Feature | Source / Mechanism | Description |
|---|---|---|
| **Silent Aim** | Memory Ray Redirection | Redirects bullet trajectories to target head transform |
| **AutoKill** | P2077kng `TakeDamage` Injection | Instant 999 damage injection to enemy entity |
| **UnderKill** | P2077kng Ray Sub-Terrain Offset | Pierces gloo walls and obstacles from underground angle |
| **Ghost Mode** | P2077kng Network Freeze | Freezes network hit-box at anchor point |
| **Speed Timer** | P2077kng Unity TimeScale | Accelerates game speed up to 4.0x |
| **AutoTeleMark** | P2077kng Transform Relocation | Instant teleportation behind target enemy |
| **Fly Hack** | RiverMod Zero-Gravity Float | Character flight with adjustable hover velocity |
| **Fly Car** | RiverMod Vehicle Float | Vehicle elevation and gravity nullification |
| **Chams ESP** | GLES3 Depth Override | Draws player meshes through obstacles via `glDepthRangef` |
| **2D/3D Box ESP** | Internal W2S Projection | Real-time player boxes, bones, names, health, distance |
| **Sakura UI** | PS Mods Sakura Styling | Frosted dark glass HUD with Sakura cherry blossom aesthetics |

---

## 🛠️ Building & Packaging

### Prerequisites
- Android NDK `r25c` or newer
- POSIX shell environment / Linux / macOS / WSL

### Compilation
```bash
cd jni
ndk-build -B V=1
```

### Packaging as Magisk / KernelSU / APatch Module
```bash
mkdir -p package/zygisk
cp libs/arm64-v8a/libbo_client.so package/zygisk/arm64-v8a.so
cp module.prop package/
cp customize.sh package/
cd package
zip -r ../bo-client-zygisk-v2.0.zip .
```

---

## 📜 License & Acknowledgements
- Developed for educational research into game security architecture.
- Powered by [ShadowHook](https://github.com/bytedance/shadowhook) and [Dear ImGui](https://github.com/ocornut/imgui).
