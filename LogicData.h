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

typedef unsigned long micros_t;

#define TRACE_HISTORY_MAX 64 // powers-of-two are faster in MOD
#define Q_MAX TRACE_HISTORY_MAX

//--------------------------------------------------
// queue
//
struct mque {
  // embedded deque; push to head; pop from tail
  micros_t trace[Q_MAX];
  unsigned head = 0;
  unsigned tail = 0;

  unsigned next(unsigned x);
  bool empty();
  bool full();
  unsigned size();

  // destructive push; pushes even if full
  bool push(micros_t t);

  // destructive pop; no-op if empty
  bool pop(micros_t * t);

  // drop elements from the tail of the queue; no range-checking!
  void drop(int n);

  // non-destructive indexed peek
  bool peek(unsigned index, micros_t * t);
};
  
// queue
//
//--------------------------------------------------

class LogicData
{
//  int rx_pin;
  int tx_pin;
  bool active = false;

  micros_t timer;  // calculated time of previous step-end
  micros_t start;  // time of Open()

  mque q;
  
  public:

  enum { SPACE=0, MARK=1 };

  //
  LogicData(int tx) : tx_pin(tx) {}

  bool is_active() { return active; }

  void Begin();

  // Receive
  micros_t prev_bit = 0;
  bool prev_level = HIGH;
 
  void PinChange(bool level);

  uint32_t ReadTrace();

  // Transmit
  void SendBit(bool bit);
  void MicroDelay(micros_t us);
  void Delay(uint16_t ms);
  void SendBit(bool bit, uint16_t ms);
  void Space(uint16_t ms=LOGICDATA_MIN_START_BIT);
  void SendStartBit();
  void Stop();
  void Send(uint32_t data);
  void OpenChannel();
  void CloseChannel();
  void Send(uint32_t * data, unsigned count);
};

//
// LOGICDATA protocol
//////////////////////////////////////////////////////////
