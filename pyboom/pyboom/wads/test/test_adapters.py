from pprint import pprint

from py.test import raises
from autopath import pyboom_base
from pyboom.wads.interfaces import IWadNameMap
from pyboom.wads.mmapwad import WadFileMMap
import pyboom.wads.adapters

def setup_module(module):
    global wad
    wads = pyboom_base / 'wads'
    pprint(wads)
    assert wads.check()
    filename = wads / 'doom.wad'
    assert filename.check()
    wad = WadFileMMap(filename.strpath)

def test_wadnamemap_adapter():
    wadname = IWadNameMap(wad)
    assert wadname.getLumpName('TEXTURE1')
    raises(KeyError, "wadname.getLumpName('FLAT3')")
    assert wadname.getLumpName('FLAT3', 'flats')
    assert wadname.getLumpName('flat3', 'flats')
    raises(KeyError, "wadname.getLumpName('TEXTURE3')")
