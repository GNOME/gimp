## Process this file with automake to produce Makefile.in

SUBDIRS = \
	tools		\
	po		\
	po-libgimp	\
	po-plug-ins	\
	intl		\
	libgimp		\
	app		\
	$(GIMP_PLUGINS)	\
	$(GIMP_MODULES)	\
	data		\
	tips		\
	help		\
	$(GIMPDOCS)	\
	devel-docs

bin_SCRIPTS = gimptool @GIMPINSTALL@

EXTRA_SCRIPTS = gimpinstall

EXTRA_DIST = \
	cursors/bad		\
	cursors/badmsk		\
	cursors/dropper		\
	cursors/droppermsk	\
	cursors/mouse1		\
	cursors/mouse1msk	\
	cursors/mouse1_ap	\
	cursors/mouse1_apmsk	\
	cursors/mouse1_cp	\
	cursors/mouse1_cpmsk	\
	cursors/mouse1_m	\
	cursors/mouse1_mmsk	\
	cursors/mouse1_mm	\
	cursors/mouse1_mmmsk	\
	cursors/mouse1_p	\
        cursors/mouse1_pmsk     \
	cursors/mouse1_sel	\
	cursors/mouse1_selmsk	\
	cursors/mouse1_selm	\
	cursors/mouse1_selmmsk	\
	cursors/mouse1_selp	\
	cursors/mouse1_selpmsk	\
	cursors/mouse1_selu	\
	cursors/mouse1_selumsk	\
	cursors/mouse1_u	\
	cursors/mouse1_umsk	\
	pixmaps/anchor.xpm	\
	pixmaps/chain.xpm	\
	pixmaps/channel.xbm	\
	pixmaps/delete.xpm	\
	pixmaps/duplicate.xpm	\
	pixmaps/eek.xpm		\
	pixmaps/eye.xbm		\
	pixmaps/first.xpm	\
	pixmaps/last.xpm	\
	pixmaps/layer.xbm	\
	pixmaps/linked.xbm	\
	pixmaps/locked.xbm	\
	pixmaps/lower.xpm	\
	pixmaps/mask.xbm	\
	pixmaps/navbutton.xpm	\
	pixmaps/next.xpm	\
	pixmaps/new.xpm		\
	pixmaps/no.xpm		\
	pixmaps/raise.xpm	\
	pixmaps/path.xbm	\
	pixmaps/penadd.xpm	\
	pixmaps/pendel.xpm	\
	pixmaps/penedit.xpm	\
	pixmaps/pennorm.xpm	\
	pixmaps/penstroke.xpm	\
	pixmaps/play.xpm	\
	pixmaps/prev.xpm	\
	pixmaps/qmasksel.xpm	\
	pixmaps/qmasknosel.xpm	\
	pixmaps/question.xpm	\
	pixmaps/topath.xpm	\
	pixmaps/toselection.xpm	\
	pixmaps/update.xpm      \
	pixmaps/wilber.xpm	\
	pixmaps/yes.xpm         \
	pixmaps/zoom_in.xpm	\
	pixmaps/zoom_out.xpm	\
	README.i18n		\
	README.win32		\
	README.perl		\
	ChangeLog.pre-1-0	\
	PLUGIN_MAINTAINERS	\
	MAINTAINERS		\
	TODO			\
	gtkrc			\
	unitrc			\
	gimp_logo.ppm		\
	gimp_splash.ppm		\
	gimp1_1_splash.ppm	\
	rmshm			\
	user_install		\
	user_install.bat	\
	ps-menurc		\
	gimp.1			\
	gimptool.1		\
	gimp.m4			\
	gimprc.win32		\
	config.h.win32		\
	gimp.spec

gimpdata_DATA = \
	gimprc			 \
	gimprc_user		 \
	gtkrc			 \
	unitrc			 \
	gimp_logo.ppm		 \
	gimp_splash.ppm		 \
	gimp1_1_splash.ppm	 \
	ps-menurc

gimpdata_SCRIPTS = user_install

man_MANS = gimp.1 gimptool.1 gimprc.5

m4datadir = $(datadir)/aclocal
m4data_DATA = gimp.m4

scriptdata =

gimpinstall:
	$(LN_S) $(srcdir)/install-sh $(srcdir)/gimpinstall

install-exec-local:
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-config \
	&& $(LN_S) gimptool gimp-config

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gimp-config

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
