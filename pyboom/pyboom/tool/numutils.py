import types

from protocols import advise, Interface
from numarray import array, getTypeObject, ArrayType

class IAsNumArray(Interface):
    def asNumArray():
        """Returns the data as an numeric array."""

class AsNumArrayForString(Interface):

    advise(
        instancesProvide=[IAsNumArray],
        asAdapterForTypes=[str],
    )

    def __init__(self, obj):
        self.obj = obj

    def asNumArray(self):
        return array(self.obj, type='u1')

class AsNumArrayForNumArray(Interface):

    advise(
        instancesProvide=[IAsNumArray],
        asAdapterForTypes=[ArrayType],
    )

    def __init__(self, obj):
        self.obj = obj

    def asNumArray(self):
        return array(self.obj, type='u1')

def array_view(sequence, typecode=None, type=None, shape=None):
    if sequence._bytestride != sequence._strides[-1]:
        raise ValueError, 'sequence contains unsupported strides'
    old_type = getTypeObject(sequence, None, None)
    type = getTypeObject(sequence, type, typecode)

    if shape is None:
        shape = sequence.getshape()
        size = shape[-1] * old_type.bytes / type.bytes
        shape = shape[:-1]+(size,)
    if isinstance(shape, types.IntType):
        shape = (shape,)

    strides = sequence._strides
    size = strides[-1] * type.bytes / old_type.bytes
    strides = strides[:-1]+(size,)

    result = sequence.view()
    result._type = type
    result._shape = shape
    result._itemsize = type.bytes
    result._bytestride = type.bytes
    result._strides = strides

    return result
