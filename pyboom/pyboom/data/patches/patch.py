from protocols import advise
from numarray import array, ArrayType
from numarray.records import array as recordarray
from numarray.objects import array as objectarray

from pyboom.wads.interfaces import ILumpAsNumArray
from pyboom.tool.numutils import array_view, IAsNumArray

from interfaces import IPatch

class Patch(object):

    advise(
        instancesProvide=[IPatch],
    )

    def __init__(self, data):
        self._array = IAsNumArray(data).asNumArray()
        header = array_view(self._array[:8], type='i2')
        self._width = header[0]
        self._height = header[1]
        self._offsets = array_view(self._array[8:], type='i4', shape=(self._width,))

    def getWidth(self):
        return self._width

    def getHeight(self):
        return self._height

    def _getSizes(self):
        sizes = getattr(self, '_sizes', None)
        if sizes is None:
            sizes = self._sizes = array(shape=(self._width,), type='i4')
            offsets = self._offsets
            sizes[:-1] = offsets[1:] - offsets[:-1]
            sizes[-1] = len(self._array) - offsets[-1]
        return sizes

    def getRawColumn(self, index):
        sizes = self._getSizes()
        offset = self._offsets[index]
        return self._array[offset:offset+sizes[index]]

    def getRawColumns(self):
        sizes = self._getSizes()
        result = objectarray(None, shape=(self._width,))
        for index, offset in enumerate(self._offsets):
            result[index] = self._array[offset:offset+sizes[index]]
        return result
