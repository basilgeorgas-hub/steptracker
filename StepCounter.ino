// ============================================================
//  StepCounter.ino
//
//  Peak/valley threshold step detection using magnitude in g.
//  Stores interval between last two steps for cadence calculation.
// ============================================================

#include "config.h"

void updateStepDetection() {
    if (selfTestRunning) return;
    int rawX = analogRead(ACCEL_X_PIN);
    int rawY = analogRead(ACCEL_Y_PIN);
    int rawZ = analogRead(ACCEL_Z_PIN);

    float Xg  = (rawX - offsetX) / sensitivityX;
    float Yg  = (rawY - offsetY) / sensitivityY;
    float Zg  = (rawZ - offsetZ) / sensitivityZ;
    float mag = sqrt(Xg*Xg + Yg*Yg + Zg*Zg);
    Serial.println(mag, 3);

    // Reset timing so next step is treated as first after stationary
    // Leave stepPending alone so in-progress detection is not cancelled
    if (lastStepTime > 0 && (millis() - lastStepTime) > STEP_MAX_INTERVAL_MS) {
        lastStepTime     = 0;
        lastStepInterval = 0;
    }

    if (!stepPending && mag > STEP_THRESHOLD_HIGH) {
        stepPending = true;
    }
    else if (stepPending && mag < STEP_THRESHOLD_LOW) {
        unsigned long now      = millis();
        unsigned long interval = now - lastStepTime;

        bool firstStep   = (lastStepTime == 0);
        bool validTiming = (interval >= STEP_MIN_INTERVAL_MS &&
                            interval <= STEP_MAX_INTERVAL_MS);

        if (firstStep || validTiming) {
            if (!firstStep) {
                lastStepInterval = interval;
            }
            steps++;
            lastStepTime = now;

            Serial.print("# Step: ");     Serial.print(steps);
            Serial.print("  interval: "); Serial.print(firstStep ? 0 : (int)interval);
            Serial.println("ms");
        }

        stepPending = false;
    }
}