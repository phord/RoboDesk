#include "pins_trinket_pro.h"

//#define DISABLE_LATCHING

unsigned long last_signal = 0;

enum {
  HS1 = 1,
  HS2 = 2,
  HS3 = 4,
  HS4 = 8
};

#define UP    HS1
#define DOWN  HS2
#define SET   (HS1 + HS2)
#define MEM1  HS3
#define MEM2  HS4
#define MEM3  (HS2 + HS4)
#define NONE  0

unsigned latched = 0;

void setup() {
  pinMode(MOD_TX, INPUT);
//  digitalWrite(MOD_TX, HIGH); // turn on pullups (Needed for attiny85)

  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);
  #ifdef LED
    pinMode(LED, OUTPUT);
  #endif
  Serial.begin(115200);
  Serial.println("Robodesk v0.9  build: " __DATE__ " " __TIME__);
}

int last_state = 0;
void check_display() {
  int state = digitalRead(MOD_TX);
  if (state == last_state) return;

  last_state = state;
  last_signal = millis();
}

// Display anything that changed since last time
void display_buttons(unsigned buttons, const char * msg = "") {
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

void read_latch() {
  if (latched) return;

  latched = read_buttons_debounce();

  // Only latch MEM buttons
  if (latched != MEM1 && latched != MEM2 && latched != MEM3) latched = 0;

  // Restart idle timer when we get an interesting button
  if (latched) last_signal = millis();

  if (latched) display_buttons(latched, "Latched");
}

void break_latch() {
  latched = 0;
  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);
}

void latch_pin(int pin)
{
#ifndef DISABLE_LATCHING
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
#endif
}

void hold_latch() {
  #ifdef LED
    digitalWrite(LED, !!latched);
  #endif

  if (!latched) return;

  unsigned long delta = millis() - last_signal;

  // Let go after 1.5 seconds with no signals
  if (delta > 1500) {
    display_buttons(latched, "Idle");
    break_latch();
    return;
  }

  // Break the latch if some other button is detected
  unsigned buttons = read_buttons();
  if ((buttons | latched) != latched) {
    break_latch();
    
    display_buttons(buttons, "Interrupted");

    #ifdef LED
      // Blink angrily at the user for 3000ms
      for (int i = 0 ; i < 60; i++) {
        delay(50);
        digitalWrite(LED, i&1);
      }
    #endif
    
    // HANG until no buttons are pressed anymore
    while (buttons) {
      buttons = read_buttons_debounce();
    }

    return;
  }

  // Re-assert latched buttons
  if (latched & HS1) latch_pin(MOD_HS1);
  if (latched & HS2) latch_pin(MOD_HS2);
  if (latched & HS3) latch_pin(MOD_HS3);
  if (latched & HS4) latch_pin(MOD_HS4);
}

void loop() {
  // Monitor panel buttons for our commands and take over when we see one

  check_display();
  read_latch();
  hold_latch();
}
