if WITH_PDBGEN
D_pdbgen=pdbgen
else
D_pdbgen=
endif

if WITH_AUTHORSGEN
D_authorgen=authorsgen
else
D_authorgen=
endif

bin_PROGRAMS = $(GIMP_REMOTE)

EXTRA_PROGRAMS = \
	gimp-remote-1.2	\
	kernelgen

gimp_remote_1_2_SOURCES = gimp-remote.c

gimp_remote_1_2_LDADD = \
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

SUBDIRS = $(D_pdbgen) $(D_authorgen) #$(D_gcg)

EXTRA_DIST = gimppath2svg.py

if DEFAULT_BINARY
install-exec-hook:
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-remote \
	&& $(LN_S) gimp-remote-1.2 gimp-remote

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gimp-remote
endif
