// ============================================================
//  LCD.ino
//  Screen drawing functions.
//
//  SCREEN_STEPS   — step counter with pace label
//  SCREEN_ANALOG  — placeholder until WiFi/NTP implemented
//  SCREEN_DIGITAL — placeholder until WiFi/NTP implemented
// ============================================================

#include "config.h"
static float oldSecA = 0.0f;
// ============================================================
//  STEP COUNTER SCREEN
// ============================================================
void drawStepScreen() {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("STEPS", 55, GREEN, 2);

    char stepBuf[8];
    itoa(displayStepCount, stepBuf, 10);
    int w = strlen(stepBuf) * 24;
    gfx->setTextColor(WHITE);
    gfx->setTextSize(4);
    gfx->setCursor(CX - w / 2, 88);
    gfx->println(stepBuf);

    const char    *labels[]  = { "STATIONARY", "WALKING",  "RUNNING" };
    const uint16_t colours[] = { RED,           YELLOW,     GREEN    };
    printCentred(labels[paceState], 155, colours[paceState], 1);

    // WiFi icon — bottom right inside circle, clear of all other elements
    gfx->fillRect(150, 104, 68, 32, BLACK);
    if (wifiState == WIFI_CONNECTED) {
        drawWifiIcon(183, 120, GREEN);
    } else if (wifiState == WIFI_CONNECTING) {
        gfx->setTextSize(1);
        gfx->setTextColor(YELLOW);
        gfx->setCursor(171, 111);    // "WiFi" centred at x=183
        gfx->print("WiFi");
        gfx->setCursor(153, 121);    // "connecting" centred at x=183
        gfx->print("connecting");
    }

}

void updateStepScreen() {
    static int lastDisplayedSteps = -1;
    static int lastDisplayedPace  = -1;
    static int lastWifiState      = -1;

    if (displayStepCount == lastDisplayedSteps &&
        paceState        == lastDisplayedPace  &&
        wifiState        == lastWifiState) return;

    lastDisplayedSteps = displayStepCount;
    lastDisplayedPace  = paceState;
    lastWifiState      = wifiState;

    gfx->fillRect(30, 80, 180, 52, BLACK);
    char stepBuf[8];
    itoa(displayStepCount, stepBuf, 10);
    int w = strlen(stepBuf) * 24;
    gfx->setTextColor(WHITE);
    gfx->setTextSize(4);
    gfx->setCursor(CX - w / 2, 88);
    gfx->println(stepBuf);

    gfx->fillRect(20, 148, 200, 20, BLACK);
    const char    *labels[]  = { "STATIONARY", "WALKING",  "RUNNING" };
    const uint16_t colours[] = { RED,           YELLOW,     GREEN    };
    printCentred(labels[paceState], 152, colours[paceState], 1);

    // Refresh WiFi icon area
    gfx->fillRect(150, 104, 68, 32, BLACK);
    if (wifiState == WIFI_CONNECTED) {
        drawWifiIcon(183, 120, GREEN);
    } else if (wifiState == WIFI_CONNECTING) {
        gfx->setTextSize(1);
        gfx->setTextColor(YELLOW);
        gfx->setCursor(171, 111);    // "WiFi" centred at x=183
        gfx->print("WiFi");
        gfx->setCursor(153, 121);    // "connecting" centred at x=183
        gfx->print("connecting");
    }

    setLEDs(paceState == PACE_STATIONARY,
            paceState == PACE_WALKING,
            paceState == PACE_RUNNING);
}

// ============================================================
//  ANALOG CLOCK
// ============================================================

static int  oldHrX,  oldHrY;
static int  oldMinX, oldMinY;
static int  oldSecX, oldSecY;
static bool analogFirstDraw = true;

// Draws the static face — tick marks and hour numbers.
// Called on full redraws and after erasing hands (which can clip ticks).
static void drawClockFace() {
    gfx->drawCircle(CX, CY, RADIUS, WHITE);

    // Minute ticks — 60 positions, skip the 12 hour positions
    for (int i = 0; i < 60; i++) {
        if (i % 5 == 0) continue;
        float a = i * 6.0f * (PI / 180.0f);
        gfx->drawLine(CX + (int)(sinf(a) * 93), CY - (int)(cosf(a) * 93),
                      CX + (int)(sinf(a) * 97), CY - (int)(cosf(a) * 97), GREY);
    }

    // Hour ticks — longer
    for (int i = 0; i < 12; i++) {
        float a = i * 30.0f * (PI / 180.0f);
        gfx->drawLine(CX + (int)(sinf(a) * 86), CY - (int)(cosf(a) * 86),
                      CX + (int)(sinf(a) * 97), CY - (int)(cosf(a) * 97), WHITE);
    }

    // Hour numbers
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    for (int n = 1; n <= 12; n++) {
        float a  = n * 30.0f * (PI / 180.0f);
        int   tx = CX + (int)(sinf(a) * 70) - (n < 10 ? 6 : 12);
        int   ty = CY - (int)(cosf(a) * 70) - 8;
        gfx->setCursor(tx, ty);
        gfx->println(n);
    }
}

void drawAnalogClock() {
    gfx->fillScreen(BLACK);
    drawClockFace();
    analogFirstDraw = true;
}

void updateAnalogClock() {
    static bool noWifiShown = false;

    if (wifiState != WIFI_CONNECTED) {
        if (!noWifiShown) {
            gfx->fillScreen(BLACK);
            drawClockFace();
            printCentred("NO WiFi", 112, GREY, 2);
            noWifiShown     = true;
            analogFirstDraw = true;   // force full hand draw when WiFi arrives
        }
        return;
    }

    if (noWifiShown) {
        // WiFi just connected while on this screen — rebuild face
        noWifiShown = false;
        gfx->fillScreen(BLACK);
        drawClockFace();
    }

    if (WiFi.status() != WL_CONNECTED) {
        wifiForceRetry();
        return;
    }
    timeClient.update();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 1000) return;
    lastUpdate = millis();

    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    int s = timeClient.getSeconds();

    time_t    epoch    = (time_t)timeClient.getEpochTime();
    struct tm *ti      = gmtime(&epoch);
    const char *monNames[] = { "JAN","FEB","MAR","APR","MAY","JUN",
                                "JUL","AUG","SEP","OCT","NOV","DEC" };

    // Erase previous hands by drawing in black, then restore the face
    // (hands can clip tick marks and numbers)
    if (!analogFirstDraw) {
        // Second hand + tail
        gfx->drawLine(CX - (int)(sinf(oldSecA) * 15),
                      CY + (int)(cosf(oldSecA) * 15),
                      CX + (int)(sinf(oldSecA) * 82),
                      CY - (int)(cosf(oldSecA) * 82), BLACK);
        // Minute (2 px wide)
        gfx->drawLine(CX,   CY,   oldMinX,   oldMinY, BLACK);
        gfx->drawLine(CX+1, CY,   oldMinX+1, oldMinY, BLACK);
        // Hour (3 px wide)
        gfx->drawLine(CX,   CY,   oldHrX,   oldHrY, BLACK);
        gfx->drawLine(CX+1, CY,   oldHrX+1, oldHrY, BLACK);
        gfx->drawLine(CX,   CY+1, oldHrX,   oldHrY+1, BLACK);
        drawClockFace();
    }

    char dateBuf[7];
    snprintf(dateBuf, sizeof(dateBuf), "%02d %s", ti->tm_mday, monNames[ti->tm_mon]);
    gfx->setTextSize(1);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(CX - (strlen(dateBuf) * 3), 163);
    gfx->println(dateBuf);

    float secA = s * 6.0f               * (PI / 180.0f);
    float minA = (m * 6.0f  + s * 0.1f) * (PI / 180.0f);
    float hrA  = ((h % 12) * 30.0f + m * 0.5f) * (PI / 180.0f);

    int sx = CX + (int)(sinf(secA) * 82),  sy = CY - (int)(cosf(secA) * 82);
    int mx = CX + (int)(sinf(minA) * 65),  my = CY - (int)(cosf(minA) * 65);
    int hx = CX + (int)(sinf(hrA)  * 44),  hy = CY - (int)(cosf(hrA)  * 44);

    // Hour hand — white, 3 px
    gfx->drawLine(CX,   CY,   hx,   hy,   WHITE);
    gfx->drawLine(CX+1, CY,   hx+1, hy,   WHITE);
    gfx->drawLine(CX,   CY+1, hx,   hy+1, WHITE);

    // Minute hand — green, 2 px
    gfx->drawLine(CX,   CY, mx,   my, GREEN);
    gfx->drawLine(CX+1, CY, mx+1, my, GREEN);

    // Second hand — red, thin, with a short counterbalance tail
    gfx->drawLine(CX - (int)(sinf(secA) * 15),
                  CY + (int)(cosf(secA) * 15),
                  sx, sy, RED);

    gfx->fillCircle(CX, CY, 4, WHITE);   // centre boss

    oldHrX  = hx; oldHrY  = hy;
    oldMinX = mx; oldMinY = my;
    oldSecX = sx; oldSecY = sy;
    oldSecA = secA;                       // save angle for erase
    analogFirstDraw = false;
}

// ============================================================
//  DIGITAL CLOCK
// ============================================================

void drawDigitalClockLayout() {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    // updateDigitalClock() will draw content on its first call
}

void updateDigitalClock() {
    static bool          noWifiShown = false;
    static unsigned long lastUpdate  = 0;

    if (wifiState != WIFI_CONNECTED) {
        if (!noWifiShown) {
            gfx->fillScreen(BLACK);
            gfx->drawCircle(CX, CY, RADIUS, GREY);
            printCentred("NO WiFi", 108, GREY, 2);
            noWifiShown = true;
        }
        return;
    }

    if (noWifiShown) {
        noWifiShown = false;
        drawDigitalClockLayout();
        lastUpdate = 0;   // force immediate draw
    }

    if (WiFi.status() != WL_CONNECTED) {
        wifiForceRetry();
        return;
    }
    timeClient.update();

    if (millis() - lastUpdate < 1000) return;
    lastUpdate = millis();

    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    int s = timeClient.getSeconds();

    // Derive date from epoch (NTPClient adds UTC offset before returning epoch)
    time_t    epoch = (time_t)timeClient.getEpochTime();
    struct tm *ti   = gmtime(&epoch);

    const char *dayNames[] = { "SUNDAY","MONDAY","TUESDAY","WEDNESDAY",
                                "THURSDAY","FRIDAY","SATURDAY" };
    const char *monNames[] = { "JAN","FEB","MAR","APR","MAY","JUN",
                                "JUL","AUG","SEP","OCT","NOV","DEC" };

    // Day of week
    gfx->fillRect(0, 60, 240, 20, BLACK);
    printCentred(dayNames[ti->tm_wday], 62, YELLOW, 2);

    // HH:MM large
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", h, m);
    gfx->fillRect(10, 80, 220, 38, BLACK);
    printCentred(timeBuf, 82, WHITE, 4);

    // Seconds smaller, below
    char secBuf[3];
    snprintf(secBuf, sizeof(secBuf), "%02d", s);
    gfx->fillRect(95, 124, 50, 16, BLACK);
    printCentred(secBuf, 125, GREY, 2);

    // Date   DD MON YYYY
    char dateBuf[13];
    snprintf(dateBuf, sizeof(dateBuf), "%02d %s %04d",
             ti->tm_mday, monNames[ti->tm_mon], ti->tm_year + 1900);
    gfx->fillRect(10, 148, 220, 14, BLACK);
    printCentred(dateBuf, 150, YELLOW, 1);
}

// ============================================================
//  WiFi ICON  (drawn on the step screen)
//
//  Classic three-arc symbol. cx,cy = centre of the dot.
//  Drawn in GREEN when connected, erased (BLACK) otherwise.
// ============================================================
static void drawWifiIcon(int cx, int cy, uint16_t col) {
    gfx->fillCircle(cx, cy, 2, col);

    const int radii[3] = { 6, 10, 14 };
    for (int ri = 0; ri < 3; ri++) {
        int r = radii[ri];
        for (int deg = -50; deg <= 50; deg += 5) {
            float rad = deg * (PI / 180.0f);
            gfx->fillCircle(cx + (int)(sinf(rad) * r),
                            cy - (int)(cosf(rad) * r),
                            1, col);
        }
    }
}



// ============================================================
//  ATTITUDE INDICATOR (Artificial Horizon)
//
//  Derives roll and pitch from the static gravity vector.
//  Accurate when stationary; low-pass filtered for walking use.
//
//  Sky condition at pixel (px, py):
//    (px - CX)*sinR  -  (py - horizY)*cosR  >  0
//  where horizY = CY + pitchDeg * AI_PIX_PER_DEG
//
//  If roll or pitch appears inverted on your hardware, negate
//  rollNew or pitchNew in readAttitudeAngles() below.
// ============================================================

// ---- Raw angle derivation from calibrated g-values --------
void readAttitudeAngles(float &rollDeg, float &pitchDeg) {
    float Xg = (analogRead(ACCEL_X_PIN) - offsetX) / sensitivityX;
    float Yg = (analogRead(ACCEL_Y_PIN) - offsetY) / sensitivityY;
    float Zg = (analogRead(ACCEL_Z_PIN) - offsetZ) / sensitivityZ;

    rollDeg  =  atan2f(Yg, Zg)                       * (180.0f / PI);
    pitchDeg = -atan2f(Xg, sqrtf(Yg*Yg + Zg*Zg))    * (180.0f / PI);
}

// ---- Scanline sky/earth fill -------------------------------
static void aiBackground(float sinR, float cosR, float horizY) {
    for (int y = CY - RADIUS; y <= CY + RADIUS; y++) {
        int dy    = y - CY;
        int xHalf = (int)sqrtf((float)(RADIUS * RADIUS - dy * dy));
        if (xHalf == 0) continue;

        int xLeft  = CX - xHalf;
        int xRight = CX + xHalf;

        if (fabsf(sinR) < 0.01f) {
            // Near-level roll — whole row is one colour
            gfx->drawFastHLine(xLeft, y, 2 * xHalf,
                ((float)y < horizY) ? SKY_COLOUR : EARTH_COLOUR);
        } else {
            // x-coordinate where the horizon line crosses this row
            int xH = CX + (int)(((float)y - horizY) * cosR / sinR);

            // sky if (px-CX)*sinR - (py-horizY)*cosR > 0
            float valLeft = (float)(xLeft - CX) * sinR
                          - ((float)y - horizY)  * cosR;
            bool leftSky  = (valLeft > 0.0f);

            if (xH <= xLeft || xH >= xRight) {
                // Horizon doesn't cross this segment — whole row one colour
                gfx->drawFastHLine(xLeft, y, 2 * xHalf,
                    leftSky ? SKY_COLOUR : EARTH_COLOUR);
            } else {
                if (leftSky) {
                    gfx->drawFastHLine(xLeft, y, xH - xLeft,  SKY_COLOUR);
                    gfx->drawFastHLine(xH,    y, xRight - xH, EARTH_COLOUR);
                } else {
                    gfx->drawFastHLine(xLeft, y, xH - xLeft,  EARTH_COLOUR);
                    gfx->drawFastHLine(xH,    y, xRight - xH, SKY_COLOUR);
                }
            }
        }
    }
}

// ---- Horizon line + pitch ladder --------------------------
// Each mark at 'm' degrees is offset from the horizon in the
// perpendicular-to-horizon direction by m * AI_PIX_PER_DEG px.
static void aiPitchLadder(float sinR, float cosR, float horizY) {
    const int marks[]    = { -20, -10, 0, 10, 20 };
    const int halfLens[] = {  14,  14, 0, 14, 14 };  // 0 = full horizon line

    for (int i = 0; i < 5; i++) {
        int   m      = marks[i];
        float offset = (float)m * AI_PIX_PER_DEG;  // px away from horizon

        // Centre of this mark in screen coordinates
        float lx = (float)CX + offset * sinR;
        float ly = horizY    - offset * cosR;

        // Skip if the centre has scrolled outside the usable circle area
        float d2 = (lx - CX)*(lx - CX) + (ly - CY)*(ly - CY);
        if (d2 > (float)((RADIUS - 16) * (RADIUS - 16))) continue;

        if (m == 0) {
            // Full horizon line — compute exact circle intersection
            float dY   = horizY - (float)CY;
            float disc = (float)(RADIUS * RADIUS) - dY * dY * cosR * cosR;
            if (disc >= 0.0f) {
                float sq = sqrtf(disc);
                float t1 = -dY * sinR + sq;
                float t2 = -dY * sinR - sq;
                gfx->drawLine(
                    CX + (int)(t1 * cosR), (int)(horizY + t1 * sinR),
                    CX + (int)(t2 * cosR), (int)(horizY + t2 * sinR),
                    WHITE);
            }
        } else {
            int   hl = halfLens[i];
            float dx = cosR * hl;
            float dy = sinR * hl;

            // Main horizontal mark
            gfx->drawLine((int)(lx - dx), (int)(ly - dy),
                          (int)(lx + dx), (int)(ly + dy), WHITE);

            // Small end-ticks pointing toward the horizon
            float tickDir = (m > 0) ? 1.0f : -1.0f;
            float tx =  sinR * 5.0f * tickDir;
            float ty = -cosR * 5.0f * tickDir;
            gfx->drawLine((int)(lx - dx),      (int)(ly - dy),
                          (int)(lx - dx + tx), (int)(ly - dy + ty), WHITE);
            gfx->drawLine((int)(lx + dx),      (int)(ly + dy),
                          (int)(lx + dx + tx), (int)(ly + dy + ty), WHITE);
        }
    }
}

// ---- Roll arc: fixed ticks + fixed 0° marker + moving pointer
static void aiRollArc(float rollDeg) {
    // Tick angles in roll degrees (0 = top of display)
    const int ticks[] = { -60, -45, -30, -20, -10, 10, 20, 30, 45, 60 };

    for (int i = 0; i < 10; i++) {
        // Convert to screen angle: 0 roll = top of circle = -PI/2 in screen coords
        float aRad    = ((float)ticks[i] - 90.0f) * (PI / 180.0f);
        int   tickLen = (abs(ticks[i]) % 30 == 0) ? 8 : 5;
        gfx->drawLine(
            CX + (int)(cosf(aRad) * AI_ARC_RADIUS),
            CY + (int)(sinf(aRad) * AI_ARC_RADIUS),
            CX + (int)(cosf(aRad) * (AI_ARC_RADIUS - tickLen)),
            CY + (int)(sinf(aRad) * (AI_ARC_RADIUS - tickLen)),
            WHITE);
    }

    // Fixed white triangle at 12 o'clock pointing inward — the zero marker
    gfx->fillTriangle(CX - 5, CY - AI_ARC_RADIUS + 1,
                      CX + 5, CY - AI_ARC_RADIUS + 1,
                      CX,     CY - AI_ARC_RADIUS + 9,
                      WHITE);

    // Yellow moving pointer at current roll position
    float pRad = (rollDeg - 90.0f) * (PI / 180.0f);
    float pcx  = (float)CX + cosf(pRad) * (float)(AI_ARC_RADIUS - 9);
    float pcy  = (float)CY + sinf(pRad) * (float)(AI_ARC_RADIUS - 9);
    float nx   = -sinf(pRad) * 5.0f;
    float ny   =  cosf(pRad) * 5.0f;
    int   tipX = CX + (int)(cosf(pRad) * AI_ARC_RADIUS);
    int   tipY = CY + (int)(sinf(pRad) * AI_ARC_RADIUS);
    gfx->fillTriangle((int)(pcx + nx), (int)(pcy + ny),
                      (int)(pcx - nx), (int)(pcy - ny),
                      tipX, tipY,
                      YELLOW);
}

// ---- Fixed aircraft symbol (never rotates) ----------------
static void aiAircraftSymbol() {
    gfx->fillRect(CX - 32, CY - 3, 22, 6, YELLOW);   // left wing
    gfx->fillRect(CX + 10, CY - 3, 22, 6, YELLOW);   // right wing
    gfx->fillCircle(CX, CY, 4, YELLOW);               // centre dot
    gfx->fillRect(CX -  4, CY - 3,  8, 6, YELLOW);   // centre stub
}

// ---- Full redraw (call when entering screen or from update) 
void drawAttitudeScreen(float rollDeg, float pitchDeg) {
    float rollRad = rollDeg * (PI / 180.0f);
    float sinR    = sinf(rollRad);
    float cosR    = cosf(rollRad);
    float horizY  = (float)CY + pitchDeg * AI_PIX_PER_DEG;

    aiBackground(sinR, cosR, horizY);
    aiPitchLadder(sinR, cosR, horizY);
    aiRollArc(rollDeg);
    aiAircraftSymbol();

    // Clip and border — overdraw anything that crept outside RADIUS
    for (int r = RADIUS; r <= RADIUS + 3; r++)
        gfx->drawCircle(CX, CY, r, BLACK);
    gfx->drawCircle(CX, CY, RADIUS - 1, GREY);
}

// ---- Called from loop() when SCREEN_ATTITUDE is active ----
void updateAttitudeScreen() {
    if (selfTestRunning) return;

    float rollNew, pitchNew;
    readAttitudeAngles(rollNew, pitchNew);

    // Low-pass filter: smooths step-induced jitter, ~300 ms time constant
    static float rollF  = 0.0f;
    static float pitchF = 0.0f;
    rollF  = 0.75f * rollF  + 0.25f * rollNew;
    pitchF = 0.75f * pitchF + 0.25f * pitchNew;

    // Clamp pitch so the ladder stays within the circle
    if (pitchF >  30.0f) pitchF =  30.0f;
    if (pitchF < -30.0f) pitchF = -30.0f;

    // Skip the redraw if nothing meaningful has changed
    static float lastRoll  = 999.0f;
    static float lastPitch = 999.0f;
    if (fabsf(rollF - lastRoll) < 0.5f && fabsf(pitchF - lastPitch) < 0.5f) return;
    lastRoll  = rollF;
    lastPitch = pitchF;

    drawAttitudeScreen(rollF, pitchF);
}


// ============================================================
//  MENU SCREEN
//
//  Three items: Self Test / Calibration / WiFi
//  Selected item shown in WHITE with a green triangle indicator.
//  Unselected items in GREY.
//  WiFi status (ON/OFF) always shown in its full colour.
// ============================================================
void drawMenuScreen() {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);

    // Title
    printCentred("MENU", 44, WHITE, 3);

    // Decorative separator under title
    gfx->drawLine(CX - 55, 74, CX + 55, 74, GREY);

    // Item vertical positions
    const int itemY[MENU_ITEM_COUNT] = { 82, 107, 132 };

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        bool     sel     = (i == menuItem);
        uint16_t textCol = sel ? WHITE : GREY;
        int      y       = itemY[i];

        // Selection indicator — small right-pointing triangle
        if (sel) {
            gfx->fillTriangle(46, y + 1,
                              46, y + 13,
                              54, y + 7,  GREEN);
        }

        if (i == MENU_SELF_TEST) {
            printCentred("Self Test", y, textCol, 2);

        } else if (i == MENU_CALIBRATION) {
            printCentred("Calibration", y, textCol, 2);

        } else {
            // WiFi item — "WiFi - " in item colour, status always full colour
            const char *status    = wifiEnabled ? "ON" : "OFF";
            uint16_t    statusCol = wifiEnabled ? GREEN : RED;

            // Centre the combined string: "WiFi - " (7 chars) + status
            int totalWidth = (7 + (int)strlen(status)) * 6 * 2;
            int startX     = CX - totalWidth / 2;

            gfx->setTextSize(2);
            gfx->setTextColor(textCol);
            gfx->setCursor(startX, y);
            gfx->print("WiFi - ");
            gfx->setTextColor(statusCol);
            gfx->print(status);
        }
    }

    // Hint at bottom
    printCentred("hold to select", 175, GREY, 1);
}