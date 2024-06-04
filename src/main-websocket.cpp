#include <Arduino.h>
#include <Ethernet_HTTPClient/Ethernet_WebSocketClient.h>
#include "defines_ethernet.h"

int internalLed = 2;
const char* TAG = "websocket test";
const int port = 8000;

unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

uint8_t count = 0;

IPAddress serverIp(192, 168, 2, 113);
int serverPort = 8000;

EthernetClient client;
EthernetWebSocketClient wsClient(client, serverIp, serverPort);

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
}

void loop() {
  // put your main code here, to run repeatedly:
  ESP_LOGI(TAG, "starting WebSocket client");

  wsClient.begin();

  while (wsClient.connected())
  {
    ESP_LOGI(TAG, "Sending Hello ");
    ESP_LOGI(TAG, "%d\n", count);

    // send a hello #
    wsClient.beginMessage(TYPE_TEXT);
    wsClient.print(count);
    String data = " => Hello from SimpleWebSocket on " + String(ARDUINO_BOARD) + ", millis = " + String(millis());
    wsClient.print(data);
    wsClient.endMessage();

    // increment count for next message
    count++;

    // check if a message is available to be received
    int messageSize = wsClient.parseMessage();

    if (messageSize > 0)
    {
      ESP_LOGI(TAG, "Received a message:");
      ESP_LOGI(TAG, "%s\n", wsClient.readString().c_str());
    }

    // wait 5 seconds
    delay(10);
  }

  ESP_LOGI(TAG, "disconnected");
}
