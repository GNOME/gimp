GEN_CODE = ostream.c file_ostream.c

# actual automake variables

noinst_LIBRARIES = libgcgexample.a
noinst_PROGRAMS = strtest

CFLAGS = -g -Wall -W -I.. @GLIB_CFLAGS@

SUFFIXES = .gc .gh

libgcgexample_a_SOURCES = $(GEN_CODE)

strtest_SOURCES = strtest.c
strtest_LDADD = libgcgexample.a -lgtk -lgdk -lgmodule -lglib


# tools

GCG = ../gcg
SED = sed
MAKEDEPEND = ../gcgmakedepend


# dependencies and other rules for the def files

GCG_DEFS = $(subst .c,.gc,$(GEN_CODE))

# automake uses .P for .c files, we use .d for .gc files, should
# be no conflict..
DEF_DEPS = $(patsubst %.gc,.deps/%.d,$(GCG_DEFS))

GEN_HEADER =$(subst .gc,.t.h,$(GCG_DEFS)) \
	    $(subst .gc,.p.h,$(GCG_DEFS)) \
	    $(subst .gc,.i.h,$(GCG_DEFS)) \
	    $(subst .gc,.h,$(GCG_DEFS)) 

# gcg include path

DEF_INC = -I ../gh

$(GEN_CODE): %.c: %.gc
	$(GCG) -o $@ $(DEF_INC) $<

$(DEF_DEPS): .deps/%.d: %.gc
	$(MAKEDEPEND) $(DEF_INC) $< >$@

# the real dependency magic

-include $(DEF_DEPS)

# stuff to clean properly

BUILT_SOURCES = $(GEN_CODE) $(GEN_HEADER)
