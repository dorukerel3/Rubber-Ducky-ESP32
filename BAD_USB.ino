#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"

USBHIDKeyboard Keyboard;
USBHIDMouse Mouse;

#define SD_CS   10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK  12

#define LED_PIN 46 

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Keyboard.begin();
  Mouse.begin();
  USB.begin();

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Kart bulunamadi veya baslatilamadi!");
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }
  Serial.println("SD Kart baglantisi basarili.");

  delay(500); 

  File file = SD.open("/payload.txt");
  if (!file) {
    Serial.println("payload.txt dosyasi acilamadi!");
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }

  digitalWrite(LED_PIN, HIGH);

  String line = "";
  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      line.trim();
      executeCommand(line);
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  
  if (line.length() > 0) {
    line.trim();
    executeCommand(line);
  }
  
  file.close();

  digitalWrite(LED_PIN, LOW);
}

void loop() {
}

void executeCommand(String cmd) {
  if (cmd.length() == 0 || cmd.startsWith("//")) return;

  int spaceIndex = cmd.indexOf(' ');
  String instruction = cmd;
  String argument = "";

  if (spaceIndex != -1) {
    instruction = cmd.substring(0, spaceIndex);
    argument = cmd.substring(spaceIndex + 1);
  }

  instruction.toUpperCase();

  if (instruction == "STRING") {
    for (int i = 0; i < argument.length(); i++) {
      Keyboard.print(argument[i]);
      delay(50); 
    }
  }
  else if (instruction == "DELAY") {
    delay(argument.toInt());
  }
  else if (instruction == "ENTER") {
    Keyboard.write(KEY_RETURN);
  }
  else if (instruction == "GUI" || instruction == "WINDOWS") {
    Keyboard.press(KEY_LEFT_GUI);
    if (argument.length() > 0) {
      Keyboard.press(argument[0]); 
    }
    delay(100);
    Keyboard.releaseAll();
  }
  else if (instruction == "CTRL") {
    Keyboard.press(KEY_LEFT_CTRL);
    if (argument.length() > 0) {
      Keyboard.press(argument[0]);
    }
    delay(100);
    Keyboard.releaseAll();
  }
  
  else if (instruction == "MOUSE_MOVE") {
    int commaIndex = argument.indexOf(' ');
    if (commaIndex != -1) {
      int x = argument.substring(0, commaIndex).toInt();
      int y = argument.substring(commaIndex + 1).toInt();
      Mouse.move(x, y);
    }
  }
  else if (instruction == "MOUSE_CLICK") {
    argument.toUpperCase();
    if (argument == "RIGHT") Mouse.click(MOUSE_RIGHT);
    else if (argument == "MIDDLE") Mouse.click(MOUSE_MIDDLE);
    else Mouse.click(MOUSE_LEFT); 
  }
  else if (instruction == "SCROLL") {
    Mouse.move(0, 0, argument.toInt());
  }

  delay(50); 
}
