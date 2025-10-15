/**
 * This example demonstrates a little of what this library can do with the ESP32, even without USB Host Mode
 */
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  //Name must be set before calling bleKeyboard.begin()
  bleKeyboard.setName("Keyboard Demo");
  bleKeyboard.begin();
  bleKeyboard.use6KRO(); //NKRO is turned on by default, 6KRO must be explicitly specified
  bleKeyboard.setBatteryLevel(100); //Any number can be put here and it will report properly
}

void loop() {
  if(bleKeyboard.isConnected()) {
    Serial.println("Sending 'Hello world'...");
    bleKeyboard.print("Hello world");

    delay(1000);

    Serial.println("Sending Enter key...");
    bleKeyboard.write(KC_ENT);

    delay(1000);

    Serial.println("Sending Play/Pause media key...");
    bleKeyboard.write(KC_MPLY);

    delay(1000);

    Serial.println("Maximizing the screen brightness...");
    bleKeyboard.press(KC_BRIU);
    delay(2000);
    bleKeyboard.release(KC_BRIU);

    delay(1000);
    
   //
   // Below is an example of pressing multiple keyboard modifiers 
   // which by default is commented out. 
   // 
   /* Serial.println("Sending Ctrl+Alt+Delete...");
    bleKeyboard.press(KC_LCTL);
    bleKeyboard.press(KC_LALT);
    bleKeyboard.press(KC_DEL);
    delay(100);
    bleKeyboard.releaseAll();
    */

  }
  Serial.println("Waiting 5 seconds...");
  delay(5000);
}
