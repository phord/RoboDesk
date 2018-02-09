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
#define NONE  0

enum Action {
  None = 0,
  Unknown,    // Some invalid key mapping, but it wasn't "None"
  
  Up,
  Down,
  Set,
  Mem1,
  Mem2,
  Mem3,

  // Double-click versions
  Up_Dbl,
  Down_Dbl,
  Set_Dbl,
  Mem1_Dbl,
  Mem2_Dbl,
  Mem3_Dbl,
};

// Display anything that changed since last time
void display_buttons(unsigned buttons, const char * msg = "");
unsigned read_buttons();
unsigned read_buttons_debounce();

// Control button io pins
void set_latch(unsigned latch_pins, unsigned long max_latch_time = 15000);
void break_latch();
bool is_latched();
unsigned get_latched();

Action get_action();
Action get_action_enh();
const char * action_str(Action action);

