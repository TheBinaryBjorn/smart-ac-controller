#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>  // LG AC protocol
#include <WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "secrets.h"

#define WEB_SERVER_PORT 80
#define SERVER_DOMAIN "smart-ac-controller"

WebServer server(WEB_SERVER_PORT);

#define IR_LED_PIN 13  // GPIO pin where your IR LED is connected

IRsend irsend(IR_LED_PIN);  // create IR sender object

// Captured LG AC codes
uint32_t LG_ON  = 0x8800606;
uint32_t LG_OFF = 0x88C0051;

void handleRoot() {
  // Open index.html (gui) in read mode.
  File file = LittleFS.open("/index.html","r");

  // Check if the file opened correctly.
  if(!file) {
    // If it didn't send a 404 not found error.
    // server.send(response number, response type, response message)
    server.send(404, "text/plain", "index.html not found");
    Serial.println("FileSystem(LittleFS): Failed to open index.html.");
    return;
  }

  // Read the html content into a string.
  String html = file.readString();

  // Close the file descriptor (it's a good practice!).
  file.close();

  // Return the stringifyied html to the one that accessed the endpoint.
  server.send(200, "text/html", html);
  Serial.println("Server: index.html served successfully.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 IR LED LG AC Test (IRremoteESP8266)");

  if(LittleFS.begin(true)) {
    Serial.println("FileSystem(LittleFS): Up.");
  } else {
    Serial.println("FileSystem(LittleFS): Failed.");
  }

  // Initialize Wi-Fi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for WiFi connection
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if(MDNS.begin(SERVER_DOMAIN)) {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }
  
  // Initialize IR sender
  irsend.begin();
  Serial.println("IR Sender ready on GPIO13");
  
  // Initialize Web Server endpoints
  server.on("/", handleRoot);
  server.on("/turn-on", [](){
    turnOnAC();
    server.send(200, "text/plain", "AC turned on!");
  });
  server.on("/turn-off", [](){
    turnOffAC();
    server.send(200, "text/plain", "AC turned off!");
  });
  
  server.begin();
}

void loop() {
  server.handleClient();
}

void turnOnAC() {
  Serial.println("Sending AC ON command...");
  irsend.sendLG(LG_ON, 28);  // 28-bit LG code 
  Serial.println("Sent ON command");

}

void turnOffAC() {
  Serial.println("Sending AC OFF command...");
  irsend.sendLG(LG_OFF, 28); // 28-bit LG code
  Serial.println("Sent OFF command");
}
