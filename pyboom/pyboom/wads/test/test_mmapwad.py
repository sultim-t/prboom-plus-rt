from pprint import pprint

import numarray
from py import path

from autopath import pyboom_base
from pyboom.wads.mmapwad import WadFileMMap
from pyboom.wads.interfaces import ILumpInfo, ILumpAsString, ILumpAsNumArray

def setup_module(module):
    global wad
    wads = pyboom_base / 'wads'
    pprint(wads)
    assert wads.check()
    filename = wads / 'doom.wad'
    assert filename.check()
    wad = WadFileMMap(filename.strpath)

def test_simple():
    pprint(len(wad))
    pprint(wad[0])
    pprint(wad[0].asNumArray())
    pprint(wad[0].asString())
    l = wad[1]
    pprint(l)
    pprint(l.asNumArray())
    pprint(l.asString())

def test_lumpinfo_interface():
    l = wad[0]
    i = ILumpInfo(l)
    print i
    assert i.getName() == 'PLAYPAL'
    assert len(i) > 0

def test_lumpasstring_interface():
    l = wad[0]
    i = ILumpAsString(l)
    s = i.asString()
    print len(s)
    assert isinstance(s, str)
    assert len(s) == 768*14

def test_lumpasnumarray_interface():
    l = wad[0]
    i = ILumpAsNumArray(l)
    a = i.asNumArray()
    print len(a)
    assert isinstance(a, numarray.ArrayType)
    assert len(a) == 768*14
