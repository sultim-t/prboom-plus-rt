from pprint import pprint

from autopath import pyboom_base

from pyboom.wads import open_wadfile
from pyboom.wads.multisourcewad import MultiSourceWad
from pyboom.wads.interfaces import ILumpAsString, IWadNameMap
from pyboom.data.textures.textures import Textures

def setup_module(module):
    global wad
    wads = pyboom_base / 'wads'
    pprint(wads)
    assert wads.check()
    filename = wads / 'doom.wad'
    assert filename.check()
    doom = open_wadfile(filename.strpath)
    # MultiSourceWad objects have a much faster getLumpName method
    wad = MultiSourceWad()
    wad.appendWad(doom)

def test_simple():
    textures = Textures(wad)
    pprint(textures._textures)
