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
- IRremote by shirriff, z3t0, ArminJo
- Something I can't remember for SHT30, but I still haven't implemented it anyway so I'll update in the future.

### What I did so far
- Connected the breadboard prototype circuit.
- Spent hours figuring out which side of the Transistor is C and which is E and why I even need it since I'm a software engineer and not an electrical engineer XD.
- Captured my remote codes for on and off.
- Used a hash set (unordered_set) to filter out the noise on the IR Receiver, cheap ones detect a lot of noise as captured codes.
- Made a basic loop for On and Off for testing.
- **(03/10/2025):** 
  - Converted the server into an async web server with web socket for realtime communication with the clients.
  - Separated HTML, CSS and JavaScript files, now serving them with static serve instead of using `server.send(200, 'text/html', html);`
  - Started using `IRremoteESP8266`'s LG objects with `IrLgAc ac(IR_LED_PIN);`.
  - Started using `IRremoteESP8266`'s `irReceiver` class for the IR Receiver part of the code, though there is no use to it anymore since I'm
    using the built in LG remote codes that come with `IRremoteESP8266` so I might delete that in the future.
  - Refactored code to fit Clean Code guidelines.
  - Added a background image also served statically.

### Issues I ran into (and how I fixed them)
- ESP32 Core 3.3.1 being incompatible with IRremoteESP8266, Solution was to downgrade to ESP32 2.0.17
- IR not activating AC with captured code, Solution was to replace the original black IR LED I put in with a clear one,
  I figured this one out with a web cam test, looking at my old remote I saw a big purple flash when I used it and didn't when
  I used my prototype, changed to a clear IR LED and viola, it suddenly worked. Maybe the IR LED was just faulty but if it ain't broken
  I ain't trying to fix XD.
  
