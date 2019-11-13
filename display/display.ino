#include <Adafruit_TFTLCD.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <qrcode.h>

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
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

const uint8_t lightning[22][15] = {
    {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
int c = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    initDisplay();

//    drawQRCode("lnbc50u1pwm79t2pp5ekujth2kuuxd92ydrhzzpu2uyx7x45qrt2smd332azqguh8zquuqdp9d35kw6r5de5kueeqve6hxcnpd3kzqvfsxqcrqcqzpg36h5ww50y2270n0d3m6l2reepcr85ttrwgy9vjtu4pse3ca6n3e4d40d4krzd2xpjmdxs4duv5a9q8dauvt2aq6a7df08692d0jw5jcplzt6na\0");
    drawLightning(YELLOW);
}

void loop() {
   TSPoint p = ts.getPoint();  //Get touch point

   if (p.z > ts.pressureThreshhold) {
     p.x = map(p.x, TS_MAXX, TS_MINX, 0, 320);
     p.y = map(p.y, TS_MAXY, TS_MINY, 0, 240);

     Serial.print("X = "); Serial.print(p.x);
     Serial.print("\tY = "); Serial.print(p.y);
     Serial.print("\n");

     pinMode(XM, OUTPUT);
     pinMode(YP, OUTPUT);
     
//     drawQRCode("lnbc50u1pwm79t2pp5ekujth2kuuxd92ydrhzzpu2uyx7x45qrt2smd332azqguh8zquuqdp9d35kw6r5de5kueeqve6hxcnpd3kzqvfsxqcrqcqzpg36h5ww50y2270n0d3m6l2reepcr85ttrwgy9vjtu4pse3ca6n3e4d40d4krzd2xpjmdxs4duv5a9q8dauvt2aq6a7df08692d0jw5jcplzt6na");
     if (c == 0) {
       drawLightning(BLACK);
       c = 1;
       delay(1000);
       return;
     }
     if (c == 1) {
       drawLightning(YELLOW);
       c = 0;
       delay(1000);
       return; 
     }
     
   }
}

void initDisplay() {
    tft.reset();
    tft.begin(0x9341);
    tft.setRotation(1);

    tft.fillScreen(WHITE);
}

void drawQRCode(char str[]) {

    Serial.println("start qr code calc");

    QRCode qrcode;
    uint8_t qrcodeBytes[352];
    qrcode_initText(&qrcode, qrcodeBytes, 9, ECC_LOW, str);

    Serial.println("qr code calculated");

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(x*4+54, y*4+14, 4, 4, BLACK);
            }
        }
    }

    Serial.println("qr code ploted");
}

void drawLightning(uint16_t color) {
    tft.fillScreen(WHITE);
    for (int y = 0; y < 22; y++) {
        for (int x = 0; x < 15; x++) {
            if (lightning[y][x]) {
                tft.fillRect(x*10+90, y*10+10, 10, 10, color);
            }
        }
    }
}
