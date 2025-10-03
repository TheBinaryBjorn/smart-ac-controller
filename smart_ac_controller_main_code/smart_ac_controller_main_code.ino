#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "secrets.h"
#include <unordered_set>

// ============================
// Constants
// ============================
#define IR_RECEIVE_PIN 14
#define IR_LED_PIN 13
#define DEFAULT_TEMP 24
#define MIN_TEMP 18
#define MAX_TEMP 30
#define SERVER_DOMAIN "smart-ac-controller"

// ============================
// Globals
// ============================
IRLgAc ac(IR_LED_PIN);
IRrecv irReceiver(IR_RECEIVE_PIN);
decode_results irResults;
uint8_t currentTemp = DEFAULT_TEMP;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
std::unordered_set<uint32_t> receivedCodes;

// ============================
// Function Declarations
// ============================
void connectToWiFi();
void startMDNS();
void initLittleFS();
void initIR();
void initWebServer();
void handleIRReceive();
void turnOnAC(uint8_t temp = DEFAULT_TEMP);
void turnOffAC();
void setACTemperature(uint8_t temp);
void notifyTempChange();

// ============================
// Setup
// ============================
void setup() {
    Serial.begin(115200);
    delay(1000);

    initLittleFS();
    connectToWiFi();
    startMDNS();
    initIR();
    initWebServer();

    Serial.println("Setup complete. Server running!");
}

// ============================
// Main Loop
// ============================
void loop() {
    handleIRReceive();
}

// ============================
// Functions
// ============================
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
}

void startMDNS() {
    if (MDNS.begin(SERVER_DOMAIN)) {
        Serial.println("mDNS responder started");
    }
}

void initLittleFS() {
    if (LittleFS.begin(true)) {
        Serial.println("LittleFS mounted successfully");
    } else {
        Serial.println("LittleFS mount failed");
    }
}

void initIR() {
    ac.begin();
    irReceiver.enableIRIn();
    Serial.println("IR Sender and Receiver initialized");
}

void notifyTempChange() {
    ws.textAll(String(currentTemp));
}

void initWebServer() {
    // Serve static files
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/script.js", LittleFS, "/script.js");
    server.serveStatic("/background.png", LittleFS, "/background.png");

    // Root endpoint
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Power endpoints
    server.on("/turn-on", HTTP_GET, [](AsyncWebServerRequest *request){
        turnOnAC();
        request->send(200, "text/plain", "AC turned ON at 24°C");
        notifyTempChange();
    });

    server.on("/turn-off", HTTP_GET, [](AsyncWebServerRequest *request){
        turnOffAC();
        request->send(200, "text/plain", "AC turned OFF");
        notifyTempChange();
    });

    // Temperature endpoint
    server.on("/set-temp", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->hasParam("temp")) {
            request->send(400, "text/plain", "Missing 'temp' parameter");
            return;
        }
        int temp = request->getParam("temp")->value().toInt();
        if (temp < MIN_TEMP || temp > MAX_TEMP) {
            request->send(400, "text/plain", "Invalid temperature (18-30)");
            return;
        }
        setACTemperature(temp);
        request->send(200, "text/plain", "Temperature set to " + String(temp) + "°C");
        notifyTempChange();
    });

    // WebSocket handler
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if(type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket client #%u connected\n", client->id());
            client->text(String(currentTemp)); // send current temp immediately
        }
    });

    server.addHandler(&ws);

    server.begin();
    Serial.println("Async web server started!");
}

void handleIRReceive() {
    if(!irReceiver.decode(&irResults)) return;

    uint32_t code = irResults.value;
    if (code != 0 && code != 0xFFFFFFFF && receivedCodes.insert(code).second) {
        Serial.println("New Raw IR Code: 0x" + String(code, HEX));
    }
    irReceiver.resume();
}

void turnOnAC(uint8_t temp) {
    Serial.printf("Turning AC ON at %d°C...\n", temp);
    currentTemp = temp;
    ac.on();
    ac.setTemp(temp);
    ac.setMode(kLgAcCool);
    ac.setFan(kLgAcFanAuto);
    ac.send();
    Serial.println("AC ON command sent");
}

void turnOffAC() {
    Serial.println("Turning AC OFF...");
    ac.off();
    ac.send();
    Serial.println("AC OFF command sent");
}

void setACTemperature(uint8_t temp) {
    Serial.printf("Setting AC temperature to %d°C...\n", temp);
    currentTemp = temp;
    ac.on();
    ac.setTemp(temp);
    ac.setMode(kLgAcCool);
    ac.setFan(kLgAcFanAuto);
    ac.send();
    Serial.println("AC temperature command sent");
}
