#include "buttons.h"
#include "Arduino.h"
#include "pins_trinket_pro.h"

/** Interface to Robodesk handset buttons
 */
 
// Display anything that changed since last time
void display_buttons(unsigned buttons, const char * msg ) {
  static unsigned prev = 0;

  if (msg[0] || prev!=buttons) {
    Serial.print((buttons & HS1) ? " HS1" :  " ---");
    Serial.print((buttons & HS2) ? " HS2" :  " ---");
    Serial.print((buttons & HS3) ? " HS3" :  " ---");
    Serial.print((buttons & HS4) ? " HS4" :  " ---");

    switch (buttons) {
      case UP:    Serial.print("    UP          ");    break;
      case DOWN:  Serial.print("    DOWN        ");    break;
      case SET:   Serial.print("    SET         ");    break;
      case MEM1:  Serial.print("    MEM1        ");    break;
      case MEM2:  Serial.print("    MEM2        ");    break;
      case MEM3:  Serial.print("    MEM3        ");    break;
      case NONE:  Serial.print("    ----        ");    break;
      default:    Serial.print(" ** UNKNOWN **  ");    break;
    }
    Serial.println(msg);
  }
  prev = buttons;
}

unsigned read_buttons() {
  unsigned buttons = 0;
  if (digitalRead(MOD_HS1)) buttons |= HS1;
  if (digitalRead(MOD_HS2)) buttons |= HS2;
  if (digitalRead(MOD_HS3)) buttons |= HS3;
  if (digitalRead(MOD_HS4)) buttons |= HS4;
  
  return buttons;
}

unsigned read_buttons_debounce() {
  static unsigned long debounce = 0;
  static unsigned prev_buttons = 0;

  unsigned diff = prev_buttons;
  prev_buttons = read_buttons();

  unsigned buttons = prev_buttons;

  // Ignore spurious signals
  if (diff && diff != prev_buttons) buttons = NONE;

  // Reset timer if buttons are drifting or unpressed
  if (!buttons) debounce = millis();

  // ignore signal until stable for 20ms
  if (millis()-debounce < 20) {
    buttons = NONE;
  }

  display_buttons(buttons);
  
  return buttons;
}


