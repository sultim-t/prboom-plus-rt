from protocols import Interface

class IPatch(Interface):
    """Represents a patch."""

    def getWidth():
        """Returns the original width of this patch."""

    def getHeight():
        """Returns the original height of this patch."""

class IPatches(Interface):
    """Represents a directory of patches."""

    def __getitem__(index):
        """Returns the patch at the given index."""

class INamedPatches(IPatches):
    def getPatchName(name):
        """Returns the patch with the given name."""
