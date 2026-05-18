#include <config.h>

// -------- Global definitions --------
Arduino_DataBus *bus;
Arduino_GC9A01  *gfx;

float offsetX,      offsetY,      offsetZ;
float sensitivityX, sensitivityY, sensitivityZ;
long  avg[3][3];

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN,    INPUT);
    pinMode(LED_RED_PIN,   OUTPUT);
    pinMode(LED_YELLOW_PIN,OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);

    bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
    gfx = new Arduino_GC9A01(bus, TFT_RST, 3, true);
    gfx->begin();
    gfx->fillScreen(BLACK);

    runCalibration();

    // Draw live view header
    gfx->fillScreen(BLACK);
    gfx->drawCircle(CX, CY, RADIUS, GREY);
    printCentred("ACCELERATION", 28, GREEN, 1);
}

void loop() {
    int rawX = analogRead(ACCEL_X_PIN);
    int rawY = analogRead(ACCEL_Y_PIN);
    int rawZ = analogRead(ACCEL_Z_PIN);

    float Xg  = (rawX - offsetX) / sensitivityX;
    float Yg  = (rawY - offsetY) / sensitivityY;
    float Zg  = (rawZ - offsetZ) / sensitivityZ;
    float mag = sqrt(Xg*Xg + Yg*Yg + Zg*Zg);

    // Large magnitude number
    gfx->fillRect(30, 88, 180, 48, BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(4);
    char magBuf[8];
    dtostrf(mag, 4, 2, magBuf);
    int w = strlen(magBuf) * 24;
    gfx->setCursor(CX - w / 2, 92);
    gfx->println(magBuf);

    // Individual axis values
    gfx->fillRect(30, 148, 180, 16, BLACK);
    gfx->setTextColor(GREY);
    gfx->setTextSize(1);
    char buf[32];
    snprintf(buf, sizeof(buf), "X%+.2f Y%+.2f Z%+.2f", Xg, Yg, Zg);
    int bw = strlen(buf) * 6;
    gfx->setCursor(CX - bw / 2, 150);
    gfx->println(buf);

    // State label
    gfx->fillRect(30, 170, 180, 16, BLACK);
    if (mag < 1.1) {
        printCentred("STILL",  172, GREEN,  1);
        setLEDs(true, false, false);
    } else if (mag < 2.5) {
        printCentred("MOVING", 172, YELLOW, 1);
        setLEDs(false, true, false);
    } else {
        printCentred("ACTIVE", 172, RED,    1);
        setLEDs(false, false, true);
    }

    delay(100);
}