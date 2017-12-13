
#include "LogicData.h"

#ifdef OAK
// Input
const uint8_t MOD_TX  = P5;

// Output
const uint8_t MOD_HS1 = P11;
const uint8_t MOD_HS2 = P10;
const uint8_t MOD_HS3 = P6;
const uint8_t MOD_HS4 = P4;
//const uint8_t MOD_RX  = P5;

// Output
const uint8_t INTF_TX  = LED_BUILTIN;

// Input
const uint8_t INTF_HS1 = P6;
const uint8_t INTF_HS2 = P7;
const uint8_t INTF_HS3 = P8;
const uint8_t INTF_HS4 = P9;

#else // digistump ATTiny85

// Input
const uint8_t MOD_TX  = 2;
const uint8_t MOD_TX_interrupt  = 0;

// Input/Output
const uint8_t MOD_HS1 = 1;
const uint8_t MOD_HS2 = 3;  // FIXME: pin3 is unusable (connected to USB; idles at 2.5v)
const uint8_t MOD_HS3 = 4;
const uint8_t MOD_HS4 = 5;

// Output
const uint8_t INTF_TX  = 0;

#endif

LogicData ld(INTF_TX);


//-- Pass through mode sends input signal straight to output
inline void passThrough(uint8_t out, uint8_t in) {
  digitalWrite(out, digitalRead(in));
}

// Pass module TX straight through to interface RX
void dataPassThrough() {
  if (!ld.is_active()) passThrough(INTF_TX, MOD_TX);
}

//-- Buffered mode parses input words and sends them to output separately
void dataGather() {
  ld.PinChange(HIGH == digitalRead(MOD_TX));
//  passThrough(5, MOD_TX);
}


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

//#define HACK

void setup() {
  pinMode(MOD_TX, INPUT);
  digitalWrite(MOD_TX, HIGH); // turn on pullups (weird that this is needed, but it is)

  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);

  // HACK
  pinMode(5, OUTPUT);
  
//  pinMode(MOD_HS2, OUTPUT);
//  digitalWrite(MOD_HS2, LOW); // force pin low HACK

//  dataPassThrough();
//  attachInterrupt(MOD_TX_interrupt, dataPassThrough, CHANGE);

  dataGather();
  attachInterrupt(MOD_TX_interrupt, dataGather, CHANGE);

  ld.Begin();
  delay(1000);

  unsigned size = sizeof(test_display_on) / sizeof(test_display_on[0]);
  ld.Send(test_display_on, size);
}

uint32_t sample = 0x40600495;

void loop() {
  // Monitor panel buttons for our commands and take over when we see one
  // TODO

  uint32_t msg = ld.ReadTrace();
  
  // Forward messages to HS interface
  if (msg>0) {
    ld.OpenChannel();
    ld.Send(msg);
  } else {
    ld.CloseChannel();
  }

#ifdef HACK
  ld.OpenChannel();
  // HACK testing
  ld.Send(sample);
  ld.Send(sample);
  ld.Send(sample);
  ld.Delay(300);
  ld.CloseChannel();
  ++sample;
  sample = sample & ~(0x00001000);
#endif
}
