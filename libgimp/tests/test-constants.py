#!/usr/bin/env python3

# Test for the existence of constants that are part of our public API and
# thus should be available using gobject introspection.

# 1. Defined in libgimpbase

# from libgimpbase/gimpchecks.h

gimp_assert('Gimp.CHECK_SIZE',           hasattr(Gimp, "CHECK_SIZE"))
gimp_assert('Gimp.CHECK_SIZE_SM',        hasattr(Gimp, "CHECK_SIZE_SM"))
gimp_assert('Gimp.CHECK_DARK',           hasattr(Gimp, "CHECK_DARK"))
gimp_assert('Gimp.CHECK_LIGHT',          hasattr(Gimp, "CHECK_LIGHT"))


# from libgimpbase/gimplimits.h

gimp_assert('Gimp.MIN_IMAGE_SIZE',       hasattr(Gimp, "MIN_IMAGE_SIZE"))
gimp_assert('Gimp.MAX_IMAGE_SIZE',       hasattr(Gimp, "MAX_IMAGE_SIZE"))
gimp_assert('Gimp.MIN_RESOLUTION',       hasattr(Gimp, "MIN_RESOLUTION"))
gimp_assert('Gimp.MAX_RESOLUTION',       hasattr(Gimp, "MAX_RESOLUTION"))
gimp_assert('Gimp.MAX_MEMSIZE',          hasattr(Gimp, "MAX_MEMSIZE"))


# from libgimpbase/gimpparamspecs.h

gimp_assert('Gimp.PARAM_NO_VALIDATE',    hasattr(Gimp, "PARAM_NO_VALIDATE"))
gimp_assert('Gimp.PARAM_DONT_SERIALIZE', hasattr(Gimp, "PARAM_DONT_SERIALIZE"))
gimp_assert('Gimp.PARAM_FLAG_SHIFT',     hasattr(Gimp, "PARAM_FLAG_SHIFT"))
gimp_assert('Gimp.PARAM_STATIC_STRINGS', hasattr(Gimp, "PARAM_STATIC_STRINGS"))
gimp_assert('Gimp.PARAM_READABLE',       hasattr(Gimp, "PARAM_READABLE"))
gimp_assert('Gimp.PARAM_WRITABLE',       hasattr(Gimp, "PARAM_WRITABLE"))
gimp_assert('Gimp.PARAM_READWRITE',      hasattr(Gimp, "PARAM_READWRITE"))


# from libgimpbase/gimpparasite.h

gimp_assert('Gimp.PARASITE_PERSISTENT',             hasattr(Gimp, "PARASITE_PERSISTENT"))
gimp_assert('Gimp.PARASITE_UNDOABLE',               hasattr(Gimp, "PARASITE_UNDOABLE"))
gimp_assert('Gimp.PARASITE_ATTACH_PARENT',          hasattr(Gimp, "PARASITE_ATTACH_PARENT"))
gimp_assert('Gimp.PARASITE_PARENT_PERSISTENT',      hasattr(Gimp, "PARASITE_PARENT_PERSISTENT"))
gimp_assert('Gimp.PARASITE_PARENT_UNDOABLE',        hasattr(Gimp, "PARASITE_PARENT_UNDOABLE"))
gimp_assert('Gimp.PARASITE_ATTACH_GRANDPARENT',     hasattr(Gimp, "PARASITE_ATTACH_GRANDPARENT"))
gimp_assert('Gimp.PARASITE_GRANDPARENT_PERSISTENT', hasattr(Gimp, "PARASITE_GRANDPARENT_PERSISTENT"))
gimp_assert('Gimp.PARASITE_GRANDPARENT_UNDOABLE',   hasattr(Gimp, "PARASITE_GRANDPARENT_UNDOABLE"))


# from libgimpbase/gimpparasiteio.h

gimp_assert('Gimp.PIXPIPE_MAXDIM',            hasattr(Gimp, "PIXPIPE_MAXDIM"))


# from libgimpbase/gimpversion.h.in

gimp_assert('Gimp.MAJOR_VERSION',             hasattr(Gimp, "MAJOR_VERSION"))
gimp_assert('Gimp.MINOR_VERSION',             hasattr(Gimp, "MINOR_VERSION"))
gimp_assert('Gimp.MICRO_VERSION',             hasattr(Gimp, "MICRO_VERSION"))
gimp_assert('Gimp.VERSION',                   hasattr(Gimp, "VERSION"))
gimp_assert('Gimp.API_VERSION',               hasattr(Gimp, "API_VERSION"))


# 2. Defined in libgimpconfig

# from libgimpconfig/gimpconfig-params.h

gimp_assert('Gimp.CONFIG_PARAM_SERIALIZE',    hasattr(Gimp, "CONFIG_PARAM_SERIALIZE"))
gimp_assert('Gimp.CONFIG_PARAM_AGGREGATE',    hasattr(Gimp, "CONFIG_PARAM_AGGREGATE"))
gimp_assert('Gimp.CONFIG_PARAM_RESTART',      hasattr(Gimp, "CONFIG_PARAM_RESTART"))
gimp_assert('Gimp.CONFIG_PARAM_CONFIRM',      hasattr(Gimp, "CONFIG_PARAM_CONFIRM"))
gimp_assert('Gimp.CONFIG_PARAM_DEFAULTS',     hasattr(Gimp, "CONFIG_PARAM_DEFAULTS"))
gimp_assert('Gimp.CONFIG_PARAM_IGNORE',       hasattr(Gimp, "CONFIG_PARAM_IGNORE"))
gimp_assert('Gimp.CONFIG_PARAM_DONT_COMPARE', hasattr(Gimp, "CONFIG_PARAM_DONT_COMPARE"))
gimp_assert('Gimp.CONFIG_PARAM_FLAG_SHIFT',   hasattr(Gimp, "CONFIG_PARAM_FLAG_SHIFT"))
gimp_assert('Gimp.CONFIG_PARAM_FLAGS',        hasattr(Gimp, "CONFIG_PARAM_FLAGS"))


# 3. Defined in libgimpmodule

# from libgimpmodule/gimpmodule.h

gimp_assert('Gimp.MODULE_ABI_VERSION',        hasattr(Gimp, "MODULE_ABI_VERSION"))
