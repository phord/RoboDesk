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

#define LOGICDATA_MIN_WINDOW_MS  50 // 500
#define LOGICDATA_MIN_START_BIT  50

#include <stdint.h>
#include "Arduino.h"

typedef uint32_t micros_t;

#define TRACE_HISTORY_MAX 80 // powers-of-two are faster in MOD
#define Q_MAX TRACE_HISTORY_MAX

#define BIG_IDLE (micros_t(-1))         // An eternity
#define IDLE_TIME (micros_t(1)<<16)     // if the signal is idle for this long, we consider it an eternity

//--------------------------------------------------
// queue
//
typedef uint16_t index_t;
struct mque {
  // embedded deque; push to head; pop from tail
  micros_t trace[Q_MAX];
  index_t head = 0;
  index_t tail = 0;

  index_t next(index_t x);
  bool empty();
  bool full();
  index_t size();

  // destructive push; pushes even if full
  void push(micros_t t);

  // destructive pop; no-op if empty
  bool pop(micros_t * t);

  // drop elements from the tail of the queue; no range-checking!
  void drop(index_t n);

  // non-destructive indexed peek
  bool peek(index_t index, micros_t * t);
};
  
// queue
//
//--------------------------------------------------

class LogicData
{
//  int rx_pin;
  int tx_pin;
  bool active = false;
  bool pin_idle = false;

  micros_t timer;  // calculated time of previous step-end
  micros_t start;  // time of Open()

  mque q;

  micros_t prev_bit = 0;
  bool prev_level = HIGH;
   
  public:

  enum { SPACE=0, MARK=1 };

  LogicData(int tx) : tx_pin(tx) {}

  bool is_active() { return active; }

  void Begin();

  // Receive
  void PinChange(bool level);
  void Service();

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
