from pprint import pprint

from protocols import advise
from numarray import array
from numarray.records import array as recordarray
from numarray.strings import array as chararray
from numarray.objects import array as objectarray

from pyboom.tool.numutils import array_view
from pyboom.wads.interfaces import ILumpAsString, ILumpAsNumArray, IWadNameMap

from interfaces import ITextures
from texture import Texture, TexPatch

class Textures(object):

    advise(
        instancesProvide=[ITextures],
    )

    def __init__(self, src):
        self._src = src
        src = IWadNameMap(src)
        pnames_lump = src.getLumpName('PNAMES')
        num_pnames = ILumpAsNumArray(pnames_lump).asNumArray('i4')[0]
        pnames_a = ILumpAsNumArray(pnames_lump).asNumArray()
        pnames = chararray(
            buffer=pnames_a._data[4:],
            itemsize=8,
            shape=num_pnames,
        )
        patchlookup=array(shape=(num_pnames,), type='i4')
        patchlookup[:] = -1
        for index, name in enumerate(pnames):
            try:
                lump = src.getLumpName(name)
            except KeyError:
                try:
                    lump = src.getLumpName(name, 'sprites')
                except KeyError:
                    continue
            patchlookup[index] = lump.getIndex()

        self._textures = {}

        tex_lump = src.getLumpName('TEXTURE1')
        tex_num = ILumpAsNumArray(tex_lump).asNumArray('i4')[0]
        tex_a = ILumpAsNumArray(tex_lump).asNumArray()
        tex_offsets = array_view(tex_a[4:], shape=tex_num, type='i4')
        self._extract_textures(tex_a, tex_offsets, patchlookup)

        try:
            tex_lump = src.getLumpName('TEXTURE2')
        except KeyError:
            tex_lump = None
        if tex_lump is not None:
            tex_num = ILumpAsNumArray(tex_lump).asNumArray('i4')[0]
            tex_a = ILumpAsNumArray(tex_lump).asNumArray()
            tex_offsets = array_view(tex_a[4:], shape=tex_num, type='i4')
            self._extract_textures(tex_a, tex_offsets, patchlookup)

    def _extract_textures(self, tex_a, tex_offsets, patchlookup):
        for offset in tex_offsets:
            _texture = recordarray(
                buffer = tex_a._data[offset:],
                shape=1,
                formats='1a8,1i4,1i2,1i2,1i4,1i2',
                names=('name','masked','width','height','pad','patchcount'),
            )
            _texture = _texture[0]
            _patches = recordarray(
                buffer = tex_a._data[offset+22:],
                shape=_texture.field('patchcount'),
                formats='1i2,1i2,1i2,1i2,1i2',
                names=('originx','originy','patchindex','stepdir','colormap'),
            )
            patches = []
            for patch in _patches:
                originx = patch.field('originx')
                originy = patch.field('originy')
                index = patchlookup[patch.field('patchindex')]
                stepdir = patch.field('stepdir')
                colormap = patch.field('colormap')
                patches.append(
                    TexPatch(originx, originy, index, stepdir, colormap)
                )
            name = _texture.field('name')
            texture = Texture(
                self,
                name,
                _texture.field('width'),
                _texture.field('height'),
                _texture.field('masked'),
                patches
            )
            self._textures[name] = texture

    def __len__(self):
        return len(self._textures)

    def __getitem__(self, key):
        return self._textures[key]

    def keys(self):
        return self._textures.keys()
