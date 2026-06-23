#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include <SPI.h>
#include <SD.h>
#include <vector>
#include <algorithm>

static constexpr uint8_t  PIN_SD_CS    = 10;
static constexpr uint8_t  PIN_SD_MOSI  = 11;
static constexpr uint8_t  PIN_SD_SCK   = 12;
static constexpr uint8_t  PIN_SD_MISO  = 13;
static constexpr uint8_t  PIN_LED      = 46;

static constexpr uint8_t KEY_PRESS_MS   = 10;
static constexpr uint8_t KEY_RELEASE_MS =  5;

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

struct CharMap {
    uint16_t cp;
    char     k1;
    char     k2;
    bool     ralt;
};

static const CharMap EN_MAP[] = {};
static const uint16_t EN_MAP_LEN = 0;

static const CharMap TR_MAP[] = {
    {0x0069, (char)0x27, 0,   false},
    {0x0130, (char)0x22, 0,   false},
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
static const uint16_t TR_MAP_LEN = sizeof(TR_MAP) / sizeof(TR_MAP[0]);

static const CharMap FR_MAP[] = {
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
static const uint16_t FR_MAP_LEN = sizeof(FR_MAP) / sizeof(FR_MAP[0]);

static const CharMap DE_MAP[] = {
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
static const uint16_t DE_MAP_LEN = sizeof(DE_MAP) / sizeof(DE_MAP[0]);

static const CharMap JP_MAP[] = {};
static const uint16_t JP_MAP_LEN = 0;

enum LayoutID : uint8_t { LAY_EN = 0, LAY_TR, LAY_FR, LAY_DE, LAY_JP };

struct LayoutInfo {
    const char*    name;
    const CharMap* map;
    uint16_t       len;
};

static const LayoutInfo LAYOUTS[] = {
    {"EN", EN_MAP, EN_MAP_LEN},
    {"TR", TR_MAP, TR_MAP_LEN},
    {"FR", FR_MAP, FR_MAP_LEN},
    {"DE", DE_MAP, DE_MAP_LEN},
    {"JP", JP_MAP, JP_MAP_LEN},
};
static constexpr uint8_t LAYOUT_COUNT = sizeof(LAYOUTS) / sizeof(LAYOUTS[0]);

static uint8_t g_layout = LAY_EN;

static USBHIDKeyboard Keyboard;
static USBHIDMouse    Mouse;
static SPIClass       sdSPI(HSPI);

static uint16_t g_defaultDelay = 0;
static String   g_lastLine;

static void blinkHalt(uint16_t onMs, uint16_t offMs) {
    for (;;) {
        digitalWrite(PIN_LED, HIGH); delay(onMs);
        digitalWrite(PIN_LED, LOW);  delay(offMs);
    }
}

static int tokenize(const String& line, String* out, int maxTok) {
    int n = 0, pos = 0;
    const int len = (int)line.length();
    while (pos < len && n < maxTok) {
        while (pos < len && line[pos] == ' ') ++pos;
        if (pos >= len) break;
        const int start = pos;
        while (pos < len && line[pos] != ' ') ++pos;
        out[n++] = line.substring(start, pos);
    }
    return n;
}

static uint32_t utf8Next(const String& s, int* pos) {
    const int len = (int)s.length();
    if (*pos >= len) return 0;
    const uint8_t b0 = (uint8_t)s[(*pos)++];
    if (b0 < 0x80) return b0;
    if ((b0 & 0xE0) == 0xC0 && *pos < len) {
        const uint8_t b1 = (uint8_t)s[(*pos)++];
        return ((uint32_t)(b0 & 0x1F) << 6) | (b1 & 0x3F);
    }
    if ((b0 & 0xF0) == 0xE0 && *pos + 1 < len) {
        const uint8_t b1 = (uint8_t)s[(*pos)++];
        const uint8_t b2 = (uint8_t)s[(*pos)++];
        return ((uint32_t)(b0 & 0x0F) << 12) |
               ((uint32_t)(b1 & 0x3F) <<  6) |
               (          (b2 & 0x3F));
    }
    if ((b0 & 0xF8) == 0xF0 && *pos + 2 < len) {
        const uint8_t b1 = (uint8_t)s[(*pos)++];
        const uint8_t b2 = (uint8_t)s[(*pos)++];
        const uint8_t b3 = (uint8_t)s[(*pos)++];
        return ((uint32_t)(b0 & 0x07) << 18) |
               ((uint32_t)(b1 & 0x3F) << 12) |
               ((uint32_t)(b2 & 0x3F) <<  6) |
               (          (b3 & 0x3F));
    }
    return 0xFFFD;
}

static void pressCombo(uint8_t mods, uint8_t key) {
    if (mods & MOD_CTRL)  Keyboard.press(KEY_LEFT_CTRL);
    if (mods & MOD_SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
    if (mods & MOD_ALT)   Keyboard.press(KEY_LEFT_ALT);
    if (mods & MOD_GUI)   Keyboard.press(KEY_LEFT_GUI);
    if (mods & MOD_RALT)  Keyboard.press(KEY_RIGHT_ALT);
    if (key)              Keyboard.press(key);
    delay(KEY_PRESS_MS);
    Keyboard.releaseAll();
    delay(KEY_RELEASE_MS);
}

static void typeCodepoint(uint32_t cp) {
    const LayoutInfo& L = LAYOUTS[g_layout];
    for (uint16_t i = 0; i < L.len; i++) {
        if ((uint32_t)L.map[i].cp == cp) {
            const CharMap& e = L.map[i];
            if (e.ralt) Keyboard.press(KEY_RIGHT_ALT);
            Keyboard.press(e.k1);
            delay(KEY_PRESS_MS);
            Keyboard.releaseAll();
            delay(KEY_RELEASE_MS);
            if (e.k2) {
                Keyboard.press(e.k2);
                delay(KEY_PRESS_MS);
                Keyboard.release(e.k2);
                delay(KEY_RELEASE_MS);
            }
            return;
        }
    }
    if (cp >= 0x20 && cp < 0x80) {
        Keyboard.press((char)cp);
        delay(KEY_PRESS_MS);
        Keyboard.release((char)cp);
        delay(KEY_RELEASE_MS);
    }
}

static void cmdString(const String& text, bool appendEnter, uint16_t charDelayMs = 0) {
    int pos = 0;
    while (pos < (int)text.length()) {
        const uint32_t cp = utf8Next(text, &pos);
        if (cp == 0) break;
        typeCodepoint(cp);
        if (charDelayMs > 0) delay(charDelayMs);
    }
    if (appendEnter) pressCombo(MOD_NONE, KEY_RETURN);
}

static uint8_t tokenToMod(const String& tok) {
    if (tok == "CTRL"  || tok == "CONTROL")                      return MOD_CTRL;
    if (tok == "SHIFT")                                          return MOD_SHIFT;
    if (tok == "ALT")                                            return MOD_ALT;
    if (tok == "GUI" || tok == "WINDOWS" || tok == "COMMAND"
                     || tok == "META")                           return MOD_GUI;
    if (tok == "ALTGR" || tok == "RALT")                         return MOD_RALT;
    return MOD_NONE;
}

static uint8_t tokenToKey(const String& tok) {
    if (tok == "ENTER"  || tok == "RETURN")          return KEY_RETURN;
    if (tok == "TAB")                                return KEY_TAB;
    if (tok == "SPACE")                              return ' ';
    if (tok == "BACKSPACE" || tok == "BKSP")         return KEY_BACKSPACE;
    if (tok == "DELETE"    || tok == "DEL")          return KEY_DELETE;
    if (tok == "ESCAPE"    || tok == "ESC")          return KEY_ESC;
    if (tok == "HOME")                               return KEY_HOME;
    if (tok == "END")                                return KEY_END;
    if (tok == "INSERT")                             return KEY_INSERT;
    if (tok == "PAGEUP"   || tok == "PAGE_UP")       return KEY_PAGE_UP;
    if (tok == "PAGEDOWN" || tok == "PAGE_DOWN")     return KEY_PAGE_DOWN;
    if (tok == "UP")                                 return KEY_UP_ARROW;
    if (tok == "DOWN")                               return KEY_DOWN_ARROW;
    if (tok == "LEFT")                               return KEY_LEFT_ARROW;
    if (tok == "RIGHT")                              return KEY_RIGHT_ARROW;
    if (tok == "CAPSLOCK"   || tok == "CAPS_LOCK")   return KEY_CAPS_LOCK;
    if (tok == "NUMLOCK"    || tok == "NUM_LOCK")    return KEY_NUM_LOCK;
    if (tok == "SCROLLLOCK" || tok == "SCROLL_LOCK") return KEY_SCROLL_LOCK;
    if (tok == "PRINTSCREEN"|| tok == "PRTSC")       return KEY_PRINT_SCREEN;
    if (tok == "PAUSE" || tok == "BREAK")            return KEY_PAUSE;
    if (tok == "MENU"  || tok == "APP")              return KEY_MENU;
    if (tok == "F1")  return KEY_F1;   if (tok == "F2")  return KEY_F2;
    if (tok == "F3")  return KEY_F3;   if (tok == "F4")  return KEY_F4;
    if (tok == "F5")  return KEY_F5;   if (tok == "F6")  return KEY_F6;
    if (tok == "F7")  return KEY_F7;   if (tok == "F8")  return KEY_F8;
    if (tok == "F9")  return KEY_F9;   if (tok == "F10") return KEY_F10;
    if (tok == "F11") return KEY_F11;  if (tok == "F12") return KEY_F12;
    if (tok == "F13") return KEY_F13;  if (tok == "F14") return KEY_F14;
    if (tok == "F15") return KEY_F15;  if (tok == "F16") return KEY_F16;
    if (tok == "F17") return KEY_F17;  if (tok == "F18") return KEY_F18;
    if (tok == "F19") return KEY_F19;  if (tok == "F20") return KEY_F20;
    if (tok == "F21") return KEY_F21;  if (tok == "F22") return KEY_F22;
    if (tok == "F23") return KEY_F23;  if (tok == "F24") return KEY_F24;
    if (tok.length() == 1) return (uint8_t)tok[0];
    return 0;
}

static uint8_t mouseMask(const String& name) {
    if (name == "LEFT")   return MOUSE_LEFT;
    if (name == "RIGHT")  return MOUSE_RIGHT;
    if (name == "MIDDLE") return MOUSE_MIDDLE;
    return 0;
}

static void executeLine(const String& line) {
    if (line.startsWith("REM") || line.startsWith("//") || line.startsWith("#"))
        return;

    if (line.startsWith("LAYOUT ")) {
        String code = line.substring(7);
        code.trim();
        for (uint8_t i = 0; i < LAYOUT_COUNT; i++) {
            if (code == LAYOUTS[i].name) { g_layout = i; return; }
        }
        return;
    }

    if (line.startsWith("REPEAT ")) {
        const int n = line.substring(7).toInt();
        for (int i = 0; i < n; ++i) {
            executeLine(g_lastLine);
            if (g_defaultDelay) delay(g_defaultDelay);
        }
        return;
    }

    g_lastLine = line;

    if (line.startsWith("DELAY ")) {
        delay((uint32_t)line.substring(6).toInt());
        return;
    }

    if (line.startsWith("DEFAULT_DELAY ") || line.startsWith("DEFAULTDELAY ")) {
        g_defaultDelay = (uint16_t)line.substring(line.indexOf(' ') + 1).toInt();
        return;
    }

    if (line.startsWith("STRING ")) {
        cmdString(line.substring(7), false);
        return;
    }

    if (line.startsWith("STRINGLN ")) {
        cmdString(line.substring(9), true);
        return;
    }

    if (line.startsWith("STRING_DELAY ")) {
        const String rest = line.substring(13);
        const int    sp   = rest.indexOf(' ');
        if (sp > 0) {
            const uint16_t ms = (uint16_t)rest.substring(0, sp).toInt();
            cmdString(rest.substring(sp + 1), false, ms);
        }
        return;
    }

    if (line.startsWith("STRINGLN_DELAY ")) {
        const String rest = line.substring(15);
        const int    sp   = rest.indexOf(' ');
        if (sp > 0) {
            const uint16_t ms = (uint16_t)rest.substring(0, sp).toInt();
            cmdString(rest.substring(sp + 1), true, ms);
        }
        return;
    }

    if (line.startsWith("MOUSE_MOVE ")) {
        const String args = line.substring(11);
        const int    sp   = args.indexOf(' ');
        if (sp > 0) {
            int16_t rx = (int16_t)args.substring(0, sp).toInt();
            int16_t ry = (int16_t)args.substring(sp + 1).toInt();
            while (rx != 0 || ry != 0) {
                const int8_t dx = (int8_t)constrain(rx, -127, 127);
                const int8_t dy = (int8_t)constrain(ry, -127, 127);
                Mouse.move(dx, dy, 0);
                rx -= dx; ry -= dy;
                delay(KEY_PRESS_MS);
            }
        }
        return;
    }

    if (line.startsWith("MOUSE_CLICK ")) {
        String btn = line.substring(12); btn.trim();
        const uint8_t m = mouseMask(btn);
        if (m) { Mouse.click(m); delay(KEY_PRESS_MS); }
        return;
    }

    if (line.startsWith("MOUSE_SCROLL ")) {
        const int8_t w = (int8_t)constrain(line.substring(13).toInt(), -127, 127);
        Mouse.move(0, 0, w);
        delay(KEY_PRESS_MS);
        return;
    }

    if (line.startsWith("MOUSE_PRESS ")) {
        String btn = line.substring(12); btn.trim();
        const uint8_t m = mouseMask(btn);
        if (m) Mouse.press(m);
        return;
    }

    if (line == "MOUSE_RELEASE") {
        Mouse.release(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);
        return;
    }

    String   toks[6];
    const int count = tokenize(line, toks, 6);
    uint8_t  mods   = MOD_NONE;
    uint8_t  key    = 0;
    for (int i = 0; i < count; ++i) {
        const uint8_t m = tokenToMod(toks[i]);
        if (m)         mods |= m;
        else if (!key) key = tokenToKey(toks[i]);
    }
    if (mods || key) pressCombo(mods, key);
}

void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    Keyboard.begin();
    Mouse.begin();
    USB.begin();

    delay(300);

    sdSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, -1);
    if (!SD.begin(PIN_SD_CS, sdSPI, 4000000)) blinkHalt(100, 100);

    digitalWrite(PIN_LED, HIGH);
}

void loop() {
    File root = SD.open("/");
    if (!root) blinkHalt(500, 500);

    std::vector<int> stages;

    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = String(entry.name());
            int dotPos = name.lastIndexOf('.');
            if (dotPos > 0) {
                String ext  = name.substring(dotPos + 1);
                String base = name.substring(0, dotPos);
                ext.toLowerCase();
                if (ext == "txt" && base.length() > 0) {
                    bool allDigits = true;
                    for (int i = 0; i < (int)base.length(); i++) {
                        if (!isDigit(base[i])) { allDigits = false; break; }
                    }
                    if (allDigits) stages.push_back(base.toInt());
                }
            }
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    if (stages.empty()) blinkHalt(500, 500);

    std::sort(stages.begin(), stages.end());

    for (int i = 0; i < (int)stages.size(); i++) {
        g_layout = LAY_EN;

        String path = "/" + String(stages[i]) + ".txt";
        File f = SD.open(path, FILE_READ);
        if (!f) blinkHalt(500, 500);

        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (!line.length()) continue;
            executeLine(line);
            if (g_defaultDelay) delay(g_defaultDelay);
        }

        f.close();

        if (i < (int)stages.size() - 1) delay(500);
    }

    digitalWrite(PIN_LED, LOW);
    while (true) { delay(1000); }
}
