## Process this file with automake to produce Makefile.in

if WITH_PDBGEN
D_pdbgen = pdbgen
else
D_pdbgen =
endif

SUBDIRS = $(D_pdbgen)

bin_PROGRAMS = $(GIMP_REMOTE)

EXTRA_PROGRAMS = \
	gimp-remote-2.2	\
	kernelgen

gimp_remote_2_2_SOURCES = gimp-remote.c

gimp_remote_2_2_LDADD = \
	$(GTK_LIBS)	\
	$(LIBXMU)

kernelgen_SOURCES = kernelgen.c

AM_CPPFLAGS = -DGIMP_APP_VERSION=\"@GIMP_APP_VERSION@\"

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	gimp-mkenums	\
	gimppath2svg.py	\
	rmshm

install-exec-hook:
if DEFAULT_BINARY
	if test -f "$(DESTDIR)$(bindir)/gimp-remote-@GIMP_APP_VERSION@"; then \
	  echo "cd $(DESTDIR)$(bindir)"; \
	  cd $(DESTDIR)$(bindir); \
	  echo "rm -f gimp-remote"; \
	  rm -f gimp-remote; \
	  echo "$(LN_S) gimp-remote-@GIMP_APP_VERSION@ gimp-remote"; \
	  $(LN_S) gimp-remote-@GIMP_APP_VERSION@ gimp-remote; \
	fi
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp-remote
endif
