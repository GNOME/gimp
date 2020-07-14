import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

# !!! Vectors => Drawable => Item => Adapter
from adapters.drawable import GimpfuDrawable





class GimpfuVectors( GimpfuDrawable ) :

    '''
    Notes on properties:
    name inherited
    '''
    @classmethod
    def DynamicWriteableAdaptedProperties(cls):
        return () + super().DynamicWriteableAdaptedProperties()

    @classmethod
    def DynamicReadOnlyAdaptedProperties(cls):
        return () + super().DynamicReadOnlyAdaptedProperties()

    @classmethod
    def DynamicTrueAdaptedProperties(cls):
        return () + super().DynamicTrueAdaptedProperties()




    def __init__(self, img=None, adaptee=None):

        if img is not None:
            # Totally new adaptee, created at behest of author
            # Gimp constructor named "new"
            super().__init__( Gimp.Vectors.new(img.unwrap() ) )
        else:
            # Create wrapper for existing adaptee (from Gimp)
            # Adaptee was created earlier at behest of Gimp user and is being passed into GimpFu plugin
            assert adaptee is not None
            super().__init__(adaptee)

        print("new GimpfuVectors with adaptee", self._adaptee)




    '''
    Methods for convenience.
    i.e. these were defined in PyGimp v2

    They specialize methods that might exist in Gimp.
    Specializations:
    - add convenience parameters
    - alias or rename: old name => new or simpler name
    - encapsulate: one call => many subroutines

    see other examples  adapters.image.py
    '''


    # copy is inherited

    def to_selection(self):
        raise RuntimeError("Obsolete: use image.select_item()")


    '''
    Properties.

    For convenience, GimpFu makes certain attributes have property semantics.
    I.E. get without parenthesises, and set by assignment, without calling setter() func.

    Properties that are canonically (with get_foo, and set_foo) implemented by Adaptee Gimp.Vectors
    are handled aby Adaptor.

    TODO, does Gimp GI provide properties?
    '''
