## Makefile.am for gimp/desktop

desktop_in_files = gimp.desktop.in.in
desktop_files = $(desktop_in_files:.desktop.in.in=.desktop)


if DESKTOP_DATADIR

applicationsdir = @DESKTOP_DATADIR@/applications
applications_DATA = $(desktop_files)

iconsdir = @DESKTOP_DATADIR@/icons/hicolor/48x48/apps
icons_DATA = gimp.png

mimeinfodir = @DESKTOP_DATADIR@/mime-info
mimeinfo_DATA = gimp.keys

registrydir = @DESKTOP_DATADIR@/application-registry
registry_DATA = gimp.applications

endif


EXTRA_DIST = \
	gimp.applications.in	\
	gimp.desktop.in.in	\
	gimp.keys.in		\
	gimp.png


DISTCLEANFILES = $(desktop_files)


gimp.desktop: gimp.desktop.in $(wildcard $(top_srcdir)/po/*.po)
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $(@) -d -u -c $(top_builddir)/po/.intltool-merge-cachemake

validate: gimp.desktop
	( dfvalidate=`which desktop-file-validate`; \
	  if test x$$dfvalidate != x && test -x $$dfvalidate; then \
            $$dfvalidate $< || ( echo "* $< INVALID *"; exit 1 ) \
	  else \
	    echo "Can't find desktop-file-validate to validate $<; proceed with fingers crossed..."; \
          fi )

dist-hook: validate
