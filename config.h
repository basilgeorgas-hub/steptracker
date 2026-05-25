#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino_GFX_Library.h>
#include <math.h>

// -------- Pins --------
#define BUTTON_PIN     2
#define ACCEL_X_PIN    A2
#define ACCEL_Y_PIN    A1
#define ACCEL_Z_PIN    A0
#define LED_RED_PIN    5
#define LED_YELLOW_PIN 4
#define LED_GREEN_PIN  3
#define TFT_DC         9
#define TFT_CS         8
#define TFT_RST        10
#define ST_PIN         6

// -------- Display constants --------
#define CX      120
#define CY      120
#define RADIUS  100

// -------- Colours (RGB565) --------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define GREEN   0x07E0
#define RED     0xF800
#define YELLOW  0xFFE0
#define GREY    0x4208
#define LIGHT_GREY  0xC618

// -------- Calibration --------
#define SAMPLES 100

// -------- Step detection --------
#define STEP_THRESHOLD_HIGH  1.4
#define STEP_THRESHOLD_LOW   0.70
#define STEP_MIN_INTERVAL_MS 250
#define STEP_MAX_INTERVAL_MS 3000

// -------- Pace classification --------
#define PACE_STATIONARY     0
#define PACE_WALKING        1
#define PACE_RUNNING        2
#define CADENCE_RUNNING_MIN 140

// -------- Screen states --------
#define SCREEN_STEPS    0
#define SCREEN_ANALOG   1
#define SCREEN_DIGITAL  2
#define SCREEN_ATTITUDE 3

// -------- Menu --------
#define MENU_SELF_TEST    0
#define MENU_CALIBRATION  1
#define MENU_WIFI         2
#define MENU_ITEM_COUNT   3

// -------- Attitude indicator --------
#define SKY_COLOUR      0x1C9F
#define EARTH_COLOUR    0x8A02
#define AI_PIX_PER_DEG  2.0f
#define AI_ARC_RADIUS   88

// -------- Button timing --------
#define LONG_PRESS_MS   2000
#define DEBOUNCE_MS     50

// -------- Shared globals --------
extern Arduino_DataBus *bus;
extern Arduino_GC9A01  *gfx;

extern float offsetX, offsetY, offsetZ;
extern float sensitivityX, sensitivityY, sensitivityZ;
extern long  avg[3][3];

extern int           steps;
extern bool          stepPending;
extern unsigned long lastStepTime;
extern unsigned long lastStepInterval;
extern int           displayStepCount;

extern int  paceState;
extern int  currentScreen;
extern bool selfTestRunning;

// -------- Menu / navigation --------
extern bool inMenu;
extern int  menuItem;
extern bool wifiEnabled;

// -------- WiFi / NTP --------
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

char ssid[] = "harrison worrall";
char pass[] = "ffffffff";

#define NTP_UTC_OFFSET_SEC   36000
#define WIFI_CONNECT_TIMEOUT 15000UL
#define WIFI_RETRY_INTERVAL  120000UL

#define WIFI_IDLE        0
#define WIFI_CONNECTING  1
#define WIFI_CONNECTED   2
#define WIFI_FAILED      3

extern NTPClient timeClient;
extern int       wifiState;

#endif
