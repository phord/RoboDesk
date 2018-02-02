#include "buttons.h"
#include "Arduino.h"
#include "pins_trinket_pro.h"

/** Interface to Robodesk handset buttons
 */

// Display anything that changed since last time
void display_buttons(unsigned buttons, const char * msg ) {
  static unsigned prev = 0;

  if (msg[0] || prev!=buttons) {
    Serial.print((buttons & HS1) ? " HS1" :  " ---");
    Serial.print((buttons & HS2) ? " HS2" :  " ---");
    Serial.print((buttons & HS3) ? " HS3" :  " ---");
    Serial.print((buttons & HS4) ? " HS4" :  " ---");

    switch (buttons) {
      case UP:    Serial.print("    UP          ");    break;
      case DOWN:  Serial.print("    DOWN        ");    break;
      case SET:   Serial.print("    SET         ");    break;
      case MEM1:  Serial.print("    MEM1        ");    break;
      case MEM2:  Serial.print("    MEM2        ");    break;
      case MEM3:  Serial.print("    MEM3        ");    break;
      case NONE:  Serial.print("    ----        ");    break;
      default:    Serial.print(" ** UNKNOWN **  ");    break;
    }
    Serial.println(msg);
  }
  prev = buttons;
}

unsigned read_buttons() {
  unsigned buttons = 0;
  if (digitalRead(MOD_HS1)) buttons |= HS1;
  if (digitalRead(MOD_HS2)) buttons |= HS2;
  if (digitalRead(MOD_HS3)) buttons |= HS3;
  if (digitalRead(MOD_HS4)) buttons |= HS4;
  
  return buttons;
}

unsigned read_buttons_debounce() {
  static unsigned long debounce = 0;
  static unsigned prev_buttons = 0;

  unsigned diff = prev_buttons;
  prev_buttons = read_buttons();

  unsigned buttons = prev_buttons;

  // Ignore spurious signals
  if (diff && diff != prev_buttons) buttons = NONE;

  // Reset timer if buttons are drifting or unpressed
  if (!buttons) debounce = millis();

  // ignore signal until stable for 20ms
  if (millis()-debounce < 20) {
    buttons = NONE;
  }

  display_buttons(buttons);
  
  return buttons;
}

// Debounce buttons; interpret as actions

Action get_action_immed() {
  switch (read_buttons_debounce()) {
    case NONE: return Action::None;
    case UP:   return Action::Up;
    case DOWN: return Action::Down;
    case MEM1: return Action::Mem1;
    case MEM2: return Action::Mem2;
    case MEM3: return Action::Mem3;
    case SET:  return Action::Set;
  }

  return Action::Unknown;
}

Action double_click(Action action) {
  switch (action) {
    case Up:     return Up_Dbl;
    case Down:   return Down_Dbl;
    case Set:    return Set_Dbl;
    case Mem1:   return Mem1_Dbl;
    case Mem2:   return Mem2_Dbl;
    case Mem3:   return Mem3_Dbl;
    default:     return action;
  }
}

#define SINGLE_CLICK_THRESHOLD 300
// Interpret actions based on active-time and double-click, etc.
Action get_action_enh() {
  static Action queue = None;
  static Action curr = None;
  static unsigned long start;

  Action action = get_action_immed();

  unsigned long duration = millis() - start ;
  if (action != curr) {
    // New action; start recording it
    start = millis();
    Action prev = curr;
    curr = action;
    
    Serial.print("queue=");
    Serial.print(action_str(queue));
    Serial.print("  curr=");
    Serial.print(action_str(prev));
    Serial.print("  duration=");
    Serial.println(duration);
    if (action == Action::None) {
      // We just finished some button; remember it, maybe
      if (duration <= SINGLE_CLICK_THRESHOLD) {
        if (queue == prev) {
          // Double-Click encountered
          auto ret = double_click(queue);
          queue = Action::None;
          return ret;
        } else if (queue != Action::None) {
          // Two buttons in the hold; first is in "queue", second is in "prev"
          auto ret = queue;
          // Stuff prev back in curr so we will see it when we come this way again
          queue = prev;
          return ret;
        }

        // Previous action has not been sent yet; put in queue and wait for double-click
        queue = prev;
        return Action::None;
      }
    }
    // Started a new button, but we don't know what to do with it yet
    return Action::None;
  }

  // action is active for some duration now; decide how to proceed
  if (duration > SINGLE_CLICK_THRESHOLD) {
    // Current action is too long to be part of a double-click
    if (action != queue && queue != Action::None) {
      auto ret = queue;
      queue = Action::None;
  
      // BUG There's a race here if the next call thinks we already sent "curr" because the user let go in-between.  Oh well.
      return ret;
    }

    return action;
  }

  // Current action is too short to know what to do, still.
  return Action::None;
}

const char * action_str(Action action) {
  switch (action) {
    case Up:     return "Up";
    case Down:   return "Down";
    case Set:    return "Set";
    case Mem1:   return "Mem1";
    case Mem2:   return "Mem2";
    case Mem3:   return "Mem3";

    case Up_Dbl:     return "Up_Dbl";
    case Down_Dbl:   return "Down_Dbl";
    case Set_Dbl:    return "Set_Dbl";
    case Mem1_Dbl:   return "Mem1_Dbl";
    case Mem2_Dbl:   return "Mem2_Dbl";
    case Mem3_Dbl:   return "Mem3_Dbl";
    
    case None:   return "None";
    default:     return "Unknown";
  }
}

Action get_action() {
  static Action prev = Action::None;
  Action action = get_action_enh();
  if (action != prev) {
    Serial.print("  Action=");
    Serial.println(action_str(action));
    prev = action;
  }
  return action;
}

