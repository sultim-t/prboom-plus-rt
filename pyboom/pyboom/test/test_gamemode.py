from autopath import pyboom_base

from pyboom.gamemode import GameMode
from pyboom.wads import open_wadfile

def setup_module(module):
    global wads
    wads = pyboom_base / 'wads'
    assert wads.check()

def test_gamemode():
    files_mode = [
        ('doom1.wad', ['shareware'], 'doom'),
        ('doom.wad', ['registered','retail'], 'doom'),
        ('udoom.wad', ['retail'], 'doom'),
        ('doom2.wad', ['commercial'], 'doom2'),
        ('plutonia.wad', ['commercial'], 'plutonia'),
        ('tnt.wad', ['commercial'], 'tnt'),
    ]
    for wadname, modes, mission in files_mode:
        filename = wads / wadname
        if filename.check():
            iwad = open_wadfile(filename.strpath)
            gamemode = GameMode(iwad)
            result = False
            for mode in modes:
                result = result or gamemode.mode == mode
            print wadname, modes
            print '%s == %s: %s' % (gamemode.mode, mode, result)
            assert result
            print 'Mission: %s == %s: %s' % (gamemode.mission,
                                             mission,
                                             gamemode.mission == mission)
            assert gamemode.mission == mission
