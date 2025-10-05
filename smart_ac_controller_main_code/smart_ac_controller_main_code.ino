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
#define FAN_AUTO kLgAcFanAuto
#define FAN_LOW kLgAcFanLow
#define FAN_MED kLgAcFanMedium
#define FAN_HIGH kLgAcFanHigh
#define MODE_COOL kLgAcCool
#define MODE_HEAT kLgAcHeat
#define MODE_DRY kLgAcDry
#define MODE_FAN kLgAcFan
#define POWER_OFF false
#define POWER_ON true
// ============================
// Globals
// ============================
IRLgAc ac(IR_LED_PIN);
IRrecv irReceiver(IR_RECEIVE_PIN);
decode_results irResults;

bool currentPower = POWER_OFF;
uint8_t currentMode = MODE_COOL;
uint8_t currentFan = FAN_AUTO;
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
void toggleACPower();
void turnOnAC();
void turnOffAC();
void setACTemperature(uint8_t temp);
void sendStateToClients();
void setACFan(uint8_t fanMode);

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

void sendStateToClients() {
    String state = "{";
    state += "\"power\":" + String(currentPower) + ",";
    state += "\"mode\":" + String(currentMode) + ",";
    state += "\"temp\":" + String(currentTemp) + ",";
    state += "\"fan\":" + String(currentFan);
    state += "}";
    ws.textAll(state);
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
        sendStateToClients();
    });

    server.on("/turn-off", HTTP_GET, [](AsyncWebServerRequest *request){
        turnOffAC();
        request->send(200, "text/plain", "AC turned OFF");
        sendStateToClients();
    });

    server.on("/toggle-power", HTTP_GET, [](AsyncWebServerRequest *request){
        toggleACPower();
        request->send(200, "text/plain", "AC power toggled");
        sendStateToClients();
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
        sendStateToClients();
    });

    // Fan endpoint
    server.on("/set-fan", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->hasParam("fan")) {
            request->send(400, "text/plain", "Missing 'fan' parameter");
            return;
        }
        String fan = request->getParam("fan")->value();
        if(fan == "auto") setACFan(FAN_AUTO);
        else if(fan == "low") setACFan(FAN_LOW);
        else if(fan == "med") setACFan(FAN_MED);
        else if(fan == "high") setACFan(FAN_HIGH);
        else {
            request->send(400, "text/plain", "Invalid fan mode (auto, low, med, high)");
            return;
        }
        sendStateToClients();
        request->send(200, "text/plain", "Fan set to " + fan);

    });

    // WebSocket handler
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if(type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket client #%u connected\n", client->id());
            sendStateToClients();
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
void toggleACPower() {
    currentPower = !currentPower;
    currentPower ? ac.on() : ac.off();

    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(currentFan);

    ac.send();
    Serial.println("AC power command sent");
}

void turnOnAC() {
    Serial.printf("Turning AC ON...\n");
    ac.on();

    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(currentFan);
    
    ac.send();
    Serial.println("AC ON command sent");
}

void turnOffAC() {
    Serial.println("Turning AC OFF...");
    ac.off();

    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(currentFan);

    ac.send();
    Serial.println("AC OFF command sent");
}

void setACTemperature(uint8_t temp) {
    Serial.printf("Setting AC temperature to %d°C...\n", temp);
    currentTemp = temp;
    ac.on();

    ac.setTemp(temp);
    ac.setMode(currentMode);
    ac.setFan(currentFan);

    ac.send();
    Serial.println("AC temperature command sent");
}

void setACFan(uint8_t fanMode) {
    Serial.printf("Setting AC fan to %d...\n", fanMode);
    currentFan = fanMode;

    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(fanMode);

    ac.send();
    Serial.println("AC fan command sent");
}