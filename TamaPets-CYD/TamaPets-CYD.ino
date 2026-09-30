#include <BluetoothSerial.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <SPI.h>
#include "TamaPets_CYD_TFT_Setup.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>

#include <time.h>

// CYD 2.8-inch (ESP32 + ILI9341 + XPT2046) touch wiring.
static constexpr int TOUCH_CS = 33;
static constexpr int TOUCH_IRQ = 36;
static constexpr int TOUCH_SCK = 25;
static constexpr int TOUCH_MISO = 39;
static constexpr int TOUCH_MOSI = 32;

static constexpr int SCREEN_ROTATION = 1;
static constexpr uint32_t SAVE_INTERVAL_MS = 60000;
static constexpr uint32_t NEED_TICK_MS = 60000;
static constexpr uint32_t EGG_HATCH_SECONDS = 600;

TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
BluetoothSerial bluetooth;
Preferences preferences;
String otaWifiSsid;
String otaWifiPassword;
String otaPassword;
bool otaEnabled = false;
bool otaStarted = false;
uint32_t otaLastConnectAttempt = 0;

enum Screen : uint8_t { SCREEN_HOME, SCREEN_CARE, SCREEN_GAME, SCREEN_PROFILE, SCREEN_SITTER, SCREEN_SLEEP };
enum PetStage : uint8_t { STAGE_EGG, STAGE_KITTEN, STAGE_CAT };

struct PetData {
  char name[17] = "Mochi";
  char birthday[11] = "--/--/----";
  uint16_t hunger = 850;
  uint16_t happiness = 800;
  uint16_t energy = 900;
  uint16_t care = 0;
  uint32_t ageMinutes = 0;
  uint32_t eggSeconds = 0;
  uint32_t sitterDay = 0;
  uint8_t stage = STAGE_EGG;
  uint8_t level = 1;
  bool sitterCalled = false;
  bool sitterPenaltyPaid = false;
};

PetData pet;
Screen currentScreen = SCREEN_HOME;
Screen returnScreen = SCREEN_HOME;
uint32_t lastNeedTick = 0;
uint32_t lastSave = 0;
uint32_t lastClockTick = 0;
uint16_t localMinutes = 0;
uint16_t localYear = 0;
uint8_t localMonth = 0;
uint8_t localDay = 0;
bool clockWasSet = false;
bool dateWasSet = false;
String bluetoothLine;
String notice = "Touch to care";
uint32_t noticeUntil = 0;
uint8_t gameRound = 0;
uint8_t gameTarget = 0;

uint16_t clampMeter(uint32_t value) {
  return value > 1000 ? 1000 : static_cast<uint16_t>(value);
}

bool isLeapYear(uint16_t year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) return 29;
  return month >= 1 && month <= 12 ? monthDays[month - 1] : 0;
}

String dateText() {
  if (!dateWasSet) return "DATE NOT SET";
  char value[11];
  snprintf(value, sizeof(value), "%04u-%02u-%02u", localYear, localMonth, localDay);
  return String(value);
}

bool setDateFromText(const String& value) {
  int year, month, day;
  char trailing;
  if (sscanf(value.c_str(), "%4d-%2d-%2d%c", &year, &month, &day, &trailing) != 3) return false;
  if (year < 2000 || year > 9999 || month < 1 || month > 12 || day < 1 ||
      day > daysInMonth(year, month)) return false;
  localYear = year;
  localMonth = month;
  localDay = day;
  dateWasSet = true;
  return true;
}

void advanceDate(uint32_t days) {
  if (!dateWasSet) return;
  while (days-- > 0) {
    if (localYear == 9999 && localMonth == 12 && localDay == 31) return;
    if (++localDay > daysInMonth(localYear, localMonth)) {
      localDay = 1;
      if (++localMonth > 12) {
        localMonth = 1;
        ++localYear;
      }
    }
  }
}

uint16_t scaleX(int raw) {
  return static_cast<uint16_t>(constrain(map(raw, 544, 3720, 0, 319), 0, 319));
}

uint16_t scaleY(int raw) {
  return static_cast<uint16_t>(constrain(map(raw, 562, 3604, 0, 239), 0, 239));
}

void showNotice(const String& text) {
  notice = text;
  noticeUntil = millis() + 2500;
}

void savePet() {
  preferences.begin("tamapets", false);
  preferences.putBytes("pet", &pet, sizeof(pet));
  preferences.putUShort("clock", localMinutes);
  preferences.putBool("clockSet", clockWasSet);
  preferences.putUShort("year", localYear);
  preferences.putUChar("month", localMonth);
  preferences.putUChar("day", localDay);
  preferences.putBool("dateSet", dateWasSet);
  preferences.end();
  lastSave = millis();
}

void loadPet() {
  preferences.begin("tamapets", true);
  if (preferences.getBytesLength("pet") == sizeof(pet)) {
    preferences.getBytes("pet", &pet, sizeof(pet));
  }
  localMinutes = preferences.getUShort("clock", 0);
  clockWasSet = preferences.getBool("clockSet", false);
  localYear = preferences.getUShort("year", 0);
  localMonth = preferences.getUChar("month", 0);
  localDay = preferences.getUChar("day", 0);
  dateWasSet = preferences.getBool("dateSet", false);
  otaWifiSsid = preferences.getString("otaSsid", "");
  otaWifiPassword = preferences.getString("otaWifi", "");
  otaPassword = preferences.getString("otaPass", "");
  preferences.end();
  pet.name[sizeof(pet.name) - 1] = '\0';
  pet.birthday[sizeof(pet.birthday) - 1] = '\0';
}

void saveOtaSettings() {
  preferences.begin("tamapets", false);
  preferences.putString("otaSsid", otaWifiSsid);
  preferences.putString("otaWifi", otaWifiPassword);
  preferences.putString("otaPass", otaPassword);
  preferences.end();
}

void startOtaWifi() {
  if (otaWifiSsid.isEmpty()) {
    bluetooth.println("ERROR set WIFI_SSID first");
    showNotice("Set WIFI_SSID over Bluetooth");
    return;
  }
  if (otaPassword.length() < 8) {
    bluetooth.println("ERROR set OTA_PASS to at least 8 characters");
    showNotice("Set OTA_PASS (8+ chars) first");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(otaWifiSsid.c_str(), otaWifiPassword.c_str());
  otaEnabled = true;
  otaLastConnectAttempt = millis();
  bluetooth.println("Connecting to Wi-Fi for OTA");
  showNotice("Connecting for OTA...");
}

void stopOtaWifi() {
  otaEnabled = false;
  if (otaStarted) ArduinoOTA.end();
  otaStarted = false;
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
}

void serviceOta() {
  if (!otaEnabled) return;
  if (WiFi.status() != WL_CONNECTED) {
    if (otaStarted) {
      ArduinoOTA.end();
      otaStarted = false;
    }
    if (millis() - otaLastConnectAttempt >= 15000) {
      WiFi.reconnect();
      otaLastConnectAttempt = millis();
    }
    return;
  }

  if (!otaStarted) {
    ArduinoOTA.setHostname("TamaPets-CYD");
    ArduinoOTA.setPassword(otaPassword.c_str());
    ArduinoOTA.onStart([]() { Serial.println("OTA update started"); });
    ArduinoOTA.onEnd([]() {
      Serial.println("OTA update complete; restarting");
      bluetooth.println("OTA complete; restarting");
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA error: %u\n", error);
      bluetooth.printf("OTA error: %u\n", error);
    });
    ArduinoOTA.begin();
    otaStarted = true;
    bluetooth.printf("OTA ready at %s (port 3232)\n", WiFi.localIP().toString().c_str());
    Serial.printf("TamaPets OTA ready at %s\n", WiFi.localIP().toString().c_str());
    showNotice("OTA ready on local Wi-Fi");
  }
  ArduinoOTA.handle();
}

void drawCat(int x, int y, int pixel, bool happy) {
  const uint16_t fur = 0xFD20;
  const uint16_t shade = 0xA4A0;
  const uint16_t innerEar = 0xF986;
  const uint16_t dark = 0x2104;
  auto block = [&](int px, int py, int w, int h, uint16_t color) {
    tft.fillRect(x + px * pixel, y + py * pixel, w * pixel, h * pixel, color);
  };

  block(2, 0, 2, 1, fur); block(8, 0, 2, 1, fur);
  block(1, 1, 4, 1, fur); block(7, 1, 4, 1, fur);
  block(1, 2, 10, 6, fur); block(2, 8, 8, 2, fur);
  block(3, 1, 2, 2, innerEar); block(8, 1, 2, 2, innerEar);
  block(2, 4, 2, 2, dark); block(8, 4, 2, 2, dark);
  block(5, 6, 2, 1, 0xF800);
  block(3, 7, 1, 1, dark); block(7, 7, 1, 1, dark);
  block(2, 9, 8, 1, shade);
  block(0, 11, 3, 1, fur); block(3, 10, 6, 3, fur); block(9, 11, 3, 1, fur);
  block(2, 13, 3, 1, shade); block(7, 13, 3, 1, shade);
  if (happy) {
    block(4, 8, 1, 1, dark); block(6, 8, 1, 1, dark);
  }
}

void drawMeter(int x, int y, const char* label, uint16_t value, uint16_t color) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(label, x, y, 2);
  tft.drawRoundRect(x, y + 17, 118, 10, 2, 0x7BEF);
  int width = map(value, 0, 1000, 0, 114);
  if (width > 0) tft.fillRect(x + 2, y + 19, width, 6, color);
  tft.setTextColor(0xBDF7, TFT_BLACK);
  tft.drawRightString(String(value / 10) + "%", x + 186, y + 1, 2);
}

void drawButton(int x, int y, int w, const char* label, uint16_t color) {
  tft.fillRoundRect(x, y, w, 32, 5, color);
  tft.setTextColor(TFT_BLACK, color);
  tft.drawCentreString(label, x + w / 2, y + 11, 2);
}

void drawNavigation() {
  tft.fillRect(0, 195, 320, 45, 0x18E3);
  const char* labels[] = {"HOME", "CARE", "GAME", "PET"};
  for (int i = 0; i < 4; ++i) {
    uint16_t color = static_cast<int>(currentScreen) == i ? 0x07E0 : 0xC618;
    tft.setTextColor(color, 0x18E3);
    tft.drawCentreString(labels[i], 40 + i * 80, 211, 2);
  }
}

void drawScreen() {
  tft.fillScreen(currentScreen == SCREEN_SLEEP ? 0x0008 : TFT_BLACK);
  if (currentScreen == SCREEN_SITTER) {
    tft.setTextColor(0x07E0, TFT_BLACK);
    tft.drawCentreString("POKETCHI IS SITTING", 160, 20, 2);
    drawCat(140, 46, 3, pet.happiness > 400);
    tft.setTextColor(0xBDF7, TFT_BLACK);
    tft.drawCentreString(clockWasSet ? "CURRENT TIME" : "SET CLOCK BY BLUETOOTH", 160, 98, 2);
    char clockText[8];
    if (clockWasSet) snprintf(clockText, sizeof(clockText), "%02u:%02u", localMinutes / 60, localMinutes % 60);
    else snprintf(clockText, sizeof(clockText), "--:--");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(clockText, 160, 119, 7);
    tft.setTextColor(0xFFE0, TFT_BLACK);
    tft.drawCentreString(dateText().c_str(), 160, 157, 4);
    tft.setTextColor(0xBDF7, TFT_BLACK);
    tft.drawCentreString("Tap when you're back", 160, 185, 2);
    tft.fillRoundRect(20, 207, 280, 29, 4, 0x07E0);
    tft.setTextColor(TFT_BLACK, 0x07E0);
    tft.drawCentreString("I'M BACK - LET HER GO", 160, 214, 2);
    return;
  }
  if (currentScreen == SCREEN_SLEEP) {
    tft.setTextColor(0x7BEF, 0x0008);
    tft.drawCentreString("SLEEPING", 160, 18, 2);
    char clockText[8];
    if (clockWasSet) snprintf(clockText, sizeof(clockText), "%02u:%02u", localMinutes / 60, localMinutes % 60);
    else snprintf(clockText, sizeof(clockText), "--:--");
    tft.setTextColor(TFT_WHITE, 0x0008);
    tft.drawCentreString(clockText, 160, 48, 7);
    tft.setTextColor(0xBDF7, 0x0008);
    tft.drawCentreString(dateText().c_str(), 160, 95, 4);
    drawCat(146, 128, 3, false);
    tft.setTextColor(0x7BEF, 0x0008);
    tft.drawString("Z", 194, 133, 4);
    tft.drawString("z", 207, 122, 2);
    tft.drawCentreString("TAP ANYWHERE TO WAKE", 160, 191, 2);
    return;
  }
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 30, 0x2104);
  tft.setTextColor(0x07E0, 0x2104);
  tft.drawString("TAMAPETS", 10, 7, 2);

  char clockText[8];
  if (clockWasSet) {
    snprintf(clockText, sizeof(clockText), "%02u:%02u", localMinutes / 60, localMinutes % 60);
  } else {
    snprintf(clockText, sizeof(clockText), "--:--");
  }
  tft.setTextColor(TFT_WHITE, 0x2104);
  tft.drawRightString(clockText, 309, 7, 2);

  if (currentScreen == SCREEN_HOME) {
    if (pet.stage == STAGE_EGG) {
      tft.fillEllipse(161, 71, 26, 32, 0xFFFF);
      tft.drawLine(151, 70, 160, 62, 0x7BEF);
      tft.drawLine(160, 62, 168, 68, 0x7BEF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString("A tiny egg is warming up", 160, 116, 2);
      if (!clockWasSet) tft.drawCentreString("Set clock in Bluetooth first", 160, 141, 2);
      else {
        uint32_t remaining = pet.eggSeconds >= EGG_HATCH_SECONDS ? 0 : EGG_HATCH_SECONDS - pet.eggSeconds;
        tft.drawCentreString((String("Hatches in ") + String(remaining / 60) + " min").c_str(), 160, 141, 2);
      }
    } else {
      drawCat(119, 34, 7, pet.happiness > 400);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString(pet.name, 160, 135, 4);
      tft.setTextColor(0xBDF7, TFT_BLACK);
      String stage = pet.stage == STAGE_KITTEN ? "KITTEN" : "CAT";
      tft.drawCentreString((stage + "  /  LV " + String(pet.level)).c_str(), 160, 166, 2);
      drawButton(239, 39, 72, "SLEEP", 0x7BEF);
    }
    tft.setTextColor(0xFFE0, TFT_BLACK);
    tft.drawCentreString(notice.c_str(), 160, 177, 2);
  } else if (currentScreen == SCREEN_CARE) {
    drawMeter(18, 34, "HUNGER", pet.hunger, 0xFBE0);
    drawMeter(18, 76, "HAPPINESS", pet.happiness, 0x07E0);
    drawMeter(18, 118, "ENERGY", pet.energy, 0x07FF);
    drawButton(5, 158, 98, "FEED", 0xFFE0);
    drawButton(111, 158, 98, "PLAY", 0x07E0);
    drawButton(217, 158, 98, "TRAIN", 0x07FF);
  } else if (currentScreen == SCREEN_GAME) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("PAW PICK", 160, 35, 4);
    tft.drawCentreString("Tap the same color as the paw", 160, 72, 2);
    const uint16_t colors[] = {0xF800, 0x07E0, 0x001F};
    const char* names[] = {"RED", "GREEN", "BLUE"};
    for (int i = 0; i < 3; ++i) {
      tft.fillRoundRect(22 + i * 100, 108, 80, 54, 5, colors[i]);
      tft.setTextColor(TFT_WHITE, colors[i]);
      tft.drawCentreString(names[i], 62 + i * 100, 127, 2);
    }
    uint16_t pawColor = colors[gameTarget];
    tft.fillCircle(160, 96, 8, pawColor);
    tft.fillCircle(149, 86, 4, pawColor);
    tft.fillCircle(157, 82, 4, pawColor);
    tft.fillCircle(165, 82, 4, pawColor);
    tft.fillCircle(173, 86, 4, pawColor);
    if (gameRound) {
      tft.setTextColor(0xFFE0, TFT_BLACK);
      tft.drawCentreString("Pick the paw color!", 160, 171, 2);
    } else {
      tft.setTextColor(0xFFE0, TFT_BLACK);
      tft.drawCentreString(notice.c_str(), 160, 171, 2);
    }
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("YOUR PET", 18, 37, 2);
    tft.drawString((String("Name: ") + pet.name).c_str(), 18, 63, 2);
    tft.drawString((String("Birthday: ") + pet.birthday).c_str(), 18, 87, 2);
    tft.drawString((String("Age: ") + String(pet.ageMinutes / 1440) + " days").c_str(), 18, 111, 2);
    tft.drawString((String("Care level: ") + String(pet.level)).c_str(), 18, 135, 2);
    tft.drawString("Bluetooth: TamaPets-CYD", 18, 159, 2);
    tft.drawString(pet.sitterCalled ? "Sitter: called" : "Sitter: not called", 18, 179, 2);
    tft.setTextColor(0xBDF7, TFT_BLACK);
    tft.drawString("NAME= / BIRTHDAY= / TIME= in Bluetooth", 18, 187, 1);
  }
  drawNavigation();
}

void careAction(const String& action) {
  if (pet.stage == STAGE_EGG) {
    showNotice("Your egg needs to hatch first");
    return;
  }
  if (action == "FEED") {
    pet.hunger = clampMeter(pet.hunger + 260);
    pet.energy = clampMeter(pet.energy + 40);
    showNotice("Nom nom! Thanks!");
  } else if (action == "PLAY") {
    pet.happiness = clampMeter(pet.happiness + 180);
    pet.energy = pet.energy > 140 ? pet.energy - 140 : 0;
    pet.hunger = pet.hunger > 70 ? pet.hunger - 70 : 0;
    showNotice("That was fun!");
  } else if (action == "TRAIN") {
    pet.care = clampMeter(pet.care + 100);
    pet.energy = pet.energy > 100 ? pet.energy - 100 : 0;
    pet.happiness = clampMeter(pet.happiness + 30);
    if (pet.care >= 500 && pet.stage == STAGE_KITTEN) {
      pet.stage = STAGE_CAT;
      showNotice("Your kitten grew up!");
    } else showNotice("Training complete!");
  }
  savePet();
}

void callSitter(bool called) {
  if (!called) {
    pet.sitterCalled = false;
    pet.sitterPenaltyPaid = false;
    if (currentScreen == SCREEN_SITTER) currentScreen = returnScreen;
    showNotice("Sitter Poketchi says goodbye");
    savePet();
    return;
  }

  if (!clockWasSet) {
    showNotice("Set clock before calling sitter");
    return;
  }
  if (localMinutes / 60 < 5 || localMinutes / 60 >= 19) {
    showNotice("Sitter works 5 AM to 7 PM");
    return;
  }

  pet.sitterCalled = true;
  pet.sitterPenaltyPaid = false;
  if (currentScreen != SCREEN_SITTER && currentScreen != SCREEN_SLEEP) returnScreen = currentScreen;
  if (returnScreen == SCREEN_SITTER || returnScreen == SCREEN_SLEEP) returnScreen = SCREEN_HOME;
  currentScreen = SCREEN_SITTER;
  showNotice("Poketchi is on the way!");
  savePet();
}

void sleepPet() {
  if (currentScreen != SCREEN_SLEEP && currentScreen != SCREEN_SITTER) returnScreen = currentScreen;
  if (returnScreen == SCREEN_SLEEP || returnScreen == SCREEN_SITTER) returnScreen = SCREEN_HOME;
  currentScreen = SCREEN_SLEEP;
  showNotice("Good night");
}

void wakePet() {
  if (currentScreen != SCREEN_SLEEP) return;
  currentScreen = returnScreen;
  showNotice("Good morning!");
}

void sendStatus() {
  bluetooth.printf("NAME=%s,STAGE=%u,LEVEL=%u,HUNGER=%u,HAPPINESS=%u,ENERGY=%u,CLOCK=%02u:%02u,DATE=%s,SITTER=%s\n",
                   pet.name, pet.stage, pet.level, pet.hunger / 10, pet.happiness / 10,
                   pet.energy / 10, localMinutes / 60, localMinutes % 60,
                   dateText().c_str(), pet.sitterCalled ? "CALLED" : "OFF");
}

void handleBluetoothCommand(String command) {
  command.trim();
  int equalsAt = command.indexOf('=');
  String key = equalsAt < 0 ? command : command.substring(0, equalsAt);
  key.toUpperCase();
  String value = equalsAt < 0 ? "" : command.substring(equalsAt + 1);

  if (key == "HELP") {
    bluetooth.println("STATUS | NAME=... | BIRTHDAY=MM/DD/YYYY | DATE=YYYY-MM-DD | TIME=HH:MM | WIFI_SSID=... | WIFI_PASS=... | OTA_PASS=8+ chars | WIFI_CONNECT | WIFI_OFF | OTA_STATUS | FEED | PLAY | TRAIN | SITTER=ON/OFF | SLEEP | WAKE | GAME");
  } else if (key == "STATUS") {
    sendStatus();
  } else if (key == "WIFI_SSID" && value.length() > 0 && value.length() <= 32) {
    if (otaEnabled) stopOtaWifi();
    otaWifiSsid = value;
    saveOtaSettings();
    bluetooth.println("OK WIFI_SSID; send WIFI_PASS then WIFI_CONNECT");
  } else if (key == "WIFI_PASS") {
    if (otaEnabled) stopOtaWifi();
    otaWifiPassword = value;
    saveOtaSettings();
    bluetooth.println("OK WIFI_PASS");
  } else if (key == "OTA_PASS" && value.length() >= 8 && value.length() <= 50) {
    otaPassword = value;
    saveOtaSettings();
    if (otaStarted) {
      ArduinoOTA.end();
      otaStarted = false;
    }
    bluetooth.println("OK OTA_PASS");
  } else if (key == "WIFI_CONNECT") {
    startOtaWifi();
  } else if (key == "WIFI_OFF") {
    stopOtaWifi();
    bluetooth.println("OK WIFI_OFF");
  } else if (key == "OTA_STATUS") {
    if (otaStarted) bluetooth.printf("OTA_READY=%s\n", WiFi.localIP().toString().c_str());
    else if (otaEnabled) bluetooth.println("OTA_CONNECTING");
    else bluetooth.println("OTA_OFF");
  } else if (key == "NAME" && value.length() > 0) {
    value.toCharArray(pet.name, sizeof(pet.name));
    showNotice(String("Hello, ") + pet.name + "!");
    savePet();
    bluetooth.println("OK NAME");
  } else if (key == "BIRTHDAY" && value.length() > 0 && value.length() < sizeof(pet.birthday)) {
    value.toCharArray(pet.birthday, sizeof(pet.birthday));
    savePet();
    bluetooth.println("OK BIRTHDAY");
  } else if (key == "DATE") {
    if (setDateFromText(value)) {
      savePet();
      bluetooth.println("OK DATE");
    } else bluetooth.println("ERROR use DATE=YYYY-MM-DD");
  } else if (key == "TIME") {
    int separator = value.indexOf(':');
    if (separator > 0) {
      int hours = value.substring(0, separator).toInt();
      int minutes = value.substring(separator + 1).toInt();
      if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60) {
        localMinutes = hours * 60 + minutes;
        clockWasSet = true;
        lastClockTick = millis();
        savePet();
        bluetooth.println("OK CLOCK");
      } else bluetooth.println("ERROR use TIME=HH:MM");
    } else bluetooth.println("ERROR use TIME=HH:MM");
  } else if (key == "FEED" || key == "PLAY" || key == "TRAIN") {
    careAction(key);
    bluetooth.println("OK " + key);
  } else if (key == "SITTER") {
    callSitter(value.equalsIgnoreCase("ON"));
    bluetooth.println("OK SITTER");
  } else if (key == "SLEEP") {
    sleepPet();
    bluetooth.println("OK SLEEP");
  } else if (key == "WAKE") {
    wakePet();
    bluetooth.println("OK WAKE");
  } else if (key == "GAME") {
    currentScreen = SCREEN_GAME;
    gameTarget = random(0, 3);
    gameRound = 1;
    bluetooth.println("OK GAME: tap color 0=red 1=green 2=blue");
  } else {
    bluetooth.println("ERROR unknown command; send HELP");
  }
  drawScreen();
}

void pollBluetooth() {
  while (bluetooth.available()) {
    char incoming = static_cast<char>(bluetooth.read());
    if (incoming == '\n' || incoming == '\r') {
      if (bluetoothLine.length()) handleBluetoothCommand(bluetoothLine);
      bluetoothLine = "";
    } else if (bluetoothLine.length() < 80) bluetoothLine += incoming;
  }
}

void handleTouch() {
  if (!touch.touched()) return;
  TS_Point point = touch.getPoint();
  uint16_t x = scaleX(point.x);
  uint16_t y = scaleY(point.y);
  delay(160);

  if (currentScreen == SCREEN_SLEEP) {
    wakePet();
  } else if (currentScreen == SCREEN_SITTER) {
    if (y >= 207) callSitter(false);
  } else if (currentScreen == SCREEN_HOME && pet.stage != STAGE_EGG &&
             x >= 239 && y >= 39 && y <= 71) {
    sleepPet();
  } else if (y >= 195) {
    Screen selectedScreen = static_cast<Screen>(min(3, static_cast<int>(x / 80)));
    if (selectedScreen == SCREEN_GAME && currentScreen != SCREEN_GAME) {
      gameTarget = random(0, 3);
      gameRound = 1;
    }
    currentScreen = selectedScreen;
  } else if (currentScreen == SCREEN_CARE && y >= 158) {
    if (x < 108) careAction("FEED");
    else if (x < 214) careAction("PLAY");
    else careAction("TRAIN");
  } else if (currentScreen == SCREEN_GAME && y >= 108 && y <= 162) {
    uint8_t choice = min(2, static_cast<int>(x / 100));
    if (choice == gameTarget) {
      pet.happiness = clampMeter(pet.happiness + 120);
      pet.energy = pet.energy > 60 ? pet.energy - 60 : 0;
      showNotice("Purrfect! You won!");
    } else {
      pet.happiness = pet.happiness > 50 ? pet.happiness - 50 : 0;
      showNotice("So close! Try again!");
    }
    gameRound = 0;
    savePet();
  }
  drawScreen();
  while (touch.touched()) delay(10);
}

void updatePet() {
  uint32_t now = millis();
  if (now - lastNeedTick >= NEED_TICK_MS) {
    uint32_t ticks = (now - lastNeedTick) / NEED_TICK_MS;
    lastNeedTick += ticks * NEED_TICK_MS;
    uint32_t daysElapsed = 0;
    if (clockWasSet) {
      uint32_t totalMinutes = localMinutes + ticks;
      daysElapsed = totalMinutes / 1440;
      localMinutes = totalMinutes % 1440;
      advanceDate(daysElapsed);
    }
    pet.ageMinutes += ticks;
    if (currentScreen == SCREEN_SLEEP) {
      pet.energy = clampMeter(pet.energy + ticks * 20);
    } else {
      pet.hunger = pet.hunger > ticks * 5 ? pet.hunger - ticks * 5 : 0;
      pet.happiness = pet.happiness > ticks * 3 ? pet.happiness - ticks * 3 : 0;
      pet.energy = pet.energy > ticks * 2 ? pet.energy - ticks * 2 : 0;
    }

    if (pet.stage == STAGE_EGG && clockWasSet) {
      pet.eggSeconds += ticks * 60;
      if (pet.eggSeconds >= EGG_HATCH_SECONDS) {
        pet.stage = STAGE_KITTEN;
        showNotice("Crack! Meet your new kitten!");
      }
    }

    if (pet.sitterCalled && clockWasSet) {
      uint32_t day = pet.ageMinutes / 1440;
      uint16_t hour = localMinutes / 60;
      if (day != pet.sitterDay) {
        pet.sitterDay = day;
        pet.sitterPenaltyPaid = false;
      }
      if (hour >= 19 && !pet.sitterPenaltyPaid) {
        pet.happiness = pet.happiness > 350 ? pet.happiness - 350 : 0;
        pet.sitterPenaltyPaid = true;
        pet.sitterCalled = false;
        showNotice("Poketchi: Pick me up sooner!");
        if (currentScreen == SCREEN_SITTER) currentScreen = returnScreen;
      }
    }
    if (pet.stage == STAGE_KITTEN && pet.ageMinutes >= 1440 * 2 && pet.care >= 300) {
      pet.stage = STAGE_CAT;
      showNotice("Your kitten grew up!");
    }
    if (pet.stage != STAGE_EGG && pet.ageMinutes > 0 && pet.ageMinutes % 1440 == 0 && pet.level < 10) {
      ++pet.level;
      showNotice("Happy birthday! Level up!");
    }
    if (noticeUntil && static_cast<int32_t>(now - noticeUntil) >= 0) notice = "Touch to care";
    drawScreen();
  }
  if (now - lastSave >= SAVE_INTERVAL_MS) savePet();
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  loadPet();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  tft.init();
  tft.setRotation(SCREEN_ROTATION);
  tft.invertDisplay(false);

  touchSpi.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI);
  touch.begin(touchSpi);
  touch.setRotation(SCREEN_ROTATION);

  bluetooth.begin("TamaPets-CYD");
  bluetooth.setTimeout(100);
  WiFi.mode(WIFI_OFF);
  lastNeedTick = millis();
  lastClockTick = millis();
  lastSave = millis();
  showNotice("Pair Bluetooth: TamaPets-CYD");
  drawScreen();
  if (!otaWifiSsid.isEmpty() && otaPassword.length() >= 8) startOtaWifi();
  Serial.println("TamaPets-CYD ready. Pair Bluetooth and send HELP.");
}

void loop() {
  pollBluetooth();
  serviceOta();
  handleTouch();
  updatePet();
  delay(10);
}