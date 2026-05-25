// ============================================================
//  PaceIdentification.ino
//
//  Uses stored lastStepInterval for stable cadence calculation.
//  Cadence is computed from the interval between the last two
//  confirmed steps — not from millis() - lastStepTime which
//  changes every millisecond and causes flickering.
// ============================================================

#include "config.h"

void updatePaceState() {
    if (lastStepTime == 0 ||
        (millis() - lastStepTime) > STEP_MAX_INTERVAL_MS) {
        paceState = PACE_STATIONARY;
        return;
    }

    if (lastStepInterval == 0) {
        paceState = PACE_WALKING;
        return;
    }

    // No /2 — works correctly whether detecting one or both legs:
    // Walking strong leg only: 60000/1032 = 58 spm → WALKING
    // Walking both legs:       60000/537  = 112 spm → WALKING
    // Running both legs:       60000/350  = 171 spm → RUNNING
    int cadence = (int)(60000UL / lastStepInterval);

    Serial.print("Cadence: "); Serial.println(cadence);

    if (cadence >= CADENCE_RUNNING_MIN) {
        paceState = PACE_RUNNING;
    } else {
        paceState = PACE_WALKING;
    }
}