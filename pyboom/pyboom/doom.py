import textwrap

from protocols import AdaptationFailure

from wads.interfaces import IMultisourceWad
from gamemode import GameMode

class DoomError(Exception):
    pass

class Doom(object):
    def __init__(self, wads):
        self.wads = wads
        iwad = IMultisourceWad(wads).getWads()[0]
        self.gamemode = GameMode(iwad)
