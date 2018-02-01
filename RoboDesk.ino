#include "pins_trinket_pro.h"

#define DISABLE_LATCHING

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

unsigned latched = 0;

void setup() {
  pinMode(MOD_TX, INPUT);
//  digitalWrite(MOD_TX, HIGH); // turn on pullups (Needed for attiny85?)

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

unsigned read_buttons() {
  static unsigned prev = 0;
  unsigned buttons = 0;
  if (digitalRead(MOD_HS1)) buttons |= HS1;
  if (digitalRead(MOD_HS2)) buttons |= HS2;
  if (digitalRead(MOD_HS3)) buttons |= HS3;
  if (digitalRead(MOD_HS4)) buttons |= HS4;

  if (buttons && prev!=buttons) {
    Serial.print(buttons);
    if (buttons & HS1) Serial.print(" HS1");
    if (buttons & HS2) Serial.print(" HS2");
    if (buttons & HS3) Serial.print(" HS3");
    if (buttons & HS4) Serial.print(" HS4");

    switch (buttons) {
      case UP:    Serial.print("    UP");      break;
      case DOWN:  Serial.print("    DOWN");    break;
      case SET:   Serial.print("    SET");     break;
      case MEM1:  Serial.print("    MEM1");    break;
      case MEM2:  Serial.print("    MEM2");    break;
      case MEM3:  Serial.print("    MEM3");    break;
      default:    Serial.print(" ** UNKNOWN **");      break;
    }
    Serial.println("");
  }
  prev = buttons;

  return buttons;
}

unsigned long debounce = 0;
unsigned prev_buttons = 0;
void read_latch() {
  if (latched) return;

  if (!prev_buttons) debounce = millis();

  unsigned diff = prev_buttons;
  prev_buttons = read_buttons();

  // Only latch MEM buttons
  if (prev_buttons != MEM1 && prev_buttons != MEM2 && prev_buttons != MEM3) prev_buttons = 0;

  // Ignore spurious signals
  if (diff && diff != prev_buttons) prev_buttons = 0;
  
  // latch when signal is stable for 20ms
  if (millis()-debounce > 20) {
    latched = prev_buttons;
    last_signal = millis();
  }

  if (latched) {
    Serial.print(" LATCH ");
    Serial.println(latched);
  }
}

void break_latch() {
  latched = 0;
  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);
  #ifdef LED
    digitalWrite(LED, LOW);
  #endif
}

void latch_pin(int pin)
{
#ifndef DISABLE_LATCHING
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
#endif
}

void hold_latch() {
  if (!latched) return;

  #ifdef LED
    digitalWrite(LED, HIGH);
  #endif

  unsigned long delta = millis() - last_signal;

  // Let go after 1.5 seconds with no signals
  if (delta > 1500) {
    break_latch();
    return;
  }

  // Break the latch if some other button is detected
  if ((read_buttons() | latched) != latched) {
    break_latch();
    return;
  }

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
