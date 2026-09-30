# Deskbuddy
Deskbuddy is a compact ESP32-based smart desk companion built around a touchscreen display. The project combines 3D printing, simple hardware, and software to turn a raw ESP32 screen into a practical mini dashboard for your workspace. It is designed to be easy to set up and easy to personalize.

## ESP32 Toolchain

Use either the Arduino IDE or Arduino CLI to compile and flash the firmware:

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install TFT_eSPI ArduinoJson XPT2046_Touchscreen
arduino-cli compile --fqbn esp32:esp32:esp32 ./deskbuddy-firmware
```

The project folder includes its Arduino sketch entrypoint and display configuration. After Wi-Fi setup, the device supports mDNS and Arduino OTA: select its network port in Arduino IDE while your computer is on the same network. Change the OTA password in the Deskbuddy settings page; the default is `deskbuddy123`.
