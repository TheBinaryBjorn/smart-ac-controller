#include <Arduino.h>
#include <IRremote.hpp>
#include <unordered_set>

#define IR_RECEIVE_PIN 14

std::unordered_set<uint32_t> receivedCodes;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("IR Receiver - Raw Code Capture with unordered_set");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {
  if (IrReceiver.decode()) {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;

    // Insert returns true if the element was actually inserted (i.e., new)
    if (receivedCodes.insert(code).second) {
      Serial.println("");
      Serial.print("New Raw Code: 0x");
      Serial.println(code, HEX);
      Serial.print("Total Unique Codes: ");
      Serial.println(receivedCodes.size());
      Serial.println("------------------------");
    }

    IrReceiver.resume();
  }
}