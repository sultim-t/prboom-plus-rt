class Texture(object):
    def __init__(self, src, name, width, height, masked, patches):
        self._src = src
        self._name = name
        self._width = width
        self._height = height
        self._masked = masked
        self._patches = patches

    def __repr__(self):
        return "<Texture(%r,%r,%r,%r,%r)>" % (
            self._name, self._width, self._height,
            self._masked, self._patches
        )

class TexPatch(object):
    __slots__ = ('originx','originy','patchindex','stepdir','colormap')
    def __init__(self, originx, originy, patchindex, stepdir, colormap):
        self.originx = originx
        self.originy = originy
        self.patchindex = patchindex
        self.stepdir = stepdir
        self.colormap = colormap

    def __repr__(self):
        return "TexPatch(%r,%r,%r,%r,%r)" % (
            self.originx, self.originy,
            self.patchindex, self.stepdir,
            self.colormap
        )
