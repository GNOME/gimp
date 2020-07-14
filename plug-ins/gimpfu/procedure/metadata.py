
import string   # v2 as _string to hide from authors

from message.proceed_error import  do_proceed_error
from message.deprecation import Deprecation

from procedure.formal_param import GimpfuFormalParam

from gimpfu_enums import *  # PF_ enums


# v3 _registered_plugins_ is a dictionary of GimpfuProcedureMetadata

class GimpfuProcedureMetadata():
    '''
    Responsible for sanity checking and fixups to author's declaration.
    This is in the nature of compiling:
       - give warnings
       - or throw exceptions (sanity)
    both of which printed in the console.
    At install time!  Once registered with Gimp, no further warnings.
    Author must delete ~/.config/.../pluginrc to see warnings again.
    '''

    '''
    Constant class data
    '''

    file_params = [GimpfuFormalParam(PF_STRING, "filename", "The name of the file", ""),
                   GimpfuFormalParam(PF_STRING, "raw-filename", "The name of the file", "")]
    image_param = GimpfuFormalParam(PF_IMAGE, "image", "Input image", None)
    drawable_param = GimpfuFormalParam(PF_DRAWABLE, "drawable", "Input drawable", None)

    '''
    Since: 3.0 Gimp enforces: Identifiers for procedure names and parameter names:
    Characters from set: '-', 'a-z', 'A-Z', '0-9'.
    v2 allowed "_"
    Gimp will check also.  So redundant, but catch it early.
    '''
    proc_name_allowed = string.ascii_letters + string.digits + "-"
    param_name_allowed = string.ascii_letters + string.digits + "-_"



    '''
    __init__ fixes deprecations
    May raise exceptions
    '''
    def __init__(self,
       blurb, help, author, copyright,
       date, label, imagetypes,
       plugin_type, params, results,
       function, menu, domain,
       on_query, on_run):

        label =GimpfuProcedureMetadata.substitute_empty_string_for_none(label, "label")
        imagetypes = GimpfuProcedureMetadata.substitute_empty_string_for_none(imagetypes, "imagetypes")
        menu = GimpfuProcedureMetadata.substitute_empty_string_for_none(menu, "menupath")

        # wild data, soon to be fixed up
        self.BLURB=blurb
        self.HELP= help
        self.AUTHOR= author
        self.COPYRIGHT=copyright
        self.DATE= date
        self.MENUITEMLABEL= label
        self.IMAGETYPES= imagetypes
        self.PLUGIN_TYPE= plugin_type
        self.RESULTS= results
        self.FUNCTION= function
        self.MENUPATH= menu
        self.DOMAIN= domain
        self.ON_QUERY= on_query
        self.ON_RUN= on_run

        self.PARAMS= []
        for param in params:
             # assert param is a tuple
             self.PARAMS.append(GimpfuFormalParam(*param))



        '''
        Fix author's mistakes and allow deprecated constructs.
        Generates data into self.
        These are in the nature of patches.
        And hard to maintain.
        '''
        # May not return, throws exceptions
        self._sanity_test_registration()

        self.did_fix_menu = self._deriveMissingMenu()
        self._deriveMissingParams()
        '''
        !!! When we insert image params,
        signature registered with Gimp
        differs from signature of run_func.
        '''
        self.did_insert_image_params = self._deriveMissingImageParams()


    @classmethod
    def fix_underbar(cls, proc_name):
        new_proc_name = proc_name.replace( '_' , '-')
        if (new_proc_name != proc_name):
            Deprecation.say("Underbar in procedure name.")
        return new_proc_name

    @classmethod
    def canonicalize_prefix(cls, proc_name):
        if (not proc_name.startswith("python-") and
            not proc_name.startswith("extension-") and
            not proc_name.startswith("plug-in-") and
            not proc_name.startswith("file-") ):
               result = "python-fu-" + proc_name
               message = f"Procedure name canonicalized to {result}"
               Deprecation.say(message)
        else:
           result = proc_name
        return result


    '''
    public: metadata knows how to fix procedure name
    '''
    @classmethod
    def makeProcNamePrefixCanonical(cls, proc_name):
        '''
        if given name not canonical, make it so.

        Canonical means having a prefix from a small set,
        so that from the canonical name, PDB browsers know how implemented.

        v2 allowed underbars, i.e. python_, extension_, plug_in_, file_ prefixes.

        Note that prefix python-fu intends to suggest (to browsers of PDB):
        - fu: simplified API i.e. uses GimpFu module
        - python: language is python
        script-fu is similar for Scheme language: simplified API
        You can write a Python language plugin without the simplified API
        and then it is author's duty to choose a canonically prefixed name.
        '''

        # FBC.  Gimp v3 does not allow underbar
        proc_name = GimpfuProcedureMetadata.fix_underbar(proc_name)
        proc_name = GimpfuProcedureMetadata.canonicalize_prefix(proc_name)
        if not GimpfuProcedureMetadata.letterCheck(proc_name, GimpfuProcedureMetadata.proc_name_allowed):
            raise Exception(f"Procedure name: {proc_name} contains illegal characters.")
        print(f"Procedure name: {proc_name}")
        return proc_name




    '''
    Utility functions
    '''
    def substitute_empty_string_for_none(arg, argname):
        if arg is None:
            Deprecation.say(f"Deprecated: Registered {argname} should be empty string, not None")
            result = ""
        else:
            result = arg
        return result



    def letterCheck(str, allowed):
        for ch in str:
            if not ch in allowed:
                return False
        else:
            return True


    '''
    Private methods.
    Fixups to self.  !!! Side effects on self
    '''


    def _deriveMissingMenu(self):
        '''
        if menu is not given, derive it from label
        Ability to omit menu is deprecated, so this is FBC.
        '''
        result = False
        if not self.MENUPATH:
            if self.MENUITEMLABEL:
                # label but no menu. Possible menu path in the label.
                fields = self.MENUITEMLABEL.split("/")
                if fields:
                    self.MENUITEMLABEL = fields.pop()
                    self.MENUPATH = "/".join(fields)

                    result = True

                    message = (f" Use the 'menu' parameter instead"
                               f" of passing a full menu path in 'label'.")
                    Deprecation.say(message)
                else:
                    # 'label' is not a path, can't derive menu path
                    message = f" Simple menu item 'label' without 'menu' path."
                    # TODO will GIMP show it in the GUI in a fallback place?
                    Deprecation.say(message)
            else:
                # no menu and no label
                # Normal, user intends to create plugin only callable by other plugins
                message = (f" No 'menu' and no 'label'."
                           f"Plugin will not appear in Gimp GUI.")
                # TODO Not really a deprecation, a UserWarning??
                Deprecation.say(message)
        else:
            if  self.MENUITEMLABEL:
                # menu and label given
                # Normal, user intends plugin appear in GUI
                pass
            else:
                # menu but no label
                # TODO Gimp will use suffix of 'menu' as 'label' ???
                message = (f" Use the 'label' parameter instead"
                           f"of passing menu item at end of 'menu'.")
                Deprecation.say(message)
        return result



    def _deriveMissingParams(self):
        ''' FBC Add missing params according to plugin type. '''

        '''
        FBC.
        In the distant past, an author could specify menu like <Load>
        and not provide a label
        and not provide the first two params,
        in which case GimpFu inserts two params.
        Similarly for other cases.
        '''
        # v2 if self.did_fix_menu and plugin_type == PLUGIN:
        # require _deriveMissingMenu called earlier
        if not self.did_fix_menu:
            return


        if self.MENUPATH is None:
            pass
        elif self.MENUPATH.startswith("<Load>"):
            # insert into slice
            self.PARAMS[0:0] = GimpfuFormalParam.file_params
            message = f" Fixing two file params for Load plugin"
            Deprecation.say(message)
        elif self.MENUPATH.startswith("<Image>") or self.MENUPATH.startswith("<Save>"):
            self.PARAMS.insert(0, GimpfuProcedureMetadata.image_param)
            self.PARAMS.insert(1, GimpfuProcedureMetadata.drawable_param)
            message = f" Fixing two image params for Image or Save plugin"
            Deprecation.say(message)
            if self.MENUPATH.startswith("<Save>"):
                self.PARAMS[2:2] = file_params
                message = f" Fixing two file params for Save plugin"
                Deprecation.say(message)
        #print(self.PARAMS)


    def _deriveMissingImageParams(self):
        '''
        Some plugins declare they are <Image> plugins,
        but don't have first two params equal to (image, drawable),
        and have imagetype == "" (menu item enabled even if no image is open).
        And then Gimp refuses to create procedure
        (but does create an item in pluginrc!)
        E.G. sphere.py

        So we diverge the signature of the plugin from the signature of the run_func.
        The author might be unaware, unless they explore,
        or try to call the PDB procedure from another PDB procedure.

        TODO after we are done, the count of args to run_func
        and the count of formal parameters (PARAMS) should be the same.
        Can we count the formal parameters of run_func?
        '''
        if not self.MENUPATH:
            return

        result = False
        # if missing params (never there, or not fixed by earlier patch)
        # TODO if params are missing altogether
        if ( self.MENUPATH.startswith("<Image>")
            and self.PARAMS[0].PF_TYPE != PF_IMAGE
            and self.PARAMS[1].PF_TYPE != PF_DRAWABLE
            ):
            self.PARAMS.insert(0, GimpfuProcedureMetadata.image_param)
            self.PARAMS.insert(1, GimpfuProcedureMetadata.drawable_param)
            message = f" Fixing two image params for Image plugin"
            Deprecation.say(message)
            result = True
        return result


    def _sanity_test_registration(self):
        ''' Quit when detect exceptional results or params. '''

        for param in self.PARAMS:
            """
            TODO
            if len(ent) < 4:
                raise Exception( ("parameter definition must contain at least 4 "
                              "elements (%s given: %s)" % (len(ent), ent)) )
            """

            if not isinstance(param.PF_TYPE, int):
                # TODO check in range
                exception_str = f"Plugin parameter type {param.PF_TYPE} not a valid PF_ enum"
                raise Exception(exception_str)


            # Common mistake is a space in the LABEL
            param.LABEL = param.LABEL.replace( ' ' , '_')

            if not GimpfuProcedureMetadata.letterCheck(param.LABEL, GimpfuProcedureMetadata.param_name_allowed):
                # Not fatal since we don't use it, args are a sequence, not by keyword
                # But Gimp may yet complain.
                # TODO transliterate space to underbar
                do_proceed_error(f"parameter name '{param.LABEL}'' contains illegal characters")

        for ent in self.RESULTS:
            # ent is tuple
            # TODO a class
            if len(ent) < 3:
                # TODO proceed
                raise Exception( ("result definition must contain at least 3 elements "
                              "(%s given: %s)" % (len(ent), ent)))

            if not isinstance(ent[0], int):
                raise Exception("result must be of integral type")

            if not GimpfuProcedureMetadata.letterCheck(ent[1], GimpfuProcedureMetadata.param_name_allowed):
                # not fatal unless we use it?
                do_proceed_error(f"result name '{ent[1]}' contains illegal characters")
