import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

from adaption.adapter import Adapter

from message.proceed_error import *



class GimpfuDisplay( Adapter ) :

    # TODO properties?

    def __init__(self, img=None, name=None, width=None, height=None, type=None, opacity=None, layer_mode=None, adaptee=None):

        if img is not None:
            # Totally new adaptee, created at behest of author
            # Gimp constructor named "new"
            super().__init__( Gimp.Display.new(img.unwrap()))
        else:
            # Create wrapper for existing adaptee (from Gimp)
            # Adaptee was created earlier at behest of Gimp user and is being passed into GimpFu plugin
            # Rarely does Gimp pass a Display to a plugin?
            # But not inconceivable that one plugin passes a Display to another.
            assert adaptee is not None
            super().__init__(adaptee)

        print("new GimpfuDisplay with adaptee", self._adaptee)




    '''
    Methods for convenience.
    '''

    '''
    Override superclass Adapter.copy()
    Displays should not be copied.
    '''
    def copy(self):
        do_proceed_error("Cannot copy Display")
        return None

    '''
    Properties.
    '''

    # GimpFu provides no properties for Display
