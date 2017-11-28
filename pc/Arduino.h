#define HIGH 1
#define OUTPUT 0
#define pinMode(x,y)
#define digitalWrite(x,y)
#define interrupts() void(0)
#define noInterrupts() void(0)

extern unsigned long NOW;
#define micros() NOW

