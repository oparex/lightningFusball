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

extern uint8_t lightning[];

const char* ssid = SECRET_SSID;
const char* pass = SECRET_PASS;
const char* lndFingerprint = LND_FINGERPRINT;
const char* macaroon = LND_MACAROON;
const char* lndHost = "89.212.162.6";
const char* lndUrl = "/v1/invoices";
const int lndPort = 8080;

//long invoiceExpiery = -1;
//char* invoiceBuffer;
uint8_t qrcode[353];
uint8_t tempBuffer[353];
enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
  
WiFiClientSecure client;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046 touch(/*cs=*/ 4, /*irq=*/ 5);

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

    SPI.setFrequency(ESP_SPI_FREQ);

    tft.begin();
    touch.begin(tft.width(), tft.height());
    tft.setRotation(3);
    touch.setCalibration(209, 1759, 1775, 273);
    drawLightning();

//    Serial.println();
//    Serial.println("connecting");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

//    Serial.println();
//    Serial.println("connected");
    
    client.setFingerprint(lndFingerprint);

    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
    
}

void loop() {

    if (touch.isTouching()) {

//      Serial.println("touch");
        
//        if (millis() - invoiceExpiery > 0) {
//            invoiceBuffer = getNewInvoice(1000);
//            invoiceExpiery = millis() + 3600000;
            qrcodegen_encodeText(getNewInvoice(1000), tempBuffer, qrcode, errCorLvl,
                qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
//        }

        drawQr();
        
        if (waitForPayment() > 0) {
//            Serial.println("paid");
             digitalWrite(0, LOW);
             delay(1000);
             digitalWrite(0, HIGH);
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
    while (client.connected()) {
        String line = client.readStringUntil('\n');
//        if (line.indexOf(invoiceBuffer) > 0 && line.indexOf("SETTLED") > 0) {
          if (line.indexOf("SETTLED") > 0) {
            client.stop();
            delay(1000);
            return 1;
        }
        if (millis() - startTimer > 30000) {
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
    data += "\", \"memo\": \"lightning foosball\"}";

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
    
    return pay_req;
}

static void drawQr() {
    tft.fillScreen(ILI9341_WHITE);
    for (int y = -4; y < 57; y++) {
        for (int x = -4; x < 57; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                tft.fillRect(x*4+54, y*4+14, 4, 4, ILI9341_BLACK);  
            }
        }
    }
}

void drawLightning() {
    tft.fillScreen(ILI9341_WHITE);
    int y = -1;
    int x = 0;
    for (int k = 0; k < 330; k++) {
        if (k % 15 == 0) {
            x = 0;
            y++;
        }
        uint8_t pixel = pgm_read_byte_near(lightning + k);
        if (pixel) {
            tft.fillRect(x*10+90, y*10+10, 10, 10, ILI9341_YELLOW);
        }
        x++;
    }
}
