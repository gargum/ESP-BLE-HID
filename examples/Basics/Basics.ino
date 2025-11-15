/**
 * This example exists to demonstrate the basic functions of SQUIDHID
 *
 * Please feel free to use this example as a reference or template!
 */


#include <SQUIDHID.h>

SQUIDHID esp("ESP32-TEST", "gargum", 100);   // Here, we set the name to "ESP32-TEST", the manufacturer to "gargum", and the battery level to 100%

void setup() {
  esp.setAppearance(CYCLING_COMPUTER);          // You can set the device to advertise itself as a wide array of different things!
  esp.begin();                                  // Here, we start up our ESP32 running SQUIDHID
}

void loop() {
  esp.update();            // SQUIDHID requires you to call the 'update()' function in sketches. This is used to control things like the scanning interval.

  esp.press(KC_SLSH);      // SQUIDHID allows you to press down individual keys using the 'press' function!
  delay(2000);
  esp.press(KC_RSFT);      // NKRO is enabled by default, meaning you can just keep pushing down buttons without letting anything go!
  delay(2000);
  esp.press(KC_VOLD);      // Any keyboard button in SQUIDHID should be compatible with the 'press' function.
  delay(2000);
  esp.release(KC_RSFT);    // Using the 'release' function, you can let go of individual keys you've previously 'pressed'.
  delay(2000);
  esp.releaseAll();        // If you're holding down lots of buttons, you can also simply call the 'releaseAll' function.
  delay(2000);
  esp.write(KC_SLSH);      // You can also simply output any keyboard key one time in one command using 'write'.
  delay(2000);             // Please note that 'write' is NOT compatible with gamepad or mouse inputs at this time.
  esp.print("You can print text strings using SQUIDHID as well!");
  delay(2000);

  esp.press(MO_BTN1);  // The press function allows supports mouse buttons. 
  delay(2000);  
  esp.press(GB_R1);   // All gamepad buttons are also supported by the press function.
  delay(2000);
  esp.releaseAll();
  delay(2000);
  
  esp.press(GB_L1);   // The 'release' function on the other hand can parse all the same things 'press' can.
  delay(2000);
  esp.release(GB_L1); // This means you can always use 'release' when you want to let go of just one button.
  delay(2000);

         //X    Y
  esp.move(100, 100);       // The 'move' function allows you to perform relative movements, like a computer mouse.
  delay(2000);
         //X    Y    W  hW
  esp.move(100, 100, 1, 1);    // 'move' also allows you to set the state of the vertical and horizontal mouse wheels respectively.
  delay(2000);
           //X     Y
  esp.moveTo(1000, 1000);    // 'moveTo' parses absolute coordinates, like a finger on a touchscreen or like a Wacom tablet.
  delay(2000);
           //X     Y     W  Stylus button
  esp.moveTo(1000, 1000, 1, DI_BTN3); // Just like 'move', 'moveTo' also allows you to set the state of the vertical and horizontal mouse wheels.
  delay(2000);

  esp.click(MO_BTN1);    // The 'click' function is the mouse's version of the 'write' function. I decided to call it 'click' because 'write' seemed confusing in this context.
  delay(2000);

  esp.click(5000, 5000, DI_BTN1); // Just like the 'moveTo' function, 'click' is able to parse absolute coordinates. (By default, it will just click wherever the cursor is.)
  delay(2000);
  
}
