#include <Arduino.h>
#include <WebServer.h> //only to get http_method, must be defined before defines_ethernet.h
#include "defines_ethernet.h"

int internalLed = 2;
const char* TAG = "simple webserver test";
const int port = 80;

unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

uint8_t count = 0;

EthernetWebServer server(port);

void setupEthernet()
{
  // Check for Ethernet hardware present
  Ethernet.init(22);
  while (!Ethernet.begin(mac[0], 5000, 1000))
  {
    ESP_LOGI(TAG, "failed to connect");
  }
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    // Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    ESP_LOGI(TAG, "Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    ESP_LOGI(TAG, "Ethernet cable is not connected.");
  }
  ESP_LOGI(TAG, "Connected to IP : %s\n", Ethernet.localIP().toString());
  ESP_LOGI(TAG, "MAC Address %02X::%02X::%02X::%02X::%02X::%02X", mac[0][0], mac[0][1], mac[0][2], mac[0][3], mac[0][4], mac[0][5]);
  digitalWrite(internalLed, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(internalLed, OUTPUT);
  Serial.begin(115200);
  SPI.begin(21, 18, 19, 22);
  setupEthernet();
  server.on("/api/test", HTTP_GET, []{
    server.send(200, "text/plain", "success");
  });
  server.on("api/test-post", HTTP_POST, []{
    server.send(200, "text/plain", "success");
  });
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}
