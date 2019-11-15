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
bool messageProcessed = true;

char* invoiceBuffer;
uint8_t qrcode[353];
uint8_t tempBuffer[353];
enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
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
  
  if (messageProcessed == false && isClickMsg()) {

    invoiceBuffer = getNewInvoice(1000);

    bool ok = qrcodegen_encodeText(invoiceBuffer, tempBuffer, qrcode, errCorLvl,
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

  for (int j = 0; j < 500; j++) {
    previousByte = currentByte;
    currentByte = messageBuffer[j];

    if (mode == 0 && previousByte == 0xfe && currentByte == 0xfe) {
      mode++;
    }
    if (mode == 1 && previousByte == 0xff && currentByte == 0xff) {
      mode++;
    }
  }
  if (mode == 2) {
    messageProcessed = true;
    return true;  
  }
  return false;
}

void sendQRCodeMessage(void) {
  // send start bytes
  Serial.print(0xfd);
  Serial.print(0xfd);

  for (int i = 0; i < 353; i++) {
     Serial.print(qrcode[i]);
  }

  Serial.print(0xff);
  Serial.print(0xff);
}

void sendPaidInvoiceMsg(void) {
  Serial.print(0xfc);
  Serial.print(0xfc);
  Serial.print(0xff);
  Serial.print(0xff);
}

char* getNewInvoice(int price) {
  WiFiClientSecure client;
  client.setFingerprint(lndFingerprint);

  if (!client.connect(lndHost, lndPort)) {
    Serial.println("https connection failed");
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
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "-2";
  }

  char* pay_req = doc["payment_request"];

  return pay_req; 
}

int getBtcEurPrice() {
  WiFiClientSecure client;
  client.setFingerprint(bitstampFingerprint);

  if (!client.connect(bitstampHost, bitstampPort)) {
    Serial.println("https connection failed");
    return -1;
  }

  client.print(String("GET ") + bitstampUrl + " HTTP/1.1\r\n" +
               "Host: " + bitstampHost + "\r\n" +
               "User-Agent: LightningFusballESP8266\r\n" +
               "Connection: close\r\n\r\n");

  // read headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readStringUntil('\n');

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return -2;
  }

  String bid = doc["bid"];
  int price = gamePrice / bid.toInt();

  return price;
}

static bool calcQr(char* text) {
  // Make and print the QR Code symbol
  
  bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
    qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
  if (ok) {
    return true;
  }
  return false;
}

void readSerial(void) {
  int cnt = 0;
  while (Serial.available() > 0) {
    byte currentByte = Serial.read();
    if (cnt < 500) {
      messageBuffer[cnt] = currentByte;
      cnt++;
    }
  }
  messageProcessed = false;
}
