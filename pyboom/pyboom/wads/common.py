def get_namespaces(names):
    namespaces = [('S_START','S_END','sprites'),
                  ('F_START','F_END','flats'),
                  ('C_START','C_END','colormaps'),
                  ('B_START','B_END','prboom')]
    namespaces_t = [list(t) for t in zip(*namespaces)]
    ns = 'global'
    result = [None] * len(names)
    for index, name in enumerate(names):
        if ns == 'global':
            result[index] = ns
            if name in namespaces_t[0]:
                index = namespaces_t[0].index(name)
                ns = namespaces[index][2]
            elif name[1:] in namespaces_t[0]:
                index = namespaces_t[0].index(name[1:])
                ns = namespaces[index][2]
        else:
            if name in namespaces_t[1] or name[1:] in namespaces_t[1]:
                ns = 'global'
            result[index] = ns
    return result
