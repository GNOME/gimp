## Process this file with automake to produce Makefile.in

tipsdatadir = $(gimpdatadir)/tips

tips_in_files = gimp-tips.xml.in

tipsdata_DATA = $(tips_in_files:.xml.in=.xml)

tips_POFILES = \
	da.po	\
	de.po	\
	pt.po	\
	sv.po

EXTRA_DIST = \
	ChangeLog			\
	POTFILES.in			\
	$(GETTEXT_PACKAGE)-tips.pot	\
	$(tips_POFILES)			\
	$(tipsdata_DATA)		\
	$(tips_in_files)		\
	gimp-tips.dtd			\
	makefile.mingw			\
	update.sh

CLEANFILES = $(GETTEXT_PACKAGE)-tips.po

DISTCLEANFILES = POTFILES

MAINTAINERCLEANFILES = $(tipsdata_DATA)


%.xml: %.xml.in $(srcdir)/$(GETTEXT_PACKAGE)-tips.pot  $(wildcard $(srcdir)/*.po)
	$(INTLTOOL_MERGE) $(srcdir) $< $(@) -x -u -c .intltool-merge-cache

GENPOT = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --pot

MSGMERGE = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --dist

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

update-po: $(srcdir)/$(GETTEXT_PACKAGE)-tips.pot
	PATH=`pwd`/../src:$$PATH; \
	cd $(srcdir); \
	pofiles='$(tips_POFILES)'; \
	for po in $$pofiles; do \
	  lang=`basename $$po .po`; \
	  cp $$lang.po $$lang.old.po; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	  fi; \
	done

dist-hook: update-po gimp-tips.xml
	( xmllint=`which xmllint`; \
	  if test x$$xmllint != x && test -x $$xmllint; then \
	    $$xmllint --noout --valid gimp-tips.xml || \
	      ( echo "* gimp-tips.xml INVALID *"; exit 1; ) \
	  else \
	    echo "Can't find xmllint to validate gimp-tips.xml; proceed with fingers crossed..."; \
	  fi )
