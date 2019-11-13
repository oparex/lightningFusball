#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println();
  Serial.println("connecting to wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("connected to wifi");
//  Serial.println(getBtcEurPrice());

  Serial.println(getNewInvoice(10000));
}

void loop() {}

String getNewInvoice(int price) {
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

  String pay_req = doc["payment_request"];

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
