from protocols import adapt, advise, declareAdapter, NO_ADAPTER_NEEDED

from pyboom.tool.numutils import IAsNumArray

from common import get_namespaces
from interfaces import *

class WadDirectoryForWad(object):

    advise(
        instancesProvide=[IWadDirectory],
        asAdapterForProtocols=[IWad],
    )

    def __init__(self, obj):
        self.obj = obj

    def getNames(self):
        return [lump.getName() for lump in self.obj]

    def getDirectory(self):
        return [(lump.getIndex(), lump.getName()) for lump in self.obj]

class WadNameMapForWad(object):

    advise(
        instancesProvide=[IWadNameMap],
        asAdapterForProtocols=[IWad],
    )

    def __init__(self, obj):
        self.obj = obj

    def getLumpName(self, name, namespace='global'):
        name = name.upper()
        names = IWadDirectory(self.obj).getNames()
        namespaces = get_namespaces(names)
        try:
            index = names.index(name)
        except ValueError:
            raise KeyError, name
        if namespaces[index] != namespace:
            raise KeyError, name
        return self.obj[index]

declareAdapter(
    NO_ADAPTER_NEEDED,
    provides = [IAsNumArray],
    forProtocols = [ILumpAsNumArray],
)
