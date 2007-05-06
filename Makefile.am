## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros	\
	tools		\
	regexrepl	\
	cursors		\
	themes		\
	po		\
	po-libgimp	\
	po-plug-ins	\
	po-python	\
	po-script-fu	\
	po-tips		\
	data		\
	desktop		\
	menus		\
	libgimpbase	\
	libgimpcolor	\
	libgimpmath	\
	libgimpconfig	\
	libgimpmodule	\
	libgimpthumb	\
	libgimpwidgets	\
	libgimp		\
	app		\
	$(GIMP_MODULES)	\
	$(GIMP_PLUGINS)	\
	etc		\
	devel-docs	\
	docs

bin_SCRIPTS = gimptool-@GIMP_TOOL_VERSION@ @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gimp-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpthumb-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpui-@GIMP_PKGCONFIG_VERSION@.pc


AUTHORS: authors.xml authors.xsl
if HAVE_XSLTPROC
	$(XSLTPROC) authors.xsl $< > $(@) || rm -f $(@)
endif


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
	NEWS			\
	NEWS.pre-2-0		\
	NEWS.pre-2-2		\
	README			\
	README.i18n		\
	README.win32		\
	authors.dtd		\
	authors.xml		\
	authors.xsl		\
	config.h.win32		\
	gimp-zip.in		\
	gimpdefs.msc		\
	gimptool-win32.c.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	makefile.msc		\
	mkinstalldirs


DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update


gimpinstall-@GIMP_TOOL_VERSION@:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall-@GIMP_TOOL_VERSION@

check-defs:
	@$(PYTHON) $(top_srcdir)/tools/defcheck.py || \
	  ( echo "* .def files inconsistent *"; exit 1; )

validate-authors:
if HAVE_XMLLINT
	@$(XMLLINT) --noout --valid authors.xml || \
	  ( echo "* authors.xml INVALID *"; exit 1; )
endif

all-local: AUTHORS

dist-hook: check-defs validate-authors
