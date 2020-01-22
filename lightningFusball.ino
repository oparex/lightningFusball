#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "qrcodegen.h"

#include <SPI.h>

#include <Adafruit_ILI9341esp.h>
#include <Adafruit_GFX.h>
#include <XPT2046.h>

#include "secrets.h"

#define TFT_DC 2
#define TFT_CS 15

extern uint16_t lightning[];

const char* ssid = SECRET_SSID;
const char* pass = SECRET_PASS;
const char* lndFingerprint = LND_FINGERPRINT;
const char* macaroon = LND_MACAROON;
const char* lndHost = "89.212.162.6";
const char* lndUrl = "/v1/invoices";
const int lndPort = 8080;

char* invoiceBuffer;
int expiry = 60;
int playPrice = 10000;
uint8_t qrcode[353];
uint8_t tempBuffer[353];
enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
  
WiFiClientSecure client;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046 touch(/*cs=*/ 4, /*irq=*/ 5);

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

//    Serial.println();
//    Serial.println("connecting");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

//    Serial.println();
//    Serial.println("connected");

    SPI.setFrequency(ESP_SPI_FREQ);

    tft.begin();
    touch.begin(tft.width(), tft.height());
    tft.setRotation(2);
    touch.setCalibration(209, 1759, 1775, 273);
    drawLightning();
//    drawThankyou();
    
    client.setFingerprint(lndFingerprint);

    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
    
}

void loop() {

    if (touch.isTouching()) {

        invoiceBuffer = strdup(getNewInvoice(playPrice));

        qrcodegen_encodeText(invoiceBuffer, tempBuffer, qrcode, errCorLvl,
            qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

        drawQr();
        
        if (waitForPayment() > 0) {
             digitalWrite(0, LOW);
             delay(1000);
             digitalWrite(0, HIGH);
             
             drawThankyou();
             delay(10000);
        }

        drawLightning();

    }

}

int waitForPayment() {
    if (client.connect(lndHost, lndPort)) {
        client.print(String("GET /v1/invoices/subscribe HTTP/1.0\r\n") +
                "Host: " + lndHost + "\r\n" +
                "Grpc-Metadata-macaroon: " + macaroon + "\r\n"
                "Connection: upgrade\r\n\r\n");
    }

    long startTimer = millis();
    int l = 1;
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line.indexOf(invoiceBuffer) > 0 && line.indexOf("SETTLED") > 0) {
//        if (line.indexOf("lightning foosball") > 0 && line.indexOf("SETTLED") > 0) {
            client.stop();
            return 1;
        }
        if (line.length() == 0) {
            tft.fillRect(210-l*15, 290, 15, 10, ILI9341_WHITE);
            l++; 
        }
        if (millis() - startTimer > expiry*1000) {
            client.stop();
            delay(1000);
            return 0;
        }
    }
    return 0;
}

const char* getNewInvoice(int price) {

    if (!client.connect(lndHost, lndPort)) {
        return "-1";
    }

    String data = "{\"value\": \"";
    data += String(price);
    data += "\", \"memo\": \"lightning foosball\", \"expiry\": \"";
    data += String(expiry);
    data += "\"}";

    client.print(String("POST ") + lndUrl + " HTTP/1.1\r\n" +
                "Host: " + lndHost + "\r\n" +
                "Content-Type: application/json"+ "\r\n" +
                "Grpc-Metadata-macaroon: " + macaroon + "\r\n" +
                "Content-Length: " + String(data.length()) + "\r\n\r\n" +
                data + "\r\n" +
                "Connection: close\r\n\r\n");

    // read headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
        break;
        }
    }

    String line = client.readStringUntil('\n');

    StaticJsonDocument<356> doc;
    DeserializationError error = deserializeJson(doc, line);

    if (error) {
        return "-2";
    }

    const char* pay_req = doc["payment_request"];
    
    return doc["payment_request"];
}

static void drawQr() {
    tft.fillScreen(ILI9341_WHITE);
    for (int y = 0; y < 53; y++) {
        for (int x = 0; x < 53; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                tft.fillRect(x*4+14, y*4+54, 4, 4, ILI9341_BLACK);  
            }
        }
    }

    tft.fillRect(30, 290, 180, 10, ILI9341_BLACK);
    tft.drawRect(28, 288, 184, 14, ILI9341_BLACK);

//    tft.fillRect(0, 310, 240, 10, ILI9341_YELLOW); 
//
    tft.setCursor(60,20);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.printf("%d sats", playPrice);
//    tft.setCursor(30,270);
//    tft.print("Please pay with");
//    tft.setCursor(60,290);
//    tft.print("lightning");
}

void drawLightning() {
    tft.fillScreen(ILI9341_WHITE);
    int y = -1;
    int x = 0;
    for (int k = 0; k < 4290; k++) {
        if (k % 55 == 0) {
            x = 0;
            y++;
        }
        uint16_t pixel = pgm_read_word_near(lightning + k);
        if (pixel < 0xffff) {
            tft.fillRect(x*3+40, y*3+40, 3, 3, pixel);
        }
        x++;
    }

    tft.setCursor(13,10);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(4);
    tft.print("LIGHTNING");
    tft.setCursor(27,280);
    tft.print("FOOSBALL");

    tft.setCursor(150,200);
    tft.setTextSize(2);
    tft.print("TAP TO");
    tft.setCursor(160,220);
    tft.print("PLAY");
    
}

void drawThankyou() {
    tft.fillScreen(ILI9341_WHITE);
    tft.setCursor(12,120);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(4);
    tft.print("Thank You");
    tft.setCursor(25,170);
    tft.print("Have Fun");
}
