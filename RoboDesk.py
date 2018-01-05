#!/usr/bin/python3

#
## RoboDesk
#
# Adds latching memory buttons to my LOGICDATA desk controller
# By Phil Hord,  This code is in the public domain January 2, 2018
#

from gpiozero import LED, Button
from time import sleep, localtime, time
import math
import os

# Input
MOD_TX  = 5

# Input/Output
MOD_HS1 = 2
MOD_HS2 = 0
MOD_HS3 = 4
MOD_HS4 = 1

last_signal = 0

HS1 = 1
HS2 = 2
HS3 = 4
HS4 = 8

UP   = HS1
DOWN = HS2
SET  = HS1 + HS2
MEM1 = HS3
MEM2 = HS4
MEM3 = HS2 + HS4

latched = 0

class LatchingButton:
    is_latched = False

    def __init__(self, pin, mask=0, pull_up=False, bounce_time=0):
        self.pin = pin
        self.pull_up = pull_up
        self.bounce_time = bounce_time
        self.led = None
        self.mask = mask
        self.reset()

    def latch(self):
        if not self.is_latched:
            self.is_latched = True
            self.button.close()
            self.led = LED(pin)
            self.led.on()

    def __int__(self):
        return self.mask

    def __bool__(self):
        self.button.read_pressed()
        return self.button.is_pressed

    def reset(self):
        if self.is_latched:
            self.is_latched = False
            self.led.close()

        self.button = Button(self.pin, pull_up=self.pull_up, bounce_time=self.bounce_time)

# Globals
tx  = LatchingButton(MOD_TX, pull_up=True)
hs1 = LatchingButton(MOD_HS1, mask=HS1, bounce_time=0.020)
hs2 = LatchingButton(MOD_HS2, mask=HS2, bounce_time=0.020)
hs3 = LatchingButton(MOD_HS3, mask=HS3, bounce_time=0.020)
hs4 = LatchingButton(MOD_HS4, mask=HS4, bounce_time=0.020)

# TODO: does this work?  is `buttons[0] is hs1`
buttons = [hs1, hs2, hs3, hs4]

def break_latch():
    for b in buttons:
        b.reset()

def latch_pin(button):
    button.latch()

def millis():
    return int(round(time() * 1000))

last_state = False
def check_display():
  global last_state
  state = bool(tx)
  if state == last_state:
      return

  last_state = state
  last_signal = millis()

def read_buttons():
  button_mask = 0
  for b in buttons:
      if b: button_mask += int(b)
  return button_mask

debounce = 0
prev_buttons = 0
def read_latch():
  global latched, prev_buttons
  if latched != 0:
      return

  if not prev_buttons:
      debounce = millis()

  diff = prev_buttons
  prev_buttons = read_buttons()

  # Only latch MEM buttons
  if prev_buttons != MEM1 and prev_buttons != MEM2 and prev_buttons != MEM3:
      prev_buttons = 0

  # Ignore spurious signals
  if diff and diff != prev_buttons:
      prev_buttons = 0

  # latch when signal is stable for 20ms
  if millis()-debounce > 20:
    latched = prev_buttons
    last_signal = millis()


def hold_latch():
  if latched == 0: return

  delta = millis() - last_signal

  # Let go after 1.5 seconds with no signals
  if delta > 1500:
    break_latch()
    return

  # Break the latch if some other button is detected
  if (read_buttons() | latched) != latched:
    break_latch()
    return

  if latched & HS1: hs1.latch()
  if latched & HS2: hs2.latch()
  if latched & HS3: hs3.latch()
  if latched & HS4: hs4.latch()


# MAIN
# Monitor panel buttons for our commands and take over when we see one
while True:
    check_display()
    read_latch()
    hold_latch()
