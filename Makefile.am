## Process this file with automake to produce Makefile.in

SUBDIRS = \
	m4macros		\
	tools			\
	regexrepl		\
	pixmaps			\
	themes			\
	libgimpbase		\
	libgimpcolor		\
	libgimpmath		\
	libgimpwidgets		\
	libgimp			\
	app			\
	$(GIMP_PLUGINS)		\
	$(GIMP_MODULES)		\
	po			\
	po-libgimp		\
	po-plug-ins		\
	$(GIMP_PERL_PO)		\
	po-script-fu		\
	cursors			\
	data			\
	tips			\
	docs			\
	devel-docs		\
	build

bin_SCRIPTS = gimptool-1.4 @GIMPINSTALL@

EXTRA_SCRIPTS = gimpinstall

EXTRA_DIST = \
	README.i18n		\
	README.win32		\
	README.perl		\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	PLUGIN_MAINTAINERS	\
	MAINTAINERS		\
	TODO			\
	gtkrc_user		\
	unitrc			\
	gimp_logo.ppm		\
	gimp_splash.ppm		\
	rmshm			\
	user_install		\
	user_install.bat	\
	ps-menurc		\
	gimprc.win32		\
	config.h.win32

gimpdata_DATA = \
	gimp_logo.ppm		\
	gimp_splash.ppm

gimpsysconf_DATA = \
	gimprc			\
	gimprc_user		\
	gtkrc_user		\
	unitrc			\
	ps-menurc

gimpdata_SCRIPTS = user_install

scriptdata =

gimpinstall:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall

install-exec-local:
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-config-1.4 \
	&& $(LN_S) gimptool-1.4 gimp-config-1.4

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gimp-config-1.4

.PHONY: files populate checkin release

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done

populate:
	@echo "populating project"
	@files=`$(MAKE) files | grep -v "make\[[1-9]\]"`; prcs populate -d gimp.prj $$files

checkin: populate
	@echo "checking in project"
	@prcs checkin

release:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%y%m%d"`
