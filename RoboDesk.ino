#include "pins_trinket_pro.h"
#include "LogicData.h"
#include "buttons.h"

//#define DISABLE_LATCHING

unsigned long last_signal = 0;
unsigned long last_action = 0;

LogicData ld(INTF_TX);


//-- Pass through mode sends input signal straight to output
inline void passThrough(uint8_t out, uint8_t in) {
  digitalWrite(out, digitalRead(in));
}

// Pass module TX straight through to interface RX
void dataPassThrough() {
  if (!ld.is_active()) passThrough(INTF_TX, MOD_TX);
}

unsigned gather_test = 0;
//-- Buffered mode parses input words and sends them to output separately
void dataGather() {
  digitalWrite(LED, digitalRead(MOD_TX));
  ld.PinChange(HIGH == digitalRead(MOD_TX));
  gather_test++;
//  passThrough(5, MOD_TX);
}



uint32_t test_display_stream[] = {
    0x40600495,
    0x40600594,
    0x40600455,
    0x40600554,
    0x406004d4,
    0x406005d5,
    0x40600435,
    0x40600534,
    0x406004b4,
    0x406005b5,
    0x40600474,
    0x40600575,
    0x406004f5,
    0x406005f4,
    0x4060040c,
    0x4060050d,
    0x4060048d,
    0x4060058c,
    0x4060044d,
    0x4060054c,
    0x406004cc,
    0x406005cd,
    0x4060042d,
    0x4060052c,
    0x406004ac,
    0x406005ad,
    0x4060046c,
    0x4060056d,
    0x40601dfe,
    0x406004ed,
    0x40649dfb,
    0x406005ec,
    0x4060041d,
    0x4060051c,
    0x4060049c
};
uint32_t test_display_word[] = {
    // display word?
    0x40601dfe,
    0x40600515,
    0x40649dfb
};

uint32_t test_display_off[] = {
    // display OFF
    0x40601dfe,
    0x40600515,
    0x40649dfb,
    0x406e1400
};
uint32_t test_display_set[] = {
    // Enters "SET" mode; display commands have no effect until undone; Display: "S -"
    0x40641400,

    // Set 3 chosen; Display: "S 3"
    0x406c1d80,

    // "S 3" idle
    0x40601dfe,
    0x406c1d80,
    0x40649dfb,

    // ----------------------------------------
    // Display "120"
    0x40681400,
    0x4060043c,
    0x40649dfb,
    // ----------------------------------------
    // Display "120"
    0x40601dfe,
    0x4060043c,
    0x40649dfb,
    // ----------------------------------------
    // Display "120"
    0x40601dfe,
    0x4060043c,
    0x40649dfb,
    // ----------------------------------------
    // Display off
    0x406e1400
};

unsigned latched = 0;
unsigned long latch_time = 0;

void setup() {
  pinMode(MOD_TX, INPUT);
//  digitalWrite(MOD_TX, HIGH); // turn on pullups (Needed for attiny85)

  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);

  dataGather();
  attachInterrupt(MOD_TX_interrupt, dataGather, CHANGE);

  ld.Begin();
  delay(1000);

  unsigned size = sizeof(test_display_on) / sizeof(test_display_on[0]);
  ld.Send(test_display_on, size);

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
 
  uint32_t msg = ld.ReadTrace();
  if ((msg & 0xFFFFF000) == 0x40600000) {
    Serial.println(msg, HEX);
  }
}
