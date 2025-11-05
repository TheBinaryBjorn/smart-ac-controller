#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>
#include <unordered_map>
#include <map>
#include <IRutils.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "secrets.h"
#include <Wire.h>
#include "Adafruit_SHT31.h"


// ============================
// Constants
// ============================

// SHT31
#define SDA_PIN 22
#define SCL_PIN 23
#define DEFAULT_I2C_ADDRESS 0x44
// IR LED
#define IR_LED_PIN 13
#define DEFAULT_TEMP 24
// AC
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

// ===========================
// Enums
// ===========================
enum AutomationMode {
    Off,
    Temperature,
    Time,
    TimeAndTemperature
};

std::map<String, int> modeStringToConstant = {{"cool", MODE_COOL}, {"heat", MODE_HEAT}, {"dry", MODE_DRY}, {"fan", MODE_FAN}};

struct ActiveAutomation {
    AutomationMode automationMode;
    int roomTempToActivateAutomation;
    int acTempToSetOnActivation;
    int acModeToSetOnActivation;
};

ActiveAutomation automation = {Off, 0, 0, 0};



// ============================
// Globals
// ============================
IRLgAc ac(IR_LED_PIN);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

float roomTemperature;
float humidity;

bool currentPower = POWER_OFF;
uint8_t currentMode = MODE_COOL;
uint8_t currentFan = FAN_AUTO;
uint8_t currentTemp = DEFAULT_TEMP;

std::unordered_map<uint8_t, String> fanToString = {{FAN_LOW, "low"}, {FAN_MED, "med"}, {FAN_HIGH, "high"}, {FAN_AUTO,"auto"}};
std::unordered_map<uint8_t, String> modeToString = {{MODE_COOL, "cool"}, {MODE_HEAT, "heat"}, {MODE_DRY,"dry"}, {MODE_FAN, "fan"}};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");



// ============================
// Function Declarations
// ============================
void connectToWiFi();
void startMDNS();
void initLittleFS();
void initIR();
void initWebServer();
void initSHT31();
void toggleACPower();
void turnOnAC();
void turnOffAC();
void setACTemperature(uint8_t temp);
void setACMode(uint8_t mode);
void sendStateToClients();
void sendTempAndHumidityToClients();
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
    initSHT31();

    Serial.println("Setup complete. Server running!");
}

// ============================
// Main Loop
// ============================
void loop() {
    roomTemperature = sht31.readTemperature();
    humidity = sht31.readHumidity();
    if(!isnan(roomTemperature)&&!isnan(humidity)) {
        Serial.printf("Room Temperature: %.0f, Humidity: %.0f%%\n", roomTemperature, humidity);
        sendTempAndHumidityToClients();
    } else {
        Serial.println("Failed to get Room Temperature & Humidity readings.");
    }
    switch(automation.automationMode) {
        case Temperature:
            Serial.println("Temperature Automation!");
            if(roomTemperature >= automation.roomTempToActivateAutomation) {
                if(currentTemp != automation.acTempToSetOnActivation || currentMode != automation.acModeToSetOnActivation || !currentPower) {
                    setACTemperature(automation.acTempToSetOnActivation);
                    setACMode(automation.acModeToSetOnActivation);
                    sendStateToClients();
                }
            }
            break;
        case Off:
            break;
    }
    delay(1000);
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
    Serial.println("IR Sender initialized");
}

void initSHT31() {
    Wire.begin(SDA_PIN, SCL_PIN);
    if(!sht31.begin(DEFAULT_I2C_ADDRESS)) {
        Serial.println("Couldn't find SHT31");
        // maybe we can handle different logic.
        return;
    }
    Serial.println("SHT31 up.");
}

void sendStateToClients() {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), 
    "{\"type\":\"stateUpdate\",\"power\":%d,\"mode\":\"%s\",\"temp\":%d,\"fan\":\"%s\"}",
    currentPower,
    modeToString[currentMode].c_str(),
    currentTemp,
    fanToString[currentFan].c_str());
    ws.textAll(buffer);
}

void sendTempAndHumidityToClients() {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"type\":\"sht31Update\",\"roomTemperature\":%.0f,\"humidity\":%.0f}", roomTemperature, humidity);
    ws.textAll(buffer);
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

    // Mode endpoint
    server.on("/set-mode", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(!request->hasParam("mode")) {
            request->send(400, "text/plain", "Missing 'mode' parameter");
            return;
        }
        String mode = request->getParam("mode")->value();
        if(mode == "cool")
            setACMode(MODE_COOL);
        else if(mode == "heat")
            setACMode(MODE_HEAT);
        else if(mode == "fan")
            setACMode(MODE_FAN);
        else if(mode == "dry")
            setACMode(MODE_DRY);
        else {
            request->send(400, "text/plain", "Invalid mode (cool, heat, fan, dry)");
            return;
        }
        sendStateToClients();
        request->send(200, "text/plain", "Mode set to " + mode);
    });

    // Automation endpoint
    server.on("/set-automation", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(!request->hasParam("automation_mode")) {
            automation.automationMode = Off;
            request->send(400, "text/plain", "Missing 'automation_mode' parameter");
            return;
        }
        String receivedAutomationMode = request->getParam("automation_mode")->value();
        if(receivedAutomationMode == "off") {
            automation.automationMode = Off;
            request->send(200, "text/plain", "Automation mode set to " + receivedAutomationMode);
            return;
        }

        if(!request->hasParam("room_temp")) {
            automation.automationMode = Off;
            request->send(400, "text/plain", "Missing 'room_temp' parameter");
            return;
        }
        if(!request->hasParam("ac_temp")) {
            automation.automationMode = Off;
            request->send(400, "text/plain", "Missing 'ac_temp' parameter");
            return;
        }
        if(!request->hasParam("ac_mode")) {
            automation.automationMode = Off;
            request->send(400, "text/plain", "Missing 'ac_mode' parameter");
            return;
        }
        int receivedRoomTemp = request->getParam("room_temp")->value().toInt();
        int receivedAcTemp = request->getParam("ac_temp")->value().toInt();
        String receivedAcMode = request->getParam("ac_mode")->value();
        if(!modeStringToConstant.count(receivedAcMode)) {
            request->send(400, "text/plain", "Invalid AC Mode value!");
            return;
        }
        if(receivedAutomationMode == "temperature_automation") {
            automation.automationMode = Temperature;
            automation.roomTempToActivateAutomation = receivedRoomTemp;
            automation.acTempToSetOnActivation = receivedAcTemp;
            // should be validated.
            automation.acModeToSetOnActivation = modeStringToConstant[receivedAcMode];
        } else if(receivedAutomationMode == "time_automation") {
            automation.automationMode = Time;
        } else if(receivedAutomationMode == "time_and_temperature_automation") {
            automation.automationMode = TimeAndTemperature;
        } else {
            automation.automationMode = Off;
            request->send(400, "text/plain", "Invalid automation mode");
            return;
        }
        request->send(200, "text/plain", "Automation mode set to " + receivedAutomationMode);
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

void toggleACPower() {
    currentPower = !currentPower;
    currentPower ? ac.on() : ac.off();

    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(currentFan);

    ac.send();
    Serial.println("AC power command sent");
}

void setACTemperature(uint8_t temp) {
    Serial.printf("Setting AC temperature to %d°C...\n", temp);
    currentTemp = temp;
    currentPower = POWER_ON;
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
    currentPower = POWER_ON;

    ac.on();
    ac.setTemp(currentTemp);
    ac.setMode(currentMode);
    ac.setFan(fanMode);

    ac.send();
    Serial.println("AC fan command sent");
}

void setACMode(uint8_t mode) {
    Serial.printf("Setting AC mode to %d...", mode);
    currentMode = mode;
    currentPower = POWER_ON;
    ac.on();
    ac.setTemp(currentTemp);
    ac.setMode(mode);
    ac.setFan(currentFan);
    ac.send();
    Serial.println("AC mode command sent");
}
