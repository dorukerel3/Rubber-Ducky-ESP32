#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <cstring>
#include <cstdlib>
#include <cctype>

static constexpr uint8_t PIN_SD_CS   = 10;
static constexpr uint8_t PIN_SD_MOSI = 11;
static constexpr uint8_t PIN_SD_SCK  = 12;
static constexpr uint8_t PIN_SD_MISO = 13;
static constexpr uint8_t PIN_LED     = 46;
static constexpr uint8_t PIN_OTA_TRIGGER = 4;

static const char*      OTA_AP_SSID            = "BadUSB-OTA";
static const char*      OTA_AP_PASSWORD        = "CHANGE_ME_AP_PW";
static const char*      OTA_HOSTNAME           = "badusb-esp32";
static const char*      OTA_UPLOAD_PASSWORD    = "CHANGE_ME_UPLOAD_PW";
static const char*      OTA_TRIGGER_FILE       = "/OTA.TXT";
static constexpr uint32_t OTA_IDLE_TIMEOUT_MS  = 180000;

static constexpr uint32_t KEY_PRESS_US   = 1000;
static constexpr uint32_t KEY_RELEASE_US = 1000;

static constexpr uint8_t MOD_NONE  = 0x00;
static constexpr uint8_t MOD_CTRL  = 0x01;
static constexpr uint8_t MOD_SHIFT = 0x02;
static constexpr uint8_t MOD_ALT   = 0x04;
static constexpr uint8_t MOD_GUI   = 0x08;
static constexpr uint8_t MOD_RALT  = 0x10;

#ifndef KEY_PRINT_SCREEN
#  define KEY_PRINT_SCREEN  0xCE
#endif
#ifndef KEY_SCROLL_LOCK
#  define KEY_SCROLL_LOCK   0xCF
#endif
#ifndef KEY_PAUSE
#  define KEY_PAUSE         0xD0
#endif
#ifndef KEY_NUM_LOCK
#  define KEY_NUM_LOCK      0xDB
#endif
#ifndef KEY_MENU
#  define KEY_MENU          0xED
#endif
#ifndef KEY_F13
#  define KEY_F13  0xF0
#  define KEY_F14  0xF1
#  define KEY_F15  0xF2
#  define KEY_F16  0xF3
#  define KEY_F17  0xF4
#  define KEY_F18  0xF5
#  define KEY_F19  0xF6
#  define KEY_F20  0xF7
#  define KEY_F21  0xF8
#  define KEY_F22  0xF9
#  define KEY_F23  0xFA
#  define KEY_F24  0xFB
#endif

struct CharDef {
    uint32_t cp;
    uint8_t  k1;
    uint8_t  k2;
    bool     ralt;
};

static const CharDef EN_DEFS[] = {};

static const CharDef JP_DEFS[] = {
    {0x0022, '@',  0, false},
    {0x0026, '^',  0, false},
    {0x0027, '&',  0, false},
    {0x0028, '*',  0, false},
    {0x0029, '(',  0, false},
    {0x002A, '"',  0, false},
    {0x002B, ':',  0, false},
    {0x003A, '\'', 0, false},
    {0x003D, '_',  0, false},
    {0x005B, ']',  0, false},
    {0x005D, '\\', 0, false},
    {0x005E, '=',  0, false},
    {0x0060, '{',  0, false},
    {0x007B, '}',  0, false},
    {0x007D, '|',  0, false},
    {0x007E, '+',  0, false},
    {0x0040, '[',  0, false},
};

static const CharDef TR_DEFS[] = {
    {0x0069, (uint8_t)0x27, 0,   false},
    {0x0130, (uint8_t)0x22, 0,   false},
    {0x0131, 'i',  0,   false},
    {0x011F, '[',  0,   false},
    {0x011E, '{',  0,   false},
    {0x00FC, ']',  0,   false},
    {0x00DC, '}',  0,   false},
    {0x015F, ';',  0,   false},
    {0x015E, ':',  0,   false},
    {0x00F6, ',',  0,   false},
    {0x00D6, '<',  0,   false},
    {0x00E7, '.',  0,   false},
    {0x00C7, '>',  0,   false},
    {0x002E, '/',  0,   false},
    {0x003A, '?',  0,   false},
    {0x0027, '@',  0,   false},
    {0x005E, '#',  0,   false},
    {0x002B, '$',  0,   false},
    {0x0026, '^',  0,   false},
    {0x002F, '&',  0,   false},
    {0x0028, '*',  0,   false},
    {0x0029, '(',  0,   false},
    {0x003D, ')',  0,   false},
    {0x002A, '-',  0,   false},
    {0x003F, '_',  0,   false},
    {0x002D, '=',  0,   false},
    {0x005F, '+',  0,   false},
    {0x0040, 'q',  0,   true },
    {0x0023, '3',  0,   true },
    {0x0024, '4',  0,   true },
    {0x005B, '8',  0,   true },
    {0x005D, '9',  0,   true },
    {0x007B, '7',  0,   true },
    {0x007D, '0',  0,   true },
    {0x007E, ']',  0,   true },
    {0x005C, '-',  0,   true },
    {0x007C, '_',  0,   true },
    {0x003C, ',',  0,   true },
    {0x003E, '.',  0,   true },
    {0x20AC, 'e',  0,   true },
};

static const CharDef FR_DEFS[] = {
    {0x00E9, '\'', 'e', false},
    {0x00C9, '\'', 'E', false},
    {0x00E1, '\'', 'a', false},
    {0x00C1, '\'', 'A', false},
    {0x00FA, '\'', 'u', false},
    {0x00DA, '\'', 'U', false},
    {0x00F3, '\'', 'o', false},
    {0x00D3, '\'', 'O', false},
    {0x00ED, '\'', 'i', false},
    {0x00CD, '\'', 'I', false},
    {0x00E8, '`',  'e', false},
    {0x00C8, '`',  'E', false},
    {0x00E0, '`',  'a', false},
    {0x00C0, '`',  'A', false},
    {0x00F9, '`',  'u', false},
    {0x00D9, '`',  'U', false},
    {0x00E2, '^',  'a', false},
    {0x00C2, '^',  'A', false},
    {0x00EA, '^',  'e', false},
    {0x00CA, '^',  'E', false},
    {0x00EE, '^',  'i', false},
    {0x00CE, '^',  'I', false},
    {0x00F4, '^',  'o', false},
    {0x00D4, '^',  'O', false},
    {0x00FB, '^',  'u', false},
    {0x00DB, '^',  'U', false},
    {0x00E4, '"',  'a', false},
    {0x00C4, '"',  'A', false},
    {0x00EB, '"',  'e', false},
    {0x00CB, '"',  'E', false},
    {0x00EF, '"',  'i', false},
    {0x00CF, '"',  'I', false},
    {0x00F6, '"',  'o', false},
    {0x00D6, '"',  'O', false},
    {0x00FC, '"',  'u', false},
    {0x00DC, '"',  'U', false},
    {0x00E7, ',',  0,   true },
    {0x00C7, '<',  0,   true },
    {0x0153, 'q',  0,   true },
    {0x0152, 'Q',  0,   true },
    {0x20AC, 'e',  0,   true },
    {0x00F1, '~',  'n', false},
    {0x00D1, '~',  'N', false},
    {0x00E3, '~',  'a', false},
    {0x00C3, '~',  'A', false},
    {0x00F5, '~',  'o', false},
    {0x00D5, '~',  'O', false},
};

static const CharDef DE_DEFS[] = {
    {0x00E4, 'a',  0,   true },
    {0x00C4, 'A',  0,   true },
    {0x00F6, 'o',  0,   true },
    {0x00D6, 'O',  0,   true },
    {0x00FC, 'u',  0,   true },
    {0x00DC, 'U',  0,   true },
    {0x00DF, 's',  0,   true },
    {0x20AC, 'e',  0,   true },
    {0x00E9, '\'', 'e', false},
    {0x00E0, '`',  'a', false},
};

enum LayoutID : uint8_t { LAY_EN = 0, LAY_TR, LAY_FR, LAY_DE, LAY_JP, LAYOUT_COUNT };

struct LayoutDefSet {
    const char*    name;
    const CharDef* defs;
    uint16_t       len;
};

static const LayoutDefSet LAYOUT_DEFS[LAYOUT_COUNT] = {
    { "EN", EN_DEFS, (uint16_t)(sizeof(EN_DEFS) / sizeof(CharDef)) },
    { "TR", TR_DEFS, (uint16_t)(sizeof(TR_DEFS) / sizeof(CharDef)) },
    { "FR", FR_DEFS, (uint16_t)(sizeof(FR_DEFS) / sizeof(CharDef)) },
    { "DE", DE_DEFS, (uint16_t)(sizeof(DE_DEFS) / sizeof(CharDef)) },
    { "JP", JP_DEFS, (uint16_t)(sizeof(JP_DEFS) / sizeof(CharDef)) },
};

struct CharEntry {
    bool    mapped = false;
    bool    ralt   = false;
    uint8_t k1     = 0;
    uint8_t k2     = 0;
};

struct ExtEntry {
    uint32_t  cp;
    CharEntry entry;
};

static constexpr uint8_t MAX_EXT_PER_LAYOUT = 8;

static CharEntry g_table256[LAYOUT_COUNT][256];
static ExtEntry  g_ext[LAYOUT_COUNT][MAX_EXT_PER_LAYOUT];
static uint8_t   g_extCount[LAYOUT_COUNT] = {0};

static void buildCharTables() {
    for (uint8_t L = 0; L < LAYOUT_COUNT; L++) {
        g_extCount[L] = 0;
        const CharDef* defs = LAYOUT_DEFS[L].defs;
        const uint16_t n    = LAYOUT_DEFS[L].len;
        for (uint16_t i = 0; i < n; i++) {
            const CharDef& d = defs[i];
            CharEntry e;
            e.mapped = true;
            e.ralt   = d.ralt;
            e.k1     = d.k1;
            e.k2     = d.k2;
            if (d.cp < 256) {
                g_table256[L][d.cp] = e;
            } else if (g_extCount[L] < MAX_EXT_PER_LAYOUT) {
                g_ext[L][g_extCount[L]++] = { d.cp, e };
            }
        }
        for (uint8_t i = 1; i < g_extCount[L]; i++) {
            ExtEntry key = g_ext[L][i];
            int j = (int)i - 1;
            while (j >= 0 && g_ext[L][j].cp > key.cp) {
                g_ext[L][j + 1] = g_ext[L][j];
                j--;
            }
            g_ext[L][j + 1] = key;
        }
    }
}

static inline bool lookupChar(uint8_t layout, uint32_t cp, CharEntry* out) {
    if (cp < 256) {
        *out = g_table256[layout][cp];
        return out->mapped;
    }
    const uint8_t   n   = g_extCount[layout];
    const ExtEntry* arr = g_ext[layout];
    int lo = 0, hi = (int)n - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) >> 1;
        if (arr[mid].cp == cp) { *out = arr[mid].entry; return true; }
        if (arr[mid].cp < cp)  lo = mid + 1; else hi = mid - 1;
    }
    return false;
}

static uint8_t g_layout = LAY_EN;

static USBHIDKeyboard Keyboard;
static USBHIDMouse    Mouse;
static SPIClass       sdSPI(HSPI);

static uint16_t g_defaultDelay = 0;

static void blinkHalt(uint16_t onMs, uint16_t offMs) {
    for (;;) {
        digitalWrite(PIN_LED, HIGH); delay(onMs);
        digitalWrite(PIN_LED, LOW);  delay(offMs);
    }
}

static volatile bool g_otaActivity = false;

static void runOtaMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(OTA_AP_SSID, OTA_AP_PASSWORD);

    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_UPLOAD_PASSWORD);

    ArduinoOTA.onStart([]() {
        g_otaActivity = true;
        digitalWrite(PIN_LED, HIGH);
    });
    ArduinoOTA.onEnd([]() {
        digitalWrite(PIN_LED, LOW);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        g_otaActivity = true;
        digitalWrite(PIN_LED, (progress / 4096) % 2);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        digitalWrite(PIN_LED, LOW);
    });

    ArduinoOTA.begin();

    uint32_t lastBlink    = millis();
    uint32_t lastActivity = millis();
    bool     ledState     = false;
    for (;;) {
        ArduinoOTA.handle();
        if (g_otaActivity) {
            g_otaActivity = false;
            lastActivity  = millis();
        }
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            ledState  = !ledState;
            digitalWrite(PIN_LED, ledState);
        }
        if (millis() - lastActivity > OTA_IDLE_TIMEOUT_MS) {
            ArduinoOTA.end();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            digitalWrite(PIN_LED, LOW);
            return;
        }
    }
}

static constexpr size_t CHUNK_SIZE = 512;
static constexpr size_t DUCKY_LINE_MAX   = 512;

struct LineReader {
    File   file;
    char   buf[CHUNK_SIZE];
    size_t len = 0;
    size_t pos = 0;
    bool   eof = false;

    void begin(File f) {
        file = f;
        len = 0; pos = 0; eof = false;
    }

    bool refill() {
        if (eof) return false;
        int n = file.read((uint8_t*)buf, CHUNK_SIZE);
        if (n <= 0) { eof = true; len = 0; pos = 0; return false; }
        len = (size_t)n;
        pos = 0;
        return true;
    }

    bool readLine(char* out, size_t outCap) {
        size_t outLen = 0;
        for (;;) {
            if (pos >= len) {
                if (!refill()) {
                    if (outLen > 0) { out[outLen] = '\0'; return true; }
                    return false;
                }
            }
            while (pos < len) {
                const char c = buf[pos++];
                if (c == '\n') {
                    if (outLen > 0 && out[outLen - 1] == '\r') outLen--;
                    out[outLen] = '\0';
                    return true;
                }
                if (outLen < outCap - 1) out[outLen++] = c;
            }
        }
    }
};

static char* trimInPlace(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') return s;
    char* end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\r')) *end-- = '\0';
    return s;
}

static uint32_t utf8Next(const char** pp) {
    const uint8_t* p = (const uint8_t*)*pp;
    if (*p == 0) return 0;
    const uint8_t b0 = *p++;
    uint32_t cp;
    if (b0 < 0x80) {
        cp = b0;
    } else if ((b0 & 0xE0) == 0xC0 && p[0]) {
        cp = ((uint32_t)(b0 & 0x1F) << 6) | (p[0] & 0x3F);
        p += 1;
    } else if ((b0 & 0xF0) == 0xE0 && p[0] && p[1]) {
        cp = ((uint32_t)(b0 & 0x0F) << 12) | ((uint32_t)(p[0] & 0x3F) << 6) | (p[1] & 0x3F);
        p += 2;
    } else if ((b0 & 0xF8) == 0xF0 && p[0] && p[1] && p[2]) {
        cp = ((uint32_t)(b0 & 0x07) << 18) | ((uint32_t)(p[0] & 0x3F) << 12) |
             ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        p += 3;
    } else {
        cp = 0xFFFD;
    }
    *pp = (const char*)p;
    return cp;
}

static inline void pressKeys(uint8_t mods, uint8_t k1, uint8_t k2 = 0) {
    if (mods & MOD_CTRL)  Keyboard.press(KEY_LEFT_CTRL);
    if (mods & MOD_SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
    if (mods & MOD_ALT)   Keyboard.press(KEY_LEFT_ALT);
    if (mods & MOD_GUI)   Keyboard.press(KEY_LEFT_GUI);
    if (mods & MOD_RALT)  Keyboard.press(KEY_RIGHT_ALT);
    if (k1) Keyboard.press(k1);
    delayMicroseconds(KEY_PRESS_US);
    Keyboard.releaseAll();
    delayMicroseconds(KEY_RELEASE_US);
    if (k2) {
        Keyboard.press(k2);
        delayMicroseconds(KEY_PRESS_US);
        Keyboard.release(k2);
        delayMicroseconds(KEY_RELEASE_US);
    }
}

static inline void pressCombo(uint8_t mods, uint8_t key) {
    pressKeys(mods, key);
}

static void typeCodepoint(uint32_t cp) {
    CharEntry e;
    if (lookupChar(g_layout, cp, &e)) {
        pressKeys(e.ralt ? MOD_RALT : MOD_NONE, e.k1, e.k2);
        return;
    }
    if (cp >= 0x20 && cp < 0x80) {
        pressKeys(MOD_NONE, (uint8_t)cp);
    }
}

static void cmdString(const char* text, bool appendEnter, uint32_t charDelayUs = 0) {
    const char* p = text;
    for (;;) {
        const uint32_t cp = utf8Next(&p);
        if (cp == 0) break;
        typeCodepoint(cp);
        if (charDelayUs > 0) delayMicroseconds(charDelayUs);
    }
    if (appendEnter) pressCombo(MOD_NONE, KEY_RETURN);
}

static uint8_t tokenToMod(const char* tok) {
    if (!strcmp(tok, "CTRL") || !strcmp(tok, "CONTROL"))                     return MOD_CTRL;
    if (!strcmp(tok, "SHIFT"))                                               return MOD_SHIFT;
    if (!strcmp(tok, "ALT"))                                                 return MOD_ALT;
    if (!strcmp(tok, "GUI") || !strcmp(tok, "WINDOWS") ||
        !strcmp(tok, "COMMAND") || !strcmp(tok, "META"))                     return MOD_GUI;
    if (!strcmp(tok, "ALTGR") || !strcmp(tok, "RALT"))                       return MOD_RALT;
    return MOD_NONE;
}

static uint8_t tokenToKey(const char* tok) {
    if (!strcmp(tok, "ENTER")  || !strcmp(tok, "RETURN"))      return KEY_RETURN;
    if (!strcmp(tok, "TAB"))                                   return KEY_TAB;
    if (!strcmp(tok, "SPACE"))                                 return ' ';
    if (!strcmp(tok, "BACKSPACE") || !strcmp(tok, "BKSP"))     return KEY_BACKSPACE;
    if (!strcmp(tok, "DELETE")    || !strcmp(tok, "DEL"))      return KEY_DELETE;
    if (!strcmp(tok, "ESCAPE")    || !strcmp(tok, "ESC"))      return KEY_ESC;
    if (!strcmp(tok, "HOME"))                                  return KEY_HOME;
    if (!strcmp(tok, "END"))                                   return KEY_END;
    if (!strcmp(tok, "INSERT"))                                return KEY_INSERT;
    if (!strcmp(tok, "PAGEUP")   || !strcmp(tok, "PAGE_UP"))   return KEY_PAGE_UP;
    if (!strcmp(tok, "PAGEDOWN") || !strcmp(tok, "PAGE_DOWN")) return KEY_PAGE_DOWN;
    if (!strcmp(tok, "UP"))                                    return KEY_UP_ARROW;
    if (!strcmp(tok, "DOWN"))                                  return KEY_DOWN_ARROW;
    if (!strcmp(tok, "LEFT"))                                  return KEY_LEFT_ARROW;
    if (!strcmp(tok, "RIGHT"))                                 return KEY_RIGHT_ARROW;
    if (!strcmp(tok, "CAPSLOCK")    || !strcmp(tok, "CAPS_LOCK"))    return KEY_CAPS_LOCK;
    if (!strcmp(tok, "NUMLOCK")     || !strcmp(tok, "NUM_LOCK"))     return KEY_NUM_LOCK;
    if (!strcmp(tok, "SCROLLLOCK")  || !strcmp(tok, "SCROLL_LOCK"))  return KEY_SCROLL_LOCK;
    if (!strcmp(tok, "PRINTSCREEN") || !strcmp(tok, "PRTSC"))        return KEY_PRINT_SCREEN;
    if (!strcmp(tok, "PAUSE") || !strcmp(tok, "BREAK"))        return KEY_PAUSE;
    if (!strcmp(tok, "MENU")  || !strcmp(tok, "APP"))          return KEY_MENU;
    if (!strcmp(tok, "F1"))  return KEY_F1;   if (!strcmp(tok, "F2"))  return KEY_F2;
    if (!strcmp(tok, "F3"))  return KEY_F3;   if (!strcmp(tok, "F4"))  return KEY_F4;
    if (!strcmp(tok, "F5"))  return KEY_F5;   if (!strcmp(tok, "F6"))  return KEY_F6;
    if (!strcmp(tok, "F7"))  return KEY_F7;   if (!strcmp(tok, "F8"))  return KEY_F8;
    if (!strcmp(tok, "F9"))  return KEY_F9;   if (!strcmp(tok, "F10")) return KEY_F10;
    if (!strcmp(tok, "F11")) return KEY_F11;  if (!strcmp(tok, "F12")) return KEY_F12;
    if (!strcmp(tok, "F13")) return KEY_F13;  if (!strcmp(tok, "F14")) return KEY_F14;
    if (!strcmp(tok, "F15")) return KEY_F15;  if (!strcmp(tok, "F16")) return KEY_F16;
    if (!strcmp(tok, "F17")) return KEY_F17;  if (!strcmp(tok, "F18")) return KEY_F18;
    if (!strcmp(tok, "F19")) return KEY_F19;  if (!strcmp(tok, "F20")) return KEY_F20;
    if (!strcmp(tok, "F21")) return KEY_F21;  if (!strcmp(tok, "F22")) return KEY_F22;
    if (!strcmp(tok, "F23")) return KEY_F23;  if (!strcmp(tok, "F24")) return KEY_F24;
    if (strlen(tok) == 1) return (uint8_t)tok[0];
    return 0;
}

static uint8_t mouseMask(const char* name) {
    if (!strcmp(name, "LEFT"))   return MOUSE_LEFT;
    if (!strcmp(name, "RIGHT"))  return MOUSE_RIGHT;
    if (!strcmp(name, "MIDDLE")) return MOUSE_MIDDLE;
    return 0;
}

static inline const char* skipSpaces(const char* p) {
    while (*p == ' ') p++;
    return p;
}

static char g_lastLine[DUCKY_LINE_MAX];
static bool g_hasLastLine = false;

static void executeLine(char* line) {
    if (!strncmp(line, "REM", 3) || !strncmp(line, "//", 2) || line[0] == '#')
        return;

    if (!strncmp(line, "LAYOUT ", 7)) {
        char* code = trimInPlace(line + 7);
        for (uint8_t i = 0; i < LAYOUT_COUNT; i++) {
            if (!strcmp(code, LAYOUT_DEFS[i].name)) { g_layout = i; return; }
        }
        return;
    }

    if (!strncmp(line, "REPEAT ", 7)) {
        const long n = strtol(line + 7, nullptr, 10);
        if (g_hasLastLine) {
            char scratch[DUCKY_LINE_MAX];
            for (long i = 0; i < n; ++i) {
                strncpy(scratch, g_lastLine, DUCKY_LINE_MAX - 1);
                scratch[DUCKY_LINE_MAX - 1] = '\0';
                executeLine(scratch);
                if (g_defaultDelay) delay(g_defaultDelay);
            }
        }
        return;
    }

    strncpy(g_lastLine, line, DUCKY_LINE_MAX - 1);
    g_lastLine[DUCKY_LINE_MAX - 1] = '\0';
    g_hasLastLine = true;

    if (!strncmp(line, "DELAY ", 6)) {
        delay((uint32_t)strtoul(line + 6, nullptr, 10));
        return;
    }

    if (!strncmp(line, "DEFAULT_DELAY ", 14) || !strncmp(line, "DEFAULTDELAY ", 13)) {
        const char* sp = strchr(line, ' ');
        if (sp) g_defaultDelay = (uint16_t)strtoul(sp + 1, nullptr, 10);
        return;
    }

    if (!strncmp(line, "STRINGLN_DELAY ", 15)) {
        char* rest = line + 15;
        char* end;
        const uint32_t ms = (uint32_t)strtoul(rest, &end, 10);
        if (end != rest && *end == ' ') cmdString(end + 1, true, ms * 1000UL);
        return;
    }

    if (!strncmp(line, "STRING_DELAY ", 13)) {
        char* rest = line + 13;
        char* end;
        const uint32_t ms = (uint32_t)strtoul(rest, &end, 10);
        if (end != rest && *end == ' ') cmdString(end + 1, false, ms * 1000UL);
        return;
    }

    if (!strncmp(line, "STRINGLN ", 9)) {
        cmdString(line + 9, true);
        return;
    }

    if (!strncmp(line, "STRING ", 7)) {
        cmdString(line + 7, false);
        return;
    }

    if (!strncmp(line, "MOUSE_MOVE ", 11)) {
        const char* p = skipSpaces(line + 11);
        char* end;
        long rx = strtol(p, &end, 10);
        p = skipSpaces(end);
        long ry = strtol(p, &end, 10);
        while (rx != 0 || ry != 0) {
            const int8_t dx = (int8_t)constrain(rx, -127, 127);
            const int8_t dy = (int8_t)constrain(ry, -127, 127);
            Mouse.move(dx, dy, 0);
            rx -= dx; ry -= dy;
            delayMicroseconds(KEY_PRESS_US);
        }
        return;
    }

    if (!strncmp(line, "MOUSE_CLICK ", 12)) {
        char* btn = trimInPlace(line + 12);
        const uint8_t m = mouseMask(btn);
        if (m) { Mouse.click(m); delayMicroseconds(KEY_PRESS_US); }
        return;
    }

    if (!strncmp(line, "MOUSE_SCROLL ", 13)) {
        const int8_t w = (int8_t)constrain(strtol(line + 13, nullptr, 10), -127, 127);
        Mouse.move(0, 0, w);
        delayMicroseconds(KEY_PRESS_US);
        return;
    }

    if (!strncmp(line, "MOUSE_PRESS ", 12)) {
        char* btn = trimInPlace(line + 12);
        const uint8_t m = mouseMask(btn);
        if (m) Mouse.press(m);
        return;
    }

    if (!strcmp(line, "MOUSE_RELEASE")) {
        Mouse.release(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);
        return;
    }

    static constexpr int MAX_TOKENS = 6;
    char*   toks[MAX_TOKENS];
    int     count = 0;
    char*   saveptr = nullptr;
    char*   tok = strtok_r(line, " ", &saveptr);
    while (tok && count < MAX_TOKENS) {
        toks[count++] = tok;
        tok = strtok_r(nullptr, " ", &saveptr);
    }

    uint8_t mods = MOD_NONE;
    uint8_t key  = 0;
    for (int i = 0; i < count; ++i) {
        const uint8_t m = tokenToMod(toks[i]);
        if (m)         mods |= m;
        else if (!key) key = tokenToKey(toks[i]);
    }
    if (mods || key) pressCombo(mods, key);
}

static constexpr int MAX_STAGES = 256;
static int g_stages[MAX_STAGES];
static int g_stageCount = 0;

static void discoverStages() {
    g_stageCount = 0;
    File root = SD.open("/");
    if (!root) blinkHalt(500, 500);

    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            const char* dot  = strrchr(name, '.');
            if (dot && dot != name) {
                const bool isTxt = (tolower((unsigned char)dot[1]) == 't') &&
                                    (tolower((unsigned char)dot[2]) == 'x') &&
                                    (tolower((unsigned char)dot[3]) == 't') &&
                                    (dot[4] == '\0');
                if (isTxt) {
                    bool allDigits = true;
                    for (const char* p = name; p < dot; p++) {
                        if (!isdigit((unsigned char)*p)) { allDigits = false; break; }
                    }
                    if (allDigits && g_stageCount < MAX_STAGES) {
                        g_stages[g_stageCount++] = (int)strtol(name, nullptr, 10);
                    }
                }
            }
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    for (int i = 1; i < g_stageCount; i++) {
        const int key = g_stages[i];
        int j = i - 1;
        while (j >= 0 && g_stages[j] > key) { g_stages[j + 1] = g_stages[j]; j--; }
        g_stages[j + 1] = key;
    }
}

void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
    pinMode(PIN_OTA_TRIGGER, INPUT_PULLUP);

    if (digitalRead(PIN_OTA_TRIGGER) == LOW) {
        runOtaMode();
    }

    sdSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, -1);
    if (!SD.begin(PIN_SD_CS, sdSPI, 4000000)) blinkHalt(100, 100);

    if (SD.exists(OTA_TRIGGER_FILE)) {
        SD.remove(OTA_TRIGGER_FILE);
        runOtaMode();
    }

    buildCharTables();

    Keyboard.begin();
    Mouse.begin();
    USB.begin();

    delay(300);

    digitalWrite(PIN_LED, HIGH);
}

void loop() {
    discoverStages();
    if (g_stageCount == 0) blinkHalt(500, 500);

    static LineReader reader;
    static char lineBuf[DUCKY_LINE_MAX];
    char path[32];

    for (int i = 0; i < g_stageCount; i++) {
        g_layout       = LAY_EN;
        g_hasLastLine  = false;

        snprintf(path, sizeof(path), "/%d.txt", g_stages[i]);
        File f = SD.open(path, FILE_READ);
        if (!f) blinkHalt(500, 500);

        reader.begin(f);
        while (reader.readLine(lineBuf, DUCKY_LINE_MAX)) {
            char* line = trimInPlace(lineBuf);
            if (*line == '\0') continue;
            executeLine(line);
            if (g_defaultDelay) delay(g_defaultDelay);
        }

        f.close();

        if (i < g_stageCount - 1) delay(500);
    }

    digitalWrite(PIN_LED, LOW);
    while (true) { delay(1000); }
}
