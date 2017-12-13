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
const uint8_t MOD_TX  = 5;

// Input/Output
const uint8_t MOD_HS1 = 2;
const uint8_t MOD_HS2 = 0;
const uint8_t MOD_HS3 = 4;
const uint8_t MOD_HS4 = 1;

#endif

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
  digitalWrite(MOD_TX, HIGH); // turn on pullups (weird that this is needed, but it is)

  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);
}

int last_state = 0;
void check_display() {
  int state = digitalRead(MOD_TX);
  if (state == last_state) return;

  last_state = state;
  last_signal = millis();
}

unsigned read_buttons() {
  unsigned buttons = 0;
  if (digitalRead(MOD_HS1)) buttons |= HS1;
  if (digitalRead(MOD_HS2)) buttons |= HS2;
  if (digitalRead(MOD_HS3)) buttons |= HS3;
  if (digitalRead(MOD_HS4)) buttons |= HS4;

  return buttons;
}

void read_latch() {
  if (latched) return;

  unsigned prev_latched = 0;
  unsigned long t = millis();
  do {
    latched = read_buttons();

    if (!prev_latched) prev_latched = latched;

    // Only latch MEM buttons
    if (latched != MEM1 && latched != MEM2 && latched != MEM3) latched = 0;

    // keep checking display
    check_display();

    // Loop until signal held for 20ms to debounce
  } while (latched && latched == prev_latched && millis()-t < 20);
  
  if (latched) last_signal = millis();
}

void break_latch() {
  latched = 0;
  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);
}

void hold_latch() {
  if (!latched) return;

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

  if (latched & HS1) pinMode(MOD_HS1, OUTPUT);
  if (latched & HS2) pinMode(MOD_HS2, OUTPUT);
  if (latched & HS3) pinMode(MOD_HS3, OUTPUT);
  if (latched & HS4) pinMode(MOD_HS4, OUTPUT);

  if (latched & HS1) digitalWrite(MOD_HS1, HIGH);
  if (latched & HS2) digitalWrite(MOD_HS2, HIGH);
  if (latched & HS3) digitalWrite(MOD_HS3, HIGH);
  if (latched & HS4) digitalWrite(MOD_HS4, HIGH);
}

void loop() {
  // Monitor panel buttons for our commands and take over when we see one

  check_display();
  read_latch();
  hold_latch();
}
