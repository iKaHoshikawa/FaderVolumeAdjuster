#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

unsigned long lastRefreshTime;
unsigned long lastResponseTime = 0;
int ifButtonPressed = 0;
bool ifScreenOn = true;
String ScreenLine1 = "", ScreenLine2 = "", ScreenLine3 = "", ScreenVol = "", ScreenLine4 = "", ScreenLine5 = "", Device = "";
bool handshaking = true;
bool connectedornot = false;
int status = 0; //0为静止，1为正在由推子调节电脑音量，2为正在根据上位机音量调节推子。根据状态，有一方的大小比对要停止。

String dataBuffer = "";
String cmdBuffer = "";

int getValue(String data, int index) {
  int start = 0;
  for (int i = 0; i < index; i++) {
    start = data.indexOf(',', start) + 1;
  }
  int end = data.indexOf(',', start);
  if (end == -1) end = data.length();
  return data.substring(start, end).toInt();
}

void setup() {
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(2, 1);
  ScreenLine1 = "Fader Volume Adjuster";
  ScreenLine2 = "  by iKa Hoshikawa   ";
  ScreenLine3 = "   Starting up...    ";
  oled.println(ScreenLine1);
  oled.println(ScreenLine2);
  oled.println(ScreenLine3);
  oled.display();
  lastRefreshTime = millis();
}

void loop() {
  pinMode(4, INPUT_PULLUP);
  if ((ifButtonPressed == 1 &&  digitalRead(4) == 0)) {
    if (ifScreenOn == false) {
        oled.setTextSize(1);
        oled.setCursor(0, 1);
        oled.println(ScreenLine1);
        oled.println(ScreenLine2);
        oled.println(ScreenLine3);
        oled.display();
      ifScreenOn = true;
    }
    lastRefreshTime = millis();
  }
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() > 0) {
        cmdBuffer.trim();
        if (cmdBuffer == "INEEDU") {
          Serial.println("IMCOMING");
        } else if (cmdBuffer == "CONNECTING_") {
          Serial.println("CON_DONE_");
          if (!connectedornot){
            oled.clearDisplay();
            oled.setTextSize(2);
            oled.setCursor(0, 0);
            oled.println("Connected");
            oled.println("^_^");
            oled.display();
            ifScreenOn = true;
            lastRefreshTime = millis();
          }
          handshaking = false;
          connectedornot = true;
        }
        cmdBuffer = "";
      }
      continue;
    }
    if (c == '#') {
      dataBuffer = "#";
      continue;
    }
    if (c == '$') {
      if (dataBuffer.startsWith("#")) {
        String raw = dataBuffer.substring(1);
        /*T = getValue(raw, 0);
        CL = getValue(raw, 1);
        CT = getValue(raw, 2);
        CP = getValue(raw, 3);
        GL = getValue(raw, 4);
        GT = getValue(raw, 5);
        GP = getValue(raw, 6);
        RL = getValue(raw, 7);
        CTH = getValue(raw, 8);
        GTH = getValue(raw, 9);*/
        lastResponseTime = millis();
        connectedornot = true;
        handshaking = false;
      }
      dataBuffer = "";
      continue;
    }
    if (handshaking) {
      cmdBuffer += c;
    } else {
      dataBuffer += c;
    }
  }
  if (connectedornot && (millis() - lastResponseTime >= 5000)) {
    connectedornot = false;
    handshaking = true;
    cmdBuffer = "";
    dataBuffer = "";
  }
  if (handshaking) {
    ScreenLine1 = "Fader Volume Adjuster";
    ScreenLine2 = "  by iKa Hoshikawa   ";
    ScreenLine3 = "Zzzzzzzzzzzzzzz......";
  }else{
    ScreenLine1 = "Current Volume:";
    ScreenLine2 = "";
    ScreenLine3 = "         %         ";
  }
  if(millis() - lastRefreshTime > 3000){
    oled.clearDisplay();
    oled.display();
    ifScreenOn = false;
    lastRefreshTime = millis();
  }
  ifButtonPressed = digitalRead(4);
}