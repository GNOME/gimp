## Makefile.am for gimp/data/misc

miscdatadir = $(gimpdatadir)/misc

miscdata_SCRIPTS = \
	user_install

EXTRA_DIST = \
	$(miscdata_SCRIPTS)	\
	gimp.desktop.in.in	\
	user_install.bat

install-data-local:
	sed -e 's|.[{]prefix[}]|${prefix}|' \
	${srcdir}/gimp.desktop.in > ${srcdir}/gimp.desktop \
	&& $(INSTALL) ${srcdir}/gimp.desktop $(DESTDIR)$(miscdatadir)
if GNOME_DESKTOP_PATH
	if test -d @GNOME_DESKTOP_PATH@; then \
		cd @GNOME_DESKTOP_PATH@ \
		&& rm -f gimp-1.3.desktop \
		&& $(LN_S) $(DESTDIR)$(miscdatadir)/gimp.desktop gimp-1.3.desktop \
	; fi
endif

uninstall-local:
if GNOME_DESKTOP_PATH
	cd @GNOME_DESKTOP_PATH@ \
	rm -f $(GNOME_DESKTOP_FILE)
endif
