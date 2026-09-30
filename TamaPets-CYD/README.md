# TamaPets-CYD

A cat-pet game for the classic ESP32 CYD 2.8-inch touchscreen (ESP32, ILI9341, XPT2046). Pet care runs on the device. Bluetooth Classic handles setup and commands; optional Wi-Fi is used only for local-network OTA updates. There are no cloud services or account requirements.

## Features

- Egg hatches into a kitten after ten minutes once the clock is set.
- Hunger, happiness, and energy meters decay while powered on.
- Touch controls for feeding, playing, training, and the Paw Pick mini-game.
- Kitten growth, care levels, pet name, birthday, and care state saved in ESP32 NVS.
- Calling Sitter Poketchi opens a full-screen clock and date view with an early-pickup button. She works from 5:00 AM until 7:00 PM; leaving her called past 7:00 PM reduces happiness and displays her reminder.
- Sleep mode shows a full-screen clock and date until any touchscreen tap wakes the pet. The clock continues and energy recovers while sleeping.
- Bluetooth status, clock, name, birthday, care, and sitter commands.
- Optional authenticated ArduinoOTA updates over the local Wi-Fi network.

## Build and flash over USB

1. Install Arduino IDE 2 and the **ESP32 by Espressif Systems** board package.
2. Install these Arduino libraries using Library Manager: **TFT_eSPI** and **XPT2046_Touchscreen**. `BluetoothSerial`, `Preferences`, and `SPI` come with the ESP32 board package.
3. Open `TamaPets-CYD.ino` from this folder. The project includes its own CYD TFT_eSPI setup, so do not replace the library's global `User_Setup.h`.
4. Select **ESP32 Dev Module** and **Tools > Partition Scheme > Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**. The larger dual-slot partition layout is required for OTA.
5. Connect the CYD with a USB data cable and choose **Upload**. If upload stalls at connecting, hold the board's BOOT button until upload begins. The Projects-page browser installer writes this matching partition layout for you.

Arduino CLI build:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs TamaPets-CYD
```

The sketch targets the original ESP32 with Bluetooth Classic (SPP). ESP32-S2, ESP32-S3, and ESP32-C3 boards do not support this BluetoothSerial interface.

## Bluetooth commands

Pair with **TamaPets-CYD** from a Bluetooth Classic serial terminal app. Send one command per line:

```text
HELP
STATUS
TIME=06:30
DATE=2026-09-30
WIFI_SSID=YourNetwork
WIFI_PASS=your-wifi-password
OTA_PASS=choose-a-long-password
WIFI_CONNECT
OTA_STATUS
NAME=Mochi
BIRTHDAY=09/30/2026
FEED
PLAY
TRAIN
SITTER=ON
SITTER=OFF
SLEEP
WAKE
GAME
```

The clock is local 24-hour time. Set the time and date over Bluetooth; both advance while the device is powered. Set them again after a power-off if you need the sitter schedule or egg timer to remain accurate. Pet stats and egg progress are stored in flash and resume after reboot. `SITTER=OFF` or the on-screen pickup button dismisses Poketchi early without a happiness penalty. Bluetooth game command `GAME` starts Paw Pick on the touchscreen.

## OTA updates

Flash the firmware once over USB with the OTA-capable partition scheme above. Pair a Bluetooth Classic serial terminal with `TamaPets-CYD`, then set `WIFI_SSID`, `WIFI_PASS`, and an `OTA_PASS` of at least 8 characters. Send `WIFI_CONNECT`; `OTA_STATUS` reports `OTA_READY=<ip>` when the device joins Wi-Fi. The device reconnects automatically after reboot once configured.

Keep the computer and pet on the same local network. In Arduino IDE, select the discovered `TamaPets-CYD` network port and upload the sketch with the same `min_spiffs` partition scheme. OTA updates only the application slot; the initial USB flash installs the partition table. Wi-Fi credentials and the OTA password are stored in device Preferences, so provision them only from a trusted paired device and private network. `WIFI_OFF` disables Wi-Fi while retaining the saved settings.

## CYD display and touch

The included TFT_eSPI configuration targets the common CYD wiring: ILI9341 on TFT pins 12/13/14/15/2, backlight on 21, and XPT2046 touch on 25/32/39/33/36. Touch input uses the common CYD calibration range; different CYD revisions may need touch calibration adjusted in the sketch.