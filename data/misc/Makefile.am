## Makefile.am for gimp/data/misc

miscdatadir = $(gimpdatadir)/misc

miscdata_in_files = gimp.desktop.in.in

miscdata_data_files = $(miscdata_in_files:.desktop.in.in=.desktop)

miscdata_DATA = \
	$(miscdata_data_files)	\
	gimp.applications	\
	gimp.keys

EXTRA_DIST = \
	$(miscdata_in_files)	\
	gimp.applications	\
	gimp.keys

gen_sources = desktop-foo
CLEANFILES = $(gen_sources)

DISTCLEANFILES = gimp.desktop

gimp.desktop: gimp.desktop.in $(wildcard $(top_srcdir)/po/*.po)
	sed -e 's|.[{]prefix[}]|${prefix}|' gimp.desktop.in > desktop-foo \
	&& $(INTLTOOL_MERGE) $(top_srcdir)/po desktop-foo $(@) -d -u -c $(top_builddir)/po/.intltool-merge-cachemake \
	&& rm -r desktop-foo

install-data-local:
if GNOME_DATADIR
	if test -d $(DESTDIR)@GNOME_DATADIR@/applications; then \
		cd $(DESTDIR)@GNOME_DATADIR@/applications \
		&& rm -f gimp-1.3.desktop \
		&& $(LN_S) $(miscdatadir)/gimp.desktop gimp-1.3.desktop \
	; fi
	if test -d $(DESTDIR)@GNOME_DATADIR@/application-registry; then \
		cd $(DESTDIR)@GNOME_DATADIR@/application-registry \
		&& rm -f gimp-1.3.applications \
		&& $(LN_S) $(miscdatadir)/gimp.applications gimp-1.3.applications \
	; fi
	if test -d $(DESTDIR)@GNOME_DATADIR@/mime-info; then \
		cd $(DESTDIR)@GNOME_DATADIR@/mime-info \
		&& rm -f gimp-1.3.keys \
		&& $(LN_S) $(miscdatadir)/gimp.keys gimp-1.3.keys \
	; fi
endif

uninstall-local:
if GNOME_DATADIR
	if test -d $(DESTDIR)@GNOME_DATADIR@/applications; then \
		cd $(DESTDIR)@GNOME_DATADIR@/applications \
		&& rm -f gimp-1.3.desktop \
	; fi
	if test -d $(DESTDIR)@GNOME_DATADIR@/application-registry; then \
		cd $(DESTDIR)@GNOME_DATADIR@/application-registry \
		&& rm -f gimp-1.3.applications \
	; fi
	if test -d $(DESTDIR)@GNOME_DATADIR@/mime-info; then \
		cd $(DESTDIR)@GNOME_DATADIR@/mime-info \
		&& rm -f gimp-1.3.keys \
	; fi
endif
