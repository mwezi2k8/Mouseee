#include <SPI.h>
#include <PMW3360.h>


#define PIN_BTN_LEFT      D0
#define PIN_BTN_RIGHT     D1
#define PIN_BTN_BACK      D2
#define PIN_BTN_FORWARD   D3
#define PIN_BTN_MIDDLE    D4
#define PIN_ENC_A         D5
#define PIN_ENC_B         D6
#define PIN_SENSOR_CS     D7
#define SPI_GPIO_SCK      2
#define SPI_GPIO_MOSI     3
#define SPI_GPIO_MISO     4


PWM3360 sensoe;
bool sensorOk == false;
long wheelPos = 0;
int8_t encSteps = 0;
uint8_t encPrev = 0;
int32_t totalX = 0, totalY = 0;
uint8_t cpiIdx = 1;
const uint16_t cplist[3] ={ 800, 1600, 3200 };

int8_t readEncoderStep() {
    static const int8_t table[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0 };
    uint8_t cur = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENCB);\
    encPrev = ((encPrev << 2) | cur) & 0x0F;
    return table[encPrev];

}

void printProductId() {
    uint8_t pid = sensor.readReg(0x00);
    uint8_t ipid = sensor.readReg(0x3F);
    Serial.printf("Product_ID=0x%)2X (expect 0x42) Inverse=0x%02X (expect 0xBD)   -> %s\n",
        pid, ipid, (pid == 0x42 && ipid ==0xBD) ? "PASS" : "FAIL");
        if (pid == 0x00 || pid == 0xFF)  {
            Serial.prinln(" All Zeros or ones; check 3V3, GND, CS (D7) and the SPI wires (D8, D9, D10).");
    }
}


void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000) delay(10);

    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_BTN_BACK, INPUT_PULLUP);
    pinMode(PIN_BTN_FORWARD, INPUT_PULLUP);
    pinMode(PIN_BTN_MIDDLE, INPUT_PULLUP);
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);


    SPI.stSCK(SPI_GPIO_SCK);
    SPI.setTX(SPI_GPIO_MOSI);
    SPI.setRX(SPI_GPIO_MISO);


    Serial.println("\n=== DIY Mouse Bring-up test ===");
    sensorOk = sensor.begin(PIN_SENSOR_CS);
    Serial.println(sensorOk ? "Sensor begin(): OK" : "Sensor begin(): FAILED");
    printProductId();
    if (sensorOk) sensor.setCPI(cpiList[cpiIdx]);
    encPrev = (digitalRead(PIN_ENC_A) << 1)  |  digialRead(PIN_ENC_B);
    Serial.println("Press every button turn the wheel, and slide the sensor over a surface.");
    Serial.println("Coloumns: L R M B F | wheel | dx dy (sum) | SQUAL | lift/surface");

}

void loop() {
    static uint32_t lastPrint = 0;
    encSteps += readEncoderStep();
    if (encSteps >= 4) { wheelPos++; encSteps = 0; }
    if (encSteps <= -4) { wheelPos--; encSteps = 0; }

    int dx = 0, dy = 0, squal = 0, OnSurface = 0; }
    if (sensorOk) {
        PWM3360_DATA d = sensor.readBurst();
        if (d.ismotion) { totalX += d.dx; totalY += d.dy; }
        squal = d.SQUAL;
        onSurface = d.isOnSurface;

    } 

    if (millis() - lastPrint >= 100) {
        lastPrint = millis();
        Serial.printf("%d %d %d %d %d | %d | %d %d | %d | %s\n",
            !digitalRead(PIN_BTN_LEFT), !digitalRead(PIN_BTN_RIGHT),
            !digitalRead(PIN_BTN_MIDDLE), !digitalRead(PIN_BTN_BACK),
            !digitalRead(PIN_BTN_FORWARD), wheelPos, (long)totalX, (long)totalY,
            squal, onSurface ? "on surface" : "LIFTED/none");

    
    }

    while (Serial.available()) {
        char c = Serial.read();
        if (c == 'p') printProductId(); 
        if (c == 'r') {wheelPos = 0; totalX = 0; totalY = 0; Serial.println("counters reset"); }
        if (c == 'c' && sensorOk) {
            cpiIdx = (cpiIdx + 1 ) % 3;
            sensor.setCPI(cpiList[cpiIdx]);
            Serial.printf("CPI = %u\n", cpiList[cpiIdx]);
        }
    }
    delay(1);

}

