/**
 * This example exists to illustrate the use of SquidHID's Spacemouse functionality.
 * The example consists of a simple test wherein the Spacemouse is translated and rotated around all 6 axes over and over again.
 *
 * Please feel free to use this example as a reference or template!
 */


#include <SQUIDHID.h>

SQUIDHID space("Spinner", "SquidHID", 100);

void setup() {
  space.setLogLevel(LOGGER_VERBOSE);
  space.setAppearance(TABLET);

  space.begin();
}

void loop() {
  
  space.update();
  spin();
  
}

void spin() {

  space.spacemousePress(SM_01);                 // The keys SM_01 to SM_32 are the codes for the 32 3DConnexion buttons! All 32 of them can even be included in keymaps!
  space.spacemouseTranslate(100, -50, 200);     // The "spacemouseTranslate" helper allows you to define a linear motion in the X, Y, and Z axes respectively.
  delay(1000);
  space.spacemouseTranslate(-100, 50, -200);    // Remember, these inputs use absolute coordinates from -32768 to 32768
  delay(1000);
  space.spacemouseRotate(-500, 100, 300);       // The "spacemouseRotate" helper allows you to define a rotational motion along the X, Y, and Z axes respectively.
  delay(1000);
  space.spacemouseRotate(500, -100, -300);      // Just like translations, these inputs use absolute coordinates from -32768 to 32768
  delay(1000);
  space.spacemouseRelease(SM_01);               // Remember, "spacemousePress" causes a 3DConnexion key to be held down. To release it, one must call "spacemouseRelease".
  delay(1000);
  
}
