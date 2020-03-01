
import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

from adaption.adapter import Adapter



'''
Not instantiable by itself, only inheritable.
No author should/can use "Item.foo"
In other words, private to GimpFu
'''


class GimpfuItem( Adapter ) :
    '''
    GimpFu convenience methods.
    E.G. alias Gimp methods, etc.

    Attributes common to Drawable and Vector
    '''

    @classmethod
    def DynamicWriteableAdaptedProperties(cls):
        # !!! return ('name') is not a tuple, use tuple('name') or ('name',)
        return tuple( 'name' )

    @classmethod
    def DynamicReadOnlyAdaptedProperties(cls):
        return ()

    @classmethod
    def DynamicTrueAdaptedProperties(cls):
        return ()


    '''
    methods
    '''

    # Reason: alias
    def translate(self, x, y):
        # assert adaptee is-a Gimp.Item that has transform methods
        self._adaptee.transform_translate(x,y)


    '''
    Properties
    '''

    # Reason:alies, upper case to lower case
    def ID(self):
        # !!! id is property of Item
        return self._adaptee.id


    """
    OLD now Dynamic
    @property
    def name(self):
        print("Calling Foo.get_name(): ")
        #print(dir(self._adaptee))
        result = self._adaptee.get_name()
        print("name() returns item name: ", result)
        return result
    @name.setter
    def name(self, name):
        return self._adaptee.set_name(name)
    """
