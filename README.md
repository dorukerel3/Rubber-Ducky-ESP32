# Rubber-Ducky-ESP32 (Silicon Ghost)

Custom **ESP32-S3** HID-injection tool in a USB flash-drive form factor. Runs a DuckyScript-compatible payload interpreter directly from a MicroSD card, with keyboard + mouse injection, five keyboard layouts, sequential multi-stage execution, and a browser based OTA firmware updater, all in a single-`.ino` sketch built around a heap-fragmentation-free, industrial-grade hot path.

The firmware is engineered to reach the same effective per-keystroke throughput class as commercial injectors (Hak5 Rubber Ducky / Flipper Zero BadUSB), while still reading uncompiled `.txt` payloads from the SD card on the fly, no `.bin` pre-compilation step required.

---

## Table of Contents

- [Feature Summary](#feature-summary)
- [Hardware](#hardware)
- [Architecture Highlights](#architecture-highlights)
- [SD Card Layout](#sd-card-layout)
- [DuckyScript Command Reference](#duckyscript-command-reference)
- [Keyboard Layouts](#keyboard-layouts)
- [OTA Firmware Update](#ota-firmware-update)
- [LED Status Codes](#led-status-codes)
- [Build & Flash](#build--flash)
- [Configuration Constants](#configuration-constants)
- [Repository Contents](#repository-contents)
- [Disclaimer](#disclaimer)
- [License](#license)

---

## Feature Summary

| Capability | Detail |
|---|---|
| USB enumeration | Native composite HID (keyboard + mouse) via ESP32-S3 TinyUSB stack, no host-side driver install |
| Payload source | Uncompiled `.txt` DuckyScript files, read live from MicroSD, no pre-compilation |
| Multi-stage execution | Sequential run of every `N.txt` file in card root, sorted numerically |
| Keyboard layouts | 5 layouts: **EN, TR, FR, DE, JP**, switchable mid-payload with `LAYOUT` |
| Character resolution | **O(1)** 256-entry lookup tables per layout + tiny sorted overflow table for code points > 255 |
| I/O strategy | 512-byte chunked SD reads into a fixed RAM buffer; zero character-by-character file reads |
| Memory model | **Zero use of the Arduino `String` class**. All parsing on `char[]` buffers with `strtok_r`/`strtol`/`strchr`. No heap allocations after `setup()` |
| HID timing | `delayMicroseconds()`-based press/release intervals (default 1 ms), an order of magnitude tighter than a naive Arduino sketch |
| OTA updates | Browser based upload page over a self-hosted WiFi Access Point, gated by a boot-time trigger |
| Status feedback | single LED signaling for ready / SD error / no payload / OTA modes |

---

## Hardware

| Aspect | Part / Value |
|---|---|
| Microcontroller | **ESP32-S3-WROOM-2** (N32R8V recommended, 32 MB flash / 8 MB PSRAM) |
| Power regulation | AP2112K-3.3 (5 V USB → 3.3 V LDO) |
| USB connector | Molex 48037-2200 Type-A |
| SD connector | Hirose DM3AT-SF-PEJM5 (MicroSD, SPI mode) |
| Status LED | GPIO 46 |
| SD SPI bus | HSPI: CS 10, MOSI 11, SCK 12, MISO 13 |
| OTA trigger pin | GPIO 4 (`INPUT_PULLUP`, hold to GND at boot) |

PCB schematic and layout designed in **KiCad**. Full Gerbers, BOM/CPL, 3D models and assembly notes for independent fabrication live under `Bad_Usb_Files/`.

---

## Architecture Highlights

The firmware is written to industrial-embedded standards, not hobby Arduino conventions.

**Zero-allocation hot path.** Every text-processing routine (line reading, tokenisation, command dispatch, UTF-8 decoding) operates on stack-resident `char[]` buffers. The Arduino `String` class is not linked anywhere in the runtime, eliminating heap fragmentation entirely.

**Chunked buffered I/O.** SD payloads are read in 512-byte blocks into a `LineReader` struct that walks a pointer through the buffer, transparently stitching lines across chunk boundaries. `File::readStringUntil()` is not used.

**O(1) character lookup.** Every layout's charmap is expanded once at boot (`buildCharTables()`) into a flat 256-entry array indexed directly by Unicode code point. A single memory dereference (`g_table256[layout][cp]`) retrieves the HID report (modifier + keycode) for any Latin-1 character. Code points above 255 (TR `İığĞşŞ`, FR `œŒ`, the shared `€`) live in a per-layout overflow table of ≤ 8 entries, serviced by binary search: mathematically O(log n) on a single-digit n, effectively free.

**Microsecond HID timing.** Press/release spacing runs on `delayMicroseconds()` (default 1 ms per phase) rather than the millisecond-scale `delay()` calls a typical sketch uses. The interval sits above the standard 1 ms USB HID polling interrupt so no reports are dropped, but well below anything a human scale delay would impose, putting the effective character rate in the same class as dedicated commercial injectors.

**Fixed size stage table.** SD root discovery lands into a static `int[256]` sorted with an insertion sort. No `std::vector`, no `std::sort`, no allocator activity.

---

## SD Card Layout

Format the card as **FAT32**. The firmware only touches the root directory.

- Files are matched by exact pattern: **numeric basename + `.txt`** (case-insensitive extension).
- Between stages the interpreter waits 500 ms, resets `g_layout` to `EN`, and clears the `REPEAT` history; stages do not leak state.
- After the last stage completes the LED turns off and the device halts. It does **not** loop back and rerun stage 1 automatically.

---

## DuckyScript Command Reference

All commands are case-sensitive and one per line. Blank lines and lines beginning with `REM`, `//`, or `#` are ignored.

### Text injection

| Command | Behavior |
|---|---|
| `STRING <text>` | Type UTF-8 text through the active layout |
| `STRINGLN <text>` | Same as `STRING`, then press `ENTER` |
| `STRING_DELAY <ms> <text>` | Type text with an inter-character pause of `<ms>` milliseconds |
| `STRINGLN_DELAY <ms> <text>` | As above, then `ENTER` |

### Timing

| Command | Behavior |
|---|---|
| `DELAY <ms>` | Blocking wait for `<ms>` milliseconds |
| `DEFAULT_DELAY <ms>` / `DEFAULTDELAY <ms>` | Insert this delay after every subsequent command |

### Modifier / key combos

Combine any of `CTRL`/`CONTROL`, `SHIFT`, `ALT`, `GUI`/`WINDOWS`/`COMMAND`/`META`, `ALTGR`/`RALT` with one named key or single character.

### Named keys

`ENTER`/`RETURN`, `TAB`, `SPACE`, `BACKSPACE`/`BKSP`, `DELETE`/`DEL`, `ESCAPE`/`ESC`, `HOME`, `END`, `INSERT`, `PAGEUP`/`PAGE_UP`, `PAGEDOWN`/`PAGE_DOWN`, `UP`/`DOWN`/`LEFT`/`RIGHT`, `CAPSLOCK`/`CAPS_LOCK`, `NUMLOCK`/`NUM_LOCK`, `SCROLLLOCK`/`SCROLL_LOCK`, `PRINTSCREEN`/`PRTSC`, `PAUSE`/`BREAK`, `MENU`/`APP`, `F1` through `F24`.

### Mouse

| Command | Behavior |
|---|---|
| `MOUSE_MOVE <dx> <dy>` | Relative move; any range, auto-chunked into ±127 HID reports |
| `MOUSE_CLICK LEFT\|RIGHT\|MIDDLE` | Single click |
| `MOUSE_PRESS LEFT\|RIGHT\|MIDDLE` | Hold button down (no auto-release) |
| `MOUSE_RELEASE` | Release all mouse buttons |
| `MOUSE_SCROLL <n>` | Vertical wheel scroll, `-127..127` |

### Control flow

| Command | Behavior |
|---|---|
| `REPEAT <n>` | Run the previous non-`REPEAT` line `<n>` times |
| `LAYOUT <code>` | Switch active keyboard layout: `EN`, `TR`, `FR`, `DE`, `JP` |

---

## Keyboard Layouts

Layouts are compile-time embedded and activated at runtime with `LAYOUT`.

| Code | Coverage |
|---|---|
| `EN` | US ANSI. No remapping, all ASCII passes straight through to the HID report. |
| `TR` | Turkish-Q. Full charmap for `ı İ ğ Ğ ş Ş ö Ö ü Ü ç Ç`, all AltGr-shifted symbols, and `€`. |
| `FR` | French AZERTY with dead-key composition for accented vowels, plus `œ Œ` and `€`. |
| `DE` | German QWERTZ. `ä Ä ö Ö ü Ü ß` and `€`, with a small selection of dead-key composed accents. |
| `JP` | JIS punctuation charmap for a host configured with the Japanese JIS layout. Full kana input is not supported. |

At the start of each stage `g_layout` resets to `EN`.

---

## OTA Firmware Update

The firmware ships with a browser-based OTA updater that runs on a self-hosted WiFi Access Point. It is **off by default** and cannot be reached from the target machine or the surrounding network under normal operation.

### Trigger

Two independent triggers, either alone is enough:

1. **SD card file**:  drop an empty file named `OTA.TXT` in the SD root and reset the board. The firmware detects the file, deletes it immediately, and enters OTA mode.
2. **Hardware pin**:  hold `GPIO 4` to `GND` while the board powers on / resets.

If neither trigger is present, no radio is initialised, no AP is broadcast, and the OTA code path never runs. HID timing is completely unaffected.

### Using it

1. Trigger OTA mode. LED begins a slow **double-blink**.
2. Join the WiFi network **`BadUSB-OTA`** with `OTA_AP_PASSWORD`.
3. Browse to **`http://192.168.4.1/`**.
4. The browser prompts a Basic-Auth login: username `admin`, password `OTA_UPLOAD_PASSWORD`.
5. Pick a compiled `.bin` and click **Upload**.
6. On success the board auto-reboots. On failure it reboots back into the previous firmware. A bad or interrupted upload cannot brick the device.
7. If no upload occurs within **3 minutes**, OTA mode times out and boots normally.

### Security notes

- Both passwords **must** be set to real values before flashing; the shipped file uses `CHANGE_ME_*` placeholders on purpose.
- `OTA_AP_PASSWORD` must be at least **8 characters** or `WiFi.softAP()` will silently create an *open* network.
- Basic Auth over the AP is clear-text on the RF side. Treat OTA mode as a physical-access convenience, not a hardened remote-update channel.

---

## LED Status Codes

| Pattern | Meaning |
|---|---|
| Solid ON | Normal boot succeeded; running payloads |
| Solid OFF (after run) | All payload stages completed; device halted |
| Fast blink 100 ms on / 100 ms off | SD card failed to mount |
| Slow blink 500 ms on / 500 ms off | No `N.txt` payload files found, or a payload file failed to open |
| Slow double-blink (OTA mode) | AP broadcasting, waiting for upload |
| Solid ON / rapid flicker (OTA mode) | Firmware upload in progress |

---

## Build & Flash

**Board target:** ESP32-S3 (`Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`).

| Setting | Value |
|---|---|
| USB CDC On Boot | Enabled |
| USB Mode | `USB-OTG (TinyUSB)` |
| USB Firmware MSC On Boot | Disabled |
| USB DFU On Boot | Disabled |
| Partition Scheme | `Default 4MB with spiffs` (or `Huge APP` for 32 MB N32R8V) |
| Flash Size | Match module (16 MB / 32 MB) |

Libraries: `USB`, `USBHIDKeyboard`, `USBHIDMouse`, `SD`, `SPI`, `WiFi`, `WebServer`, `Update` all ship with the ESP32 Arduino core; no external installs required.

### First flash

1. Open `Bad_Usb_Files/BadUSB_Optimized.ino`.
2. Change the four `CHANGE_ME_*` placeholders near the top of the file.
3. Connect the board over USB, select its serial port, hit **Upload**.
4. Copy your `N.txt` payloads onto a FAT32-formatted MicroSD card and insert it. Power cycle.

### Subsequent updates

Either reflash over USB, or use the OTA flow: **Sketch → Export Compiled Binary**, drop `OTA.TXT` on the card, reboot, upload the `.bin` at `http://192.168.4.1/`.

---

## Configuration Constants

| Constant | Purpose | Default |
|---|---|---|
| `PIN_SD_CS`, `PIN_SD_MOSI`, `PIN_SD_SCK`, `PIN_SD_MISO` | SD SPI wiring | 10 / 11 / 12 / 13 |
| `PIN_LED` | Status LED | 46 |
| `PIN_OTA_TRIGGER` | OTA trigger pin (LOW at boot = enter OTA) | 4 |
| `KEY_PRESS_US`, `KEY_RELEASE_US` | HID press / release intervals | 1000 µs each |
| `CHUNK_SIZE`, `DUCKY_LINE_MAX` | SD read chunk + max line length | 512 / 512 bytes |
| `MAX_STAGES` | Cap on `N.txt` payload files scanned | 256 |
| `OTA_AP_SSID` | Broadcast AP name in OTA mode | `BadUSB-OTA` |
| `OTA_AP_PASSWORD` | WPA2 password for the AP (≥ 8 chars) | placeholder |
| `OTA_UPLOAD_USERNAME` / `OTA_UPLOAD_PASSWORD` | Basic Auth for the upload page | `admin` / placeholder |
| `OTA_TRIGGER_FILE` | SD filename that triggers OTA on boot | `/OTA.TXT` |
| `OTA_IDLE_TIMEOUT_MS` | OTA mode auto-exit after this much no-activity | 180 000 (3 min) |

> **Note:** the internal line-buffer constant is named `DUCKY_LINE_MAX` (not `LINE_MAX`) to avoid a preprocessor collision with the POSIX `LINE_MAX` macro pulled in transitively by `Arduino.h → FreeRTOS → limits.h` on the Xtensa toolchain.

---

## Repository Contents

```
/
├── Bad_Usb_Files/           ← firmware source, PCB Gerbers, BOM/CPL, 3D models
│   └── BadUSB_Optimized.ino
├── images/                  ← hardware photos and diagrams
├── LICENSE                  ← MIT
└── README.md                ← this file
```

---

## Disclaimer

This project is developed strictly for **educational purposes and authorized security research**. It is **not intended for use on systems, networks, or hardware without the explicit consent of the owner**. The author accepts no responsibility for misuse. Local law regarding possession and use of HID-injection tooling varies; verify your jurisdiction before building or carrying the device.

---

## License

MIT. See [`LICENSE`](LICENSE).

**Author:** Doruk Erel
