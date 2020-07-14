
import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

from adaption.adapter import Adapter



#TODO name just Color?

class GimpfuColor(Adapter):
    '''
    Color is mostly for parameters to PDB.

    Author CAN instantiate.
    But has no methods or properties.
    And usually easier to use 3-tuples and strings, GimpFu automatically converts
    on passing to PDB.

    That is, typically Author does not need a Color except as an arg to a PDB procedure.
    '''


    def __init__(self, r=None, g=None, b=None, name=None, a_tuple=None, adaptee=None):
        '''
        !!! In call to constructor, you must use a keyword
        assert (
            (r is not None and g is not None and b is not None) # and are numbers
            or (name is not None)       # and is str from a certain subset
            or (a_tuple is not None)    # and is a 3-tuple or sequence of number
            or (adaptee is not None)    # and is-a RGB
            )
        '''

        try:
            if r is not None:
                # Totally new adaptee, created at behest of author or GimpFu implementation
                # Gimp constructor NOT named "new"
                a_adaptee = Gimp.RGB()
                a_adaptee.set(float(r), float(g), float(b) )
            elif name is not None:
                a_adaptee = Gimp.RGB()
                a_adaptee.parse_name(name, -1)  # -1 means null terminated
            elif a_tuple is not None:
                a_adaptee = Gimp.RGB()
                a_adaptee.set(float(a_tuple[0]), float(a_tuple[1]), float(a_tuple[2]) )
            elif adaptee is not None:
                # Create wrapper for existing adaptee (from Gimp)
                # Adaptee was created earlier at behest of Gimp user and is being passed into GimpFu plugin
                a_adaptee = adaptee
            else:
                raise RuntimeError("Illegal call to GimpFuColor constructor")
        except Exception as err:
            do_proceed_error(f"Creating GimpfuColor: {err}")
            
        # Adapter
        super().__init__(a_adaptee)

        print("new GimpfuColor with adaptee", self._adaptee)

    # TODO repr
