if WITH_PDBGEN
D_pdbgen=pdbgen
else
D_pdbgen=
endif

bin_PROGRAMS = gimp-remote

gimp_remote_SOURCES = gimp-remote.c

gimp_remote_LDADD = \
	$(GTK_LIBS)	\
	-lXmu

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

#if WITH_GCG
#D_gcg=gcg
#else 
#D_gcg=
#endif

SUBDIRS = $(D_pdbgen) #$(D_gcg)

EXTRA_DIST = kernelgen.c
