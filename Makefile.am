## Process this file with automake to produce Makefile.in

SUBDIRS = libgimp plug-ins app

EXTRA_DIST = TODO TODO-DIST NOTES gtkrc gimp_logo.ppm gimp_splash.ppm rmshm user_install gimp_tips.txt ps-menurc

gimpdata_DATA = gimprc gimprc_user gtkrc gimp_logo.ppm gimp_splash.ppm gimp_tips.txt ps-menurc

gimpdata_SCRIPTS = user_install

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
