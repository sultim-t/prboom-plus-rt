from protocols import advise
from numarray import array, getTypeObject
import numarray.memmap
from numarray.records import array as recordarray

from pyboom.tool.numutils import array_view, IAsNumArray
from lumpinfo import LumpInfo
from interfaces import ILumpAsString, ILumpAsNumArray
from interfaces import IWad, IWadKind, IWadDirectory
from common import get_namespaces

__all__ = ['open_wadfile']

class LumpMMap(LumpInfo):

    advise(
        instancesProvide=[ILumpAsString, ILumpAsNumArray],
    )

    def __init__(self, *args, **kwargs):
        LumpInfo.__init__(self, *args, **kwargs)

    def asString(self):
        data = self._src._getSlice(self._index)
        return str(data)

    def asNumArray(self, typecode='u1', type=None):
        data = self._src._getSlice(self._index)
        type = getTypeObject(data._buffer, typecode, type)
        return array(
                    sequence=data._buffer,
                    shape=(self._size / type.bytes,),
                    type=type
               )

class WadFileMMap(object):

    advise(
        instancesProvide=[IWad, IWadKind, IWadDirectory],
    )

    def __init__(self, filename):
        self._filename = str(filename)
        self._wad_data = numarray.memmap.open(self._filename, 'r')
        header = recordarray(
            buffer=self._wad_data[0:12],
            shape=1,
            formats='1a4,1i4,1i4',
            names=('kind','numlumps','offset')
        )
        #header = struct.unpack('<4sii', str(self._wad_data[0:12]))
        self._kind = header[0].field('kind')
        self._numlumps = header[0].field('numlumps')
        self._directory_offset = header[0].field('offset')
        self._directory = recordarray(
            buffer=self._wad_data[self._directory_offset:],
            shape=self._numlumps,
            formats='1i4,1i4,1a8',
            names=('pos','size','name'),
        )
        self._updateNamespace()
        self._slice_cache = {}

    def _updateNamespace(self):
        names = self._directory.field('name')
        self._namespaces = get_namespaces(names)

    def __len__(self):
        return self._numlumps

    def __getitem__(self, index):
        row = self._directory[index]
        ns = self._namespaces[index]
        return LumpMMap(
            row.field('name'),
            index,
            ns,
            row.field('pos'),
            row.field('size'),
            self
        )

    def getNames(self):
        return list(self._directory.field('name'))

    def getDirectory(self):
        return [x for x in enumerate(self._directory.field('name'))]

    def getKind(self):
        return self._kind

    def __repr__(self):
        return '<%s.%s("%s")>' % (self.__class__.__module__, self.__class__.__name__, self._filename)

    def _getSlice(self, index):
        if self._slice_cache.has_key(index):
            return self._slice_cache[index]
        else:
            row = self._directory[index]
            pos = row.field('pos')
            size = row.field('size')
            result = self._wad_data[pos:pos+size]
            self._slice_cache[index] = result
            return result
