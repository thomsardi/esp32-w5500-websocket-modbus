#include <Arduino.h>
#include <ModbusClientTCP.h>
#include "defines_ethernet.h"

int internalLed = 2;
const char* TAG = "modbus tcp test";

uint8_t count = 0;

IPAddress serverIp(192, 168, 2, 113);
int serverPort = 502;

EthernetClient client;
ModbusClientTCP MB(client);

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

// Define an onData handler function to receive the regular responses
// Arguments are Modbus server ID, the function code requested, the message data and length of it, 
// plus a user-supplied token to identify the causing request
void handleData(ModbusMessage response, uint32_t token) 
{
    Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
    for (auto& byte : response) {
        Serial.printf("%02X ", byte);
    }
    Serial.println("");
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    Serial.printf("Error response: %02X - %s\n", (int)me, (const char *)me);
}

void setup() {
    // put your setup code here, to run once:
    pinMode(internalLed, OUTPUT);
    Serial.begin(115200);
    SPI.begin(21, 18, 19, 22);
    setupEthernet();
    // Set up ModbusTCP client.
    // - provide onData handler function
    MB.onDataHandler(&handleData);
    // - provide onError handler function
    MB.onErrorHandler(&handleError);
    // Set message timeout to 2000ms and interval between requests to the same host to 200ms
    MB.setTimeout(2000, 200);

    // Start ModbusTCP background task
    MB.begin();

    MB.setTarget(serverIp, serverPort);
}

void loop() {
  // put your main code here, to run repeatedly:
  Error err = MB.addRequest((uint32_t)millis(), 4, READ_HOLD_REGISTER, 2, 6);
  if (err!=SUCCESS) {
    ModbusError e(err);
    Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
  }
  delay(500);
}
