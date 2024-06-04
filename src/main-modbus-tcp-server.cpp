#include <Arduino.h>
#include <ModbusServerEthernet.h>
#include "defines_ethernet.h"
// #include "defines.h"

int internalLed = 2;
const char* TAG = "modbus tcp server test";

ModbusServerEthernet MBserver;

// Create server
uint16_t memo[32]; // Test server memory: 32 words
uint8_t id = 2;

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

// Server function to handle FC 0x03 and 0x04
ModbusMessage FC03(ModbusMessage request)
{
  ModbusMessage response; // The Modbus message we are going to give back
  uint16_t addr = 0;      // Start address
  uint16_t words = 0;     // # of words requested
  request.get(2, addr);   // read address from request
  request.get(4, words);  // read # of words from request

  // Address overflow?
  if ((addr + words) > 20)
  {
    // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
  // Request for FC 0x03?
  if (request.getFunctionCode() == READ_HOLD_REGISTER)
  {
    // Yes. Complete response
    for (uint8_t i = 0; i < words; ++i)
    {
      // send increasing data values
      response.add((uint16_t)(memo[addr + i]));
    }
  }
  else
  {
    // No, this is for FC 0x04. Response is random
    for (uint8_t i = 0; i < words; ++i)
    {
      // send increasing data values
      response.add((uint16_t)random(1, 65535));
    }
  }
  // Send response back
  return response;
}

void setup() {
    // put your setup code here, to run once:
    pinMode(internalLed, OUTPUT);
    Serial.begin(115200);
    SPI.begin(21, 18, 19, 22);
    setupEthernet();

    // Set up test memory
    for (uint16_t i = 0; i < 32; ++i)
    {
        memo[i] = 100 + i;
    }

    // Now set up the server for some function codes
    MBserver.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
    MBserver.registerWorker(1, READ_INPUT_REGISTER, &FC03);     // FC=04 for serverID=1
    MBserver.registerWorker(2, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=2

    // Start the server
    MBserver.start(502, id, 20000);
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastMillis = 0;

  if (millis() - lastMillis > 5000)
  {
    lastMillis = millis();
    Serial.printf("Millis: %10d - free heap: %d\n", lastMillis, ESP.getFreeHeap());
  }
}
