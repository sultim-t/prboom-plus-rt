from pprint import pprint

from autopath import pyboom_base
from pyboom.wads.common import get_namespaces

def test_getnamespaces():
    names = [
        'GLOB01',
        'GLOB02',
        'GLOB03',
        'GLOB04',
        'GLOB05',
        'GLOB06',
        'S_START',
        'SPR01',
        'SPR02',
        'SPR03',
        'S_END',
        'GLOB07',
        'GLOB08',
        'GLOB09',
        'GLOB10',
        'FF_START',
        'FLAT01',
        'FLAT02',
        'FLAT03',
        'FLAT04',
        'FF_END',
        'GLOB11',
        'GLOB12',
        'GLOB13',
        'GLOB14',
        'GLOB15',
        'GLOB16',
    ]
    result = get_namespaces(names)
    pprint(result)
    assert result[names.index('SPR01')] == 'sprites'
    assert result[names.index('S_START')] == 'global'
    assert result[names.index('S_END')] == 'global'
    assert result[names.index('FLAT01')] == 'flats'
    assert result[names.index('FF_START')] == 'global'
    assert result[names.index('FF_END')] == 'global'

def test_nested():
    names = [
        'GLOB01',
        'GLOB02',
        'GLOB03',
        'GLOB04',
        'GLOB05',
        'GLOB06',
        'S_START',
        'SPR01',
        'SPR02',
        'FF_START',
        'FLAT01',
        'FLAT02',
        'FLAT03',
        'FLAT04',
        'FF_END',
        'SPR03',
        'S_END',
        'GLOB07',
        'GLOB08',
        'GLOB09',
        'GLOB10',
        'GLOB11',
        'GLOB12',
        'GLOB13',
        'GLOB14',
        'GLOB15',
        'GLOB16',
    ]
    result = get_namespaces(names)
    pprint(result)
    assert result[names.index('SPR01')] == 'sprites'
    assert result[names.index('S_START')] == 'global'
    assert result[names.index('S_END')] == 'global'
    assert result[names.index('FLAT01')] != 'flats'
    assert result[names.index('FF_START')] != 'global'
    assert result[names.index('FF_END')] == 'global'
    assert result[names.index('SPR03')] != 'sprites'
    assert result[names.index('GLOB07')] == 'global'
