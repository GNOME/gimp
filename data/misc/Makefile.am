## Makefile.am for gimp/data/misc

miscdatadir = $(gimpdatadir)/misc

miscdata_SCRIPTS = \
	user_install

miscdata_in_files = gimp.desktop.in.in

miscdata_DATA = $(miscdata_in_files:.desktop.in.in=.desktop)

EXTRA_DIST = \
	$(miscdata_SCRIPTS)	\
	gimp.desktop.in.in	\
	user_install.bat

gen_sources = desktop-foo
CLEANFILES = $(gen_sources)

DISTCLEANFILES = gimp.desktop

gimp.desktop: gimp.desktop.in $(wildcard $(top_srcdir)/po/*.po)
	sed -e 's|.[{]prefix[}]|${prefix}|' gimp.desktop.in > desktop-foo \
	&& $(INTLTOOL_MERGE) $(top_srcdir)/po desktop-foo $(@) -d -u -c $(top_builddir)/po/.intltool-merge-cachemake \
	&& rm -r desktop-foo

install-data-local:
if GNOME_DESKTOP_PATH
	if test -d $(DESTDIR)@GNOME_DESKTOP_PATH@; then \
		cd $(DESTDIR)@GNOME_DESKTOP_PATH@ \
		&& rm -f gimp-1.3.desktop \
		&& $(LN_S) $(miscdatadir)/gimp.desktop gimp-1.3.desktop \
	; fi
endif

uninstall-local:
if GNOME_DESKTOP_PATH
	if test -d $(DESTDIR)@GNOME_DESKTOP_PATH@; then \
		cd $(DESTDIR)@GNOME_DESKTOP_PATH@ \
		&& rm -f gimp-1.3.desktop \
	; fi
endif
