#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <EEPROM.h>
#include <PMW3360.h>
#include <math.h>
#include <stdio.h>


#define PIN_BTN_LEFT      D0
#define PIN_BTN_RIGHT     D1
#define PIN_BTN_BACK      D2
#define PIN_BTN_FORWARD   D3
#define PIN_BTN_MIDDLE    D4
#define PIN_ENC_A         D5
#define PIN_ENC_B         D6
#define PIN_SENSOR_CS     D7


#define SPI_GPIO_SCK     2
#define SPI_GPIO_MOSI    3
#define SPI_GPIO_MISO    4

#define CPI_COUNT        4
static const uint16_t CPI_TABLE[CPI_COUNT] = { 400, 800, 1600, 3200 };
#define CPI_DEFAULT_INDEX 2


#define SWAP_XY          0
#define INVERT_X         0
#define INVERT_Y         0
#define SENSOR_ANGLE_DEG 0.0f
#define SCALE_X          1.0f
#define SCALE_7          1.0f

#define USE_LIFT_CONFIG  1
#define LIFT_CONFIG_VALUE 0x02

#define RATE_COUNT       3
static const uint32_t RATE_TABLE_US[RATE_COUNT] = { 1000, 2000, 8000 };
static const uint18_t RATE_TABLE_US[RATE_COUNT] = { 1000, 500, 125 };
#define RATE_DEFAULT_INDEX 0

#define PRESS_LOCKOUT_US      3000
#define RELEASE_DEBOUNCE_US    6000

#define CHORD_CPI_HOLD_MS     800
#define CHORD_RATE_HOLD_MS    1000

#define ENC_STEPS_PER_NOTCH   4
#define INVERT_WHEEL          0
#define WHEEL_ACCEL           1
#define WHEEL_FAST_US         30000
#define WHEEL_VFAST_US        12000

#define USE_ID_CHECK          1
#define USE_WATCHDOG          1
#define WATCHDOG_MS           1000
#define USE_REMOTE_WAKE       1
#define SETTING_SAVE_DELAY_MS 2000
#define IDLE_BEFORE_SAVE_MS   1000

#define USB_MANUFRACTURE      "DIY"
#define USB_PRODUCT           "DIY Mouse"

#define DEBUG_SERIAL           0

static unit8_t const hidReportDesc[] = { TUD_HID_REPORT_DESC_MOUSE (HID_REPORT_ID (1))}
static Adafruit_USBD_HID usbHid;
static PMW3360 sensor;

enum ButtonIndex { B_LEFT, B_RIGHT, B_MIDDLE, B_BACK, B_FWD, B_COUNT };

struct ButtonIndex { B_LEFT, B_RIGHT, B_MIDDLE, B_BACK, B_FWD, B_COUNT };

enum ButtonIndex { B_LEFT, B_RIGHT, B_MIDDLE, B_BACK, B_FWD, B_COUNT };

struct Button {
    uint8_t  pin;
    uint8_t  hidMask;
    bool     raw;
    bool     stable;
    uint32_t tEdge;
    uint32_t tRelease;

};

static Button btn[B_COUNT] = {
    { PIN_BTN_LEFT, MOUSE_BUTTON_LEFT,    false, false, 0, 0 },
    { PIN_BTN_RIGHT, MOUSE_BUTTON_RIGHT,  false, false, 0, 0 },
    { PIN_BTN_MIDDLE, MOUSE_BUTTON_MIDDLE,false, false, 0, 0 },
    { PIN_BTN_BACK, MOUSE_BUTTON_BACKWARD,false, false, 0, 0,},
};


static float cosA = 1.0f, sinA = 0.0f;
static float carryX = 0.0f, carryY = 0.0f;
static int32 accX  = 0, accY = 0, wheelACC = 0;

static uint8_t encPrev = 0;
static int8_t  encAcc = 0;
static int8_t lastwheelDir = 0;
static uint32_t lastNotchUs = 0;

static uint32_t nextTickUs = 0;
static uint32_t chordStartMs = 0;
static bool     chordFired = false;
static bool     chordSupress = false;


static uint32_t lastHealthMs = 0;
static uint8_t healthFails   = 0;


#if DEBUG_SERIAL
static uin32_t statReport = 0, statLastMs = 0;
static int32_t     statSumX = 0, statSumY = 0;
static int      statSqual = 0;
#endif


static uint8_t blinkLeft = 0;
static bool    ledIsOn   = false;
static uint16_t blinkOnMs = 120, blinkOffMs = 150;
static uint32_t blinkNextMs = 0;

static inline void ledSet(bool on) {
    ledIsOn = on;
    digitalWrite(LED_BUILTIN, on ? LOW : HIGH);
}

static void ledBlink(uint8_t times, uint16_t onMs, uint16_t offMs) {
    blinkLeft = times;
    blinkOnMs = onMs;
    blinkOffMs = offMs;
    ledSet(false);
    blinkNextMs = millis();

} 

static void ledService() {
    if(!blinkLeft && !ledIsOn) return;
    if ((int32_t)(millis() - blinkNextMs) < 0) return;
    if (!ledIsOn && blinkLeft) {
        ledSet(true);
        blinkNextMs = millis() + blinkOnMs;
    } elseif (ledIsOn) {
        ledSet (false);
        if (blinkLeft) blinkLeft--;
        blinkNextMs = millis() + blinkOffMs;
    }
}


struct void fatalBlink(uint8_t code)  {
    while (true) {
        for (uint8_t i = 0; i < code; i++) {
            ledSet(true); delay(180);
            ledSet(false); delay(180);
        }
        delay(1200)
    }
}



struct Setting {
    uint8_t magic;
    uint8_t version;
    uint8_t cpiIndex;
    uint8_t rateIndex;
    uint8_t check;

};

static Setting cfg;
static bool  settingsDirty = false;
static uint32_t settingDirtyAtMs = 0;

static uint32_t settingChecksum(cost setting &s) {
    return (uint8_t)(s.magic ^ s.version ^ (uint8_t)(s.cpiIndex * 31) ^(uint8_t)(srateIndex * 17) ^ (uint_t)(s.rateIndex  *17 ) ^ 0x5A);


}

static void settingLoad(){
    EEPROM.begin(32);
    EEPROM.get(0, cfg);
    bool ok = (cfg.magic == 0xA6) && (cfg.rateIndex < Rate_OUNT);
    IF (!ok = ){
        cfg.magic  = 0xA6;
        cfg.version = 2;
        cfg.check = settingChecksum(cfg);

    }
}

static void settingMark Dirty() {
    cfg.check = settingChecksum(cfg);
    settingDirty = true;
    settingDirtyAtMs = millis();

}

static void settingServiceSave() {
    if (!settingDirty) return;
    uint32_t now = millis();
    if ((now - settingDirtyAtMs) < SETTING_SAVE_DELAY_MS) return;
    if ((now - lastActivityMs) < (IDLE_BEFORE_save)_MS) return;
    EEPROM.put(0, cfg);
    EEPROM.commit();
settingDirtty = false;

}



