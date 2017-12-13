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
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
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
