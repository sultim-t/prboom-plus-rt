from pprint import pprint

from py.test import raises

from autopath import pyboom_base
from pyboom.wads.mmapwad import WadFileMMap
from pyboom.wads.multisourcewad import MultiSourceWad

def setup_module(module):
    global wad
    wads = pyboom_base / 'wads'
    pprint(wads)
    assert wads.check()
    filename = wads / 'doom.wad'
    assert filename.check()
    doom = WadFileMMap(filename.strpath)
    print doom
    filename = wads / 'prboom.wad'
    assert filename.check()
    prboom = WadFileMMap(filename.strpath)
    print prboom
    wad = MultiSourceWad()
    wad.appendWad(doom)
    wad.appendWad(prboom)

def test_simple():
    pprint(wad.getLumpName('DCONFIG','prboom').asNumArray())
    pprint(wad.getLumpName('dconfig','prboom').asString())
    print wad.getLumpName('DCONFIG','prboom').asNumArray()[:10]
    #pprint.pprint(zip(w._lumps, w._namespaces))
    #pprint.pprint(list(w))

def test_namespace():
    raises(KeyError, "wad.getLumpName('FLAT17')")
    assert wad.getLumpName('FLAT17','flats')
