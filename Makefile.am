## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros		\
	tools			\
	regexrepl		\
	pixmaps			\
	cursors			\
	themes			\
	po			\
	po-libgimp		\
	po-plug-ins		\
	po-script-fu		\
	$(GIMP_PERL_PO)		\
	data			\
	etc			\
	tips			\
	libgimpbase		\
	libgimpcolor		\
	libgimpmath		\
	libgimpwidgets		\
	libgimp			\
	app			\
	$(GIMP_MODULES)		\
	$(GIMP_PLUGINS)		\
	docs			\
	devel-docs		\
	build

bin_SCRIPTS = gimptool-1.3 @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gimp-1.3.pc gimpui-1.3.pc

EXTRA_SCRIPTS = gimpinstall-1.3

EXTRA_DIST = \
	AUTHORS			\
	COPYING			\
	ChangeLog		\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	HACKING			\
	INSTALL			\
	MAINTAINERS		\
	NEWS			\
	PLUGIN_MAINTAINERS	\
	README			\
	README.i18n		\
	README.perl		\
	README.win32		\
	TODO			\
	TODO.xml		\
	config.h.win32		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	po-libgimp/update.sh	\
	po-plug-ins/update.sh	\
	po-script-fu/update.sh

gimpinstall-1.3:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall-1.3

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimptool \
	&& $(LN_S) gimptool-1.3 gimptool
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimptool
endif

# This is a version of the automake-1.4 distcheck rule modified
# to pass --enable-gtk-doc to ./configure
#
mydistcheck: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) zxf $(distdir).tar.gz
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	dc_install_base=`cd $(distdir)/=inst && pwd`; \
	cd $(distdir)/=build \
	  && ../configure --srcdir=.. --prefix=$$dc_install_base --enable-gtk-doc \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
	  && $(MAKE) $(AM_MAKEFLAGS) check \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) dist
	-rm -rf $(distdir)
	@banner="$(distdir).tar.gz is ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"
