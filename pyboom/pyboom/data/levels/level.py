from numarray import array
from numarray.records import array as recordarray

from pyboom.wads.interfaces import ILumpAsNumArray, IWadNameMap

class Level(object):
    lumpnames = [
        "THINGS",       # Monsters, items..
        "LINEDEFS",     # LineDefs, from editing
        "SIDEDEFS",     # SideDefs, from editing
        "VERTEXES",     # Vertices, edited and BSP splits generated
        "SEGS",         # LineSegs, from LineDefs split by BSP
        "SSECTORS",     # SubSectors, list of LineSegs
        "NODES",        # BSP nodes
        "SECTORS",      # Sectors, from editing
        "REJECT",       # LUT, sector-sector visibility
        "BLOCKMAP"      # LUT, motion clipping, walls/grid element
    ]

    def __init__(self, src, name):
        if not self.check(src, name):
            raise ValueError, "'%s' is not a level." % name
        self._src = src
        self._name = name
        self._lumps = {}
        marker = IWadNameMap(src).getLumpName(name)
        self._lumps['marker'] = marker
        offset = marker.getIndex()+1
        for index, name in enumerate(Level.lumpnames):
            self._lumps[name] = src[offset+index]
        self._loadVertexes()

    def check(src, name):
        lump = IWadNameMap(src).getLumpName(name)
        offset = lump.getIndex()+1
        for index, name in enumerate(Level.lumpnames):
            lump = src[offset+index]
            if lump.getName() != name:
                return False
        return True
    check = staticmethod(check)

    def _loadVertexes1(self):
        lump = self._lumps['VERTEXES']
        vert_array = ILumpAsNumArray(lump).asNumArray()
        size = len(vert_array) / 4
        vertexes = recordarray(
            buffer=vert_array._data,
            shape=size,
            formats='1i2,1i2',
            names=('x','y'),
        )
        self._vertexes = recordarray(
            shape=size,
            formats='1i4,1i4',
            names=('x','y'),
        )
        self._vertexes.field('x')[:] = vertexes.field('x')
        self._vertexes.field('y')[:] = vertexes.field('y')
        self._vertexes.field('x')[:] <<= 16
        self._vertexes.field('y')[:] <<= 16

    def _loadVertexes(self):
        lump = self._lumps['VERTEXES']
        vert_array = ILumpAsNumArray(lump).asNumArray()
        size = len(vert_array) / 4
        vertexes = array(
            sequence=vert_array._data,
            shape=(size,2),
            type='i2',
        )
        self._vertexes = array(
            shape=(size,2),
            type='i4',
        )
        self._vertexes[:] = vertexes
        self._vertexes[:] <<= 16
        self._vertexes_rec = recordarray(
            buffer=self._vertexes._data,
            shape=size,
            formats='1i4,1i4',
            names=('x','y'),
        )
