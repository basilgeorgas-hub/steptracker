// ============================================================
//  Wireless.ino
//
//  WiFi is off by default. enableWifi() is called from the
//  menu when the user explicitly turns it on. updateWifi()
//  does nothing while wifiEnabled is false.
// ============================================================

#include "config.h"

static WiFiUDP ntpUDP;
NTPClient      timeClient(ntpUDP, "pool.ntp.org", NTP_UTC_OFFSET_SEC, 600000);

int wifiState = WIFI_IDLE;

static unsigned long wifiAttemptStart = 0;
static unsigned long wifiRetryAt      = 0;
static unsigned long firstLoopMs      = 0;
static unsigned long lastCheck        = 0;
static bool          beginCalled      = false;

// ---- Called from LCD clock functions when WiFi drops -------
void wifiForceRetry() {
    wifiState   = WIFI_FAILED;
    wifiRetryAt = millis();
    beginCalled = false;
}

// ---- Called from menu when user enables WiFi ---------------
void enableWifi() {
    wifiEnabled = true;
    beginCalled = false;
    wifiState   = WIFI_IDLE;
    firstLoopMs = 0;   // resets the 500ms delay so it runs from now
}

// ---- Called from menu when user disables WiFi --------------
void disableWifi() {
    wifiEnabled = false;
    if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
    wifiState   = WIFI_IDLE;
    beginCalled = false;
}

// ---- Main WiFi state machine — call every loop() -----------
void updateWifi() {
    if (!wifiEnabled) return;   // only runs after user enables via menu

    // Record first call timestamp for the startup delay
    if (firstLoopMs == 0) firstLoopMs = millis();

    if (wifiState == WIFI_IDLE) {
        // Short delay after enabling so the step screen can draw first
        if (millis() - firstLoopMs < 500) return;
        if (WiFi.status() == WL_NO_MODULE) return;

        // Set CONNECTING now — display updates to show indicator this
        // iteration, WiFi.begin() happens on the next iteration
        wifiState        = WIFI_CONNECTING;
        wifiAttemptStart = millis();
        return;
    }

    // Call begin() on first CONNECTING iteration
    if (wifiState == WIFI_CONNECTING && !beginCalled) {
        WiFi.begin(ssid, pass);
        beginCalled = true;
        return;
    }

    // Rate-limit polling to every 250ms
    if (millis() - lastCheck < 250) return;
    lastCheck = millis();

    switch (wifiState) {

        case WIFI_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                wifiState = WIFI_CONNECTED;
                timeClient.begin();
                timeClient.update();
            } else if (millis() - wifiAttemptStart > WIFI_CONNECT_TIMEOUT) {
                wifiState   = WIFI_FAILED;
                wifiRetryAt = millis();
                beginCalled = false;
            }
            break;

        case WIFI_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                wifiState   = WIFI_FAILED;
                wifiRetryAt = millis();
                beginCalled = false;
            } else {
                timeClient.update();
            }
            break;

        case WIFI_FAILED:
            if (millis() - wifiRetryAt > WIFI_RETRY_INTERVAL) {
                wifiState        = WIFI_CONNECTING;
                wifiAttemptStart = millis();
                beginCalled      = false;
            }
            break;
    }
}