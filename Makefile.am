## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4macros ${ACLOCAL_FLAGS}

if WITH_PDBGEN
PDB = pdb
endif

SUBDIRS = \
	m4macros		\
	cursors			\
	themes			\
	po			\
	po-libgimp		\
	po-plug-ins		\
	po-python		\
	po-script-fu		\
	po-tips			\
	po-windows-installer	\
	data			\
	desktop			\
	menus			\
	libgimpbase		\
	tools			\
	$(PDB)			\
	icons			\
	libgimpcolor		\
	libgimpmath		\
	libgimpconfig		\
	libgimpmodule		\
	libgimpthumb		\
	libgimpwidgets		\
	libgimp			\
	app			\
	app-tools		\
	$(GIMP_MODULES)		\
	$(GIMP_PLUGINS)		\
	etc			\
	devel-docs		\
	docs			\
	build

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gimp-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpthumb-@GIMP_PKGCONFIG_VERSION@.pc	\
	gimpui-@GIMP_PKGCONFIG_VERSION@.pc


AUTHORS: authors.xml authors.xsl
if HAVE_XSLTPROC
	$(XSLTPROC) -o $(@) $(top_srcdir)/authors.xsl $< || rm -f $(@)
endif

authors.md: authors.xml authors4gimp-web.xsl
if HAVE_XSLTPROC
	$(XSLTPROC) --stringparam today "`date --iso-8601=seconds`" -o $(@) $(top_srcdir)/authors4gimp-web.xsl $< || rm -f $(@)
endif

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
	ChangeLog.pre-git	\
	HACKING			\
	INSTALL			\
	LICENSE			\
	NEWS			\
	NEWS.pre-2-0		\
	NEWS.pre-2-2		\
	NEWS.pre-2-4		\
	NEWS.pre-2-6		\
	NEWS.pre-2-8		\
	NEWS.pre-2-10		\
	README			\
	README.i18n		\
	authors.dtd		\
	authors.xml		\
	authors.xsl		\
	config.h.win32		\
	gimp-zip.in		\
	git-version.h		\
	gtk-doc.make


DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc


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

generated_sources = \
	git-version.h

# Build git-version.h before anything in the subdirs as this is needed
# in the about and debug dialog (app/) and in the debug tool (tools/).
BUILT_SOURCES = $(generated_sources)

CLEANFILES = $(generated_sources)

# If git is available, always check if git-version.h should be
# updated. If git is not available, don't do anything if git-version.h
# already exists because then we are probably working with a tarball
# in which case the git-version.h we ship is correct.
git-version.h: update-git-version-header
	@if test -e "$(top_srcdir)/.git"; then \
	  git_version="`git --git-dir=$(top_srcdir)/.git describe --always`"; \
	  git_version_abbrev="`git --git-dir=$(top_srcdir)/.git rev-parse --short HEAD`"; \
	  git_last_commit_year="`git --git-dir=$(top_srcdir)/.git log -n1 --reverse --pretty=%ci | cut -b 1-4`"; \
	elif test ! -f "$@"; then \
	  git_version="Unknown, shouldn't happen"; \
	  git_version_abbrev="$$git_version"; \
	  git_last_commit_timestamp=-1; \
	  git_last_commit_year="`date -u '+%Y'`"; \
	fi; \
	if test -n "$$git_version"; then \
	  echo "#ifndef __GIT_VERSION_H__" > "$@.tmp"; \
	  echo "#define __GIT_VERSION_H__" >> "$@.tmp"; \
	  echo "#define GIMP_GIT_VERSION \"$$git_version\"" >> "$@.tmp"; \
	  echo "#define GIMP_GIT_VERSION_ABBREV \"$$git_version_abbrev\"" >> "$@.tmp"; \
	  echo "#define GIMP_GIT_LAST_COMMIT_YEAR \"$$git_last_commit_year\"" >> "$@.tmp"; \
	  echo "#endif /* __GIT_VERSION_H__ */" >> "$@.tmp"; \
	fi
	@if ( test -f "$@.tmp" && test -f "$@" && cmp "$@.tmp" "$@" > /dev/null ); then \
	  rm -f "$@.tmp"; \
	elif test -f "$@.tmp"; then \
	  mv "$@.tmp" "$@"; \
	  echo "  git HEAD changed: $@ regenerated"; \
	fi

.PHONY: update-git-version-header

### GObject introspection

-include $(INTROSPECTION_MAKEFILE)
AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libgimp/Makefile.gi
include $(top_srcdir)/libgimpbase/Makefile.gi
include $(top_srcdir)/libgimpcolor/Makefile.gi
include $(top_srcdir)/libgimpconfig/Makefile.gi
include $(top_srcdir)/libgimpmath/Makefile.gi
include $(top_srcdir)/libgimpmodule/Makefile.gi
include $(top_srcdir)/libgimpthumb/Makefile.gi
include $(top_srcdir)/libgimpwidgets/Makefile.gi

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path="$(srcdir)" --add-include-path="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"
INTROSPECTION_COMPILER_ARGS = --includedir="$(srcdir)" --includedir="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"
INTROSPECTION_SCANNER_ENV = CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)"

Gimp-@GIMP_API_VERSION@.gir: \
	libgimp/libgimp-@GIMP_API_VERSION@.la			\
	libgimp/libgimpui-@GIMP_API_VERSION@.la			\
	libgimpbase/libgimpbase-@GIMP_API_VERSION@.la		\
	libgimpcolor/libgimpcolor-@GIMP_API_VERSION@.la		\
	libgimpconfig/libgimpconfig-@GIMP_API_VERSION@.la	\
	libgimpmath/libgimpmath-@GIMP_API_VERSION@.la		\
	libgimpmodule/libgimpmodule-@GIMP_API_VERSION@.la	\
	libgimpthumb/libgimpthumb-@GIMP_API_VERSION@.la		\
	libgimpwidgets/libgimpwidgets-@GIMP_API_VERSION@.la	\
	Makefile

# The various other GObject-Introspected libraries our API references
# (i.e. types used in parameters or returned values).
Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_INCLUDES = \
	cairo-1.0	\
	GdkPixbuf-2.0	\
	Gegl-0.4	\
	Gio-2.0		\
	GLib-2.0	\
	GObject-2.0	\
	Gtk-3.0

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_CFLAGS = \
	-DGIMP_COMPILATION		\
	-DGIMP_BASE_COMPILATION		\
	-DGIMP_COLOR_COMPILATION	\
	-DGIMP_CONFIG_COMPILATION	\
	-DGIMP_MATH_COMPILATION		\
	-DGIMP_MODULE_COMPILATION	\
	-DGIMP_THUMB_COMPILATION	\
	-DGIMP_WIDGETS_COMPILATION	\
	$(AM_CFLAGS)			\
	$(AM_CPPFLAGS)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(BABL_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_LIBS = \
	libgimp/libgimp-@GIMP_API_VERSION@.la	      		\
	libgimp/libgimpui-@GIMP_API_VERSION@.la	       		\
	libgimpbase/libgimpbase-@GIMP_API_VERSION@.la  		\
	libgimpcolor/libgimpcolor-@GIMP_API_VERSION@.la		\
	libgimpconfig/libgimpconfig-@GIMP_API_VERSION@.la	\
	libgimpmath/libgimpmath-@GIMP_API_VERSION@.la		\
	libgimpmodule/libgimpmodule-@GIMP_API_VERSION@.la	\
	libgimpthumb/libgimpthumb-@GIMP_API_VERSION@.la		\
	libgimpwidgets/libgimpwidgets-@GIMP_API_VERSION@.la

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_FILES = 	\
	$(libgimp_introspectable)					\
	$(libgimpui_introspectable)					\
	$(libgimpbase_introspectable)					\
	$(libgimpcolor_introspectable)					\
	$(libgimpconfig_introspectable)					\
	$(libgimpmath_introspectable)					\
	$(libgimpmodule_introspectable)					\
	$(libgimpthumb_introspectable)					\
	$(libgimpwidgets_introspectable)

INTROSPECTION_GIRS += Gimp-@GIMP_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
