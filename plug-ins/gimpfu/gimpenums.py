
'''
Define Python symbols for Gimp enums.

These are aliases.
Convenience for authors.

TODO Warn about deprecated symbols.
A deprecated symbol is used by v2 plugins
but no longer has a corresponding Gimp enum.
Do not warn about symbol that are now moot.
A moot symbol is one used by v2 plugins,
but only in GimpFu methods that are adapted.
TODO think about this some more

TODO hack, just the ones reference by clothify, not real values

If nothing else, enumerate them all here.
Probably should automate it,
if the names used in Gimp are the same (except for prefix)
Probably there are some abberations.
'''

import gi
gi.require_version("Gimp", "3.0")
from gi.repository import Gimp

'''
# TODO:

code a static list of Gimp enum type names e.g. ("Gimp.ImageType",)

for each
    introspect to get list of symbols in the enum type (is that possible?)
    mangle symbol (get the upper case suffix, sometimes suffix it with "_MODE")
    define a Python symbol having the mangled name
    (meta programming, use eval? or just chunk in __dict__)
    e.g. eval("RGB_IMAGE = Gimp.ImageType.RGB_IMAGE")
'''


def get_enum_name(enum):
    ''' Return name of a Gimp enum type. '''
    '''
    e.g. short_name is 'MergeType', long_name is 'Gimp.MergeType'
    '''
    short_name = enum.__name__
    # Qualify with Gimp namespace
    long_name = "Gimp." + short_name
    # print("Enum name: ", long_name)
    return long_name


def define_symbols_for_enum(enum, prefix="", suffix=""):
    '''
    Define into the GimpFu Python global namespace:
    all the constants defined by enum.
    Require enum is a Gimp type.
    enum is-a class.  type(enum) == 'type'
    '''
    '''
    Example:
    enum is Gimp.HistogramChannel
    Statements:
        global CLIP_TO_IMAGE
        CLIP_TO_IMAGE = Gimp.MergeType.CLIP_TO_IMAGE

    More mangling is done for certain enums:
    Prepend with prefix, or suffend with suffix, where not None.
    Prefix and suffix FBC.
    Note there was not much consistency in v2: some prefixed, some suffixed.
    '''
    #print(enum, type(enum))
    enum_class_name = get_enum_name(enum)
    for attribute in dir(enum):
        if attribute.isupper():
            defining_statement = prefix + attribute + suffix + " = " + enum_class_name + "." + attribute
            #print(defining_statement)
            # Second argument insures definition goes into global namespace
            # See Python docs for exec()
            exec(defining_statement, globals())


'''
Exceptions to the rules
'''

# TODO this is a don't care, because GimpFu adapts all functions that use it??
FG_BG_RGB_MODE      = 1999

# TODO not sure this is correct
BG_BUCKET_FILL      = Gimp.FillType.BACKGROUND


"""
'''
Cruft
Iterate over dir(enum type)
Use any upper case attributes as constants
define into the current namespace
by exec()'ing appropriate statement string
'''
# Gimp.DodgeBurnType
foo = Gimp.HistogramChannel
#print(foo)
#print(dir(foo))
for attribute in dir(foo):
    if attribute.isupper():
        statement = "HISTOGRAM_" + attribute + " = Gimp.HistogramChannel." + attribute
        #print(statement)
        exec(statement)
"""

print("gimpenums defining enums...")
define_symbols_for_enum(Gimp.MergeType)
# ImageBaseType is superset of ImageType, i.e. RGB => RGB, RGBA, etc.
define_symbols_for_enum(Gimp.ImageBaseType)
#RGB                 = Gimp.ImageBaseType.RGB  # GRAY, INDEXED
define_symbols_for_enum(Gimp.ImageType)
#RGB_IMAGE           = Gimp.ImageType.RGB_IMAGE
define_symbols_for_enum(Gimp.LayerMode, suffix='_MODE')
#NORMAL_MODE         = Gimp.LayerMode.NORMAL
#MULTIPLY_MODE       = Gimp.LayerMode.MULTIPLY
define_symbols_for_enum(Gimp.FillType, suffix='_FILL')
# TODO some wild plugins refer to e.g. FILL_TRANSPARENT, with a prefix
#BACKGROUND_FILL     = Gimp.FillType.BACKGROUND
#WHITE_FILL          = Gimp.FillType.WHITE
define_symbols_for_enum(Gimp.ChannelOps, prefix='CHANNEL_OP_')
#CHANNEL_OP_REPLACE  = Gimp.ChannelOps.REPLACE
define_symbols_for_enum(Gimp.GradientType, prefix='GRADIENT_')
#GRADIENT_RADIAL     = Gimp.GradientType.RADIAL
define_symbols_for_enum(Gimp.RepeatMode, prefix='REPEAT_')
#REPEAT_NONE         = Gimp.RepeatMode.NONE
define_symbols_for_enum(Gimp.HistogramChannel, prefix='HISTOGRAM_')
#HISTOGRAM_VALUE      = Gimp.HistogramChannel.VALUE
define_symbols_for_enum(Gimp.MaskApplyMode, prefix='MASK_')
#MASK_APPLY           = Gimp.MaskApplyMode.APPLY
# New to v3
define_symbols_for_enum(Gimp.AddMaskType, prefix='ADD_MASK_')
# ADD_MASK_SELECTION = Gimp.AddMaskType.SELECTION
define_symbols_for_enum(Gimp.SizeType)
# PIXELS = Gimp.SizeType.PIXELS,     POINTS



# Cruft exploring how to get enum names from properties of enum class
#for bar in foo:
#for bar in foo.props:
#for bar in foo.list_properties():
#    print(bar)



# Rest is WIP

"""
from v2 plugin/pygimp/gimpenums-types.defs, which is perl script??
GIMP_FOREGROUND_FILL")
'("background-fill" "GIMP_BACKGROUND_FILL")
'("white-fill" "GIMP_WHITE_FILL")
'("transparent-fill" "GIMP_TRANSPARENT_FILL")
'("pattern-fill" "GIMP_PATTERN_FILL")
"""

# Not work Gimp.BucketFillMode.BG

# How to to this manually
# see /usr/local/share/gir-1.0/gimp<>.gir
# find apropos typename.  Use the short name (appears lower case, like "background") ???

'''
GIMP_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  GIMP_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  GIMP_BUCKET_FILL_PATTERN
'''


# TODO this should be obsolete
# TRUE and FALSE still deprecated, not obsolete
# !!! But this also requires upcast to G_TYPE_BOOLEAN in Marshal and Types

# start verbatim from v2

# This is from pygtk/gtk/__init__.py
# Copyright (C) 1998-2003  James Henstridge

class _DeprecatedConstant:
    def __init__(self, value, name, suggestion):
        self._v = value
        self._name = name
        self._suggestion = suggestion

    def _deprecated(self, value):
        import warnings
        message = '%s is deprecated, use %s instead' % (self._name,
                                                        self._suggestion)
        warnings.warn(message, DeprecationWarning, 3)
        return value

    __nonzero__ = lambda self: self._deprecated(self._v == True)
    __int__     = lambda self: self._deprecated(int(self._v))
    __str__     = lambda self: self._deprecated(str(self._v))
    __repr__    = lambda self: self._deprecated(repr(self._v))
    __cmp__     = lambda self, other: self._deprecated(cmp(self._v, other))

TRUE = _DeprecatedConstant(True, 'gimpenums.TRUE', 'True')
FALSE = _DeprecatedConstant(False, 'gimpenums.FALSE', 'False')

del _DeprecatedConstant

# end verbatim from v2
