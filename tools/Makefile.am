## Process this file with automake to produce Makefile.in

if WITH_PDBGEN
D_pdbgen = pdbgen
else
D_pdbgen =
endif

if WITH_AUTHORSGEN
D_authorgen = authorsgen
else
D_authorgen =
endif

SUBDIRS = $(D_pdbgen) $(D_authorgen)

bin_PROGRAMS = $(GIMP_REMOTE)

EXTRA_PROGRAMS = \
	gimp-remote	\
	kernelgen

gimp_remote_SOURCES = gimp-remote.c

gimp_remote_LDADD = @STRIP_BEGIN@ \
	$(GTK_LIBS)	\
	$(LIBXMU)	\
@STRIP_END@

kernelgen_SOURCES = kernelgen.c

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)	\
@STRIP_END@

EXTRA_DIST = gimppath2svg.py
