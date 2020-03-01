
import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

# !!! Layer => Drawable => Item => Adapter
from adapters.drawable import GimpfuDrawable





class GimpfuLayer( GimpfuDrawable ) :

    @classmethod
    def DynamicWriteableAdaptedProperties(cls):
        return ('mode', 'lock_alpha', 'opacity' ) + super().DynamicWriteableAdaptedProperties()

    @classmethod
    def DynamicReadOnlyAdaptedProperties(cls):
        return ('mask', ) + super().DynamicReadOnlyAdaptedProperties()

    @classmethod
    def DynamicTrueAdaptedTrueProperties(cls):
        return () + super().DynamicTrueAdaptedProperties()

    '''
    Notes on properties:
    name inherited from item
    mask is a layer mask, has remove_mask() but not set_mask() but some set_<foo>_mask() ???
    '''


    def __init__(self, img=None, name=None, width=None, height=None, type=None, opacity=None, layer_mode=None, adaptee=None):

        if img is not None:
            # Totally new adaptee, created at behest of author
            # Gimp constructor named "new"
            super().__init__( Gimp.Layer.new(img.unwrap(), name, width, height, type, opacity, layer_mode) )
        else:
            # Create wrapper for existing adaptee (from Gimp)
            # Adaptee was created earlier at behest of Gimp user and is being passed into GimpFu plugin
            assert adaptee is not None
            super().__init__(adaptee)

        print("new GimpfuLayer with adaptee", self._adaptee)




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


    def copy(self, alpha=False):
        ''' Return copy of self. '''
        '''
        FBC Param "alpha" is convenience, on top of Gimp.Layer.copy()

        !!! TODO alpha not used.  Code to add alpha if "alpha" param is true??
        The docs are not clear about what the param means.
        If it means "add alpha", should rename it should_add_alpha
        '''
        # delegate to adapter
        return super().copy()
        # If this class had any data members, we would need to copy their values also





    '''
    Properties.

    For convenience, GimpFu makes certain attributes have property semantics.
    I.E. get without parenthesises, and set by assignment, without calling setter() func.

    Properties that are canonically (with get_foo, and set_foo) implemented by Adaptee Gimp.Layer
    are handled aby Adaptor.

    TODO, does Gimp GI provide properties?
    '''


    """
    OLD
    @property
    def lock_alpha(self):
        return self._adaptee.get_lock_alpha()
    @lock_alpha.setter
    def lock_alpha(self, truth):
        return self._adaptee.set_lock_alpha(truth)
    """
