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
if DESKTOP_DATADIR
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/applications; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/applications \
		&& rm -f gimp-$(LT_RELEASE).desktop \
		&& $(LN_S) $(miscdatadir)/gimp.desktop gimp-$(LT_RELEASE).desktop \
	; fi
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/application-registry; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/application-registry \
		&& rm -f gimp-$(LT_RELEASE).applications \
		&& $(LN_S) $(miscdatadir)/gimp.applications gimp-$(LT_RELEASE).applications \
	; fi
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/mime-info; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/mime-info \
		&& rm -f gimp-$(LT_RELEASE).keys \
		&& $(LN_S) $(miscdatadir)/gimp.keys gimp-$(LT_RELEASE).keys \
	; fi
endif

uninstall-local:
if DESKTOP_DATADIR
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/applications; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/applications \
		&& rm -f gimp-$(LT_RELEASE).desktop \
	; fi
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/application-registry; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/application-registry \
		&& rm -f gimp-$(LT_RELEASE).applications \
	; fi
	if test -d $(DESTDIR)@DESKTOP_DATADIR@/mime-info; then \
		cd $(DESTDIR)@DESKTOP_DATADIR@/mime-info \
		&& rm -f gimp-$(LT_RELEASE).keys \
	; fi
endif

validate: gimp.desktop
	( dfvalidate=`which desktop-file-validate`; \
	  if test x$$dfvalidate != x && test -x $$dfvalidate; then \
            $$dfvalidate $< || ( echo "* $< INVALID *"; exit 1 ) \
	  else \
	    echo "Can't find desktop-file-validate to validate $<; proceed with fingers crossed..."; \
          fi )

dist-hook: validate
