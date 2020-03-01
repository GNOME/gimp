

import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp
from gi.repository import GObject    # marshalling

from adaption.marshal_pdb import MarshalPDB
from adaption.compatibility import pdb_name_map

from message.proceed_error import *



class GimpfuPDB():
    '''
    Adaptor to Gimp.PDB
    GimpFu defines a symbol "pdb", an instance of this class.
    Its attributes appear to have similar names as procedures in the Gimp.PDB.

    GimpFu v2 also allowed access using index notation.
    Obsolete? not supported here yet.

    GimpFu creates one instance named "pdb"
    TODO enforce singleton, but author doesn't even imagine this class exists.

    FBC
    Problems it solves:

    1) We can't just "pdb = Gimp.get_pdb()" at gimpfu import time
    because Gimp.main() hasn't been called yet
    (which would create the PDB instance and establish self type is GimpPlugin).
    Such code would bind "pdb" to None, forever, since that is the way import works.
    Besides, the object returned by Gimp.get_pdb() is the PDB manager,
    not having a method for each procedure IN the PDB???

    Alternatively, we could require a plugin's "func" to always first call pdb = Gimp.get_pdb().
    But that is new boilerplate for s, and not backward compatible.

    2) PyGimp v2 made an object for pdb some other way???? TODO
    '''

    '''
    Implementation:

    See:
    1)"Adaptor pattern"
    2) Override "special method" __get_attribute__.
    3) "intercept method calls"
    4) "marshalling" args.

    An author's access (on right hand side) of an attribute of this class (instance "pdb")
    is adapted to run a procedure in the PDB.
    Requires all accesses to attributes of pdb are function calls (as opposed to get data member.)
    Each valid get_attribute() returns an interceptor func.
    The interceptor func marshalls arguments for a call to PDB.run_procedure()
    and returns result of run_procedure()

    ???Some get_attributes ARE attributes on Gimp.PDB e.g. run_procedure().
    We let those pass through, with mangling of procedure name and args?

    Notes:

    Since we override __getattribute__, use idiom to avoid infinite recursion:
    call super to access self attributes without recursively calling overridden __getattribute__.
    super is "object".  Alternatively: super().__getattribute__()

    TODO should use __getattr__ instead of __getattribute__ ??

    Errors:

    Exception if cannot get a reference to PDB
    (when this is called before Gimp.main() has been called.)

    Exception if attribute name not a procedure of PDB

    No set attribute should be allowed, i.e. author use pdb on left hand side.
    The PDB (known by alias "pdb") can only be called, it has no data members.
    But we don't warn, and the effect would be to add attributes to an instance of this class.
    Or should we allow set attribute to mean "store a procedure"?
    '''



    def _nothing_adaptor_func(self, *args):
        ''' Do nothing when an unknown PDB procedure is called. '''
        print("_nothing_adaptor_func called")
        return None


    def _adaptor_func(self, *args):
        ''' run a PDB procedure whose name was used like "pdb.name()" e.g. like a method call of pdb object '''
        print ("pdb adaptor called, args", args)
        # !!! Must unpack args before passing to _marshall_args
        # !!! avoid infinite recursion
        proc_name = object.__getattribute__(self, "adapted_proc_name")

        try:
            marshaled_args = MarshalPDB.marshal_args(proc_name, *args)
        except Exception as err: # TODO catch only MarshalError ???
            do_proceed_error(f"marshalling args to pdb.{proc_name} {err}")
            marshaled_args = None

        if marshaled_args is not None:
            # require marshaled_args is not None, but it could be an empty GimpValueArray

            # Not a try: except?
            inner_result = Gimp.get_pdb().run_procedure( proc_name , marshaled_args)

            # pdb is stateful for errors, i.e. gets error from last invoke, and resets on next invoke
            error_str = Gimp.get_pdb().get_last_error()
            if error_str != 'success':   # ??? GIMP_PDB_SUCCESS
                do_proceed_error(f"Gimp PDB execution error:, {error_str}")
                result = None
            else:
                result = MarshalPDB.unmarshal_results(inner_result)
        else:
            result = None

        # This is the simplified view of what we just did, without all the error checks
        # object.__getattribute__(self, "_marshall_args")(proc_name, *args)

        # Most PDB calls have side_effects on image, but few return values?
        # ensure result is defined and (is-a list OR None)
        print(f"Return from pdb call to: {proc_name}, result:", result)
        return result




    def  __getattribute__(self, name):
        '''
        Adapts attribute access to become invocation of PDB procedure.
        Returns an adapter_func

        Override of Python special method.
        The more common purpose of such override is to compute the attribute,
        or get it from an adaptee.
        '''

        '''
        Require that author previously called main()
        which calls Gimp.main() to create PDB and establish GimpFu type is GimpPlugin
        '''
        if Gimp.get_pdb() is None:
            # Severe error in GimpFu code, or  did not call main()
            # Cannot proceed.
            raise Exception("Gimpfu: pdb accessed before calling main()")
        else:
            # TODO leave some calls unadapted, direct to PDB
            # ??? e.g. run_procedure ???, recursion ???

            # Map hyphens, and deprecated names.
            mangled_proc_name = pdb_name_map[name]

            if Gimp.get_pdb().procedure_exists(mangled_proc_name):
                print("return _adaptor_func for pdb.", mangled_proc_name)
                # remember state for soon-to-come call
                self.adapted_proc_name = mangled_proc_name
                # return intercept function soon to be called
                result = object.__getattribute__(self, "_adaptor_func")

                # TODO We could redirect deprecated names to new procedure names
                # if they have same signature
                # e.g. gimp_image_get_filename => gimp-image-get-file (but not same signature)
                # That case should be adapted, since a filename is not a GFile
                # elif name = deprecate_pdb_procedure_name_map()
            else:
                # Can proceed if we catch the forthcoming call
                # by returning a do_nothing_intercept_func
                do_proceed_error(f"unknown pdb procedure {mangled_proc_name}")
                result = object.__getattribute__(self, "_nothing_adaptor_func")

            return result

            # OLD
            # will raise AttributeError for names that are not defined by GimpPDB
            # return adapteeValue.__getattribute__(mangled_proc_name)






    # specialized, convenience

    '''
    These are pdb procedures from v2 that we don't intend to deprecate
    since some plugins may still use them.
    '''

    """
    WIP
    # Since v3 obsolete but having replacement with different return type in signature.
    def gimp_image_get_filename(image):
        file = Gimp.gimp_image_get_file(image):
        if file:
            # is-a GFile
            result = file.name ....
        else:
            result = None
        return result
    """
