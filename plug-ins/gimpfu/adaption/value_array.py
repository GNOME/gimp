

import gi

# gi.require_version("GLib", "2.32")
from gi.repository import GLib    # GArray

from gi.repository import GObject    # GObject type constants

gi.require_version("Gimp", "3.0")
from gi.repository import Gimp




from message.proceed_error import *




class FuValueArray():
    '''
    Adapts GimpValueArray.

    Keeps a list, and produces a GimpValueArray.
    Provides stack ops for the list.

    A GimpValueArray holds arguments.
    Arguments are not one-to-one with items in GimpValueArray because:
    1.) Gimp's (int, [float]) pair of formal arguments
        is mangled by PyGObject to one GValue, a GArray.
        Thus a v2 Author's (int, [float]) is mangled by GimpFu to one GValue.
    2.) GimpFu inserts RunMode arguments not present in Author's args

                       FuValueArray  GimpValueArray
    -----------------------------------------
    indexable          Y             Y
    static len         N             Y
    stack push/pop ops Y             N

    A singleton, with static methods and data

    In a push operations, the gtype may be from the value,
    or the gtype may be passed (when an upcast occurred previously.)
    '''


    # class var for the FuValueArray singleton:   _list_gvalues

    @classmethod
    def len(cls):
        return len(cls._list_gvalues)


    @classmethod
    def dump(cls):
        ''' Not a true repr, more akin to str '''
        result = f"Length: {cls.len()} :" + ', '.join(str(item) for item in cls._list_gvalues)
        return result


    @classmethod
    def reset(cls):
        cls._list_gvalues = []


    @classmethod
    def push_gvalue(cls, a_gvalue):
        ''' Push a Gvalue. '''
        cls._list_gvalues.append(a_gvalue)


    @classmethod
    def get_gvalue_array(cls):
        ''' Return a GimpValueArray for the elements in the list. '''
        result = Gimp.ValueArray.new(cls.len())

        # non-pythonic iteration over ValueArray
        index = 0
        for item in cls._list_gvalues:
            result.insert(index, item)
            index += 1
        return result
