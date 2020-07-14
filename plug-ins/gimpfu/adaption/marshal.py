
import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

from adaption.wrappable import *
from adaption.types import Types

# import wrapper classes
# These inherit Adapter which wants to use Marshal, avoid circular import
# in Adapter by selectively import Marshal
from adapters.image import GimpfuImage
from adapters.layer import GimpfuLayer
from adapters.vectors import GimpfuVectors

from message.proceed_error import *




class Marshal():
    '''
    Knows how to wrap and unwrap things.
    Abstractly: AdaptedAdaptee <=> adaptee
    Specifically: Gimp GObjects <=> Gimpfu<foo>

    Fundamental types pass through wrapping and unwrapping without change.

    Hides multiple constructors for wrappers.

    Each wrapped object (instance of AdaptedAdaptee) has an unwrap() method to return its adaptee.
    Wrapper class  AdaptedAdaptee also knows how to wrap() a new'd Gimp GObject

    But this provides 'convenience' methods for some cases, like unwrapping args.
    In that case, this hides the need to:
      - upcast (GObject frequently requires explicit upcast)
      - convert (Python will convert int to float), but GimpFu must do that for args to Gimp
      - check for certain errors (wrapping a FunctionInfo)

    For calls to Gimp, these methods suffice,
    since Gimp is a library linked to the Python interpreter and takes GObject's.

    For call to PDB, see marshal_pdb.py.
    '''



    '''
    Gimp breaks out image and drawable from other args.
    Reverse that.
    And convert type Gimp.ValueArray to type "list of GValue"
    '''
    def prefix_image_drawable_to_run_args(actual_args, image=None, drawable=None):
        '''
        return a list [image, drawable, *actual_args]
        Where:
            actual_args is-a Gimp.ValueArray
            image, drawable are optional GObjects
        !!! Returns Python list, not a Gimp.ValueArray
        '''

        args_list = Types.convert_gimpvaluearray_to_list_of_gvalue(actual_args)

        # Prepend to Python list
        if drawable:
            args_list.insert(0, drawable)
        if image:
            args_list.insert(0,image)

        # ensure result is-a list, but might be empty
        return args_list





    '''
    wrap and unwrap

    These are generic: not special to an wrapper type.

    See wrapper class, i.e. AdaptedAdaptee e.g. GimpfuLayer.
    Wrapper class also have a unwrap() method specialized to the class.
    Wrapper class also has a constructor that effectively wraps.
    '''



    @staticmethod
    def wrap(gimp_instance):
        '''
        Wrap an instance from Gimp.
        E.G. Image => GimpfuImage

        Requires gimp_instance is-a Gimp object that should be wrapped,
        else exception.
        '''
        '''
        Invoke the internal constructor for wrapper class.
        I.E. the adaptee already exists,
        the Nones mean we don't know attributes of adaptee,
        but the adaptee has and knows its attributes.
        '''
        print("Wrap ", gimp_instance)

        if is_gimpfu_wrappable(gimp_instance):
            gimp_type_name = get_type_name(gimp_instance)
            statement = 'Gimpfu' + gimp_type_name + '(adaptee=gimp_instance)'
            # e.g. statement  'GimpfuImage(adaptee=gimp_instance)'
            result = eval(statement)
        else:
            exception_str = f"GimpFu: can't wrap gimp type {gimp_type_name}"
            do_proceed_error(exception_str)
        return result

        """
        OLD
        # Dispatch on gimp type
        # This is a switch statement, default is an error
        gimp_type = type(gimp_instance).__name__    # idiom for class name
        if  gimp_type == 'Image':
            result = GimpfuImage(adaptee=gimp_instance)
        elif gimp_type == 'Layer':
            result = GimpfuLayer(adaptee=gimp_instance)
        elif gimp_type == 'Display':
            result = GimpfuDisplay(adaptee=gimp_instance)
        elif gimp_type == 'Vectors':
            result = GimpfuVectors(adaptee=gimp_instance)
        #elif gimp_type == 'Channel':
        # result = GimpfuLayer(None, None, None, None, None, None, None, gimp_instance)
        else:
            exception_str = f"GimpFu: can't wrap gimp type {gimp_type}"
            do_proceed_error(exception_str)
        """


    @staticmethod
    def unwrap(arg):
        '''
        Unwrap a single instance. Returns adaptee or a fundamental type.

        Knows which are wrapped types versus fundamental types,
        and unwraps only wrapped types.

        Unwrap any GimpFu wrapped types to Gimp types
        E.G. GimpfuImage => Gimp.Image
        For primitive Python types and GTypes, idempotent, returns given arg unaltered.

        Only fundamental Python types and Gimp types (not GimpfuFoo wrapper TYPE_STRING)
        can be passed to Gimp.
        '''
        # Unwrap wrapped types.
        if  is_gimpfu_unwrappable(arg) :
            # !!! Do not affect the original object by assigning to arg
            # !!! arg knows how to unwrap itself
            result_arg = arg.unwrap()
        else:
            # arg is already Gimp type, or a fundamental type
            result_arg = arg
        print("unwrap result:", result_arg)
        return result_arg


    '''
    TODO Do we need this?
    Knows how to unwrap a variable which is a container (list) or single instance
    If arg is a list, result is a list.
    '''




    '''
    '''
    @staticmethod
    def _try_wrap(instance):
        ''' Wrap a single instance if it is wrappable, else the arg. '''
        if is_gimpfu_wrappable(instance):
            result = Marshal.wrap(instance)
        else:   # fundamental
            result = instance
        return result


    @staticmethod
    def wrap_adaptee_results(args):
        '''
        Wrap result of calls to adaptee.

        args can be an iterable container e.g. a tuple or list or a non-iterable fundamental type.
        Result is iterable list if args is iterable, a non-iterable if args is not iterable.
        Except: strings are iterable but also fundamental.
        '''
        try:
            unused_iterator = iter(args)
        except TypeError:
            # not iterable, but not necessarily fundamental
            result = Marshal._try_wrap(args)
        else:
            # iterable, but could be a string
            if isinstance(args, str):
                # No need to unwrap
                result = args
            else:
                # is container, return container of unwrapped
                result = [Marshal._try_wrap(item) for item in args]
        return result





    '''
    wrap and unwrap list of things
    !!! note the 's'
    Generally the list is incoming or outgoing arguments.
    But distinct from arguments to the pdb, which are tuples.
    '''

    @staticmethod
    def wrap_args(args):
        '''
        args is a sequence of unwrapped objects (GObjects from Gimp) and fundamental types.
        Wraps instances that are not fundamental.
        Returns list.

        Typically args are:
        - from Gimp calling back the plugin.
        - from the plugin calling Gimp
        '''
        print("wrap_args", args)
        result = []
        for instance in args:
            if is_gimpfu_wrappable(instance):
                result.append(Marshal.wrap(instance))
            else:   # fundamental
                result.append(instance)
        return result


    @staticmethod
    def unwrap_args(args):
        '''
        args is a sequence of possibly wrapped objects (class GimpfuFoo)
        or fundamental types.
        Unwrap instances that are wrapped.
        Returns list.
        '''
        print("unwrap_args", args)
        result = []
        for instance in args:
            if is_gimpfu_unwrappable(instance):
                result.append(Marshal.unwrap(instance))
            else:   # fundamental
                result.append(instance)
        return result






    '''
    Gimp  marshal and unmarshal.
    For Gimp. , Gimp.Layer. , etc. method calls

    This will be very similar (extracted from?) PDB marsha

    marshal_gimp_args, unmarshal_gimp_results
    '''
