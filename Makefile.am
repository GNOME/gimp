## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros		\
	tools			\
	regexrepl		\
	cursors			\
	themes			\
	po			\
	po-libgimp		\
	po-plug-ins		\
	po-script-fu		\
	data			\
	menus			\
	tips			\
	libgimpbase		\
	libgimpcolor		\
	libgimpmath		\
	libgimpmodule		\
	libgimpthumb		\
	libgimpwidgets		\
	libgimp			\
	app			\
	$(GIMP_MODULES)		\
	$(GIMP_PLUGINS)		\
	etc			\
	$(GIMP_DEVEL_DOCS)	\
	docs

bin_SCRIPTS = gimptool-@GIMP_TOOL_VERSION@ @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gimp-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpthumb-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpui-@GIMP_PKGCONFIG_VERSION@.pc

EXTRA_SCRIPTS = gimpinstall-@GIMP_TOOL_VERSION@

EXTRA_DIST = \
	AUTHORS			\
	COPYING			\
	ChangeLog		\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	ChangeLog.pre-2-0	\
	HACKING			\
	INSTALL			\
	LICENSE			\
	MAINTAINERS		\
	NEWS			\
	NEWS.pre-2-0		\
	PLUGIN_MAINTAINERS	\
	README			\
	README.i18n		\
	README.win32		\
	TODO.xml		\
	config.h.win32		\
	gimp-zip.in		\
	gimpdefs.msc		\
	gimptool-win32.c.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	makefile.msc		\
	po-libgimp/update.sh	\
	po-plug-ins/update.sh	\
	po-script-fu/update.sh


DISTCHECK_CONFIGURE_FLAGS = --enable-gimp-console --enable-gtk-doc

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update


gimpinstall-@GIMP_TOOL_VERSION@:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall-@GIMP_TOOL_VERSION@
