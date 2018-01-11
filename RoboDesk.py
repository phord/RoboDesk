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
MOD_HS1 = 16
MOD_HS2 = 6
MOD_HS3 = 25
MOD_HS4 = 26

last_signal = 0

class LatchingButton:
    is_latched = False

    def __init__(self, pin, pull_up=False, bounce_time=0.001):
        self.pin = pin
        self.pull_up = pull_up
        self.bounce_time = bounce_time
        self.led = None
        self.reset()

    def latch(self):
        print("Latch: {}".format(self.pin))

        if not self.is_latched:
            self.is_latched = True
            self.button.close()
            self.led = LED(self.pin)
            self.led.on()

    def __int__(self):
        return self.pin

    def __bool__(self):
        return (not self.is_latched) and self.button.is_pressed

    def reset(self):
        print("Unlatch: {}".format(self.pin))

        if self.is_latched:
            self.is_latched = False
            self.led.close()

        self.button = Button(self.pin, pull_up=self.pull_up, bounce_time=self.bounce_time)

# Globals
tx  = LatchingButton(MOD_TX, bounce_time=0.100)
hs1 = LatchingButton(MOD_HS1, bounce_time=0.020)
hs2 = LatchingButton(MOD_HS2, bounce_time=0.020)
hs3 = LatchingButton(MOD_HS3, bounce_time=0.020)
hs4 = LatchingButton(MOD_HS4, bounce_time=0.020)

mem_switches = { tuple(sorted(x)) for x in [ (hs3.pin,), (hs4.pin,), (hs2.pin, hs4.pin) ] }

def break_latch(buttons):
    for b in buttons:
        b.reset()

def millis():
    return int(round(time() * 1000))

def check_display():
  global last_signal
  if bool(tx):
      return
  last_signal = millis()

def read_buttons():
  buttons = [hs1, hs2, hs3, hs4]
  return {b for b in buttons if b}

def read_latch():
  buttons = read_buttons()

  # Only latch MEM buttons
  hashes = tuple( sorted([ b.pin for b in buttons ]) )

  if not hashes in mem_switches:
      return set()

  for b in buttons:
      b.latch()

  global last_signal
  last_signal = millis()

  return buttons

def hold_latch(buttons):
  global last_signal

  delta = millis() - last_signal

  # Let go after 1.5 seconds with no signals
  # Break the latch if some other button is detected
  if delta < 1500 and len(read_buttons()) == 0:
      return buttons

  break_latch(buttons)
  return set()

# MAIN
# Monitor panel buttons for our commands and take over when we see one
keys=set()
print("RoboDesk v1")
while True:
    check_display()
    if len(keys) == 0:
        keys = read_latch()
    else:
        keys = hold_latch(keys)
