# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.2c from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ../..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/gimp
pkglibdir = $(libdir)/gimp
pkgincludedir = $(includedir)/gimp

top_builddir = ../..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
host_alias = i486-pc-linux-gnu
host_triplet = i486-pc-linux-gnu
CC = /usr/bin/gcc
EMACS = /usr/bin/emacs
GIMPTCL = gimptcl
JPEG = jpeg
LIBJPEG_LIB = -ljpeg
LIBMPEG_LIB = 
LIBPNG_LIB = -lpng -lz
LIBTCL_LIB = -ltcl -ltk
LIBTIFF_LIB = -ltiff
LIBTOOL = @LIBTOOL@
LIBXDELTA_LIB = 
LIBXPM_LIB = -lXpm
LN_S = ln -s
MAINT = #M#
MAKEINFO = makeinfo
MPEG = 
PACKAGE = gimp
PNG = png
RANLIB = @RANLIB@
TIFF = tiff
VERSION = 0.99.15
XD = 
XPM = xpm
gimpdatadir = ${prefix}/share/gimp
gimpdir = .gimp
gimpplugindir = ${exec_prefix}/lib/gimp/0.99.15

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = xd

xd_SOURCES = \
	xd.c

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(X_LIBS)				\
					\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

xd_DEPENDENCIES = $(DEPS)
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(pluginlib_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I../..
CPPFLAGS = 
LDFLAGS =    -L/usr/X11R6/lib -lgtk -lgdk -lglib -lXext -lX11  -lm
LIBS = 
X_CFLAGS =  -I/usr/X11R6/include
X_LIBS =  -L/usr/X11R6/lib -lgtk -lgdk -lglib -lXext -lX11  -lm
X_EXTRA_LIBS = 
X_PRE_LIBS =  -lSM -lICE
xd_OBJECTS =  xd.o
xd_LDADD = $(LDADD)
xd_LDFLAGS = 
CFLAGS =  -I/usr/X11R6/include -g -O2 -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(xd_SOURCES)
OBJECTS = $(xd_OBJECTS)

default: all

.SUFFIXES:
.SUFFIXES: .c .lo .o
$(srcdir)/Makefile.in: #M# Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps plug-ins/xd/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-pluginlibPROGRAMS:

clean-pluginlibPROGRAMS:
	-test -z "$(pluginlib_PROGRAMS)" || rm -f $(pluginlib_PROGRAMS)

distclean-pluginlibPROGRAMS:

maintainer-clean-pluginlibPROGRAMS:

install-pluginlibPROGRAMS: $(pluginlib_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(pluginlibdir)
	@list='$(pluginlib_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(pluginlibdir)/`echo $$p|sed '$(transform)'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(pluginlibdir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-pluginlibPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(pluginlib_PROGRAMS)'; for p in $$list; do \
	  rm -f $(pluginlibdir)/`echo $$p|sed '$(transform)'`; \
	done

.c.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs

distclean-libtool:

maintainer-clean-libtool:

xd: $(xd_OBJECTS) $(xd_DEPENDENCIES)
	@rm -f xd
	$(LINK) $(xd_LDFLAGS) $(xd_OBJECTS) $(xd_LDADD) $(LIBS)

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = plug-ins/xd

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: 
	@$(NORMAL_INSTALL)

install-data: install-pluginlibPROGRAMS
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-pluginlibPROGRAMS

all: Makefile $(PROGRAMS)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(pluginlibdir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-pluginlibPROGRAMS mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags \
		mostlyclean-generic

clean:  clean-pluginlibPROGRAMS clean-compile clean-libtool clean-tags \
		clean-generic mostlyclean

distclean:  distclean-pluginlibPROGRAMS distclean-compile \
		distclean-libtool distclean-tags distclean-generic \
		clean
	-rm -f config.status
	-rm -f libtool

maintainer-clean:  maintainer-clean-pluginlibPROGRAMS \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-generic \
		distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-pluginlibPROGRAMS \
distclean-pluginlibPROGRAMS clean-pluginlibPROGRAMS \
maintainer-clean-pluginlibPROGRAMS uninstall-pluginlibPROGRAMS \
install-pluginlibPROGRAMS mostlyclean-compile distclean-compile \
clean-compile maintainer-clean-compile mostlyclean-libtool \
distclean-libtool clean-libtool maintainer-clean-libtool tags \
mostlyclean-tags distclean-tags clean-tags maintainer-clean-tags \
distdir info dvi installcheck install-exec install-data install \
uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean


.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
