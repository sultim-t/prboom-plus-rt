from pprint import pprint

from autopath import pyboom_base
from py.test import raises
import numarray

from pyboom.wads import open_wadfile
from pyboom.wads.multisourcewad import MultiSourceWad
from pyboom.wads.interfaces import ILumpAsString, IWadNameMap
from pyboom.data.levels.level import Level

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

def test_check():
    assert Level.check(wad, 'E1M1')
    assert Level.check(wad, 'E1M2')
    assert Level.check(wad, 'E1M3')
    assert Level.check(wad, 'E1M4')
    assert Level.check(wad, 'E1M5')
    assert Level.check(wad, 'E1M6')
    assert Level.check(wad, 'E1M7')
    assert Level.check(wad, 'E1M8')
    assert Level.check(wad, 'E1M9')
    assert Level.check(wad, 'E2M1')
    assert Level.check(wad, 'E2M2')
    assert Level.check(wad, 'E2M3')
    assert Level.check(wad, 'E2M4')
    assert Level.check(wad, 'E2M5')
    assert Level.check(wad, 'E2M6')
    assert Level.check(wad, 'E2M7')
    assert Level.check(wad, 'E2M8')
    assert Level.check(wad, 'E2M9')
    assert Level.check(wad, 'E3M1')
    assert Level.check(wad, 'E3M2')
    assert Level.check(wad, 'E3M3')
    assert Level.check(wad, 'E3M4')
    assert Level.check(wad, 'E3M5')
    assert Level.check(wad, 'E3M6')
    assert Level.check(wad, 'E3M7')
    assert Level.check(wad, 'E3M8')
    assert Level.check(wad, 'E3M9')
    assert not Level.check(wad, 'TITLEPIC')

def test_Level():
    level = Level(wad, 'E1M1')
    assert level
    raises(ValueError, "Level(wad, 'TITLEPIC')")
    pprint(level._vertexes)
    pprint(level._vertexes_rec)
    assert level._vertexes._data == level._vertexes_rec._data
    x1,y1 = level._vertexes[:,0].min(), level._vertexes[:,1].min()
    x2,y2 = level._vertexes[:,0].max(), level._vertexes[:,1].max()
    a = (x1,y1),(x2,y2)
    print a
    a1 = (x1>>16,y1>>16),(x2>>16,y2>>16)
    print a1
    x1,y1 = level._vertexes_rec.field('x').min(), level._vertexes_rec.field('y').min()
    x2,y2 = level._vertexes_rec.field('x').max(), level._vertexes_rec.field('y').max()
    b = (x1,y1),(x2,y2)
    print b
    b1 = (x1>>16,y1>>16),(x2>>16,y2>>16)
    print b1
    assert a1==b1
