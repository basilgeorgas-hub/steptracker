#include "config.h"

void setLEDs(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void waitForButton() {
    while (digitalRead(BUTTON_PIN) == LOW)  { delay(10); }
    delay(300);
    while (digitalRead(BUTTON_PIN) == HIGH) { delay(10); }
    delay(300);
}

void printCentred(const char *text, int y, uint16_t colour, int size) {
    gfx->setTextColor(colour);
    gfx->setTextSize(size);
    int w = strlen(text) * 6 * size;
    gfx->setCursor(CX - w / 2, y);
    gfx->println(text);
}

void drawProgressDots(int currentStep) {
    int dotY = 52, dotR = 6, gap = 24;
    int startX = CX - gap;
    for (int i = 0; i < 3; i++) {
        int x = startX + i * gap;
        if      (i < currentStep)  gfx->fillCircle(x, dotY, dotR, GREEN);
        else if (i == currentStep) gfx->fillCircle(x, dotY, dotR, WHITE);
        else                       gfx->drawCircle(x, dotY, dotR, GREY);
    }
}

void drawBoardGraphic(int step) {
    int gx = CX, gy = 125;
    gfx->fillRect(gx - 45, gy - 35, 90, 60, BLACK);

    if (step == 0) {
        gfx->fillRoundRect(gx - 38, gy - 10, 76, 22, 4, GREY);
        gfx->drawRoundRect(gx - 38, gy - 10, 76, 22, 4, WHITE);
        gfx->drawLine(gx, gy + 18, gx, gy + 28, YELLOW);
        gfx->fillTriangle(gx - 4, gy + 25, gx + 4, gy + 25, gx, gy + 32, YELLOW);
        printCentred("Z", gy + 34, YELLOW, 1);
    } else if (step == 1) {
        gfx->fillRoundRect(gx - 12, gy - 28, 24, 52, 4, GREY);
        gfx->drawRoundRect(gx - 12, gy - 28, 24, 52, 4, WHITE);
        gfx->drawLine(gx - 22, gy - 5, gx - 22, gy + 10, YELLOW);
        gfx->fillTriangle(gx - 26, gy + 7, gx - 18, gy + 7, gx - 22, gy + 14, YELLOW);
        printCentred("X", gy + 16, YELLOW, 1);
    } else {
        gfx->fillRoundRect(gx - 28, gy - 12, 52, 24, 4, GREY);
        gfx->drawRoundRect(gx - 28, gy - 12, 52, 24, 4, WHITE);
        gfx->drawLine(gx, gy + 18, gx, gy + 28, YELLOW);
        gfx->fillTriangle(gx - 4, gy + 25, gx + 4, gy + 25, gx, gy + 32, YELLOW);
        printCentred("Y", gy + 34, YELLOW, 1);
    }
}

void showStepInstruction(int step) {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("CALIBRATION", 28, GREEN, 1);
    drawProgressDots(step);

    const char *titles[]   = { "Lay flat",  "X edge up", "Y edge up" };
    const char *axisInfo[] = { "Z=1g X=0g Y=0g",
                               "X=1g Y=0g Z=0g",
                               "Y=1g X=0g Z=0g" };
    printCentred(titles[step],   68, WHITE, 2);
    printCentred(axisInfo[step], 92, GREY,  1);
    drawBoardGraphic(step);
    printCentred("PRESS BUTTON", 178, YELLOW, 1);

    if (step == 0) setLEDs(true,  false, false);
    if (step == 1) setLEDs(false, true,  false);
    if (step == 2) setLEDs(false, false, true);
}

void showSampling(int step) {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("CALIBRATION", 28, GREEN, 1);
    drawProgressDots(step);
    printCentred("Sampling...", 108, WHITE, 2);
    setLEDs(false, false, false);
}

void takeSamples(int orientation) {
    long sumX = 0, sumY = 0, sumZ = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sumX += analogRead(ACCEL_X_PIN);
        sumY += analogRead(ACCEL_Y_PIN);
        sumZ += analogRead(ACCEL_Z_PIN);
        float angle = ((float)i / SAMPLES) * 2 * PI - PI / 2;
        int px = CX + (int)(cos(angle) * 75);
        int py = CY + (int)(sin(angle) * 75);
        gfx->fillCircle(px, py, 3, GREEN);
        delay(10);
    }
    avg[orientation][0] = sumX / SAMPLES;
    avg[orientation][1] = sumY / SAMPLES;
    avg[orientation][2] = sumZ / SAMPLES;
}

void showResults() {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    setLEDs(true, true, true);

    printCentred("CALIBRATION", 32, GREEN, 1);
    printCentred("COMPLETE",    46, GREEN, 1);

    gfx->setTextColor(LIGHT_GREY);
    gfx->setTextSize(1);
    gfx->setCursor(38, 72);  gfx->println("Offsets:");
    gfx->setCursor(38, 84);
    gfx->print("X="); gfx->print(offsetX, 1);
    gfx->print(" Y="); gfx->println(offsetY, 1);
    gfx->setCursor(38, 96);
    gfx->print("Z="); gfx->println(offsetZ, 1);

    gfx->setCursor(38, 114); gfx->println("Sensitivity (cts/g):");
    gfx->setCursor(38, 126);
    gfx->print("X="); gfx->print(sensitivityX, 1);
    gfx->print(" Y="); gfx->println(sensitivityY, 1);
    gfx->setCursor(38, 138);
    gfx->print("Z="); gfx->println(sensitivityZ, 1);

    printCentred("Live view in 3s", 168, YELLOW, 1);
    delay(3000);
    setLEDs(false, false, false);
}

void runCalibration() {
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("STEP",        88,  WHITE, 3);
    printCentred("TRACKER",     114, WHITE, 3);
    printCentred("Starting",    150, GREEN, 1);
    printCentred("Calibration", 160, GREEN, 1);
    delay(2000);

    for (int step = 0; step < 3; step++) {
        showStepInstruction(step);
        waitForButton();
        showSampling(step);
        takeSamples(step);
    }

    offsetX = (avg[0][0] + avg[2][0]) / 2.0f;
    offsetY = (avg[0][1] + avg[1][1]) / 2.0f;
    offsetZ = (avg[1][2] + avg[2][2]) / 2.0f;

    sensitivityX = abs(avg[1][0] - offsetX);
    sensitivityY = abs(avg[2][1] - offsetY);
    sensitivityZ = abs(avg[0][2] - offsetZ);

    Serial.println("=== CALIBRATION RESULTS ===");
    Serial.print("#define ACCEL_OFFSET_X      "); Serial.println(offsetX);
    Serial.print("#define ACCEL_OFFSET_Y      "); Serial.println(offsetY);
    Serial.print("#define ACCEL_OFFSET_Z      "); Serial.println(offsetZ);
    Serial.print("#define ACCEL_SENSITIVITY_X "); Serial.println(sensitivityX);
    Serial.print("#define ACCEL_SENSITIVITY_Y "); Serial.println(sensitivityY);
    Serial.print("#define ACCEL_SENSITIVITY_Z "); Serial.println(sensitivityZ);

    showResults();
}
