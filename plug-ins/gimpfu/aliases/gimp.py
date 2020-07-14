

import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp
from gi.repository import GObject    # marshalling


# import wrapper classes because we call their constructors
# in phrases like gimp.Layer
from adapters.image import GimpfuImage
from adapters.layer import GimpfuLayer
from adapters.display import GimpfuDisplay
from adapters.vectors import GimpfuVectors
# TODO channel, etc.

from adaption.marshal import Marshal
from adaption.wrappable import *
from adaption.compatibility import gimp_name_map

from message.proceed_error import *



class GimpfuGimp():
    '''
    Adaptor to Gimp
    GimpFu defines a symbol "gimp", an instance of this class.
    Its attributes appear to have similar names as the Gimp object (from GI).
    E.G. Gimp.Image vs gimp.Image

    Adaption kinds:
    Some methods are passed straight to Gimp.
    Some methods are passed to wrapper class in Python (constructor methods)
    Some methods are specialized (convenience)

    See gimpfu_pdb, its similar.

    Typical source constructs:
      img = gimp.Image(width, height, RGB)  invoke constructor of Gimp.Layer class, to be wrapped by GimpFu
      gimp.context_push()                   invoking method of Gimp class
      gimp.delete(img)                      specialized method
      gimp.locale_directory                 ???

    Note that Gimp.Layer refers to the Layer class in the Gimp namespace
    while Gimp.context_push invokes a method of the Gimp class.
    I.E. Gimp.Layer is NOT referring to an attribute of the Gimp class.

    Constructs you might imagine, but not seen yet in the wild:
       gimp.Layer.foo                       get attribute of the Gimp.Layer class?
       gimp.Layer.FOO                       get an enum value defined by Gimp.Layer class?
    '''




    """
    If experience shows this would be helpful,
    resurrect it.
    For now, just wrap all methods that have signaturge changes.

    def _adapt_signature(self, name, *args):
        # HACK: more generality for adapt signatures
        '''
        Some funcs have args that changed in Gimp.

        Returns a tuple
        '''
        print("_adapt_signature", name, args)
        if name is "set_background":
            # TODO create a color
            # TEMP return a constant color
            color = Gimp.RGB()
            color.parse_name("orange", 6)
            args = [color,]
            result = (color,)
        else:
            result = args
    """




    def _adaptor_func(self, *args):
        '''
        Call a callable attribute.
        Python just __getattr__'d the attribute <foo>
        and is now calling it with args <bar>.
        From a phrase like "gimp.<foo>(<bar>)"

        Becomes:
         - a call to a GimpFu adapter constructor GimpFu<foo>(<bar>)
         - OR a call to a Gimp method Gimp.<foo>(<bar>)
        '''
        print ("gimp adaptor called, args:", *args)

        # test

        '''
        # Futz with args that had property semantics in v2
        new_args = []
        for arg in args:
            print("arg type is:", type(arg))
            print(arg.invoke())
            if type(arg) is gi.FunctionInfo:
                print("arg.invoke")
                new_arg = arg.invoke()
            else:
                new_arg = arg
            new_args.append(new_arg)
        '''

        # create name string of method

        dot_name = object.__getattribute__(self, "adapted_gimp_object_name")

        # Is attribute a Gimp class name (a constructor) whose result should be wrapped?
        # E.G. Layer
        if is_gimpfu_wrappable_name(dot_name):
            '''
            The source phrase is like "layer=gimp.Layer(foo)"
            Construct a wrapper object of Gimp object
            e.g. GimpfuLayer, a GimpFu classname which is a constructor.
            Args to wrapper are in GimpFu notation.
            Constructor will in turn call Gimp.Layer constructor.
            '''
            callable_name = "Gimpfu" + dot_name
            print("Calling constructor: ", callable_name)
        else:
            '''
            The attribute is a Gimp function.
            The source phrase is like 'gimp.context_push(*args)'
            I.E. a method call on the (Gimp instance? or library?)
            Note that some method calls on the Gimp,
            e.g. gimp.delete(foo) are wrapped and intercepted earlier.
            '''
            callable_name = "Gimp." + gimp_name_map[dot_name]
            # TODO unmarshal_args

        try:
            # eval to get the callable
            func = eval(callable_name)
        except AttributeError:
            # callable_name is not adapted by GimpFu or known to Gimp
            # Probable  (i.e. user) error
            do_proceed_error(f"unknown Gimp method {callable_name}")
            result = None
        except Exception as err:
            # Probable bug in this code
            do_proceed_error(f"error getting {callable_name}:{err}")
            result = None
        else:
            # assert Callable is in Gimp, or in GimpFu<Foo>-then-Gimp.
            try:
                '''
                FUTURE adapt signature
                Now, any Gimp method that requires an adapted signature
                is adapted by a specialized method of this class (GimpfuGimp.)
                But certain adaptions in number could be done here?
                # new_args = self._adapt_signature(dot_name, args)
                # result = func(*new_args)
                '''
                # Call callable (could be in Gimp, or GimpFu)
                result = func(*args)
            except:
                '''
                TODO does Gimp return, or remember, an error message?
                TODO result is a GObject , maybe Gimp.StatusType as for PDB??
                TODO assert result is None or is_wrapper
                '''
                do_proceed_error(f"Error executing {callable_name}")
                result = None


        # ensure result is defined, even if None
        print(result)
        return result



    # Methods and properties offered dynamically.

    '''
    # TODO: fails for gimp.pdb.foo().
    Rare: see crop_flatten_save.py.
    Most authors just use 'pdb.foo()'
    I.E. can we make 'pdb' appear in the 'gimp' namespace,
    or just obsolete it?  See proceed_error below.
    '''

    def __getattr__(self, name):
        # !!! not def  __getattribute__(self, name):

        '''
        override of Python special method
        Adapts attribute access to become invocation on Gimp object.
        __getattr__ is only called for methods not found on self
        i.e. for methods not defined in this class (non-convenience methods)
        '''

        '''
        Require 'gimp' instance of Gimp object exists.  GimpFu creates it.
        Don't check, would only discover caller is not importing gimpfu.
        '''

        '''
        assert attribute name is e.g. "Image()" or "displays_flush.

        For any name that reaches here, GimpFuGimp does not have a distinct adapter method.

        We don't preflight, e.g. check that name is attribute of Gimp object.
        We can't just get attribute of Gimp object:
        "Gimp.__get_attribute__(name)" returns
        AttributeError: 'gi.repository.Gimp' object has no attribute '__get_attribute__'
        Maybe Gimp.__getattr__ works?
        Also, we need to handle constructors such as "Image()"

        We could tell the author: f"You can use Gimp.{name} instead of gimp.{name}")
        but they should already know that.
        '''

        # remember state for soon-to-come call
        self.adapted_gimp_object_name = name

        if name is "pdb":
            do_proceed_error("Use 'pdb', not 'gimp.pdb'.")
            # do more so that GimpFu really can proceed without more exceptions

        print("return gimp adaptor func")
        # return adapter function soon to be called
        return object.__getattribute__(self, "_adaptor_func")


    # specialized, convenience

    '''
    These are methods from v2 that we don't intend to deprecate
    since they have actions on the Python side
    that some plugins may require.
    '''

    def delete(self, instance):
        '''
        From PyGimp Reference:
        It deletes the gimp thing associated with the Python object given as a parameter.
        If the object is not an image, layer, channel, drawable or display gimp.delete does nothing.
        '''
        #TODO later
        # For now, assume most programs don't really need to delete,
        # since they are about to exit anyway
        '''
        Maybe:
        delete instance if it is a wrapped class.
        Also deleted the adaptee.
        Gimp.Image.delete() et al says it will not delete
        if is on display (has a DISPLAY) i.e. if Gimp user created as opposed to just the plugin.
        '''
        print("TODO gimp.delete() called")


    def set_background(self, r, g=None, b=None):
        '''
        Changed:
           name
           signature was adapted in PyGimp v2, also here v3
        '''
        # method is overloaded, with optional args
        if g is None:
            # r is-a Gimp.Color already
            color = r
        else:
            color = Gimp.RGB()
            color.set(float(r), float(g), float(b), )

        # call Gimp instead of PDB
        Gimp.context_set_background(color)


    # Changed in Gimp commit 233ac80d
    def gimp_edit_blend(self, mask, blend_mode, layer_mode,
           gradient1, gradient2, gradient3, gradient4,
           truthity1, truthity2,
           int1, int2,
           truthity3,
           x1, y1, x2, y2):
        '''
        One to many.
        !!! We are calling instance of self.
        '''
        gimp.gimp_context_set_gradient_fg_bg_rgb()   # was blend_mode
        # TODO should dispatch blend_mode to different context calls?

        # Note discarded some args
        gimp.gimp_drawable_edit_gradient_fill( mask,
           gradient1, gradient3,
           truthity2, int1, int2, truthity3,
           x1, y1, x2, y2)

    # Name change in Gimp v3?
    def image_list(self):
       list = Gimp.list_images()
       # assert typeof(list) is Python list
       result = Marshal.wrap_args(list)
       return result
