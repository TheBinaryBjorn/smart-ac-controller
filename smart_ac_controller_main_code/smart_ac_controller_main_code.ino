#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>  // LG AC protocol

#define IR_LED_PIN 13  // GPIO pin where your IR LED is connected

IRsend irsend(IR_LED_PIN);  // create IR sender object

// Captured LG AC codes
uint32_t LG_ON  = 0x8800606;
uint32_t LG_OFF = 0x88C0051;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 IR LED LG AC Test (IRremoteESP8266)");

  irsend.begin();  // Initialize IR sender
  Serial.println("IR Sender ready on GPIO13");
  delay(2000);
}

void loop() {
  Serial.println("Sending AC ON command...");
  irsend.sendLG(LG_ON, 28);  // 28-bit LG code
  Serial.println("Sent ON command");

  delay(10000); // wait 10 seconds

  Serial.println("Sending AC OFF command...");
  irsend.sendLG(LG_OFF, 28); // 28-bit LG code
  Serial.println("Sent OFF command");

  delay(10000); // wait 10 seconds
}
