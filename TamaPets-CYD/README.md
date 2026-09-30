# TamaPets-CYD

A standalone, USB-flashable cat-pet game for the classic ESP32 CYD 2.8-inch touchscreen (ESP32, ILI9341, XPT2046). Pet care runs on the device; Bluetooth Classic is used for setup and simple commands. This firmware does not use Wi-Fi, a website, cloud services, or Google sign-in.

## Features

- Egg hatches into a kitten after ten minutes once the clock is set.
- Hunger, happiness, and energy meters decay while powered on.
- Touch controls for feeding, playing, training, and the Paw Pick mini-game.
- Kitten growth, care levels, pet name, birthday, and care state saved in ESP32 NVS.
- Calling Sitter Poketchi opens a full-screen clock and date view with an early-pickup button. She works from 5:00 AM until 7:00 PM; leaving her called past 7:00 PM reduces happiness and displays her reminder.
- Sleep mode shows a full-screen clock and date until any touchscreen tap wakes the pet. The clock continues and energy recovers while sleeping.
- Bluetooth status, clock, name, birthday, care, and sitter commands.

## Build and flash over USB

1. Install Arduino IDE 2 and the **ESP32 by Espressif Systems** board package.
2. Install these Arduino libraries using Library Manager: **TFT_eSPI** and **XPT2046_Touchscreen**. `BluetoothSerial`, `Preferences`, and `SPI` come with the ESP32 board package.
3. Open `TamaPets-CYD.ino` from this folder. The project includes its own CYD TFT_eSPI setup, so do not replace the library's global `User_Setup.h`.
4. Select the ESP32 board profile matching the CYD, then select its USB serial port. For the original CYD 2.8-inch with ESP32-WROOM, **ESP32 Dev Module** is a suitable starting profile.
5. Connect the CYD with a USB data cable and choose **Upload**. If upload stalls at connecting, hold the board's BOOT button until upload begins.

The sketch targets the original ESP32 with Bluetooth Classic (SPP). ESP32-S2, ESP32-S3, and ESP32-C3 boards do not support this BluetoothSerial interface.

## Bluetooth commands

Pair with **TamaPets-CYD** from a Bluetooth Classic serial terminal app. Send one command per line:

```text
HELP
STATUS
TIME=06:30
DATE=2026-09-30
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

## CYD display and touch

The included TFT_eSPI configuration targets the common CYD wiring: ILI9341 on TFT pins 12/13/14/15/2, backlight on 21, and XPT2046 touch on 25/32/39/33/36. Touch input uses the common CYD calibration range; different CYD revisions may need touch calibration adjusted in the sketch.