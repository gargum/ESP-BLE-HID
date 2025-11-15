/**
 * This example demonstrates a little of what this library can do with the ESP32, even without USB Host Mode
 */
#include <SQUIDHID.h>

SQUIDHID esp;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  //Name must be set before calling esp.begin()
  esp.setName("Keyboard Demo");
  esp.setManufacturer("Santa's Elves");
  esp.setAppearance(DESKTOP); // The type of device the ESP32 advertises itself as. I've chosen desktop, because I can
  esp.begin();
  esp.use6KRO(); //NKRO is turned on by default, 6KRO must be explicitly specified
  esp.setBatteryLevel(100); //Any number can be put here and it will report properly
}

void loop() {
  esp.update();
  
  if(esp.isConnected()) {
    Serial.println("Sending 'Hello world'...");
    esp.print("Hello world");

    delay(1000);

    Serial.println("Sending Enter key...");
    esp.write(KC_ENT);

    delay(1000);

    Serial.println("Sending Play/Pause media key...");
    esp.write(KC_MPLY);

    delay(1000);

    Serial.println("Maximizing the screen brightness...");
    esp.press(KC_BRIU);
    delay(2000);
    esp.release(KC_BRIU);

    delay(1000);
    
   //
   // Below is an example of pressing multiple keyboard modifiers 
   // which by default is commented out. 
   // 
   /* Serial.println("Sending Ctrl+Alt+Delete...");
    esp.press(KC_LCTL);
    esp.press(KC_LALT);
    esp.press(KC_DEL);
    delay(100);
    esp.releaseAll();
    */

  }
  Serial.println("Waiting 5 seconds...");
  delay(5000);
}
