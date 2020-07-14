

'''
Hack, may go away when Gimp is fixed re add_argument_from_property
'''


import gi
from gi.repository import GObject

gi.require_version("Gimp", "3.0")
from gi.repository import Gimp   # for Gimp enum RunMode


"""
Don't touch this unless you want to deal with mysterious GObject failures.
Any mistakes give error messages that are not helpful.
At best, test after every small change.
Much of this is 'magic' that I don't understand why/how it works.
"""

'''
A class whose *instances* will have a "props" attribute.
E.G. prop_holder.props is-a dictionary
E.G. prop_holder.props.IntProp is-a GProperty

Class has one property of each type.
Properties used only for their type.
In calls to set_args_from_property
'''
class PropHolder (GObject.GObject):



    """
    ??? This breaks the comment strings.

    @GObject.Property(type=Gimp.RunMode,
                      default=Gimp.RunMode.NONINTERACTIVE,
                      nick="Run mode", blurb="The run mode")
    """

    # Seems to be required by PyGobject or GObject
    __gtype_name__ = "PropHolder"

    '''
    class attribute, but we still need an instance to pass to procedure.add_argument_from_property

    These properties are constants, no one sets them? Maybe Gimp does?
    We only use them to convey plugin arg spec declarations to Gimp
    '''
    # copied and hacked to remove '-' from Python GTK+ 3 website tutorial
    __gproperties__ = {
        "IntProp": (int, # type
                     "integer prop", # nick
                     "A property that contains an integer", # blurb
                     1, # min
                     5, # max
                     2, # default
                     GObject.ParamFlags.READWRITE # flags
                    ),
        "RunmodeProp": (Gimp.RunMode,
                 "Run mode",
                 "Run mode",
                 Gimp.RunMode.NONINTERACTIVE,  # default
                 GObject.ParamFlags.READWRITE),
        }


    def __init__(self):
        GObject.GObject.__init__(self)

        # give arbitrary value to the property of type int
        self.int_prop = 2

        # TODO give arbitrary value to the property of type Gimp.RunMode
        self.runmode_prop = Gimp.RunMode.NONINTERACTIVE



    """
    do_get_property etc. is magic part of GObject?
    Our code does not access it directly,
    but prop_holder.props.foo invokes it.
    """

    def do_get_property(self, prop):
        if prop.name == 'IntProp':
            return self.int_prop
        elif prop.name == 'RunmodeProp':
            raise AttributeError("Access RunmodeProp")
        else:
            raise AttributeError(f"unknown property {prop.name}")

    def do_set_property(self, prop, value):
        if prop.name == 'IntProp':
            self.int_prop = value
        else:
            raise AttributeError('unknown property %s' % prop.name)
