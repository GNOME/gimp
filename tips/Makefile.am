## Process this file with automake to produce Makefile.in

tipsdatadir = $(gimpdatadir)/tips

tips_in_files = gimp-tips.xml.in

tipsdata_DATA = $(tips_in_files:.xml.in=.xml)

%.xml: %.xml.in $(srcdir)/$(GETTEXT_PACKAGE)-tips.pot  $(wildcard $(srcdir)/*.po)
	$(INTLTOOL_MERGE) $(srcdir) $< $(@) -x -u -c .intltool-merge-cache

GENPOT = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --pot

# POTFILES is created from POTFILES.in by stripping comments, empty lines
# and Intltool tags (enclosed in square brackets), and appending a full
# relative path to them
POTFILES: POTFILES.in
	( if test 'x$(srcdir)' != 'x.'; then \
	    posrcprefix='$(top_srcdir)/'; \
	  else \
	    posrcprefix="../"; \
	  fi; \
	  rm -f $@-t $@ \
	    && (sed -e '/^#/d' 						\
		    -e "s/^\[.*\] +//" 					\
		    -e '/^[ 	]*$$/d' 				\
		    -e "s@.*@	$$posrcprefix& \\\\@" < $(srcdir)/$@.in	\
		| sed -e '$$s/\\$$//') > $@-t \
	    && chmod a-w $@-t \
	    && mv $@-t $@ )

$(srcdir)/$(GETTEXT_PACKAGE)-tips.pot: $(POTFILES)
	$(GENPOT)

CLEANFILES = $(GETTEXT_PACKAGE)-tips.po

dist-hook:
	( xmllint=`which xmllint`; \
	  if test x$$xmllint != x && test -x $$xmllint; then \
	    $$xmllint --noout --valid gimp-tips.xml || \
	      ( echo "* gimp-tips.xml INVALID *"; exit 1; ) \
	  else \
	    echo "Can't find xmllint to validate gimp-tips.xml; proceed with fingers crossed..."; \
	  fi )

oldtips = \
	gimp_tips.cs.txt	\
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
	gimp-tips.dtd		\
	ChangeLog		\
	makefile.mingw		\
	update.sh
