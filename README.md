# IntegralConfig

**IntegralConfig** is a lightweight, cross-platform C++ library for encoding and decoding configuration data for embedded devices.  
It provides a consistent way to pack configuration parameters into byte streams for transmission or storage, and to decode them back on the device.

This library is designed to be **platform-independent**, supporting:
- STM32 (via STM32CubeIDE, by linking the include/src folders)
- Arduino / PlatformIO (via `library.json`)
- Desktop environments (for unit tests and simulation with GoogleTest)
- Web-based configuration through **[configurator.integralmotions.com](https://configurator.integralmotions.com)** (using the same byte protocol)

---

## ✨ Features
- Encode/decode configuration data to/from byte arrays
- Cross-platform (C++17) – works on STM32, Arduino, Linux, Windows, macOS
- Web configuration support via the Integral Motion Configurator
- Lightweight (no STL heap usage if disabled)
- Header-only or minimal `src/` implementation
- Unit-tested with GoogleTest

---

## 🧩 Example
```cpp
#include <integral_config/encoder.h>
#include <integral_config/decoder.h>

struct MotorSettings {
    uint8_t id;
    float kp;
    float ki;
    float kd;
};

MotorSettings m{1, 0.12f, 0.08f, 0.004f};
uint8_t buffer[16];

size_t len = IntegralConfig::encode(m, buffer);
MotorSettings copy{};
IntegralConfig::decode(buffer, len, copy);
```

---

## 🧱 Repository Structure
```
IntegralConfig/
  ├── include/integral_config/   # Public headers
  ├── src/                       # Optional implementation files
  ├── tests/                     # GoogleTest unit tests
  ├── examples/                  # STM32 / Arduino usage samples
  ├── CMakeLists.txt             # Desktop build + GoogleTest
  ├── library.json               # PlatformIO metadata
  └── README.md
```

---

## ⚙️ Building & Testing (Desktop)
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build -V
```

---

## 🔌 PlatformIO / Arduino
In `platformio.ini`:
```ini
lib_deps = https://github.com/IntegralMotion/IntegralConfig.git
```

---

## 💡 STM32CubeIDE Integration
1. Add as a git submodule under `external/IntegralConfig`
2. In CubeIDE: link the `include/` and `src/` folders
3. Add include path and standard flags (`-std=c++17`, `-ffunction-sections`, `-Os`)

---

## 🌐 Web Configuration
Easily configure and visualize your device settings using the **[Integral Motion Configurator](https://configurator.integralmotions.com)** —  
a Chrome-compatible web tool that communicates using the same IntegralConfig byte protocol.

---

## 📄 License
MIT License © 2025 Integral Motion

---

## 🧠 About
Created by **Integral Motion** to unify configuration handling across embedded platforms and browser-based configuration tools.
# IntegralConfig
