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
const uint8_t MOD_HS1 = 5;
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

unsigned latched = 0;


//-- Buffered mode parses input words and sends them to output separately
void dataGather() {
  last_signal = millis();
}


void setup() {
  pinMode(MOD_TX, INPUT);
  digitalWrite(MOD_TX, HIGH); // turn on pullups (weird that this is needed, but it is)

  pinMode(MOD_HS1, INPUT);
  pinMode(MOD_HS2, INPUT);
  pinMode(MOD_HS3, INPUT);
  pinMode(MOD_HS4, INPUT);

  attachInterrupt(MOD_TX_interrupt, dataGather, CHANGE);

  delay(100);
  last_signal = 0;
}

void read_latch() {
  if (latched) return;

  if (digitalRead(MOD_HS1)) latched += HS1;
  if (digitalRead(MOD_HS2)) latched += HS2;
  if (digitalRead(MOD_HS3)) latched += HS3;
  if (digitalRead(MOD_HS4)) latched += HS4;

  // Ignore UP, DOWN and SET buttons
  if (latched == UP || latched == DOWN || latched == SET) latched = 0;
  
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
  unsigned long delta = millis() - last_signal;

  // Let go after 1.5 seconds with no signals
  if (delta > 1500) {
    break_latch();
    return;
  }

  pinMode(MOD_HS1, OUTPUT);
  pinMode(MOD_HS2, OUTPUT);
  pinMode(MOD_HS3, OUTPUT);
  pinMode(MOD_HS4, OUTPUT);

  digitalWrite(MOD_HS1, (latched & HS1) ? HIGH : LOW);
  digitalWrite(MOD_HS2, (latched & HS2) ? HIGH : LOW);
  digitalWrite(MOD_HS3, (latched & HS3) ? HIGH : LOW);
  digitalWrite(MOD_HS4, (latched & HS4) ? HIGH : LOW);
}


void loop() {
  // Monitor panel buttons for our commands and take over when we see one

  read_latch();
  hold_latch();
}
