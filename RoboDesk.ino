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
const uint8_t MOD_HS1 = 0;
const uint8_t MOD_HS2 = 3;
const uint8_t MOD_HS3 = 4;
const uint8_t MOD_HS4 = 5;

// Output
const uint8_t INTF_TX  = 1;

#endif


//////////////////////////////////////////////////////////
//
// LOGICDATA protocol
//
// 32-bit words
// Idle line is logic-high (5v)
// MARK is LOW
// SPACE is HIGH
// Speed is 1000bps, or 1 bit per ms
// Start data by sending MARK for 50ms
// Approximately 50ms between words
// First two bits are always(?) 01 (SPACE MARK)
// All observed words start with 010000000110 (0x406; SPACE MARK SPACEx7 MARKx2 SPACE)

#define LOGICDATA_MIN_WINDOW_MS  500
#define LOGICDATA_MIN_START_BIT  50

typedef unsigned long micros_t;

enum { SPACE=0, MARK=1 };

void send_bit(bool bit) {
  digitalWrite(INTF_TX, bit);
}

void send_bit(bool bit, micros_t & timer, uint16_t ms) {
  send_bit(bit);
  micros_t t = micros_t(ms)*1000;

  // spin-wait until timer advances
  while (true) {
    micros_t now = micros();
    now -= timer;
    if (now >= t) break;
    // TODO: if (t-now > 50) usleep(t-now-50);
  }
  timer += t;    
}

static bool ld_started = false;


inline void passThrough(uint8_t out, uint8_t in) {
  digitalWrite(out, digitalRead(in));
}

// Pass module TX straight through to interface RX
void dataPassThrough() {
  if (!ld_started) passThrough(INTF_TX, MOD_TX);
}

void logicdata_space(micros_t & timer, uint16_t ms=LOGICDATA_MIN_START_BIT) {
  send_bit(SPACE, timer, ms);
}


void logicdata_start(micros_t & timer) {
  ld_started = true;
  logicdata_space(timer, LOGICDATA_MIN_START_BIT);
}

void logicdata_stop() {
  send_bit(MARK); // IDLE
  ld_started = false;
}

void logicdata_send(uint32_t data, micros_t & timer) {
  logicdata_start(timer);

  for (uint32_t i = 0x80000000 ; i; i/=2) {
    send_bit((i&data)?SPACE:MARK, timer, 1);
  }

  // Caller must call logicdata_stop() to return to IDLE
}

void logicdata_send(uint32_t * data, unsigned count) {
  if (!count) return;

  micros_t timer = micros();
  micros_t start = timer;
  for (unsigned i = 0; i != count; i++) {
    logicdata_send(data[i], timer);
  }
  
  micros_t delta = timer-start;
  if (delta/1000 < LOGICDATA_MIN_WINDOW_MS - LOGICDATA_MIN_START_BIT) {
    timer=start;
    logicdata_space(timer, LOGICDATA_MIN_WINDOW_MS);
  } else {
    logicdata_space(timer, LOGICDATA_MIN_START_BIT);
  }
  logicdata_stop();
}

//
// LOGICDATA protocol
//////////////////////////////////////////////////////////

uint32_t test_display_on[] = {
    // display ON; display WORD
    0x40611400,
    0x40601dfe,
    0x40681400,
    0x40600515,
    0x40649dfb
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


void setup() {
  pinMode(MOD_TX, INPUT);
  digitalWrite(MOD_TX, HIGH); // turn on pullups (weird that this is needed, but it is)
  pinMode(INTF_TX, OUTPUT);
  digitalWrite(INTF_TX, HIGH); // turn on pullups

  pinMode(MOD_HS1, INPUT_PULLUP);
  pinMode(MOD_HS2, INPUT_PULLUP);
  pinMode(MOD_HS3, INPUT_PULLUP);
  pinMode(MOD_HS4, INPUT_PULLUP);

  dataPassThrough();
  attachInterrupt(MOD_TX_interrupt, dataPassThrough, CHANGE);

  delay(1000);
  logicdata_send(test_display_on, 5);
}

void loop() {
  // Monitor panel buttons for our commands and take over when we see one
}
