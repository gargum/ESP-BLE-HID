/**
 * This example takes an ESP32, and turns it into a PIN-protected BLE mouse jiggler that falsely reports itself to be an insulin pump.
 *
 * Designed for use on all operating systems. Tested and working on mobile.
 *
 * Boards like the ESP32-C3 Super Mini, which this example was originally tested on, make this especially useful!
 */

#include <SQUIDHID.h>

SQUIDHID jig("MiniMed 780G", "Medtronic", 100); // Setting name, manufacturer, battery level

void setup() {
  jig.setAppearance(INSULIN_PUMP);
  jig.begin();
}

void loop() {
  jig.update();
  
  jig.move(20,0);
  delay(50);
  jig.move(0,20);
  delay(50);
  jig.move(-20,0);
  delay(50);
  jig.move(0,-20);
  delay(50);

}
