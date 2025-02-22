#define version "2.0"

// Source code - Technik Gesellenstück 2021/22
// 2025 Rewrite

// The LEDs: https://amzn.to/3iY2oXx
// The LCD: https://amzn.to/3AEbbUj

// Libraries
#include <BasicEncoder.h>
#include <FastLED.h>
#include <MSGEQ7.h>
#include <TimerOne.h>
#include <Waveshare_LCD1602_RGB.h>  // Included in the repo as a .zip

// Adressable LEDs
#define LED_PIN 5
#define NUM_LEDS 60
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// Display grid
#define grid_x 6
#define grid_y 10
#define FPS 50
#define targetMillis (1000 / FPS)

// LCD
#define LCD_length 16
#define LCD_height 2

Waveshare_LCD1602_RGB lcd(LCD_length, LCD_height);

// MSGEQ7
#define pinAnalog A0
#define pinReset 3
#define pinStrobe 2
#define MSGEQ7_INTERVAL ReadsPerSecond(FPS)
#define MSGEQ7_SMOOTH min(255, FPS * 2)

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalog> MSGEQ7;

// Rotary Encoder
#define pinSW 8
#define pinCLK 13
#define pinDT 12
BasicEncoder encoder(12, 13);
void timer_service() { encoder.service(); }

// -- VARIABLES -- //

// LCD vars
String lcdBuffer = "";
byte cursor = 0;
const char* lines[9] = {
    "o O o O o O o O ", "*-*-*-*-*-*-*-* ", "----------------",
    "~ ~ ~ ~ ~ ~ ~ ~ ", "+-+-+-+-+-+-+-+ ", "::::::::::::::::",
    ">>> >>> >>> >>> ", " #### #### #### ", "!!! !!! !!! !!! ",
};

// -- HELPER FUNCTIONS -- //

void addToLCDBuffer(String str) {
  if (str == "\f")
    lcdBuffer += "                ";  // Clear 1 line from cursor
  else if (str == "\0")
    lcdBuffer +=
        "                                 ";  // Clear 2 lines from cursor
  else
    lcdBuffer += str;
}

int mapThreshold(int val, int threshold) {
  return map(val - min(val, threshold), 0, 255 - threshold, 0, 255);
}

int mapLimits(int val, int min, int max) { return map(val, 0, 255, min, max); }

bool buttonBuffer = false;
bool buttonPressed = false;

bool button() {
  if (!digitalRead(pinSW) && !buttonBuffer) {
    buttonBuffer = true;
    buttonPressed = true;
    return;
  }

  if (digitalRead(pinSW) && buttonBuffer) {
    buttonBuffer = false;
  }
  buttonPressed = false;
}

// -- STARTUP -- //

void setup() {
  // LCD setup
  lcd.init();

  // Adressable LED setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.clear();
  FastLED.show();

  // Rotary Encoder setup
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_service);

  // MSGEQ7 setup
  MSGEQ7.begin();

  // Nice looking fake loading screen
  lcd.setRGB(255, 0, 128);
  lcd.setCursor(0, 0);
  lcd.send_string("   -< Juky >-   ");
  lcd.setCursor(0, 1);
  lcd.send_string("Audio Visualizer");

  delay(500);

  lcd.setCursor(0, 0);
  lcd.send_string("Made by Samy  <3");
  lcd.setCursor(0, 1);
  lcd.send_string("https://zohiu.de");

  delay(1000);

  addToLCDBuffer("\r");
  addToLCDBuffer("\0");
}

// -- SETTINGS -- //

struct Settings {
  // Audio detection
  byte silenceThreshold = 40;
  float sensitivity = 1.0;

  // Display
  bool reactiveBrightness = true;
  byte idleBrightness = 10;
  byte maxBrightness = 180;

  // Colors
  byte saturation = 175;
  byte hsvOffset = 200;
  float hsvScale = 0.3;
  float hsvSpeed = 0.1;
  float rotation = 0.7;
  float randomization = 0.0;
};

Settings settings;

enum SettingType { BOOL, FLOAT, BYTE, PERCENTAGE };

struct Setting {
  void* pointer;
  String name;
  SettingType type;
};

class Page {
 public:
  virtual String getTitle() { return title; }
  virtual Setting* getMembers() { return members; }
  virtual int getMembersSize() { return membersSize; }

 protected:
  String title = "";
  Setting* members;
  int membersSize;
};

class AudioPage : public Page {
 public:
  AudioPage() {
    title = "Audio";
    membersSize = 2;
    members = new Setting[membersSize]{
        {&settings.silenceThreshold, "Audio threshold", BYTE},
        {&settings.sensitivity, "Sensitivity", FLOAT}};
  }
  ~AudioPage() { delete[] members; }
};

class DisplayPage : public Page {
 public:
  DisplayPage() {
    title = "Display";
    membersSize = 3;
    members = new Setting[membersSize]{
        {&settings.reactiveBrightness, "React brightness", BOOL},
        {&settings.maxBrightness, "Max brightness", BYTE},
        {&settings.idleBrightness, "Idle brightness", BYTE}};
  }
  ~DisplayPage() { delete[] members; }
};

class ColorsPage : public Page {
 public:
  ColorsPage() {
    title = "Colors";
    membersSize = 6;
    members = new Setting[membersSize]{
        {&settings.saturation, "Saturation", BYTE},
        {&settings.hsvOffset, "HSV offset", BYTE},
        {&settings.hsvScale, "HSV scale", FLOAT},
        {&settings.hsvSpeed, "HSV speed", FLOAT},
        {&settings.rotation, "Rotation", PERCENTAGE},
        {&settings.randomization, "Randomization", PERCENTAGE}};
  }
  ~ColorsPage() { delete[] members; }
};

bool settingsOpen = false;
bool pageOpen = false;
bool changingValue = false;
bool forceShow = false;
int currentPage = 0;
int currentSetting = 0;
unsigned long lastInput;

Page* pages[] = {new ColorsPage(), new DisplayPage(), new AudioPage()};
int pagesSize = sizeof(pages) / sizeof(pages[0]);

void openSettings() {
  settingsOpen = true;
  pageOpen = false;
  currentPage = 0;
  lcdBuffer = "";
  lcd.setRGB(255, 0, 128);
  addToLCDBuffer(String("\r"));
  addToLCDBuffer(String("Settings        "));
  addToLCDBuffer("\b");
  addToLCDBuffer("\f");
  addToLCDBuffer("\b");
  addToLCDBuffer(pages[0]->getTitle());
  encoder.reset();
  lastInput = millis();
}

void updateSettings() {
  if (millis() - lastInput > 10000 && !changingValue) {
    settingsOpen = false;
    return;
  }

  int change = encoder.get_change();
  if (change == 0 && !buttonPressed && !forceShow) return;
  lastInput = millis();

  if (!pageOpen) {
    if (buttonPressed) {
      pageOpen = true;
      currentSetting = 0;
      buttonPressed = false;
      forceShow = true;
      return;
    }

    // Page selector
    if (currentPage == max(0, min(currentPage + change, pagesSize - 1)) &&
        !forceShow)
      return;
    currentPage = max(0, min(currentPage + change, pagesSize - 1));
    lcdBuffer = "";
    addToLCDBuffer("\b");
    addToLCDBuffer(pages[currentPage]->getTitle());
    addToLCDBuffer("\f");
    addToLCDBuffer(String("\r"));
    addToLCDBuffer(String("Settings        "));
    forceShow = false;
    return;
  }

  if (buttonPressed) {
    changingValue = !changingValue;
    buttonPressed = false;
    forceShow = true;
    encoder.reset();
  }

  // Setting selector
  if (!changingValue) {
    if (currentSetting ==
            max(-1, min(currentSetting + change,
                        pages[currentPage]->getMembersSize() - 1)) &&
        !forceShow)
      return;
    currentSetting = max(-1, min(currentSetting + change,
                                 pages[currentPage]->getMembersSize() - 1));
  }

  lcdBuffer = "";

  // Back button is page -1
  if (currentSetting == -1) {
    if (changingValue) {  // Changing value = button press
      pageOpen = false;
      forceShow = true;
      currentSetting = 0;
      changingValue = false;
      return;
    }
    addToLCDBuffer("\r");
    addToLCDBuffer("\f");
    addToLCDBuffer("\b");
    addToLCDBuffer("Back");
    addToLCDBuffer("\f");
    return;
  }

  if (!changingValue) {
    addToLCDBuffer("\r");
    addToLCDBuffer(pages[currentPage]->getMembers()[currentSetting].name);
    addToLCDBuffer("\f");
  }

  addToLCDBuffer("\b");
  if (changingValue) addToLCDBuffer("> ");
  // Types
  switch (pages[currentPage]->getMembers()[currentSetting].type) {
    case BOOL: {
      bool* value = pages[currentPage]->getMembers()[currentSetting].pointer;
      if (changingValue && change != 0) *value = !*value;
      *value ? addToLCDBuffer("Yes") : addToLCDBuffer("No");
      break;
    }
    case FLOAT: {
      float* value = pages[currentPage]->getMembers()[currentSetting].pointer;
      if (changingValue) *value += change * 0.05;
      addToLCDBuffer(String(*value));
      break;
    }
    case BYTE: {
      byte* value = pages[currentPage]->getMembers()[currentSetting].pointer;
      if (changingValue) *value += change * 5;
      addToLCDBuffer(String(*value));
      break;
    }
    case PERCENTAGE: {
      float* value = pages[currentPage]->getMembers()[currentSetting].pointer;
      if (changingValue) *value = max(0.0, min(*value + change * 0.05, 1.0));
      addToLCDBuffer(String((int)(*value * 100)));
      addToLCDBuffer("%");
      break;
    }
  }
  if (changingValue) addToLCDBuffer(" <");
  addToLCDBuffer("\f");
  forceShow = false;
}

// -- MAIN LOOP -- //

void loop() {
  unsigned long timeBeforeExecution = millis();

  // If new values are available
  if (MSGEQ7.read(MSGEQ7_INTERVAL)) {
    updateDisplay();
  }

  button();

  if (buttonPressed && !settingsOpen) {
    buttonPressed = false;
    openSettings();
  }

  settingsOpen ? updateSettings() : updateLCD();

  tickLCD();

  delay(targetMillis -
        min(targetMillis, millis() - min(millis(), timeBeforeExecution)));
}

// -- DISPLAY FUNCTIONS -- //

float calculateHue(byte x, byte y) {
  return settings.hsvOffset +
         map(((settings.rotation + (5 * settings.randomization)) * x) +
                 ((1 - (settings.rotation + (5 * settings.randomization))) * y),
             0,
             (settings.rotation * grid_x) + ((1 - settings.rotation) * grid_y),
             0, 255) *
             settings.hsvScale +
         (settings.hsvSpeed * millis() / 10);
}

byte calculateBrightness() {
  if (settings.reactiveBrightness) {
    return mapLimits(
        mapThreshold(MSGEQ7.getVolume(), settings.silenceThreshold),
        settings.idleBrightness, settings.maxBrightness);
  } else
    return settings.maxBrightness;
}

void updateDisplay() {
  FastLED.clear();
  for (int x = 0; x < grid_x; x++) {
    for (byte y = mapLimits(
             mapThreshold(
                 min(255, MSGEQ7.get(grid_x - x - 1) * settings.sensitivity + 15),
                 settings.silenceThreshold),
             0, grid_y);
         y > 0; y--) {
      leds[x + ((y - 1) * grid_x)] =
          CHSV(calculateHue(x, y), settings.saturation, calculateBrightness());
    }
  }
  FastLED.show();
}

// -- LCD FUNCTIONS -- //

void updateLCD() {
  // Takes the closest display color
  CRGB lcdColor =
      hsv2rgb_spectrum(CHSV(calculateHue(1, 0), 255, calculateBrightness()));
  lcd.setRGB(lcdColor.r, lcdColor.g, lcdColor.b);
  if (lcdBuffer.length() == 0) {
    addToLCDBuffer("\r");
    addToLCDBuffer(lines[random(0, 9)]);
    addToLCDBuffer("\r");
    addToLCDBuffer("\f");
    addToLCDBuffer("\b");
    addToLCDBuffer(lines[random(0, 9)]);
    addToLCDBuffer("\b");
    addToLCDBuffer("\f");
  }
}

void tickLCD() {
  if (lcdBuffer.length() == 0) return;
  String substring = lcdBuffer.substring(0, 1);
  lcdBuffer.remove(0, 1);

  if (substring == "\r") {
    cursor = 0;
    return;
  } else if (substring == "\b") {
    cursor = 17;
    return;
  }

  cursor <= 16 ? lcd.setCursor(cursor, 0) : lcd.setCursor(cursor - 17, 1);
  lcd.send_string(substring.c_str());

  cursor++;
}
