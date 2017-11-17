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

  typedef unsigned long micros_t;
  micros_t timer;  // calculated time of previous step-end
  micros_t start;  // time of Open()

  public:

  enum { SPACE=0, MARK=1 };

  LogicData(int tx) : tx_pin(tx) {}

  bool is_active() { return active; }

  void Begin() {
    pinMode(tx_pin, OUTPUT);
    SendBit(MARK); // IDLE-CLOSED
  }

  void SendBit(bool bit) {
    digitalWrite(tx_pin, bit);
  }

  void MicroDelay(micros_t us) {
    // spin-wait until timer advances
    while (true) {
      micros_t now = micros();
      now -= timer;
      if (now >= us) break;
      // TODO: if (t-now > 50) usleep(t-now-50);
    }
    timer += us;
  }

  void Delay(uint16_t ms) {
    MicroDelay(micros_t(ms)*1000);
  }

  void SendBit(bool bit, uint16_t ms) {
    SendBit(bit);
    Delay(ms);
  }

  void Space(uint16_t ms=LOGICDATA_MIN_START_BIT) {
    SendBit(SPACE, ms);
  }

  void SendStartBit() {
    Space(LOGICDATA_MIN_START_BIT);
  }

  void Stop() {
    SendBit(MARK); // IDLE-CLOSED
  }

  void Send(uint32_t data) {
    SendStartBit();

    for (uint32_t i = 0x80000000 ; i; i/=2) {
      SendBit((i&data)?SPACE:MARK, 1);
    }

    SendBit(SPACE); // IDLE-OPEN
  }

  void OpenChannel() {
    active = true;
    start = timer = micros();
    SendBit(SPACE);
  }

  void CloseChannel() {
    // Send a SPACE at least long enough so our command-channel was open for 500ms, or
    // just start-bit length if we've already been open that long.

    micros_t delta = timer-start;
    if (delta/1000 + LOGICDATA_MIN_START_BIT < LOGICDATA_MIN_WINDOW_MS) {
      timer=start;
      Space(LOGICDATA_MIN_WINDOW_MS);
    } else {
      Space(LOGICDATA_MIN_START_BIT);
    }
    Stop();

    active = false;
  }

  void Send(uint32_t * data, unsigned count) {
    if (!count) return;

    OpenChannel();
    for (unsigned i = 0; i != count; i++) {
      Send(data[i]);
    }
    CloseChannel();
  }
};

//
// LOGICDATA protocol
//////////////////////////////////////////////////////////
