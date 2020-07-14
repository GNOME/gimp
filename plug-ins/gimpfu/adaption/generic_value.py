
import gi
from gi.repository import GObject

gi.require_version("Gimp", "3.0")
from gi.repository import Gimp


from message.proceed_error import *




class FuGenericValue():
    '''
    A (type, value) that holds any ('generic') value.
    Similar to GValue.

    Used for adapting Python types to GTypes.

    Stateful:
    Operations in this sequence:  init, apply conversions and upcasts, get_gvalue

    In type is a GType or Python type.
    Result type is also GType or Python type.
    Only get_gvalue definitely returns a GType.

    Responsible for:
    - performing conversions and Upcasts
    - produce a Gvalue

    Is not a singleton, but could be, only one is ever in use.
    '''


    def __init__(self, actual_arg, actual_arg_type):
        self._actual_arg = actual_arg
        self._actual_arg_type = actual_arg_type

        # result is None until converted or upcast
        self._result_arg = None
        self._result_arg_type = None
        self._gvalue = None

        self._did_convert = False
        self._did_upcast = False
        self._did_create_gvalue = False


    def __repr__(self):
        return ','.join((str(self.actual_arg), str(self._actual_arg_type),
                         str(self._result_arg), str(self._result_arg_type),
                         str(self._did_convert), str(self._did_upcast) ))


    def get_gvalue(self):
        ''' Return gvalue for original arg with all conversions and upcasts applied.  '''
        if self._did_create_gvalue:
            result_gvalue = self._gvalue
        elif self._did_convert or self._did_upcast:
            result_gvalue =  FuGenericValue.new_gvalue(self._result_arg_type, self._result_arg)
        else:
            # Idempotent, the original type and arg
            result_gvalue =  FuGenericValue.new_gvalue(self._actual_arg_type, self._actual_arg)
        return result_gvalue

    '''
    properties

    Not all fields are exposed to public.
    Only some are queried by callers.
    None are settable by callers.
    '''

    @property
    def actual_arg_type(self):
        return self._actual_arg_type

    @property
    def actual_arg(self):
        return self._actual_arg

    @property
    def did_convert(self):
        return self._did_convert

    @property
    def did_upcast(self):
        return self._did_upcast


    '''
    Conversions

    Some conversions (float array) require the production of a GValue right away.
    That might change.
    '''
    def convert(self, type_converter):
        ''' Convert result_type using given conversion method. '''
        print(f"FuGenericValue.convert to {type_converter}")
        self._result_arg = type_converter(self._actual_arg)  # type conversion
        self._result_arg_type = type(self._result_arg)
        self._did_convert = True


    def float(self):
        self.convert(float) # pass built-in type converter method


    def str(self):
        self.convert(str) # pass built-in type converter method


    def float_array(self):
        ''' Make self own a GValue holding a GimpFloatArray created from native list'''
        '''
        For now, use Gimp.value_set_float_array,
        which might be the only way to do it.
        But possibly there is a simpler implementation.
        '''
        # require self._actual_arg is-a sequence type

        print(">>>>>>>>>>Converting float list")

        # TODO not sure we need to float(), maybe PyGObject will do it
        #float_list = [float(item) for item in actual_arg]
        float_list = self._actual_arg

        try:
            self._gvalue = GObject.Value (Gimp.FloatArray.__gtype__)

            # PyGObject will convert using sequence and len(sequence)
            # Note that the previous arg (length) is still also passed to the PDB procedure
            Gimp.value_set_float_array(self._gvalue, float_list)
            self._did_create_gvalue = True
            self._did_convert = True

            # OLD result_gvalue = FuFloatArray.new_gimp_float_array_gvalue(float_list)
        except Exception as err:
            do_proceed_error(f"Failed to create Gimp.FloatArray: {err}.")

        # Cruft?
        #>>>>GimpFu continued past error: Exception in type conversion of: [1536, 0, 1536, 1984], type: <class 'list'>, index: 2
        #>>>>GimpFu continued past error: Creating GValue for type: <class 'list'>, value: [1536, 0, 1536, 1984], err: Must be GObject.GType, not type

        # This is not right, this is a type, not a GObject.type
        # result_arg_type = Gimp.GimpParamFloatArray




    def color(self):
        ''' Convert result_arg to instance of type Gimp.RGB '''
        assert self._did_upcast

        # TODO move this to color.py i.e. GimpfuColor.convert()
        if isinstance(self._actual_arg, tuple):
            wrapped_result = GimpfuColor(a_tuple=self._actual_arg)
        elif isinstance(self._actual_arg, str):
            wrapped_result = GimpfuColor(name=self._actual_arg)
        else:
            raise Exception("Not wrappable to color: {self._actual_arg}.")
        # !!! caller expects GObject i.e. unwrapped
        self._result_arg = wrapped_result.unwrap()
        # assert result_type is-a GType
        self._did_convert = True


    '''
    upcasts
    '''

    def upcast(self, cast_to_type):
        '''
        Change result (type and value) to cast_to_type.
        The value is not converted.
        '''
        self._result_arg_type = cast_to_type
        self._result_arg = self._actual_arg
        self._did_upcast = True


    def upcast_to_None(self, cast_to_type):
        '''
        Cast to cast_to_type and change result to None (from e.g. -1).
        I.E. result is Null instance of the type.
        None is an instance of every type in some type systems.
        '''
        self.upcast(cast_to_type)
        self._result_arg = None
        assert self._did_upcast




    '''
    Utility.
    Here for lack of a better place for it.
    Not quite private.
    '''

    '''
    !!! Can't assign GValue to python object: foo = GObject.Value(Gimp.Image, x) ???
    Must pass directly to Gimp.ValueArray.insert() ???

    ??? I don't understand why GObject.Value() doesn't determine the type of its second argument
    I suppose GObject.Value() can't know all the types, is generic.
    '''
    @staticmethod
    def new_gvalue(gvalue_type, value):
        ''' Returns GValue'''
        # assert gvalue_type is a GObject type constant like GObject.TYPE_STRING
        '''
        An exception is usually not caused by author, usually GimpFu programming error.
        Usually "Must be a GObject.GType, not a type"
        '''
        try:
            result = GObject.Value(gvalue_type, value)
        except Exception as err:
            do_proceed_error(f"Creating GValue for type: {gvalue_type}, value: {value}, err: {err}")
            # Return some bogus value so can proceed
            result = GObject.Value( GObject.TYPE_INT, 1 )
            # TODO would this work??
            # result = GObject.Value( GObject.TYPE_NONE, None )
        return result
