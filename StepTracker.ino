#include "config.h"

// -------- Global definitions --------
Arduino_DataBus *bus;
Arduino_GC9A01  *gfx;

float offsetX,      offsetY,      offsetZ;
float sensitivityX, sensitivityY, sensitivityZ;
long  avg[3][3];

int           steps            = 0;
bool          stepPending      = false;
unsigned long lastStepTime     = 0;
unsigned long lastStepInterval = 0;
int           displayStepCount = 0;

int  paceState       = PACE_STATIONARY;
int  currentScreen   = SCREEN_STEPS;
bool selfTestRunning = false;

bool inMenu     = false;
int  menuItem   = 0;
bool wifiEnabled = false;

// ============================================================
//  cycleScreen()
//  Advances to the next valid screen.
//  Clock screens are skipped when WiFi is not enabled.
// ============================================================
void cycleScreen() {
    if (wifiEnabled) {
        currentScreen = (currentScreen + 1) % 4;
    } else {
        // Only cycle between steps and attitude — clocks need WiFi
        currentScreen = (currentScreen == SCREEN_STEPS) ? SCREEN_ATTITUDE : SCREEN_STEPS;
    }
    drawCurrentScreen();
}

// ============================================================
//  selectMenuItem()
//  Executes the highlighted menu option then returns to
//  the step screen.
// ============================================================
void selectMenuItem() {
    inMenu = false;

    if (menuItem == MENU_SELF_TEST) {
        currentScreen = SCREEN_STEPS;
        runSelfTest();   // draws its own UI, calls drawCurrentScreen() at end

    } else if (menuItem == MENU_CALIBRATION) {
        runCalibration();
        currentScreen = SCREEN_STEPS;
        drawStepScreen();

    } else {
        // MENU_WIFI — toggle
        if (!wifiEnabled) {
            // Draw step screen first so it's visible before the freeze
            currentScreen = SCREEN_STEPS;
            drawStepScreen();
            enableWifi();   // sets wifiEnabled=true and kicks off connection
        } else {
            disableWifi();  // sets wifiEnabled=false, disconnects
            currentScreen = SCREEN_STEPS;
            drawStepScreen();
        }
    }
}

// ============================================================
//  checkButton()
//  Short press: cycle menu items (in menu) or cycle screens
//  Long press:  select menu item (in menu) or open menu
// ============================================================
void checkButton() {
    static unsigned long pressStart = 0;
    static bool          wasPressed = false;
    static bool          longFired  = false;

    int state = digitalRead(BUTTON_PIN);

    if (state == HIGH && !wasPressed) {
        pressStart = millis();
        wasPressed = true;
        longFired  = false;
    }

    if (wasPressed && !longFired &&
        (millis() - pressStart) >= LONG_PRESS_MS) {
        longFired = true;

        if (inMenu) {
            selectMenuItem();        // long press in menu = select
        } else {
            inMenu   = true;         // long press in operational screen = open menu
            menuItem = 0;
            drawMenuScreen();
        }
    }

    if (state == LOW && wasPressed) {
        unsigned long holdTime = millis() - pressStart;
        wasPressed = false;

        if (!longFired && holdTime >= DEBOUNCE_MS) {
            if (inMenu) {
                menuItem = (menuItem + 1) % MENU_ITEM_COUNT;
                drawMenuScreen();
            } else {
                cycleScreen();
            }
        }
    }
}

// ============================================================
//  drawCurrentScreen()
// ============================================================
void drawCurrentScreen() {
    if (currentScreen == SCREEN_STEPS) {
        drawStepScreen();
    } else if (currentScreen == SCREEN_ANALOG) {
        drawAnalogClock();
    } else if (currentScreen == SCREEN_DIGITAL) {
        drawDigitalClockLayout();
    } else {
        float r, p;
        readAttitudeAngles(r, p);
        drawAttitudeScreen(r, p);
    }
}

// ============================================================
void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN,    INPUT);
    pinMode(LED_RED_PIN,   OUTPUT);
    pinMode(LED_YELLOW_PIN,OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(ST_PIN,        OUTPUT);
    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);
    digitalWrite(ST_PIN,         HIGH);

    bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
    gfx = new Arduino_GC9A01(bus, TFT_RST, 3, true);
    gfx->begin(20000000UL);
    gfx->fillScreen(BLACK);

    runCalibration();

    // Show menu after calibration instead of going straight to step screen
    inMenu   = true;
    menuItem = 0;
    drawMenuScreen();
}

// ============================================================
void loop() {
    checkButton();

    // Step detection and pace always run regardless of menu state —
    // the step count accumulates in the background
    updateStepDetection();
    updatePaceState();

    if (wifiEnabled) updateWifi();

    // Update display step count variable
    static unsigned long prevStepInterval = 0;
    static int           lastRawSteps     = 0;

    if (steps != lastRawSteps) {
        if (lastStepInterval < 700 && prevStepInterval >= 700) {
            // x2 → x1 transition, skip
        } else if (lastStepInterval < 700) {
            displayStepCount += 1;
        } else {
            displayStepCount += 2;
        }
        prevStepInterval = lastStepInterval;
        lastRawSteps     = steps;
    }

    // Screen content updates only when not in menu
    if (!inMenu) {
        if (currentScreen == SCREEN_STEPS) {
            updateStepScreen();
        } else if (currentScreen == SCREEN_ANALOG) {
            updateAnalogClock();
        } else if (currentScreen == SCREEN_DIGITAL) {
            updateDigitalClock();
        } else if (currentScreen == SCREEN_ATTITUDE) {
            updateAttitudeScreen();
        }
    }

    delay(20);
}