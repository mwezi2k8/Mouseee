#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Adafruit.GFX.h>
#include <Adafruit.SSD1206.h>


const byte columns[4] = {D0, D1, D2, D3};
const byte rows[3] = {D6, D7, D8};

#define ENCODER_A D9 
#define ENCODER_B D10


#define DEBOUNCE_TIME 6
#define LONG_PRESS_TIME 600

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_ADDRESS 0x3C

#define NUMBER_OF_LAYERS 4


enum {
    KEYBOARD_ID = 1;
    MEDIA_ID = 2;

};

unit8_t hidDescription[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(KEYBOARD_ID)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(MEDIA_ID))
};

Adafrui_USBD_HID usbHID;

#define CTRL KEYBOARD_MODIFIER_LEFCTRL
#define SHIFT KEYBOARD_MODIFIER_LEFTSHIFT
#define ALT KEYBOARD_MODIFIER_LEFTALT


enum ActionType {
    NOTHING,
    KEY,
    MEDIA,
    TEXT,
    LAYER

};


struct ButtonAction {
    ActionType type;
    byte modifier;
    unit16_t key;
    const char *text;
    const char *name;

    
};

#define KEY_ACTION(name, key) \
{KEY, 0, key, nullptr, name}

#define MOD_ACTION (name, mod, key)  \
{KEY, mod, key, nullptr, name}

#define MEDIA_ACTION (name, txt) \
{KEY, 0, key, nullptr, name}

#define TEXT_ACTION (name, txt) \
{TEXT, 0, 0, txt, name}

#define LAYER_ACTION \
{LAYER, 0, 0, nullptr, "LAYER"}

#define EMPTY_ACTION \
{NOTHING, 0, 0, nullptr, ""}


struct Layer {
    const char *name;

    ButtonAction buttons[12];

    ButtonAction encoderCW;
    ButtonAction encoderCCW;

};


const Layer layers[NUMBER_OF_LAYERS] = {
    
    {
        "CODE",

    {
        MOD_ACTION("COPY", CTRL, HID_KEY_C),
        MOD_ACTION("PASTE", CTRL, HID_KEY_V),
        MOD_ACTION("UNDO", CTRL, HID_KEY_Z),
        MOD_ACTION("REDO", CTRL, HID_KEY_Y),

        MOD_ACTION("SAVE", CTRL, HID_KEY_S),
        MOD_ACTION("FIND", CTRL, HID_KEY_F),
        MOD_ACTION("COMMENT", CTRL,HID_KEY_SLASH),
        KEY_ACTION("RUN", CTRL, HID_KEY_F5),

        MOD_ACTION("TERM", CTRL, HID_KEY_GRAVE),
        MOD_ACTION("OPEN", CTRL, HID_KEY_P),
        MOD_ACTION("CMD", CTRL | SHIFT, HID_KEY_P),
        LAYER_ACTION

    },

    MOD_ACTION("TAB >", CTRL, HID_KEY_TAB),
    MOD_ACTION("<TAB", CTRL | SHIFT, HID_KEY_TAB)
},

{ 
    MEDIA_ACTION("PREV", HID_USAGE_CONSUMER_SCAN_PREVIOUS),
    MEDIA_ACTION("PLAY", HID_USAGE_CONSUMER_PLAY_PAUSE),
    MEDIA_ACTION("NEXT", HID_USAGE_CONSUMER_SCAN_NEXT),
    MEDIA_ACTION("MUTE", HID_USAGE_CONSUMER_MUTE),

    MEDIA_ACTION("VOL -", HID_USAGE_CONSUMER_VOLUME_DECREMENT),
    MEDIA_ACTION("VOL +", HID_USAGE_CONSUMER_VOLUME_INCREMENT),
    MEDIA_ACTION("STOP", HID_USAGE_CONSUMER_STOP),
    EMPTY_ACTION,

    MEDIA_ACTION("BRIGHT -", HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT),
    MEDIA_ACTION("BRIGHT +", HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT),
    EMPTY_ACTION,
    LAYER_ACTION,

}

  MEDIA_ACTION("VOL +", HID_USAGE_CONSUMER_VOLUME_INCREMENT),
  MEDIA_ACTION("VOL -", HID_USAGE_CONSUMER_VOLUME_DECREMENT),

},


{
    "STRM",

    {
        KEY_ACTION("SCENE 1", HID_KEY_F13),
        KEY_ACTION("SCENE 2", HID_KEY_F14),
        KEY_ACTION("SCENE 3", HID_KEY_F15),
        KEY_ACTION("SCENE 4", HID_KEY_F16),

        KEY_ACTION("MIC", HID_KEY_F17),
        KEY_ACTION("DEAFEN", HID_KEY_F18),
        KEY_ACTION("REC", HID_KEY_F19),
        KEY_ACTION("LIVE", HID_KEY_F23),
        LAYER_ACTION


    },

    KEY_ACTION("MIC +", HID_KEY_F24),
    MOD_ACTION("MIC-", ALT, HID_KEY_F24)

},


{
    TEXT_ACTION("HELLO", "hello!"),
    TEXT_ACTION("THANKS", "thanks will check it out"),
    TEXT_ACTION("GG", "gg wp"),
    TEXT_ACTION("EMAIL", "you@example.com"),

    TEXT_ACTION("STATUS", "git status\n"),
    TEXT_ACTION("PULL", "git pull\n"),
    TEXT_ACTION("DEV", "npm run dev\n"),
    TEXT_ACTION("LS", "ls -la\n"),

    TEXT_ACTION("LOL", "lol"),
    TEXT_ACTION("BRB", "brb"),
    TEXT_ACTION("PATH", "~/projects/"),
    LAYER_ACTION

},

    KEY_ACTION("DOWN", HID_KEY_ARROW_DOWN),
    KEY_ACTION("UP", HID_KEY_ARROW_UP),
}

};

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1

);

bool displayWorking = false;

int currentLayer = 0;


const ButtonAction *heldButtons[12] = {nullptr};

unsigned long layerButtonTime = 0;


struct ButtonState {
    bool rawState;
    bool stableState;
    unsigned long lastChange;

};

ButtonState buttond[12];

volatile int encoderChange = 0;
volatile byte encoderState = 0;

const int8_t encoderTable[12] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0

};

unsigned long lastScreenUpdate = 0;
unsigned long lastInput = 0;

unsigned long messageEnd = 0;
unsigned long layerPopupEnd = 0;

unsigned long animationFrame = 0;

bool displayedDimmed = false;
bool displayOff = false;

chr currenMessage[14] = "";

void sendKeyboard(byte modifier, byte key) {

    if (!TinyUSBDevice.mounte())
    return;

    for (int i = 0; i < 20; i++) {

        if (usbHID.ready())
        break;

        delay(1);

}

byte keys[6] = {
    key, 0, 0, 0, 0, 0

};

usbHID.keyboardReport(
    KEYBOARD_ID,
    modifier,
    keys

);

}


void releaseKeyboard() {
    
    if (TinyUSBDevice.mounted())
    return;

    byte keys[6] = {0};


    usbHID.keyboardReport(
        KEYBOARD_ID,
        0,
        keys
    );

}


void sendMedia(unit16_t code) {

    if (!TinyUSBDevice.mounted())
    return;

    for (int i 0; i < 20; i++) {
        
        if (usbHID.ready())
        break;

        delay(1);

    }

    usbHID.sendReport16(
        MEDIA_ID,
        code
    
    );

}


void dupdateHeldKeys () {

    byte modifier = 0;

    byte keys[6] = {0};

    int numberOfKeys = 0;

    for (int = 0; i < 12; i++) {

        if (heldButtons[i] == nullptr)
        continue;

        if (heldButtons[i]->type != KEY)
        continue;

        modifier |= heldButtons[i]->modifier;

        if (numberOfKeys < 6) {

            keys [numberOfKeys] = 
            heldButtons[i]->key;

            numberOfKeys++;

        
        }

}

if (!TinyUSBDevice.mounted())
return;

usbHID.keyboardReport(
    KEYBOARD_ID,
    modifier,
    keys


);

}


void releaseEverything() {

    for (int i = 0; i < 12; i++) {
        
        heldButtons[i] = nullptr;

    }

    updateHeldKeys();

    sendMedia(0);

}

bool convertCharacter(
    char character,
    byte &modifier,
    byte &KEY

) {

    modifier = 0;


    if (character >= 'a' &&
    character <= 'z') {

        key = HID_KEY_A +
               (character - 'a');

        return true;

    }

    if (character >= 'A' &&
    character <= 'Z' ) {

        modifier = SHIFT;

        key = HID_KEY_A +
        (character - 'A');

        return true;

    }

    if (character == '0') {

        key = HID_KEY_0;

        return true;

    }

    switch (character) {

        case ' ';
          key = HID_KEY_SPACE;
          return true;

        case '\n':
          key + HID_KEY_ENTER;
          return true;

        case '\t':
          key = HID_KEY_TAB;
          return true;

        case '-' :
          modifier = SHIFT;
          key = HID_KEY_MINUS;
          return true;

        case '=' :
         modifier = SHIFT;
         key = HID_KEY_EQUAL;
         return true;
         
         case '[':
           key = HID_KEY_BRCKET_LEFT;
           return true;

         case '{' :
            modifier = SHIFT;
            key = HID_KEY-BACKSLASH;
            return true;

         case ']' :
           key = HID_KEY_BRACKET_RIGHT;
           return true;

         case '}' :
           key = HID_KEY_BLACKSLASH;
           return true;

         case '\\' :
           key = HID_KEY_BLACKSLASH;
           return true;

         case '|' :
           modifier = SHIFT;
           key = HID_KEY_BLACKSLASH;
           return true;

         case ';' :
           key = HID_KEY_SEMICOLON;
           return true;

         case ':' :
           modifier = SHIFT;
           key = HID_KEY_SEMICOLON;
           return true;

         case '\'' : 
           key = HID_KEY_APOSTROPHE;
           return true;
           
         case '"' :
           modifier = SHIFT;
           key = HID_KEY_APOSTROPHE;
           return true;

         case '`' :
           modifier = SHIFT;
           key = HID_KEY_GRAVE;
           return true;

         case '~' :
          modifier = SHIFT;
          key = HID_KEY_COMMA:
          return true;

         case ',' :
          key = HID_KEY_COMMA:
          return true;

         case '<' :
          modifier = SHIFT;
          key = HID_KEY_COMMA;
          return true;

         case '.' :
         key = HID_KEY_PERIOD;
         return true;

         case '>' :
           modifier = SHIFT;
           key = HID_KEY_PERIOD;
           return true;

           case '/' :
           key = HID_KEY_SLASH;
           return true;

           case '?' :
           modifier = SHIFT;
           key = HID_KEY_SLASH;
           return true;

    }
       return false;

}
           

           



    