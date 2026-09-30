# Deskbuddy
Deskbuddy is a compact ESP32-based smart desk companion built around a touchscreen display. The project combines 3D printing, simple hardware, and software to turn a raw ESP32 screen into a practical mini dashboard for your workspace. It is designed to be easy to set up and easy to personalize.

## ESP32 Toolchain

Use either the Arduino IDE or Arduino CLI to compile and flash the firmware:

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli compile --fqbn esp32:esp32:esp32 ./desk_buddy_github.cpp
```

The device also supports mDNS, so after Wi‑Fi setup it can usually be reached at `http://deskbuddy.local` or whatever hostname you set in the AP setup page.
