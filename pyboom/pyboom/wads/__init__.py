import adapters
from multisourcewad import MultiSourceWad
from mmapwad import WadFileMMap

def open_wadfile(filename):
    return WadFileMMap(filename)
