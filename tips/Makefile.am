## Process this file with automake to produce Makefile.in

tipsdatadir = $(gimpdatadir)/tips

tips_in_files = gimp-tips.xml.in

tipsdata_DATA = $(tips_in_files:.xml.in=.xml)

%.xml: %.xml.in $(INTLTOOL_MERGE) $(wildcard $(srcdir)/*.po)
	$(INTLTOOL_MERGE) $(srcdir) $< $(@) -x -u -c .intltool-merge-cache

$(GETTEXT_PACKAGE)-tips.pot: gimp-tips.xml.in
	$(INTLTOOL_EXTRACT) --type=gettext/xml gimp-tips.xml.in
	$(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --pot

oldtips = \
	gimp_tips.txt		\
	gimp_tips.cs.txt	\
	gimp_tips.de.txt	\
	gimp_tips.es.txt 	\
	gimp_tips.fr.txt	\
	gimp_tips.hu.txt	\
	gimp_tips.it.txt	\
	gimp_tips.ja.txt	\
	gimp_tips.ko.txt	\
	gimp_tips.pl.txt	\
	gimp_tips.ru.txt	\
	gimp_tips.tr.txt	\
	gimp_tips.uk.txt	\
	gimp_tips.zh_CN.txt	\
	gimp_tips.zh_TW.txt

EXTRA_DIST = \
	POTFILES.in		\
	$(tips_in_files)	\
	$(tipsdata_DATA)	\
	$(oldtips)		\
	ChangeLog		\
	makefile.mingw		\
	update.sh
