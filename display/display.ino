#include <Adafruit_TFTLCD.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include "qrcodegen.h"

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define TS_MINX 146
#define TS_MINY 148
#define TS_MAXX 938
#define TS_MAXY 854

#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin

#define BLACK   0x0000
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

extern uint8_t lightning[];

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint p;

byte messageBuffer[500];
int cnt = 0;
uint8_t screen = 0;
long timer;
uint8_t qrcode[353];
//uint8_t tempBuffer[353];
//enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    Serial.println();
    Serial.println("start");
    initDisplay();
    drawLightning(YELLOW);
}

void loop() {

  if (Serial.available() > 0) {
    readSerial();
  }

  if (screen == 0) {
    p = ts.getPoint();  //Get touch point
    if (p.z > ts.pressureThreshhold) {
      sendClickMsg();
      screen++;
    }
  }
  if (screen == 1) {
    if (cnt > 0 && readQRCodeMsg()) {
      Serial.println("got qr code");
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      drawQr(qrcode);
      screen++;
    }
  }
//  if (screen == 2) {
//    if (millis() - timer > 10000) {
//      pinMode(XM, OUTPUT);
//      pinMode(YP, OUTPUT);
//      drawLightning(YELLOW);
//      screen = 0;
//    } 
//  }
}

static void initDisplay(void) {
    tft.reset();
    tft.begin(0x9341);
    tft.setRotation(1);

    tft.fillScreen(WHITE);
}

static void drawQr(const uint8_t qrcode[]) {
  tft.fillScreen(WHITE);
//  int size = qrcodegen_getSize(qrcode);
//  int border = 4;
  for (int y = -4; y < 57; y++) {
    for (int x = -4; x < 57; x++) {
      if (qrcodegen_getModule(qrcode, x, y)) {
        tft.fillRect(x*4+54, y*4+14, 4, 4, BLACK);  
      }
    }
  }
}

void drawLightning(uint16_t color) {
    tft.fillScreen(WHITE);
    int y = -1;
    int x = 0;
    for (int k = 0; k < 330; k++) {
      if (k % 15 == 0) {
        x = 0;
        y++;
      }
      uint8_t pixel = pgm_read_byte_near(lightning + k);
      if (pixel) {
          tft.fillRect(x*10+90, y*10+10, 10, 10, color);
      }
      x++;
    }
}

void sendClickMsg(void) {
  // send start bytes
  Serial.write(0xfe);
  Serial.write(0xfe);
  Serial.write(0xff);
  Serial.write(0xff);
}

bool readQRCodeMsg(void) {
  byte previousByte = 0x00;
  byte currentByte = 0x00;
  uint8_t mode = 0;
  int i = 0;

  for (int j = 0; j < 500; j++) {
    previousByte = currentByte;
    currentByte = messageBuffer[j];

    if (mode == 0 && previousByte == 0xfd && currentByte == 0xfd) {
      mode++;
    }
    if (mode == 1 && i < 353) {
      qrcode[i] = currentByte;
      i++;
    }
    if (i == 353 && previousByte == 0xff && currentByte == 0xff) {
      cnt = 0;
      return true;
    }
  }
  return false;
}

void readSerial(void) {
  while (Serial.available() > 0) {
    byte currentByte = Serial.read();
    Serial.print(cnt);
    Serial.print(" ");
    Serial.println(currentByte);
    if (cnt < 500) {
      messageBuffer[cnt] = currentByte;
      cnt++;
    }
    if (cnt == 500) {
      cnt = 0;  
    }
  }
}
