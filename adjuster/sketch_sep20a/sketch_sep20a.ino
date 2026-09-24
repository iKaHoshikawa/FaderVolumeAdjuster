#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneButton.h>
#include <Keyboard.h>

#define AIN1 5
#define AIN2 6
#define touchSensor 4
#define STBY 7
#define faderTouch A1

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
OneButton sensor(touchSensor, false);

unsigned long lastRefreshTime;
unsigned long lastResponseTime = 0;
bool ifScreenOn = true;
String ScreenLine1 = "", ScreenLine2 = "", ScreenLine3 = "", ScreenVol = "", ScreenLine4 = "", ScreenLine5 = "", Device = "";
bool handshaking = true;
bool connectedornot = false;
int status = 0; //0为静止，1为正在由推子调节电脑音量，2为正在根据上位机音量调节推子。根据状态，有一方的大小比对要停止。
int PCVolume, miconVolume, PCVolumeLF, miconVolumeLF = 0;
String deviceName = "";
String dataBuffer = "";
String cmdBuffer = "";

bool atTarget = false;

String getValue(String data, int index) {
  int start = 0;
  for (int i = 0; i < index; i++) {
    int pos = data.indexOf('|', start);
    if (pos == -1){
      return "";
    }
    start = pos + 1;
  }
  int end = data.indexOf('|', start);
  if (end == -1){
    end = data.length();
  }
  return data.substring(start, end);
}

void SingleClick(){
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

void DoubleClick(){
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

void LongPress(){
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

void setup() {
  pinMode(4, INPUT_PULLUP);
  pinMode(A0, INPUT);
  pinMode(faderTouch, INPUT_PULLUP);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(2, 1);
  sensor.attachClick(SingleClick);
  sensor.attachDoubleClick(DoubleClick);
  sensor.attachLongPressStart(LongPress);
  ScreenLine1 = "Fader Volume Adjuster";
  ScreenLine2 = "  by iKa Hoshikawa   ";
  ScreenLine3 = "   Starting up...    ";
  oled.println(ScreenLine1);
  oled.println(ScreenLine2);
  oled.println(ScreenLine3);
  oled.display();
  lastRefreshTime = millis();
  digitalWrite(7, HIGH);
}

void loop() {
  sensor.tick();
  int raw = 0;
  for (int i = 0; i < 8; i++) raw += analogRead(A0);
  raw /= 8;
  miconVolume = map(raw, 0, 1023, 0, 100);
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
            oled.setCursor(0, 1);
            oled.println(" Connected");
            oled.println("    ^_^");
            oled.display();
            ifScreenOn = true;
            lastRefreshTime = millis();
          }
          handshaking = false;
          connectedornot = true;
          status = 0;
          lastResponseTime = millis();
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
        PCVolume = getValue(raw, 0).toInt();
        PCVolumeLF = getValue(raw, 1).toInt();
        deviceName = getValue(raw, 2);
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
    ScreenLine1 = "Fader Volume Adjuster";
    ScreenLine2 = "  by iKa Hoshikawa   ";
    ScreenLine3 = "Zzzzzzzzzzzzzzz......";
    oled.setTextSize(1);
    oled.setCursor(0, 1);
    oled.println(ScreenLine1);
    oled.setTextSize(2);
    oled.println(ScreenLine2);
    oled.setTextSize(1);
    oled.println(ScreenLine3);
    oled.display();
    ifScreenOn = true;
    lastRefreshTime = millis();
  }
  if (handshaking) {
    ScreenLine1 = "Fader Volume Adjuster";
    ScreenLine2 = "  by iKa Hoshikawa   ";
    ScreenLine3 = "Zzzzzzzzzzzzzzz......";
  }else{
    ScreenLine1 = "Current Volume:";
    ScreenLine2 = String(PCVolume);
    ScreenLine3 = "         %         ";
  }
  if(millis() - lastRefreshTime > 3000){
    oled.clearDisplay();
    oled.display();
    ifScreenOn = false;
  }
  int targetADC = map(PCVolume, 0, 100, 0, 1023);
int diff = targetADC - analogRead(A0);
int delta = abs(diff);
int pwm = map(delta, 0, 1023, 135, 255);
pwm = constrain(pwm, 145, 255);
if (!atTarget && delta < 24) {
  atTarget = true;
}
if (atTarget && delta > 50) {
  atTarget = false;
}
if (atTarget) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
} else if (diff > 0) {
  analogWrite(AIN1, pwm);
  digitalWrite(AIN2, LOW);
} else if (diff < 0) {
  digitalWrite(AIN1, LOW);
  analogWrite(AIN2, pwm);
}

oled.clearDisplay();
oled.setTextSize(1);
oled.setCursor(0, 0);
oled.print("mic=");
oled.println(miconVolume);
oled.print("PC=");
oled.println(PCVolume);
oled.print("faderTouch=");
oled.println(analogRead(faderTouch));
oled.display();

  status = 0;
  miconVolumeLF = miconVolume;
}
