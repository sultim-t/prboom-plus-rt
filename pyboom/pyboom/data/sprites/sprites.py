from pprint import pprint
import sets

from pyboom.wads.interfaces import IWadDirectory

sprite_names = [
    "TROO","SHTG","PUNG","PISG","PISF","SHTF","SHT2","CHGG","CHGF","MISG",
    "MISF","SAWG","PLSG","PLSF","BFGG","BFGF","BLUD","PUFF","BAL1","BAL2",
    "PLSS","PLSE","MISL","BFS1","BFE1","BFE2","TFOG","IFOG","PLAY","POSS",
    "SPOS","VILE","FIRE","FATB","FBXP","SKEL","MANF","FATT","CPOS","SARG",
    "HEAD","BAL7","BOSS","BOS2","SKUL","SPID","BSPI","APLS","APBX","CYBR",
    "PAIN","SSWV","KEEN","BBRN","BOSF","ARM1","ARM2","BAR1","BEXP","FCAN",
    "BON1","BON2","BKEY","RKEY","YKEY","BSKU","RSKU","YSKU","STIM","MEDI",
    "SOUL","PINV","PSTR","PINS","MEGA","SUIT","PMAP","PVIS","CLIP","AMMO",
    "ROCK","BROK","CELL","CELP","SHEL","SBOX","BPAK","BFUG","MGUN","CSAW",
    "LAUN","PLAS","SHOT","SGN2","COLU","SMT2","GOR1","POL2","POL5","POL4",
    "POL3","POL1","POL6","GOR2","GOR3","GOR4","GOR5","SMIT","COL1","COL2",
    "COL3","COL4","CAND","CBRA","COL6","TRE1","TRE2","ELEC","CEYE","FSKU",
    "COL5","TBLU","TGRN","TRED","SMBT","SMGT","SMRT","HDB1","HDB2","HDB3",
    "HDB4","HDB5","HDB6","POB1","POB2","BRS1","TLMP","TLP2",
    "TNT1", # invisible sprite - phares 3/9/98
    "DOGS", # killough 7/19/98: Marine's best friend :)
]

class SpriteDef(object):
    def __init__(self, index, frame, rotation, flip):
        self.index = index
        self.frame = frame
        self.rotation = rotation
        self.flip = flip

class Sprites(object):
    def __init__(self, src, names=None):
        self._src = src
        if names is None:
            self._names = sprite_names[:]
        else:
            self._names = names[:]
        self._updateLookups()

    def _updateLookups(self):
        sprites = [x for x in self._src if x.getNamespace() == 'sprites']
        self._name_map = name_map = {}
        for lump in sprites:
            name = lump.getName()
            frame_map = name_map.setdefault(name[:4], {})
            rotation_map = frame_map.setdefault(name[4], {})
            rotation_map[name[5]] = lump.getIndex()
            if len(name)==8:
                rotation_map = frame_map.setdefault(name[6], {})
                rotation_map[name[7]] = lump.getIndex()
