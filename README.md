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
- Cross-platform (C++20) – works on STM32, Arduino, Linux, Windows, macOS
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

## Typed settings

Settings declare their persistence policy. `Memory` values are RAM-only; `Frequent` and `LongTerm` values are written
immediately to their configured `SettingsStore`.

```cpp
#include "Setting/SettingDefinition.h"
#include "Setting/SettingsRegistry.h"

using namespace IntegralMotions::Config;

int32_t speed = 0;

SettingDefinition<int32_t> speedDefinition{
    .key = {.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 0},
    .defaultValue = 1000,
    .limits = {.minimum = 0, .maximum = 2000, .step = 100, .isRange = true},
};

speedDefinition.moduleId.assign("drive");
speedDefinition.groupId.assign("motion");
speedDefinition.id.assign("speed");
speedDefinition.unit.assign("rpm");
speedDefinition.limits.options[0].value = 500;
speedDefinition.limits.options[0].id.assign("low-speed");
speedDefinition.limits.options[1].value = 1500;
speedDefinition.limits.options[1].id.assign("high-speed");
speedDefinition.limits.optionCount = 2;

SettingsRegistry<32> registry;
registry.add(speedDefinition, speed);
registry.set(speedDefinition.key, 1200);
```

Definitions and referenced live values (`speed` above) must remain valid for the lifetime of the registry. Use
`static const` definitions for device-lifetime settings. Mutable definitions are rejected as references; move a
received definition into the registry to retain it with its current value:

```cpp
registry.add(std::move(receivedDefinition), receivedValue);
```

The registry owns and releases moved definitions and values. Configure the corresponding `SettingsStore` before adding a
`Frequent` or `LongTerm` setting.

Each option has a machine value and a stable ID. MessagePack `options` arrays use the same shape:

```text
[{ "value": 500, "id": "low-speed" }]
```

## MessagePack protocol

Numeric setting types use explicit-width IDs: `i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i64`, `u64`, `f32`, and
`f64`. Boolean settings use `bool`.

`DeviceResponseSerializer` always emits `deviceInfo`, setting `limits`, limit `isRange`, and limit `options`.
Empty `min`, `max`, `step`, `unit`, `readonly`, and success `errorMessage` fields are omitted. Generic object
serialization emits `nil` for a null array pointer; use `DeviceResponseSerializer` for public device responses.

Payload classes are selected while streaming the MessagePack input. In a message envelope, `msgType` and `opCode`
must therefore appear before `payload`. Inside a setting, `type` must appear before `value`. Inputs that violate this
ordering are rejected.

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
3. Add include path and standard flags (`-std=c++20`, `-ffunction-sections`, `-Os`)

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
