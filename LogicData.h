//////////////////////////////////////////////////////////
//
// LOGICDATA protocol - Used by motorized office furniture from LogicData.
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

#include <stdint.h>
#include "Arduino.h"

class LogicData
{
//  int rx_pin;
  int tx_pin;
  bool active = false;
  
  public:

  typedef unsigned long micros_t;

  enum { SPACE=0, MARK=1 };

  LogicData(int tx) : tx_pin(tx) {}

  bool is_active() { return active; }

  void Begin() {
    pinMode(tx_pin, OUTPUT);
    digitalWrite(tx_pin, HIGH); // turn on pullups
  }

  void send_bit(bool bit) {
    digitalWrite(tx_pin, bit);
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
  
  void space(micros_t & timer, uint16_t ms=LOGICDATA_MIN_START_BIT) {
    send_bit(SPACE, timer, ms);
  }
  
  void start(micros_t & timer) {
    active = true;
    space(timer, LOGICDATA_MIN_START_BIT);
  }
  
  void stop() {
    send_bit(MARK); // IDLE
    active = false;
  }
  
  void Send(uint32_t data, micros_t & timer) {
    start(timer);
  
    for (uint32_t i = 0x80000000 ; i; i/=2) {
      send_bit((i&data)?SPACE:MARK, timer, 1);
    }
  
    // Caller must call stop() to return to IDLE
  }
  
  void Send(uint32_t * data, unsigned count) {
    if (!count) return;
  
    micros_t timer = micros();
    micros_t start = timer;
    for (unsigned i = 0; i != count; i++) {
      Send(data[i], timer);
    }
    
    micros_t delta = timer-start;
    if (delta/1000 < LOGICDATA_MIN_WINDOW_MS - LOGICDATA_MIN_START_BIT) {
      timer=start;
      space(timer, LOGICDATA_MIN_WINDOW_MS);
    } else {
      space(timer, LOGICDATA_MIN_START_BIT);
    }
    stop();
  }
};

//
// LOGICDATA protocol
//////////////////////////////////////////////////////////
