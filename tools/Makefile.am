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

## gimp_remote_1_3_SOURCES = gimp-remote.c
##
## gimp_remote_1_3_LDADD = @STRIP_BEGIN@ \
##	$(GTK_LIBS)	\
##	$(LIBXMU)	\
## @STRIP_END@

kernelgen_SOURCES = kernelgen.c

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)	\
@STRIP_END@

EXTRA_DIST = \
	gimppath2svg.py	\
	rmshm

## if DEFAULT_BINARY
## install-exec-hook:
## 	@if test -f "$(DESTDIR)$(bindir)/gimp-remote-1.3"; then \
## 	  echo "cd $(DESTDIR)$(bindir)"; \
## 	  cd $(DESTDIR)$(bindir); \
## 	  echo "rm -f gimp-remote"; \
## 	  rm -f gimp-remote; \
## 	  echo "$(LN_S) gimp-remote-1.3 gimp-remote"; \
## 	  $(LN_S) gimp-remote-1.3 gimp-remote; \
## 	fi
## 
## uninstall-local:
## 	rm -f $(DESTDIR)$(bindir)/gimp-remote
## endif
