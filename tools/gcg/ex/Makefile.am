GEN_CODE = ostream_s.c file_ostream_s.c

# actual automake variables

noinst_LIBRARIES = libgcgexample.a
noinst_PROGRAMS = strtest

CFLAGS = -g -Wall -W -I..

SUFFIXES = .gc .gh

libgcgexample_a_SOURCES = $(GEN_CODE)

strtest_SOURCES = strtest.c
strtest_LDADD = libgcgexample.a -lgtk -lgdk -lgmodule -lglib


# tools

GCG = ../gcg
SED = sed
MAKEDEPEND = sh -c '$(CC) -M -x c $$* | $(SED) -e "s/.gc.o/_s.c/g"' makedepend


# dependencies and other rules for the def files

GCG_DEFS = $(subst _s.c,.gc,$(GEN_CODE))

# automake uses .P for .c files, we use .d for .gc files, should
# be no conflict..
DEF_DEPS = $(patsubst %.gc,.deps/%.d,$(GCG_DEFS))

GEN_HEADER = $(subst .gc,_t.h,$(GCG_DEFS)) \
		    $(subst .gc,_p.h,$(GCG_DEFS)) \
		    $(subst .gc,.h,$(GCG_DEFS)) 

# gcg include path

DEF_INC = -I . -I ../gh

$(GEN_CODE): %_s.c: %.gc
	$(GCG) -o $@ $(DEF_INC) $<

$(DEF_DEPS): .deps/%.d: %.gc
	$(MAKEDEPEND) $(DEF_INC) $< >$@

# the real dependency magic

-include $(DEF_DEPS)

# stuff to clean properly

BUILT_SOURCES = $(GEN_CODE) $(GEN_HEADER)
