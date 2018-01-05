# A fake gpiozero for testing on the PC
from kbhit import KBHit

class LED:
    def __init__(self, pin):
        print("LED init on pin {}".format(pin))

    def on(self):
        pass

    def off(self):
        pass

class Button:
    def __init__(self, pin, pull_up=True, bounce_time=0):
        print("Button init on pin {} => {}".format(pin, not pull_up))
        self.is_pressed = not pull_up
        self.kb = KBHit()
        self.keys = set()

    def read_pressed(self):
        if self.kb.kbhit():
            c = self.kb.getch()
            if c in self.keys:
                self.keys = self.keys.difference({c})
            else:
                self.keys = self.keys.union({c})
            print("Got: {}".format(self.keys))
            return c in self.keys

    is_pressed = False
