#include <Arduino.h>
#include <WebServer.h>
#include <Ethernet_HTTPClient/Ethernet_WebSocketClient.h>
#include <ModbusClientTCP.h>
#include <ModbusServerEthernet.h>
#include <defines_ethernet.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

int internalLed = 2;
const char* TAG = "simple tcp/ip mix test";
IPAddress serverIp(192, 168, 2, 113);

TaskHandle_t tcpTaskHandle;
TaskHandle_t modbusTcpServerTaskHandle;
TaskHandle_t modbusTcpClientTaskHandle;
TaskHandle_t webServerTaskHandle;

EthernetServer server(8123);
EthernetClient modbusClient;
EthernetClient webSocketClient;
EthernetWebSocketClient wsClient(webSocketClient, serverIp, 8000);
EthernetWebServer webServer(80);
ModbusClientTCP mbClient(modbusClient);
ModbusServerEthernet mbServer;

// Create modbus server
uint16_t memo[32]; // Test server memory: 32 words
uint8_t id = 2;
uint8_t count = 0;
uint32_t dataCount = 0;

/**
 * Modbus TCP Server callback
*/
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

/**
 * Modbus TCP Client callback
*/

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
    dataCount++;
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    Serial.printf("Error response: %02X - %s\n", (int)me, (const char *)me);
}

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
    ESP_LOGI(TAG, "Connected to IP : %s\n", Ethernet.localIP().toString().c_str());
    ESP_LOGI(TAG, "MAC Address %02X::%02X::%02X::%02X::%02X::%02X", mac[0][0], mac[0][1], mac[0][2], mac[0][3], mac[0][4], mac[0][5]);
    digitalWrite(internalLed, HIGH);
}

void tcpTask(void *pvParameter)
{
    const char *_TAG = "tcp task";
    esp_log_level_set(_TAG, ESP_LOG_INFO);
    uint32_t msgCount = 1;
    while (1)
    {
        EthernetClient ethClient = server.available();
        if (ethClient)
        {
            while (ethClient.connected())
            {
                uint8_t buff[1024];
                String responseText = "response %d from ESP32 => ";
                responseText.replace("%d", String(msgCount));
                int len = ethClient.available();
                ESP_LOGI(_TAG, "bytes length : %d\n", len);
                mempcpy(buff, responseText.c_str(), responseText.length());
                ethClient.readBytes(buff + responseText.length(), len);
                // ESP_LOG_BUFFER_HEXDUMP(_TAG, buff, len, ESP_LOG_INFO);
                size_t length = responseText.length() + len; //ex : 0x1234
                std::vector<uint8_t> dataLengthBytes;
                dataLengthBytes.push_back(length >> 8); //fill second element with 0x12
                dataLengthBytes.push_back(length & 0xFF); //fill first element with 0x34
                int byteWritten = ethClient.write(dataLengthBytes.data(), dataLengthBytes.size()); //write to eth port as 0x1234 (send 0x12 first, then 0x34), this packet is data size of the next packet
                byteWritten += ethClient.write(buff, length); //write data to eth port
                ESP_LOGI(_TAG, "bytes written : %d\n", byteWritten);
                msgCount++;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); //delay task to give another task chance
        ethClient.stop(); //stop when finish transmit data, so the received can detect the eof character
    }
    
}

void modbusTcpClientTask(void *pvParameter)
{
    while (1)
    {
        Error err = mbClient.addRequest((uint32_t)millis(), 4, READ_HOLD_REGISTER, 2, 6);
        if (err!=SUCCESS) {
            ModbusError e(err);
            Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); //delay task to give another task chance
    }
}

void webServerTask(void *pvParameter)
{
    while (1)
    {
        webServer.handleClient();
        vTaskDelay(1 / portTICK_PERIOD_MS); //delay task to give another task chance
    }
}

void setup() 
{
    // put your setup code here, to run once:
    esp_log_level_set(TAG, ESP_LOG_INFO);
    pinMode(internalLed, OUTPUT);
    Serial.begin(115200);
    SPI.begin(21, 18, 19, 22);
    setupEthernet();

    xTaskCreate(
        tcpTask, "tcp task", 4096, NULL, 1, &tcpTaskHandle
    );

    xTaskCreate(
        modbusTcpClientTask, "modbus tcp client task", 2048, NULL, 1, &modbusTcpClientTaskHandle
    );

    xTaskCreate(
        webServerTask, "webserver task", 2048, NULL, 1, &webServerTaskHandle
    );

    // Set up test memory
    for (uint16_t i = 0; i < 32; ++i)
    {
        memo[i] = 100 + i;
    }

    mbServer.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
    mbServer.registerWorker(1, READ_INPUT_REGISTER, &FC03);     // FC=04 for serverID=1
    mbServer.registerWorker(2, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=2
    // Start the modbus server
    mbServer.start(502, id, 20000);

    // Set up ModbusTCP client.
    // - provide onData handler function
    mbClient.onDataHandler(&handleData);
    // - provide onError handler function
    mbClient.onErrorHandler(&handleError);
    // Set message timeout to 2000ms and interval between requests to the same host to 200ms
    mbClient.setTimeout(2000, 200);

    // Start ModbusTCP background task
    mbClient.begin();

    mbClient.setTarget(serverIp, 502);

    webServer.on("/api/test-get", HTTP_GET, []{
        String text = "data count : " + String(dataCount);
        webServer.send(200, "text/plain", text);
    });

    webServer.on("/api/test-post", HTTP_POST, []{
        webServer.send(200, "text/plain", "POST success");
    });

    // Start webserver
    webServer.begin();

}

void loop()
{
    //websocket handle
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
        delay(50);
    }
}