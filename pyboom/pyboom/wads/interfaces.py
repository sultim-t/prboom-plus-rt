from protocols import Interface

########################################
## Lumps
########################################

class ILumpInfo(Interface):
    """Interface to the information about a lump from a wad file."""

    def getName():
        """Returns the name of this lump."""

    def __len__():
        """Returns the size of this lump."""

    def getIndex():
        """Returns the index of this lump in the wad file."""

    def getNamespace():
        """Returns the namespace of this lump. This is one of the following:
           'global' -> the default
           'sprites' -> lumps between the S_START and S_END tags
           'flats' -> lumps between the F_START and F_END tags
           'colormaps' -> lumps between the C_START and C_END tags
           'prboom' -> lumps between the B_START and B_END tags
           """

class ILumpAsString(ILumpInfo):
    """Interface to the lump data as a string."""

    def asString():
        """Returns the data of this lump as a string."""

class ILumpAsNumArray(ILumpInfo):
    """Interface to the lump data as a string."""

    def asNumArray(typecode=None, type=None):
        """Returns the data of this lump as an numeric array."""

########################################
## Wads
########################################

class IWad(Interface):
    """Interface to a wad file."""

    def __len__():
        """Returns the number of lumps in this wad."""

    def __getitem__(index):
        """Returns the lump at the given index."""

class IWadDirectory(IWad):
    def getNames():
        """Returns the list of names in this wad."""

    def getDirectory():
        """Returns a list of tuples of the form (index, name)."""

class IWadNameMap(IWad):
    def getLumpName(name, namespace='global'):
        """Returns the lump with the given name within the given namespace."""

class IWadKind(IWad):
    def getKind():
        """Returns the kind of this wad file. This is one of the following:
           'IWAD' -> for a main wad file
           'PWAD' -> for a patch wad file
           'LUMP' -> for a single lump (i.e. a demo)
        """

class IMultisourceWad(IWad):
    """Interface to a wad combined from multiple sources."""

    def appendWad(wad):
        """Add the given wad as a new source."""

    def getWads():
        """Returns the source wads."""

class IDynamicMultisourceWad(IMultisourceWad):
    """Interface to a wad combined from multiple sources which can dynamically
       be added and removed."""

    def removeWad(wad):
        """Removes the given wad from the sources."""
