# Deskbuddy Setup Guide

This guide explains how to install and upload the Deskbuddy software to a compatible ESP32 touchscreen board using **Arduino IDE**.


## What You Need

Before you begin, make sure you have:

- A compatible ESP32 touchscreen board
- A USB cable with data support
- Arduino IDE installed
- Access to a WiFi network
- The Deskbuddy source code

## 1. Install the ESP32 Toolchain

Deskbuddy is built for ESP32, so you need the ESP32 toolchain and board package available in your Arduino workflow.

### Option A: Arduino IDE

1. Open **Arduino IDE**
2. Go to **Tools > Board > Boards Manager**
3. Search for `ESP32`
4. Install **ESP32 by Espressif Systems**

### Option B: Arduino CLI

If you use the command line instead of the IDE, install Arduino CLI and add the ESP32 board package:

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
export PATH="$HOME/.arduino15/packages/arduino/tools/arduino-cli/"$PATH
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install TFT_eSPI ArduinoJson XPT2046_Touchscreen
```

Then compile with a board target such as:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 ./deskbuddy-firmware
```

## 2. Install ESP32 Board Support

Deskbuddy is built for ESP32, so you first need to install the ESP32 board package in Arduino IDE.

1. Open **Arduino IDE**
2. Go to **Tools > Board > Boards Manager**
3. Search for `ESP32`
4. Install **ESP32 by Espressif Systems**

After installation, select the board that best matches your hardware under:

**Tools > Board**

If you are unsure which profile to use, start with the closest ESP32 option and adjust if needed.

## 2. Install Required Libraries

Open:

**Sketch > Include Library > Manage Libraries**

Install these libraries:

- `TFT_eSPI`
- `ArduinoJson`
- `XPT2046_Touchscreen`

The following are normally included automatically with the ESP32 board package:

- `WiFi`
- `HTTPClient`
- `WiFiClientSecure`
- `WebServer`
- `Preferences`
- `SPI`

## 3. Configure TFT_eSPI

This is the most important step for getting the display to work correctly.

Deskbuddy uses the **TFT_eSPI** library. Its project-local `User_Setup.h` is selected by the firmware, so do not overwrite the library's global setup file.

If the TFT_eSPI setup is wrong, you may see problems like:

- A black or white screen
- Wrong colors
- Incorrect rotation
- No visible output
- Touch and display not matching properly

### What to do

If the display remains blank, confirm that you are using the display and ESP32 board revision this project targets before changing the project-local `User_Setup.h`.

## 4. Open the Deskbuddy Code

Open the Deskbuddy project in Arduino IDE.

For the public version, use:

- [desk_buddy_github.cpp]

If you rename the file or convert it to an `.ino`, that is also fine as long as the project builds correctly in Arduino IDE.

## 5. Add Your WiFi Credentials

Before uploading, update the WiFi values in the code:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

Replace those placeholders with your own WiFi network name and password.

## 6. Select the Correct Board and Port

In Arduino IDE:

- Select the correct **ESP32 board**
- Select the correct **COM port**

You can find the port under:

**Tools > Port**

If no port appears:

- Reconnect the board
- Try another USB cable
- Make sure the cable supports data, not only charging

## 7. Upload the Code

Once everything is configured:

1. Connect the ESP32 board by USB
2. Click **Upload**
3. Wait for the sketch to compile and flash

On some ESP32 boards, you may need to hold the **BOOT** button during upload if flashing does not start automatically.

## 8. First Boot

After a successful upload, the device should:

- Power on the display
- Connect to WiFi
- Sync time
- Fetch weather data
- Start the local web interface

You can also open the Serial Monitor to check boot messages:

**Tools > Serial Monitor**

In many cases, the local IP address will be printed there after WiFi connection succeeds.

## 9. Open the Web Interface

Once the ESP32 is connected to your network, open its local IP address in your browser.

From the browser interface, you can adjust things like:

- Accent colors and theme
- Text colors
- Regional time and date format
- Timer presets
- Weather location
- Notes
- Nickname
- Alert behavior

This makes it easy to personalize the device without editing the code every time.

## OTA Updates

After Wi-Fi setup, keep the computer and Deskbuddy on the same local network. In Arduino IDE, select the discovered Deskbuddy network port and upload normally. Arduino OTA is already enabled in the firmware; change its password from the Deskbuddy settings page before relying on network updates. The initial USB flash must use the standard dual-slot OTA partition scheme.

## 10. Troubleshooting

### The display stays black or white

- Check the TFT_eSPI `User_Setup.h`
- Confirm the correct display driver is selected
- Verify that the board is receiving power

### The display works, but colors or rotation are wrong

- Check the TFT_eSPI configuration
- Verify display driver and pin mapping
- Confirm rotation settings in the code

### Touch does not work correctly

- Check touch wiring and controller support
- Verify rotation and calibration values

### Upload fails

- Make sure the correct COM port is selected
- Try another USB cable
- Hold the **BOOT** button during upload if needed

### WiFi does not connect

- Double-check SSID and password
- Make sure the network is in range
- Check the Serial Monitor for connection messages

### Time or weather does not update

- Confirm WiFi is connected
- Check that the location values are valid
- Verify that API requests are not being blocked

## Final Notes

Deskbuddy is designed to be easy to customize, but exact setup details may vary depending on your ESP32 touchscreen board version.

For most users, the **TFT_eSPI `User_Setup.h` configuration is the most important part** of the installation.

Once that is correct, the rest of the setup is usually straightforward.
