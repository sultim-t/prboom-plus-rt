from protocols import advise
from interfaces import ILumpInfo

class LumpInfo(object):

    advise(
        instancesProvide=[ILumpInfo],
    )

    def __init__(self, name, index, namespace, pos, size, src):
        self._name = name
        self._index = index
        self._namespace = namespace
        self._pos = pos
        self._size = size
        self._src = src

    def getName(self):
        return self._name

    def __len__(self):
        return self._size

    def getIndex(self):
        return self._index

    def getNamespace(self):
        return self._namespace

    def __repr__(self):
        return '<Lump("%s", %i, "%s", %s, %s)>' % (self._name,
                                               self._index,
                                               self._namespace,
                                               self._size,
                                               self._src)
