if WITH_PDBGEN
D_pdbgen=pdbgen
else
D_pdbgen=
endif

bin_PROGRAMS = $(GIMP_REMOTE)

EXTRA_PROGRAMS = \
	gimp-remote	\
	kernelgen

gimp_remote_SOURCES = gimp-remote.c

gimp_remote_LDADD = \
	$(GTK_LIBS)	\
	$(LIBXMU)

kernelgen_SOURCES = kernelgen.c

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

#if WITH_GCG
#D_gcg=gcg
#else 
#D_gcg=
#endif

SUBDIRS = $(D_pdbgen) #$(D_gcg)
