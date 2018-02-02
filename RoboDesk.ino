#include "pins_trinket_pro.h"
#include "buttons.h"

//#define DISABLE_LATCHING

unsigned long last_signal = 0;

uint32_t test_display_on[] = {
    // display ON; display WORD
    0x40611400,
    0x40601dfe,
    0x40641400,
    0x40681400,
    0x40600515,
    0x40649dfb,
    0x40600495
};


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
