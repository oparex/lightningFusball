#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "qrcodegen.h"

#include "secrets.h"

const int gamePrice = 10000000;

const char* bitstampHost = "www.bitstamp.net";
const char* bitstampUrl = "/api/v2/ticker/btceur";
const int bitstampPort = 443;

const char* lndHost = "89.212.162.6";
const char* lndUrl = "/v1/invoices";
const int lndPort = 8080;

const char* bitstampFingerprint = BITSTAMP_FINGERPRINT;
const char* lndFingerprint = LND_FINGERPRINT;
const char* macaroon = LND_MACAROON;

const char* ssid = SECRET_SSID;
const char* pass = SECRET_PASS;

byte messageBuffer[500];
int cnt = 0;

uint8_t qrcode[353];
uint8_t tempBuffer[353];
enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {

  if (Serial.available() > 0) {
    readSerial();
  }
  
  if (cnt > 0 && isClickMsg()) {
    bool ok = qrcodegen_encodeText(getNewInvoice(1000), tempBuffer, qrcode, errCorLvl,
                qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok) {
      sendQRCodeMessage();
    }
  }
}

bool isClickMsg(void) {
  byte previousByte = 0x00;
  byte currentByte = 0x00;
  uint8_t mode = 0;

  for (int j = 0; j < 4; j++) {
    previousByte = currentByte;
    currentByte = messageBuffer[j];

    if (mode == 0 && previousByte == 0xfe && currentByte == 0xfe) {
      mode++;
    }
    if (mode == 1 && previousByte == 0xff && currentByte == 0xff) {
      cnt = 0;
      return true;
    }
  }
  return false;
}

void sendQRCodeMessage(void) {
  // send start bytes
  Serial.write(0x0);
  Serial.write(0x0);
  Serial.write(0xfd);
  Serial.write(0xfd);

  for (int i = 0; i < 353; i++) {
     Serial.write(qrcode[i]);
     delay(10);
  }

  Serial.write(0xff);
  Serial.write(0xff);
}

void sendPaidInvoiceMsg(void) {
  Serial.write(0xfc);
  Serial.write(0xfc);
  Serial.write(0xff);
  Serial.write(0xff);
}

const char* getNewInvoice(int price) {
  WiFiClientSecure client;
  client.setFingerprint(lndFingerprint);

  if (!client.connect(lndHost, lndPort)) {
    return "-1";
  }

  String data = "{\"value\": \"";
  data += String(price);
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

  StaticJsonDocument<340> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    return "-2";
  }

  const char* pay_req = doc["payment_request"];

  return pay_req; 
}

void readSerial(void) {
  while (Serial.available() > 0) {
    byte currentByte = Serial.read();
    if (cnt < 500) {
      messageBuffer[cnt] = currentByte;
      cnt++;
    }
    if (cnt == 500) {
      cnt = 0;  
    }
  }
}
