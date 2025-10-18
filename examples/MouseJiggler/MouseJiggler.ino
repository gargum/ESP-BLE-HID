/**
 * This example takes an ESP32, and turns it into a BLE mouse jiggler. Designed for use on all operating systems. Tested and working on mobile.
 *
 * Boards like the ESP32-C3 Super Mini, which this example was originally tested on, make this especially useful!
 */

#include <BleKeyboard.h>

BleKeyboard jig("Tim's Airpods", "App1e", 100);

void setup() {
  jig.begin();
}

void loop() {

  jig.mouseMove(20,0);
  delay(50);
  jig.mouseMove(0,20);
  delay(50);
  jig.mouseMove(-20,0);
  delay(50);
  jig.mouseMove(0,-20);
  delay(50);

}
