noinst_LIBRARIES = libgcgexample.a

CFLAGS=-g -Wall -W -I..

SUFFIXES = .gc .gh

EXTRA_DIST = stream.gh ostream.gc fileostream.gc

BUILT_SOURCES		= \
ostream_s.c ostream.h ostream_t.h ostream_p.h \
file_ostream_s.c file_ostream.h file_ostream_t.h file_ostream_p.h

DEFS = ostream_s.c file_ostream_s.c


$(DEFS): %_s.c: %.gc
	../gcg -o $@ -I ../gh $<


libgcgexample_a_SOURCES = \
	ostream_s.c file_ostream_s.c

