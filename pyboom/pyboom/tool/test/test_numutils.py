from pprint import pprint

import numarray
from py.test import raises

from autopath import pyboom_base

from pyboom.tool.numutils import array_view

def print_array_internals(arr):
    names = [x for x in dir(arr) if x[0]=='_' and x[1]!='_']
    for name in names[1:]:
        value = getattr(arr, name, None)
        if callable(value):
            continue
        print "%s: " % name,
        pprint(value)

def test_array_view():
    a = numarray.arrayrange(16, typecode='u1')
    print 'a', a
    print_array_internals(a)
    assert len(a) == 16
    b = array_view(a, 'u2')
    print 'b', b
    print_array_internals(b)
    assert len(b) == 8
    assert a._data == b._data

    assert a[0] == 0
    assert a[1] == 1
    assert b[0] == 256
    assert b[1] == 770
    a[0] = 1
    assert b[0] == 257
    a[1] = 2
    assert b[0] == 513

    c = array_view(b, 'u4', shape=2)
    print 'c', c
    print_array_internals(c)
    assert len(c) == 2
    assert a._data == c._data
    assert b._data == c._data

    value = (3*2**24)+(2*2**16)+(2*2**8)+1
    assert c[0] == value

    c[0] = 0

    assert a[0] == 0
    assert a[1] == 0
    assert a[2] == 0
    assert a[3] == 0
    assert a[4] == 4

    assert b[0] == 0
    assert b[1] == 0
    assert b[2] == 1284

    a1 = a[4:]
    print 'a1', a1
    print_array_internals(a1)
    d = array_view(a1, 'u4', shape=1)
    print 'd', d
    print_array_internals(d)
    assert len(d) == 1
    assert a._data == d._data
    assert d[0] == 117835012

def test_array_view_slices():
    a = numarray.arrayrange(16, typecode='u1')
    print 'a', a
    print_array_internals(a)
    assert len(a) == 16
    a1 = a[::2]
    print 'a1', a1
    print_array_internals(a1)
    raises(ValueError, "array_view(a1, 'u2')")

def test_array_view_shape():
    a = numarray.arrayrange(16, typecode='u1')
    print 'a', a
    print_array_internals(a)
    assert len(a) == 16

    a.setshape((2,2,2,2))
    print 'a', a
    print_array_internals(a)
    assert len(a) == 2

    b = array_view(a, 'u2')
    print 'b', b
    print_array_internals(b)
    assert b[0][0][0][0] == 256
    assert b[0][0][1][0] == 770

    c = array_view(b, 'u1')
    print 'c', c
    print_array_internals(c)

    assert numarray.alltrue(numarray.alltrue(numarray.alltrue(numarray.alltrue(a==c))))
