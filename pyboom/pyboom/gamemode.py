from wads.interfaces import IWadNameMap, IWadKind, IWadDirectory, ILumpAsString

class GameMode(object):
    def __init__(self, iwad):
        kind = IWadKind(iwad).getKind()
        if kind != 'IWAD':
            raise DoomError, "Not an IWAD."
        names = IWadDirectory(iwad).getNames()
        sw = 0
        rg = 0
        ud = 0
        cm = 0
        sc = 0
        for name in names:
            if name[0]=='E' and name[2]=='M':
                if name[1] == '4':
                    ud += 1
                elif name[1] == '3':
                    rg += 1
                elif name[1] == '2':
                    rg += 1
                elif name[1] == '1':
                    sw += 1
            elif name.startswith('MAP'):
                cm += 1
                if name[3] == '3' and name[4] in ('1','2'):
                    sc += 1
        self.mode = 'indetermined'
        self.has_secrets = False
        if cm>=30:
            self.mode = 'commercial'
            self.has_secrets = (sc >= 2)
        elif ud>=9:
            self.mode = 'retail'
        elif rg>=18:
            self.mode = 'registered'
        elif sw>=9:
            self.mode = 'shareware'

        if self.mode in ('shareware','registered','retail'):
            self.mission = 'doom'
        elif self.mode == 'commercial':
            self.mission = 'doom2'
            lump = IWadNameMap(iwad).getLumpName('ENDOOM')
            endoom = ILumpAsString(lump).asString()
            endoom = endoom[::2]
            if 'plutonia' in endoom.lower():
                self.mission = 'plutonia'
            elif 'evilution' in endoom.lower():
                self.mission = 'tnt'
        else:
            self.mission = None

        if self.mode == 'shareware':
            self.title = "DOOM Shareware"
        elif self.mode == 'registered':
            self.title = "DOOM Registered"
        elif self.mode == 'retail':
            self.title = "The Ultimate DOOM"
        elif self.mode == 'commercial':
            if self.mission == 'plutonia':
                self.title = "DOOM 2: Plutonia Experiment"
            elif self.mission == 'tnt':
                self.title = "DOOM 2: TNT - Evilution"
            else:
                self.title = "DOOM 2: Hell on Earth"
        else:
            self.title = "Public DOOM"
