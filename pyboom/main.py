from pprint import pprint

import wx.py
from py import path
import pygame
import thread
import new

import pyboom
import pyboom.doom
import pyboom.wads
from pyboom.wads.interfaces import IWadDirectory
from pyboom.data.patches.patch import Patch

class App(wx.App):
    """PyShell standalone application."""

    def OnInit(self):
        def OnIdle(self, event):
            pygame.display.update()
            pygame.event.pump()
            return wx.py.shell.Shell.OnIdle(self, event)
        global screen
        pygame.init()
        wx.InitAllImageHandlers()
        self.frame = wx.py.crust.CrustFrame()
        #self.frame = wx.py.shell.ShellFrame()
        oldOnIdle = self.frame.shell.OnIdle
        self.frame.shell.OnIdle = new.instancemethod(OnIdle, oldOnIdle.im_self, oldOnIdle.im_class)
        wx.EVT_IDLE(self.frame.shell, self.frame.shell.OnIdle)
        self.frame.SetSize((750, 525))
        self.frame.SetPosition((0, 0))
        self.frame.Show()
        self.SetTopWindow(self.frame)
        self.frame.shell.SetFocus()
        screen = pygame.display.set_mode((320,200))
        return True

if __name__=='__main__':
    global doom, arms
    basepath = path.local(__file__)
    basepath = basepath.dirpath()
    wads = basepath / 'wads'
    assert wads.check()
    w = pyboom.wads.MultiSourceWad()
    w.appendWad(pyboom.wads.open_wadfile(wads / 'doom.wad'))
    doom = pyboom.doom.Doom(w)
    #pprint(IWadDirectory(doom.wads).getDirectory())
    arms = Patch(doom.wads.getLumpName('STARMS'))
    app = App()
    app.MainLoop()
