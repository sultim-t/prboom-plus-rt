from protocols import Interface

class ITextures(Interface):
    """A collection of textures which can be looked up by name."""

    def __len__():
        """Returns the number of textures."""

    def __getitem__(name):
        """Returns the texture with the given name."""

    def keys():
        """Returns all texture names."""
