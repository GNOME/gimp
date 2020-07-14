
import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp


from adaption.wrappable import *
from adaption.marshal import Marshal
from adaption.types import Types
from adaption.value_array import FuValueArray
from adaption.generic_value import FuGenericValue

from message.proceed_error import *



class MarshalPDB():
    '''
    Knows how to marshal args to, and unmarshal results from, PDB.

    Crux: PDB wants GParamSpec's, not just GObjects.
    I.E. more specialized than ordinary Marshal

    Many methods operate on a stateful FuGenericValue, and don't return a result.
    '''


    @staticmethod
    def _try_type_conversions(proc_name, gen_value, index):
        '''
        Attempt type conversions and upcast when passing to Gimp procedures.
        Conversion: changes Python type of fundamental types, i.e. int to float
        Upcast: not change any type, just return superclass type that Gimp wants.

        Don't assume any arg does NOT need conversion:
        a procedure can declare any arg of type float
        '''
        # hack: upcast  subclass e.g. layer to superclass drawable
        # hack that might be removed if Gimp was not wrongly stringent

        # TODO optimize, getting type is simpler when is fundamental
        # We could retain that the arg is fundamental during unwrapping

        '''
        Any upcast or conversion is the sole upcast or conversion.
        (But un upcast may also internally convert)
        We don't upcast and also convert in this list.

        The order is not important as we only expect one upcast or convert.
        '''
        Types.try_upcast_to_drawable(proc_name, gen_value, index)
        if not gen_value.did_upcast:
            Types.try_upcast_to_item(proc_name, gen_value, index)
        if not gen_value.did_upcast:
            Types.try_upcast_to_layer(proc_name, gen_value, index)
        if not gen_value.did_upcast:
            Types.try_upcast_to_color(proc_name, gen_value, index)
        if not gen_value.did_upcast:

            # Continue trying conversions
            Types.try_usual_python_conversion(proc_name, gen_value, index)
        if not gen_value.did_convert:
            Types.try_float_array_conversion(proc_name, gen_value, index)

        # !!! We don't upcast deprecated constant TRUE to G_TYPE_BOOLEAN

        # TODO is this necessary? I think it is only drawable that gets passed None
        # Types.try_convert_to_null(proc_name, gen_value, index)




    @staticmethod
    def marshal_args(proc_name, *args):
        '''
        Marshal args to a PDB procedure.

        1. Gather many args into Gimp.ValueArray and return it.
        2. Optionally prefix args with run mode
        GimpFu feature: hide run_mode from calling author
        3. Unwrap wrapped arguments so all args are GObjects
        4. Upcasts and conversions
        5. Check error FunctionInfo
        '''

        FuValueArray.reset()
        print(f">>>>>> marshalled args: {FuValueArray.dump()}" )

        formal_args_index = 0

        # TODO extract method to GimpfuProcedure class
        # TODO python-fu- ?? What procedure names signify need run_mode?
        # Better to introspect ??
        if proc_name.startswith('plug-in-'):
            # no GUI, this is a call from a plugin

            a_gvalue = FuGenericValue.new_gvalue( Gimp.RunMode.__gtype__, Gimp.RunMode.NONINTERACTIVE)
            FuValueArray.push_gvalue( a_gvalue )
            # Run mode is in the formal args, not in passed actual args
            formal_args_index = 1

        '''
        If more args than formal_args (from GI introspection), conversion will not convert.
        If less args than formal_args, Gimp might return an error when we call the PDB procedure.
        '''
        for x in args:
            print(f"\n#### marshal arg: {x} index: {formal_args_index}" )
            print(f">>>>>> marshalled args: {FuValueArray.dump()}" )

            go_arg, go_arg_type = MarshalPDB._unwrap_to_param(x)
            # assert are GObject types, i.e. fundamental or Boxed
            # We may yet convert some fundamental types (tuples) to Boxed (Gimp.RGB)

            gen_value = FuGenericValue(go_arg, go_arg_type)

            """
            """
            try:
                MarshalPDB._try_type_conversions(proc_name, gen_value, formal_args_index)
            except Exception as err:
                do_proceed_error(f"Exception in type conversion of: {gen_value}, formal_args_index: {formal_args_index}, {err}")


            if is_wrapped_function(go_arg):
                do_proceed_error("Passing function as argument to PDB.")

            a_gvalue = gen_value.get_gvalue()
            FuValueArray.push_gvalue(a_gvalue)

            formal_args_index += 1

            """
            cruft
            try:
                marshalled_args.insert(index, gvalue)
            except Exception as err:
                do_proceed_error(f"Exception inserting {gvalue}, index {index}, to pdb, {err}")
                # ??? After this exception, often get: LibGimpBase-CRITICAL **: ...
                # gimp_value_array_insert: assertion 'index <= value_array->n_values' failed
                # The PDB procedure call is usually going to fail anyway.
            """

        result = FuValueArray.get_gvalue_array()
        # print("marshalled args", result )
        print(f">>>>>> marshalled args: {FuValueArray.dump()}" )
        return result


    @staticmethod
    def unmarshal_results(values):
        ''' Convert result of a pdb call to Python objects '''

        '''
        values is-a GimpValueArray
        First element is a PDBStatusType, discard it.
        If count remaining elements > 1, return a list,
        else return the one remaining element.

        For all returned objects, wrap as necessary.
        '''
        print("PDB status result is:", values.index(0))

        # caller should have previously checked that values is not a Gimp.PDBStatusType.FAIL
        if values:
            # Remember, values is-a Gimp.ValueArray, not has Pythonic methods
            # TODO should be a method of FuValueArray
            result_list = Types.convert_gimpvaluearray_to_list_of_gvalue(values)

            # discard status by slicing
            result_list = result_list[1:]

            # Recursively convert type of elements to Python types
            result_list = Types.convert_list_elements_to_python_types(result_list)

            result_list = Marshal.wrap_adaptee_results(result_list)

            # unpack list of one element
            # TODO is this always the correct thing to do?
            # Some procedure signatures may return a list of one?
            if len(result_list) is 1:
                print("Unpacking single element list.")
                result = result_list[0]
            else:
                result = result_list
        else:
            result = None

        # ensure result is None or result is list or result is one object
        # TODO ensure wrapped?
        print("unmarshal_pdb_result", result)
        return result



    @staticmethod
    def _unwrap_to_param(arg):
        '''
        Returns a tuple: (unwrapped arg, type of unwrapped arg)
        For use as arg to PDB, which requires ParamSpec tuple.

        Only fundamental Python types and GTypes can be GObject.value()ed,
        which is what Gimp does with ParamSpec
        '''

        # TODO, possible optimization if arg is already unwrapped, or lazy?
        result_arg = Marshal.unwrap(arg)
        result_arg_type = type(result_arg)

        print("_unwrap_to_param", result_arg, result_arg_type)
        return result_arg, result_arg_type
