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

install-exec-hook:
if DEFAULT_BINARY
	@if test -f "$(DESTDIR)$(bindir)/gimp-remote-1.2"; then \
	  echo "cd $(DESTDIR)$(bindir)"; \
	  cd $(DESTDIR)$(bindir); \
	  echo "rm -f gimp-remote"; \
	  rm -f gimp-remote; \
	  echo "$(LN_S) gimp-remote-1.2 gimp-remote"; \
	  $(LN_S) gimp-remote-1.2 gimp-remote; \
	fi
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp-remote
endif
