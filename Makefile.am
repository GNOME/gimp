## Process this file with automake to produce Makefile.in

SUBDIRS = tools po po-plug-ins intl libgimp app $(GIMP_PLUGINS) $(GIMP_MODULES) data tips $(GIMPDOCS)

bin_SCRIPTS = gimptool

EXTRA_DIST = \
	cursors/bigcirc		\
	cursors/bigcircmsk	\
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
	cursors/mouse1_u	\
	cursors/mouse1_umsk	\
	pixmaps/anchor.xpm	\
	pixmaps/chain.xpm	\
	pixmaps/channel.xbm	\
	pixmaps/delete.xpm	\
	pixmaps/duplicate.xpm	\
	pixmaps/eye.xbm		\
	pixmaps/layer.xbm	\
	pixmaps/linked.xbm	\
	pixmaps/locked.xpm	\
	pixmaps/lower.xpm	\
	pixmaps/mask.xbm	\
	pixmaps/new.xpm		\
	pixmaps/no.xpm		\
	pixmaps/raise.xpm	\
	pixmaps/path.xbm	\
	pixmaps/penadd.xpm	\
	pixmaps/pendel.xpm	\
	pixmaps/penedit.xpm	\
	pixmaps/pennorm.xpm	\
	pixmaps/penstroke.xpm	\
	pixmaps/toselection.xpm	\
	pixmaps/yes.xpm         \
	pixmaps/zoom_in.xpm	\
	pixmaps/zoom_out.xpm	\
	README.i18n		\
	README.win32		\
	README.perl		\
	ChangeLog.pre-1-0	\
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
	gtkrc.forest2		\
	gimp.m4			\
	gimprc.win32		\
	config.h.win32

gimpdata_DATA = \
	gimprc			 \
	gimprc_user		 \
	gtkrc			 \
	unitrc			 \
	gimp_logo.ppm		 \
	gimp_splash.ppm		 \
	gimp1_1_splash.ppm	 \
	ps-menurc		 \
	gtkrc.forest2

gimpdata_SCRIPTS = user_install

man_MANS=gimp.1 gimptool.1 gimprc.5

m4datadir = $(datadir)/aclocal
m4data_DATA = gimp.m4

scriptdata =

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
