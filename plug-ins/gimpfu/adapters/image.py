

import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp


# absolute from SYSPATH that points to top of gimpfu package
from adaption.adapter import Adapter


'''

Adapts (wraps) Gimp.Image.

Constructor appears in GimpFu plugin code as e.g. : gimp.Image(w, h, RGB)
I.E. an attribute of gimp instance (see aliases/gimp.py.)

In v2, similar concept implemented by pygimp-image.c
Since v3, implemented in Python using GI.

FBC all specialized methods here should match the implementation in pygimp-image.c v2.

Method kinds:
Most have an identical signature method in Gimp.Image. (delegated)
Some have changed signature here, but same action as in Gimp.Image. (convenience)
Some are unique to PyGimp, not present in Gimp.Image. (augment)
    - methods
    - properties (data members)
    TODO do we need to wrap property get/set
'''






class GimpfuImage( Adapter ) :


    '''
    classmethods needed by Adapter

    Notes:
    filename is NOT canonical property of Image, in Gimp v3.  See get_name()
    '''
    @classmethod
    def DynamicWriteableAdaptedProperties(cls):
        return ( 'active_layer', )

    # Name of getter() func is property name prefixed with 'get_'
    @classmethod
    def DynamicReadOnlyAdaptedProperties(cls):
        return ('selection',  'layers', )

    # True: name of getter() func is same as name of property
    @classmethod
    def DynamicTrueAdaptedProperties(cls):
        return ('width', 'height', 'base_type')




    '''
    Constructor exported to s.
    Called internally for existing images as GimpfuImage(None, None, None, adaptee)

    See SO "How to overload __init__ method based on argument type?"
    '''
    def __init__(self, width=None, height=None, image_mode=None, adaptee=None):
        '''Initialize  GimpfuImage from attribute values OR instance of Gimp.Image. '''
        if width is None:
            final_adaptee = adaptee
        else:
            # Gimp constructor named "new"
            print("Calling Gimp.Image.new with width", width)
            final_adaptee = Gimp.Image.new(width, height, image_mode)

        # super is Adaper, and it stores adaptee
        super().__init__(final_adaptee)



    # TODO Not needed ??
    def adaptee(self):
        ''' Getter for private _adaptee '''
        # Handled by super Adaptor
        result = self._adaptee
        print("adaptee getter returns:", result)
        return result




    '''
    WIP
    Overload constructors using class methods.
    # Hidden constructor
    @classmethod
    def fromAdaptee(cls, adaptee):
         "Initialize GimpfuImage from attribute values"

         return cls(data
    '''


    '''
    Specialized methods and properties.
    Reason we must specialize is the comment ahead of each property
    '''


    # Methods


    # Reason: allow optional args
    def insert_layer(self, layer, parent=None, position=-1):
        print("insert_layer called")

        # Note that first arg "image" to Gimp comes from self
        success = self._adaptee.insert_layer(layer.unwrap(), parent, position)
        if not success:
            raise Exception("Failed insert_layer")

    # Reason: allow optional args, rename
    def add_layer(self, layer, parent=None, position=-1):
        print("add_layer called")
        success = self._adaptee.insert_layer(layer.unwrap(), parent, position)
        if not success:
            raise Exception("Failed add_layer")



    '''
    Properties
    '''

    """
    OLD Cruft
    # Reason: marshal to wrap result
    @property
    def layers(self):
        # avoid circular import, import when needed
        from adaption.marshal import Marshal

        '''
        Override:
        - to insure returned objects are wrapped ???
        - because the Gimp name is get_layers

        Typical use by authors: position=img.layers.index(drawable) + 1
        And then the result goes out of scope quickly.
        But note that the wrapped item in the list
        won't be equal to other wrappers of the same Gimp.Layer
        UNLESS we also override equality operator for wrapper.

        !!! If you break this, the error is "AttributeError: Image has no attr layers"
        '''
        print("layers property accessed")
        layer_list = self._adaptee.get_layers()
        result_list = []
        for layer in layer_list:
            # rebind item in list
            result_list.append(Marshal.wrap(layer))
        print("layers property returns ", result_list)
        return result_list
    """

    # TODO can be a property now
    @property
    def vectors(self):
        # avoid circular import, import when needed
        from adaption.marshal import Marshal

        unwrapped_list = self._adaptee.get_vectors()
        result_list = Marshal.wrap_args(unwrapped_list)
        return result_list


    # TODO all these properties are rote changes to name i.e. prefix with get_
    # Do this at runtime, or code generate?

    """
    CRUFT moved to DynamicReadOnlyAdaptedProperties
    @property
    def active_layer(self):
        # Delegate to Gimp.Image
        # TODO wrap result or lazy wrap
        return self._adaptee.get_active_layer()
    @active_layer.setter
    def active_layer(self, layer):
        # TODO:
        raise RuntimeError("not implemented")


    @property
    def base_type(self):
        # Delegate to Gimp.Image
        # Result is fundamental type (enum int)
        return self._adaptee.base_type()
    """


    """
    !!! This is not correct, since get_file() can return None.
    file = self._adaptee.get_file()
    # assert file is-a Gio.File
    result = file.get_path()
    """
    @property
    def filename(self):
        '''
        Returns string that is path to file the image was saved to.
        Returns "Untitled" if image not loaded from file, or not saved.
        '''
        # print("GimpfuImage.filename get called")
        # sic Image.get_name returns a filepath
        result = self._adaptee.get_name()
        return result
