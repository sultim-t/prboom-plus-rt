from protocols import adapt, advise

from interfaces import IWad, IWadDirectory, IWadNameMap, IMultisourceWad
from common import get_namespaces

class MultiSourceWad(object):

    advise(
        instancesProvide=[IWad, IWadNameMap, IMultisourceWad],
    )

    def __init__(self):
        self._lumps = []
        self._wads = []

    def appendWad(self, src):
        append = self._lumps.append
        for name, index in IWadDirectory(src).getDirectory():
            append((name, index, src))
        self._wads.append(src)
        self._updateLookupTables()

    def getWads(self):
        return self._wads[:]

    def _updateLookupTables(self):
        names = IWadDirectory(self).getNames()
        self._namespaces = get_namespaces(names)
        self._name_map = {}
        for index, name in enumerate(names):
            self._name_map.setdefault(name, []).insert(0, index)

    def __getitem__(self, index):
        index, name, src = self._lumps[index]
        return src[index]

    def getLumpName(self, name, namespace='global'):
        name = name.upper()
        for index in self._name_map[name]:
            lump = self[index]
            if lump.getNamespace() == namespace:
                return lump
        raise KeyError, name
