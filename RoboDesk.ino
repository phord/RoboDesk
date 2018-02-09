#include "pins_trinket_pro.h"
#include "LogicData.h"
#include "buttons.h"

//#define DISABLE_LATCHING

unsigned long last_signal = 0;
unsigned long last_latch = 0;

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

//  unsigned size = sizeof(test_display_stream) / sizeof(test_display_stream[0]);
//  ld.Send(test_display_stream, size);

  #ifdef LED
    pinMode(LED, OUTPUT);
  #endif
  Serial.begin(115200);
  Serial.println("Robodesk v1.0  build: " __DATE__ " " __TIME__);
}

// Record last time the display changed
void check_display() {
  static uint32_t prev = 0;
  uint32_t msg = ld.ReadTrace();
  if (msg) {
    uint32_t now = millis();
    Serial.print(now - prev);
    prev=now;
    Serial.print("ms  ");
    Serial.print(ld.MsgType(msg));
    Serial.print(": ");
    Serial.println(ld.Decode(msg));
  }

  if (ld.IsNumber(msg)) {
    static uint8_t prev_number;
    auto display_num = ld.GetNumber(msg);
    if (display_num != prev_number) {
      prev_number = display_num;
      last_signal = millis();
    }
  }
}

void latch(unsigned latch_pins, unsigned long max_latch_time = 15000) {
  set_latch(latch_pins, max_latch_time);

  // Restart idle timer when we get an interesting button
  last_latch = last_signal = millis();
}

void read_latch() {
  if (is_latched()) return;

  auto buttons = read_buttons_debounce();

  // Only latch MEM buttons
  if (buttons != MEM1 && buttons != MEM2 && buttons != MEM3) buttons = 0;

  if (buttons) latch(buttons);
}

void hold_latch() {
  #ifdef LED
    digitalWrite(LED, is_latched());
  #endif

  if (!is_latched()) return;

  unsigned long delta = millis() - last_signal;

  unsigned long activity_timeout = (last_signal - last_latch < 2000) ? 2500 : 1500 ;
  
  // Let go after 500ms with no signals
  if (delta > activity_timeout) {
    display_buttons(get_latched(), "Idle");
    break_latch();
    return;
  }

  // Break the latch if some other button is detected
  unsigned buttons = read_buttons();
  if (buttons &&  buttons != get_latched()) {
    break_latch();
    
    display_buttons(buttons, "Interrupted");

    // HANG until no buttons are pressed anymore
    while (buttons) {
      buttons = read_buttons_debounce();
    }

    return;
  }

  // read and display button changes
  read_buttons_debounce(); 
}

// Check for single-click and double-click actions
void check_actions() {
  if (!is_latched()) {
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
}

#ifdef DEBUG_QUEUE
void debug_queue() {
  static size_t prevQ = 0;
  index_t h, t;
  size_t Q = ld.QueueSize(h, t);
  if (Q != prevQ) {
    Serial.print("Q: t=");
    Serial.print(t);
    Serial.print(" h=");
    Serial.print(h);
    Serial.print(" size=");
    Serial.print(Q);
    Serial.print(" [");
    micros_t lead;
    size_t i=0;
    while (ld.q.peek(i++, &lead)) {
      Serial.print(lead);
      Serial.print(", ");
    }
    Serial.println("]");
    prevQ = Q;
  }
}
#endif

void loop() {
  // Monitor panel buttons for our commands and take over when we see one

  check_actions();
  check_display();
  read_latch();  // Old button read method, kept to make MEM buttons smoother
  hold_latch();
}
