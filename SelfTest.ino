// ============================================================
//  SelfTest.ino
//
//  Triggered from the menu.
//  Each axis tested individually against datasheet expected
//  values converted to g using calibrated sensitivities.
//
//  Datasheet typical self-test response (3.3V, ±2g range):
//    X: −1.08g   Y: +1.08g   Z: +1.83g
//  Pass bands: 
//    X: −1.51g to −0.65g
//    Y: +0.65g to +1.51g
//    Z: +1.10g to +2.56g
// ============================================================

#include "config.h"

void runSelfTest() {
    selfTestRunning = true;
    int returnScreen = currentScreen;

    // -- Running screen --
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("SELF TEST",  88,  WHITE, 2);
    printCentred("Running...", 112, GREY,  1);

    // -- Baseline reading (ST pin inactive) --
    delay(100);
    int rawXi = analogRead(ACCEL_X_PIN);
    int rawYi = analogRead(ACCEL_Y_PIN);
    int rawZi = analogRead(ACCEL_Z_PIN);

    Serial.print("ST baseline  X="); Serial.print(rawXi);
    Serial.print("  Y=");            Serial.print(rawYi);
    Serial.print("  Z=");            Serial.println(rawZi);

    // -- Assert ST pin → ADXL self-test force applied --
    digitalWrite(ST_PIN, LOW);   // MOSFET ON → ADXL ST = 3V3
    delay(200);

    int rawXt = analogRead(ACCEL_X_PIN);
    int rawYt = analogRead(ACCEL_Y_PIN);
    int rawZt = analogRead(ACCEL_Z_PIN);

    Serial.print("ST active    X="); Serial.print(rawXt);
    Serial.print("  Y=");            Serial.print(rawYt);
    Serial.print("  Z=");            Serial.println(rawZt);

    digitalWrite(ST_PIN, HIGH);  // MOSFET OFF → normal operation

    // -- Convert ADC deltas to g using calibrated sensitivities --
    // sensitivityX/Y/Z are in ADC counts per g, measured during calibration
    float Xg = (float)(rawXt - rawXi) / sensitivityX;
    float Yg = (float)(rawYt - rawYi) / sensitivityY;
    float Zg = (float)(rawZt - rawZi) / sensitivityZ;

    Serial.print("ST response  X="); Serial.print(Xg, 3);
    Serial.print("g  Y=");           Serial.print(Yg, 3);
    Serial.print("g  Z=");           Serial.print(Zg, 3);
    Serial.println("g");

    // -- Per-axis pass/fail --
    bool passX = (Xg >= -1.51f && Xg <= -0.65f);
    bool passY = (Yg >=  0.65f && Yg <=  1.51f);
    bool passZ = (Zg >=  1.10f && Zg <=  2.56f);
    bool passed = passX && passY && passZ;

    Serial.print("X: "); Serial.print(Xg, 3); Serial.println(passX ? "g  PASS" : "g  FAIL");
    Serial.print("Y: "); Serial.print(Yg, 3); Serial.println(passY ? "g  PASS" : "g  FAIL");
    Serial.print("Z: "); Serial.print(Zg, 3); Serial.println(passZ ? "g  PASS" : "g  FAIL");
    Serial.println(passed ? "OVERALL: PASS" : "OVERALL: FAIL");

    // -- Results screen --
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("SELF TEST", 56, WHITE, 2);
    gfx->drawLine(CX - 55, 78, CX + 55, 78, GREY);

    char buf[16];

    snprintf(buf, sizeof(buf), "X: %+.2fg", Xg);
    printCentred(buf, 86, passX ? GREEN : RED, 2);

    snprintf(buf, sizeof(buf), "Y: %+.2fg", Yg);
    printCentred(buf, 106, passY ? GREEN : RED, 2);

    snprintf(buf, sizeof(buf), "Z: %+.2fg", Zg);
    printCentred(buf, 126, passZ ? GREEN : RED, 2);

    gfx->drawLine(CX - 55, 146, CX + 55, 146, GREY);

    if (passed) {
        printCentred("PASS", 156, GREEN, 3);
    } else {
        printCentred("FAIL", 156, RED, 3);
        printCentred("Check ADXL wiring", 186, WHITE, 1);
    }

    delay(4000);

    // -- Resume --
    selfTestRunning = false;
    currentScreen = returnScreen;
    drawCurrentScreen();
}