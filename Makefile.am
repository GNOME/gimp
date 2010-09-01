## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4macros

SUBDIRS = \
	m4macros	\
	tools		\
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
	docs		\
	build

bin_SCRIPTS = @GIMPINSTALL@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gimp-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpthumb-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpui-@GIMP_PKGCONFIG_VERSION@.pc


AUTHORS: authors.xml authors.xsl
if HAVE_XSLTPROC
	$(XSLTPROC) -o $(@) authors.xsl $< || rm -f $(@)
endif


EXTRA_SCRIPTS = gimpinstall-@GIMP_TOOL_VERSION@

EXTRA_DIST = \
	AUTHORS			\
	COPYING			\
	ChangeLog		\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	ChangeLog.pre-2-0	\
	ChangeLog.pre-2-2	\
	ChangeLog.pre-2-4	\
	ChangeLog.pre-2-6	\
	HACKING			\
	INSTALL			\
	LICENSE			\
	NEWS			\
	NEWS.pre-2-0		\
	NEWS.pre-2-2		\
	NEWS.pre-2-4		\
	NEWS.pre-2-6		\
	README			\
	README.i18n		\
	authors.dtd		\
	authors.xml		\
	authors.xsl		\
	config.h.win32		\
	gimp-zip.in		\
	gimpdefs.msc		\
	makefile.msc


DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc


gimpinstall-@GIMP_TOOL_VERSION@:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall-@GIMP_TOOL_VERSION@

check-defs:
	@$(PYTHON) $(top_srcdir)/tools/defcheck.py $(top_srcdir) || \
	  ( echo "*** .def files inconsistent ***"; exit 1; )

validate-authors:
if HAVE_XMLLINT
	@cd $(srcdir); \
	$(XMLLINT) --noout --valid authors.xml || \
	  ( echo "*** authors.xml INVALID ***"; exit 1; )
endif

all-local: AUTHORS

dist-hook: check-defs validate-authors



CHANGELOG_START = 74424325abb54620b370f2595445b2b2a19fe5e7

ChangeLog: $(srcdir)/ChangeLog $(srcdir)/ChangeLog.pre-git
$(srcdir)/ChangeLog:
	@echo Creating $@ based on git log
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log $(CHANGELOG_START)^.. --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ && echo Appending ChangeLog.pre-git && cat ChangeLog.pre-git >> $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog
