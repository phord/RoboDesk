#include "pins_trinket_pro.h"
#include "buttons.h"

//#define DISABLE_LATCHING

unsigned long last_signal = 0;
unsigned long last_action = 0;

unsigned latched = 0;
unsigned long latch_time = 0;

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

void latch(unsigned latch_pins, unsigned long max_latch_time = 15000) {
  latched = latch_pins;
  latch_time = max_latch_time;

  // Restart idle timer when we get an interesting button
  last_action = last_signal = millis();

  display_buttons(latched, "Latched");
}

void read_latch() {
  if (latched) return;

  auto buttons = read_buttons_debounce();

  // Only latch MEM buttons
  if (buttons != MEM1 && buttons != MEM2 && buttons != MEM3) buttons = 0;

  if (buttons) latch(buttons);
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

  unsigned long activity_timeout = (last_signal - last_action < 2000) ? 2500 : 500 ;
  
  // Let go after 500ms with no signals
  if (delta > activity_timeout) {
    Serial.print("Delta=");
    Serial.print(delta);
    Serial.print("  Timeout=");
    Serial.println(activity_timeout);
    display_buttons(latched, "Idle");
    break_latch();
    return;
  }

  delta = millis() - last_action;

  // Let go if latch timer expires
  if (delta > latch_time) {
    display_buttons(latched, "Timed out");
    break_latch();
    return;
  }

  // Break the latch if some other button is detected
  unsigned buttons = read_buttons();
  if ((buttons | latched) != latched) {
    break_latch();
    
    display_buttons(buttons, "Interrupted");

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

  if (!latched) {
    Action action = get_action();
    switch (action) {
      case Mem1: case Mem1_Dbl: latch(MEM1); break;
      case Mem2: case Mem2_Dbl: latch(MEM2); break;
      case Mem3: case Mem3_Dbl: latch(MEM3); break;
      case Up_Dbl:   latch(UP, 4000); break;
      case Down_Dbl: latch(DOWN, 4000); break;
      default: break;
    }
  }
  
  check_display();
  read_latch();  // Old button read method, kept to make MEM buttons smoother
  hold_latch();
}
