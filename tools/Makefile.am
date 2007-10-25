## Process this file with automake to produce Makefile.in

if WITH_PDBGEN
D_pdbgen = pdbgen
else
D_pdbgen =
endif

SUBDIRS = $(D_pdbgen)

bin_PROGRAMS = $(GIMP_REMOTE)

noinst_PROGRAMS = test-clipboard

EXTRA_PROGRAMS = \
	gimp-remote-2.4	\
	kernelgen

gimp_remote_2_4_SOURCES = \
	gimp-remote.c	\
	gimp-remote.h	\
	gimp-remote-x11.c

gimp_remote_2_4_LDADD = \
	$(GTK_LIBS)	\
	$(LIBXMU)

kernelgen_SOURCES = kernelgen.c

test_clipboard_SOURCES = test-clipboard.c

test_clipboard_LDADD = \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	-DGIMP_APP_VERSION=\"@GIMP_APP_VERSION@\"	\
	-DLOCALEDIR=\""$(gimplocaledir)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	defcheck.py	\
	gimp-mkenums	\
	gimppath2svg.py


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
