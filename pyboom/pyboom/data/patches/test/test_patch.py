import sys
from pprint import pprint

from autopath import pyboom_base
from pyboom.wads import open_wadfile
from pyboom.wads.interfaces import ILumpAsString, ILumpAsNumArray, IWadNameMap
from pyboom.data.patches.patch import Patch

def setup_module(module):
    global wad
    wads = pyboom_base / 'wads'
    pprint(wads)
    assert wads.check()
    filename = wads / 'doom.wad'
    assert filename.check()
    wad = open_wadfile(filename.strpath)

def test_data_interface():
    lump = IWadNameMap(wad).getLumpName('TITLEPIC')
    assert Patch(lump)
    assert Patch(ILumpAsString(lump).asString())
    assert Patch(ILumpAsNumArray(lump).asNumArray())

def test_width():
    lump = IWadNameMap(wad).getLumpName('TITLEPIC')
    assert Patch(lump).getWidth() == 320
    assert Patch(ILumpAsString(lump).asString()).getWidth() == 320
    assert Patch(ILumpAsNumArray(lump).asNumArray()).getWidth() == 320
    lump = IWadNameMap(wad).getLumpName('STARMS')
    assert Patch(lump).getWidth() == 40
    assert Patch(ILumpAsString(lump).asString()).getWidth() == 40
    assert Patch(ILumpAsNumArray(lump).asNumArray()).getWidth() == 40

def test_height():
    lump = IWadNameMap(wad).getLumpName('TITLEPIC')
    assert Patch(lump).getHeight() == 200
    assert Patch(ILumpAsString(lump).asString()).getHeight() == 200
    assert Patch(ILumpAsNumArray(lump).asNumArray()).getHeight() == 200
    lump = IWadNameMap(wad).getLumpName('STBAR')
    assert Patch(lump).getHeight() == 32
    assert Patch(ILumpAsString(lump).asString()).getHeight() == 32
    assert Patch(ILumpAsNumArray(lump).asNumArray()).getHeight() == 32
