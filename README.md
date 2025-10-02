# smart-ac-controller
After building some smart lights with rgb strips and ESP32s, I decided to build a smart AC controller for my LG AC and integrate both systems into one diy smart home system in the future.

### Parts I ordered from AliExpress
- ESP32 Board
- IR LED
- 2N2222 NPN Transistor
- 220R Resistor
- 10KR Resistor
- IR Receiver
- SHT30 Temperature and Humidity Sensor

### Assembly
- GPIO13 -> 220R Resistor -> Transistor Base (B)
- 3.3V -> IR LED anode (+), IR LED cathode (-) -> Transistor Collector (C)
- Transistor Emitter (E) -> ESP32 GND

### Libraries I use
- IRremoteESP8266 (Yes it works with ESP32, don't worry. You'll just have to downgrade to ESP32 2.0.17 Core).
  Alternatively you can use IRremote just translate captured IR codes to it's communication format.
- Something I can't remember for SHT30, but I still haven't implemented it anyway so I'll update in the future.

### Issues I ran into (and how I fixed them)
- ESP32 Core 3.3.1 being incompatible with IRremoteESP8266, Solution was to downgrade to ESP32 2.0.17
- IR not activating AC with captured code, Solution was to replace the original black IR LED I put in with a clear one,
  I figured this one out with a web cam test, looking at my old remote I saw a big purple flash when I used it and didn't when
  I used my prototype, changed to a clear IR LED and viola, it suddenly worked. Maybe the IR LED was just faulty but if it ain't broken
  I ain't trying to fix XD.
