from protocols import advise
from interfaces import IPatches

class Patches(object):

    advise(
        instancesProvide=[IPatches],
    )

    def __init__(self, src):
        self._src = src

    def __getitem__(self, index):
        raise NotImplementedError
