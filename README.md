# RoboDesk
Arduino code for LOGICDATA protocol and remote handset hacking for automated standing desk

The `simple` branch contains only button-latching code that listens to the changing display and lets go of the MEM buttons when the display stops changing.

# Warning
This interface probably violates the LOGICDATA warranty.  Note this warning in their literature:

Danger: it is not allowed to connect self constructed products to
LOGICDATA motor controls. To prevent damage of the unit, use only
components suitable for LOGICDATA motor controls.

Next steps:
Isolate buttons and put trinket in-the-middle to handle double-click more cleanly
Isolate TX line and to provide our own messages
Decode TX commands from controller
AWS IOT
Bluetooth
AI, followed by world domination

