String version = "1.0";

// Source code - Technik Gesellenstück 2021/22 | Dennis

// The LEDs I bought are: https://amzn.to/3iY2oXx
// The LCD I bought is: https://amzn.to/3AEbbUj

// Libraries
#include <FastLED.h>
#include <TimerOne.h>
#include <BasicEncoder.h>
#include "Waveshare_LCD1602_RGB.h"

// Adressable LEDs
#define LED_PIN     5
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Normal LEDs
#define LED_red     9
#define LED_green   10
#define LED_blue    11
// The LEDs are on when LOW and off when HIGH.
// This allows me to not use resistors since I can use the Arduino's 3.3V.

// Grid
#define grid_x      6
#define grid_y      10

// RGB LCD
#define LCD_length  16
#define LCD_height  2

// Rotary Encoder
BasicEncoder encoder(12, 13);
void timer_service() {
  encoder.service();
}

// Adressable LEDs
CRGB leds[NUM_LEDS];
CRGB current_color;

// LCD
Waveshare_LCD1602_RGB lcd(LCD_length, LCD_height);

// MSGEQ7
#include "MSGEQ7.h"
#define pinAnalog A0
#define pinReset 3
#define pinStrobe 2
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH true

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalog> MSGEQ7;

// Rotary Encoder
#define pinSW 8
#define pinCLK 13
#define pinDT 12

// ------------------------------------------------------------------------------------------ //

// Setup function, executed on start
void setup() {
  // LCD setup
  lcd.init();
  lcd.setCursor(0, 0);
  lcd.send_string("      Juky      ");
  lcd.setCursor(0, 1);
  lcd.send_string("   Starting...  ");

  // Safety delay to give it some time.
  // (and to have a loading screen which looks cool)
  delay(500);

  // Adressable LED setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

  // LED setup
  pinMode(LED_red, OUTPUT);
  pinMode(LED_green, OUTPUT);
  pinMode(LED_blue, OUTPUT);
  digitalWrite(LED_red, HIGH);
  digitalWrite(LED_green, HIGH);
  digitalWrite(LED_blue, HIGH);

  // LCD clear
  lcd.setCursor(0, 0);
  lcd.send_string("      Juky      ");
  lcd.setCursor(0, 1);
  lcd.send_string("  Made with <3 ");

  delay(500);

  // Rotary Encoder setup
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_service);

  //MSGEQ7 setup
  MSGEQ7.begin();

  // Serial setup
  Serial.begin(9600);
}

// ------------------------------------------------------------------------------------------ //

// Effect settings
String allColorModes[7] = {"rgb", "r", "g", "b", "random", "rainbow-h", "rainbow-v"};

float effectSensitivity = 1;
int effectBrightness = 255;
int effectColorMode = 5;
bool effectSoundReactive = true;

// LCD settings
String LCDIdleText1 = "      Juky      ";
String LCDIdleText2 = "                ";
int LCDIdleTime = 5;

// LED settings
int LEDMinDelay = 50;
int LEDMaxDelay = 250;
int LEDIdleDelay = 1000;

int LEDSensitivity = 15;

// ------------------------------------------------------------------------------------------ //

// Vars for the Menu
int menuPage = 0;
int menuPageAmount = 1;
int menuItem = 0;
int menuEdit = false;

int idleBuffer = 0;
int millisBuffer = 0;

int encoder_countbefore = 0;
bool buttonBuffer = false;
bool buttonEvent = false;

// Menu for the LCD screen, executed every loop
void Menu_loop() {
  // Button management
  if (digitalRead(pinSW) == 0 && buttonBuffer == 1) {
    Serial.println(idleBuffer);
    // button even if display enabled
    if (idleBuffer > 0) {
      buttonEvent = true;
      // enable display if disabled
    } else if (idleBuffer < 0) {
      buttonEvent = true;
      menuItem = 0;
      menuPage = 0;
      menuEdit = false;
    }

    buttonBuffer = 0;

    // Only change numbers when the display was already on
    if (idleBuffer > 0) {
      // Main page click, send to second page
      if (menuItem == 0) {
        menuItem = 1;
      } else if (menuItem == 1) {
        menuItem = 0;

        // Second page click, enter / exit edit mode
      } else if (menuEdit == false) {
        menuEdit = true;
      } else if (menuEdit == true) {
        menuEdit = false;
      }
    } else {
      menuItem = 0;
      menuPage = 0;
      menuEdit = false;
    }

  } else if (digitalRead(pinSW) == 1) {
    buttonBuffer = 1;
  }

  // Moving through menu
  int encoder_change = encoder.get_change();
  if (encoder_countbefore != encoder.get_count() || buttonEvent) {
    if (idleBuffer < 0) {
      lcd.setCursor(0, 0);
      lcd.send_string("      Juky      ");
      lcd.setCursor(0, 1);
      lcd.send_string("      ....      ");
      idleBuffer = LCDIdleTime;
      menuItem = 0;
      menuPage = -1;
      menuEdit = false;
      if (buttonEvent) {
        encoder_countbefore = encoder.get_count() - 1;
      }
    } else {

      idleBuffer = LCDIdleTime;

      int now = (encoder_countbefore - encoder.get_count()) * -1;
      encoder_countbefore = encoder.get_count();

      if (menuItem == 0) {
        menuPage += now;
        if (menuPage > menuPageAmount) {
          menuPage = 0;
        } else if (menuPage < 0) {
          menuPage = menuPageAmount;
        }
      } else if (buttonEvent == false) {
        if (menuEdit == false) {
          menuItem += now;
          if (menuItem > 5) {
            menuItem = 1;
          } else if (menuItem < 1) {
            menuItem = 5;
          }
        }
      }

      // If it's item 1, always show the back button!
      if (menuItem == 1) {
        lcd.setCursor(0, 0);
        lcd.send_string("Back            ");
        lcd.setCursor(0, 1);
        lcd.send_string("                ");

        // Else, check for page

        // EFFECT SETTINGS
      } else if (menuPage == 0) {
        if (menuItem == 0) {
          lcd.setCursor(0, 0);
          lcd.send_string("Effect Settings");
          lcd.setCursor(0, 1);
          lcd.send_string("<              >");

          // SENSITIVITY
        } else if (menuItem == 2) {
          if (menuEdit == true) {
            effectSensitivity += now * 0.05;
            if (effectSensitivity <= 0) {
              effectSensitivity = 0;
            }
            lcd.setCursor(0, 1);
            lcd.send_string((String("* ") + String(effectSensitivity) + String("                ")).c_str());
          } else {
            lcd.setCursor(0, 0);
            lcd.send_string("Sensitivity     ");
            lcd.setCursor(0, 1);
            lcd.send_string((String(effectSensitivity) + String("                ")).c_str());
          }

          // BRIGHTNESS
        } else if (menuItem == 3) {
          if (menuEdit == true) {
            effectBrightness += now;
            if (effectBrightness <= 0) {
              effectBrightness = 0;
            } else if (effectBrightness >= 255) {
              effectBrightness = 255;
            }
            lcd.setCursor(0, 1);
            lcd.send_string((String("* ") + String(effectBrightness) + String("                ")).c_str());
          } else {
            lcd.setCursor(0, 0);
            lcd.send_string("Brightness      ");
            lcd.setCursor(0, 1);
            lcd.send_string((String(effectBrightness) + String("                ")).c_str());
          }

          // COLOR MODE
        } else if (menuItem == 4) {
          if (menuEdit == true) {
            if (now != 0) {
              effectColorMode += now;
              if (effectColorMode < 0) {
                effectColorMode = sizeof(allColorModes) / sizeof(String) - 1;
              } else if (effectColorMode >= sizeof(allColorModes) / sizeof(String)) {
                effectColorMode = 0;
              }
            }
            lcd.setCursor(0, 1);
            lcd.send_string((String("* ") + String(allColorModes[effectColorMode]) + String("                ")).c_str());
          } else {
            lcd.setCursor(0, 0);
            lcd.send_string("Color Mode      ");
            lcd.setCursor(0, 1);
            lcd.send_string((String(allColorModes[effectColorMode]) + String("                ")).c_str());
          }
          // SOUND REACTIVE
        } else if (menuItem == 5) {
          if (menuEdit == true) {
            if (now != 0) {
              effectSoundReactive = !effectSoundReactive;
            }
            lcd.setCursor(0, 1);
            if (effectSoundReactive == true) {
              lcd.send_string((String("* Yes") + String("                ")).c_str());
            } else {
              lcd.send_string((String("* No") + String("                ")).c_str());
            }
          } else {
            lcd.setCursor(0, 0);
            lcd.send_string("Sound Reactive  ");
            lcd.setCursor(0, 1);
            if (effectSoundReactive == true) {
              lcd.send_string((String("Yes") + String("                ")).c_str());
            } else {
              lcd.send_string((String("No") + String("                ")).c_str());
            }
          }
        }
      } else if (menuPage == 1) {
        if (menuItem == 0) {
          lcd.setCursor(0, 0);
          lcd.send_string("Info            ");
          lcd.setCursor(0, 1);
          lcd.send_string("<              >");
        } else if (menuItem == 2) {
          lcd.setCursor(0, 0);
          lcd.send_string("Chip            ");
          lcd.setCursor(0, 1);
          lcd.send_string("Arduino Uno     ");
        } else if (menuItem == 3) {
          lcd.setCursor(0, 0);
          lcd.send_string("Version         ");
          lcd.setCursor(0, 1);
          lcd.send_string(String(version + "                ").c_str());
        } else if (menuItem == 4) {
          lcd.setCursor(0, 0);
          lcd.send_string("Year            ");
          lcd.setCursor(0, 1);
          lcd.send_string("2021/22         ");
        }
      }
      buttonEvent = false;
    }
  }

  // Idle delay, show idle message
  if (millis() % 1000 <= 50 && menuEdit == false) {
    if (idleBuffer > 0) {
      idleBuffer -= 1;
    }
    if (idleBuffer == 0) {
      idleBuffer = -1;
      lcd.setCursor(0, 0);
      lcd.send_string(LCDIdleText1.c_str());
      lcd.setCursor(0, 1);
      lcd.send_string(LCDIdleText2.c_str());
    }
  }
}

// ------------------------------------------------------------------------------------------ //

// Vars needed for audio stuff
int visVal[5];
bool newReading;
uint8_t led_value;
uint8_t brightness;

// Loop function, loops constantly, calls all other loops
void loop()
{
  // MSGEQ7 read
  newReading = MSGEQ7.read(MSGEQ7_INTERVAL);
  led_value = (MSGEQ7.get(MSGEQ7_BASS) + MSGEQ7.get(MSGEQ7_1) + MSGEQ7.get(MSGEQ7_2) / 3) * 0.1 * MSGEQ7.getVolume() * 100;

  // The 245 needs to stay, trust me!
  visVal[5] = map(MSGEQ7.get(MSGEQ7_0) * effectSensitivity, 0, 245, 0, grid_y);
  visVal[4] = map(MSGEQ7.get(MSGEQ7_1) * effectSensitivity, 0, 245, 0, grid_y);
  visVal[3] = map(MSGEQ7.get(MSGEQ7_2) * effectSensitivity, 0, 245, 0, grid_y);
  visVal[2] = map(MSGEQ7.get(MSGEQ7_3) * effectSensitivity, 0, 245, 0, grid_y);
  visVal[1] = map(MSGEQ7.get(MSGEQ7_4) * effectSensitivity, 0, 245, 0, grid_y);
  visVal[0] = map((MSGEQ7.get(MSGEQ7_5) + MSGEQ7.get(MSGEQ7_6)) / 2 * effectSensitivity, 0, 245, 0, grid_y);

  for (int x = 0; x < grid_x; x++) {
    if (visVal[x] < 0) {
      visVal[x] = 0;
    }
  }

  // Bars effect loop call
  Eff_Bars_loop();

  // Menu loop - Interactions call
  Menu_loop();

  // LCD effect loop call
  Eff_LCD_loop();

  // ms delay after every loop
  delay(50);
}

// ------------------------------------------------------------------------------------------ //

// Vars for the LCD effect
int count = 1;

int prevmillis = 0;
int currentmillis = 0;
int millisdelay = 1000;

// LCD effect loop function, executed every loop
void Eff_LCD_loop() {
  // Brightness and speed of the LEDs
  if (effectSoundReactive == true) {
    led_value = led_value * LEDSensitivity;
    brightness = map(MSGEQ7.getVolume() * 1.5, 0, 255, 0, effectBrightness);
  } else {
    led_value = 0;
    brightness = effectBrightness;
  }

  millisdelay = map(led_value, 0, 128, LEDMaxDelay, LEDMinDelay);

  if (led_value < 25) {
    millisdelay = LEDIdleDelay;
    led_value = 50;
  }

  if (allColorModes[effectColorMode] == "r") {
    lcd.setRGB(brightness, 0, 0);
    analogWrite(LED_red, brightness);
    digitalWrite(LED_green, HIGH);
    digitalWrite(LED_blue, HIGH);
    current_color = CRGB(brightness, 0, 0);
  } else if (allColorModes[effectColorMode] == "g") {
    lcd.setRGB(0, brightness, 0);
    digitalWrite(LED_red, HIGH);
    analogWrite(LED_green, brightness);
    digitalWrite(LED_blue, HIGH);
    current_color = CRGB(0, brightness, 0);
  } else if (allColorModes[effectColorMode] == "b") {
    lcd.setRGB(0, brightness, brightness);
    digitalWrite(LED_red, HIGH);
    digitalWrite(LED_green, HIGH);
    analogWrite(LED_blue, brightness);
    current_color = CRGB(0, 0, brightness);
  } else if (allColorModes[effectColorMode] == "random") {
    lcd.setRGB(random(0, brightness), random(0, brightness), random(0, brightness));
    analogWrite(LED_red, random(0, brightness));
    analogWrite(LED_green, random(0, brightness));
    analogWrite(LED_blue, random(0, brightness));
    current_color = CRGB(random(0, brightness), random(0, brightness), random(0, brightness));
  } else {
    // Speed control of the LEDS
    currentmillis = millis();

    if (currentmillis - prevmillis > millisdelay) {
      prevmillis = currentmillis;
      count += 1;
      if (count >= 4)
        count = 1;
      MSGEQ7.reset();
    }

    // Set the colors
    if (count == 1) {
      lcd.setRGB(brightness, 0, 0);
      analogWrite(LED_red, brightness);
      digitalWrite(LED_green, HIGH);
      digitalWrite(LED_blue, HIGH);
      current_color = CRGB(brightness, 0, 0);
    } else if (count == 2) {
      lcd.setRGB(0, brightness, 0);
      digitalWrite(LED_red, HIGH);
      analogWrite(LED_green, brightness);
      digitalWrite(LED_blue, HIGH);
      current_color = CRGB(0, brightness, 0);
    } else if (count == 3) {
      lcd.setRGB(0, brightness, brightness);
      digitalWrite(LED_red, HIGH);
      digitalWrite(LED_green, HIGH);
      analogWrite(LED_blue, brightness);
      current_color = CRGB(0, 0, brightness);
    }
  }
}

// ------------------------------------------------------------------------------------------ //

// Debug loop, shows the order of LEDs, useful for debug.
void Eff_Debug_loop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    int v = i * 3 + 10;
    if (i <= 17) {
      leds[i] = CRGB(v, v, v);
    } else if (i <= 29) {
      leds[i] = CRGB(0, 0, v);
    } else if (i <= 41) {
      leds[i] = CRGB(0, v, 0);
    } else {
      leds[i] = CRGB(v, 0, 0);
    }
  }
  FastLED.show();
}

// ------------------------------------------------------------------------------------------ //

// Bars effect loop function, executed every loop
void Eff_Bars_loop() {
  for (int x = 0; x < grid_x; x++) {
    for (int y = 0; y < grid_y; y++) {
      if (y < visVal[x]) {
        if (allColorModes[effectColorMode] == "rainbow-h") {
          CRGB rainbow[grid_x] = {CRGB(brightness / 2, 0, brightness), CRGB(0, 0, brightness), CRGB(0, brightness, brightness / 2),
                                  CRGB(0, brightness, 0), CRGB(brightness, brightness, 0), CRGB(brightness, 0, 0)
                                 };
          leds[x + (y * grid_x)] = rainbow[x];
        } else if (allColorModes[effectColorMode] == "rainbow-v") {
          CRGB rainbow[grid_y] = {CRGB(brightness, 0, 0), CRGB(brightness, brightness / 2, 0), CRGB(brightness, brightness, 0),
                                  CRGB(0, brightness, 0), CRGB(0, brightness, brightness / 4), CRGB(0, brightness, brightness / 2),
                                  CRGB(0, brightness, brightness), CRGB(0, 0, brightness), CRGB(brightness / 2, 0, brightness),
                                  CRGB(brightness, 0, brightness)
                                 };
          leds[x + (y * grid_x)] = rainbow[y];
        } else {
          leds[x + (y * grid_x)] = current_color;
        }
      } else {
        leds[x + (y * grid_x)] = CRGB(0, 0, 0);
      }
    }
  }
  FastLED.show();
}
