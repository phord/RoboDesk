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
    char buf[80];
    uint32_t now = millis();
    sprintf(buf, "%6lums %s: %s", now - prev, ld.MsgType(msg), ld.Decode(msg));
    Serial.println(buf);
    prev=now;
  }

  // Reset idle-activity timer if display number changes or if any other display activity occurs (i.e. display-ON)
  if (ld.IsNumber(msg)) {
    static uint8_t prev_number;
    auto display_num = ld.GetNumber(msg);
    if (display_num == prev_number) {
      return;
    }
    prev_number = display_num;
  }
  if (msg)
    last_signal = millis();
}

void latch(unsigned latch_pins, unsigned long max_latch_time = 15000) {
  set_latch(latch_pins, max_latch_time);

  // Restart idle timer when we get an interesting button
  last_latch = last_signal = millis();
}

// Time out stale latches
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
}

// Check the buttons
void check_actions() {
  // quick-read buttons
  auto buttons = read_buttons(); 
  
  if (buttons && is_latched() && buttons != get_latched()) {
    break_latch();
    
    display_buttons(buttons, "Interrupted");

    // Drain all button events from handset interface so the interruption doesn't become a command
    clear_buttons();

    return;
  }

  // read and display button changes
  static unsigned prev = NONE;
  buttons = read_buttons_debounce();
   
  // Ignore unchanged state
  if (buttons == prev) return;
  prev = buttons;
  
  // If we're latched here, then buttons is same as latched or else buttons is NONE
  switch (buttons) {
    case UP:
    case DOWN:
      // Add 4s to travel for each button press
      if (is_latched()) latch(buttons, 4000);
      else latch(buttons, 1000);
      break;
      
    case MEM1:
    case MEM2:
    case MEM3:
      latch(buttons);
      break;

    // Ignore all other buttons (SET, NONE)
    default:
      return;
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
  hold_latch();
}
