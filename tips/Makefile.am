## Process this file with automake to produce Makefile.in

tipsdatadir = $(gimpdatadir)/tips

tips_in_files = gimp-tips.xml.in

tipsdata_DATA = $(tips_in_files:.xml.in=.xml)

tips_POFILES = \
	cs.po		\
	da.po		\
	de.po		\
	en_CA.po	\
	en_GB.po	\
	es.po		\
	eu.po		\
	fr.po		\
	he.po		\
	hr.po		\
	id.po		\
	it.po		\
	ja.po		\
	ms.po		\
	nl.po		\
	no.po		\
	pl.po		\
	pt.po		\
	pt_BR.po	\
	ru.po		\
	sk.po		\
	sr.po		\
	sr@Latn.po	\
	sv.po		\
	zh_CN.po	\
	zh_TW.po

EXTRA_DIST = \
	ChangeLog			\
	POTFILES.in			\
	$(GETTEXT_PACKAGE)-tips.pot	\
	$(tips_POFILES)			\
	$(tipsdata_DATA)		\
	$(tips_in_files)		\
	gimp-tips.dtd			\
	update.sh

MAINTAINERCLEANFILES = $(GETTEXT_PACKAGE)-tips.pot $(tipsdata_DATA)


%.xml: %.xml.in $(srcdir)/$(GETTEXT_PACKAGE)-tips.pot  $(wildcard $(srcdir)/*.po)
	$(INTLTOOL_MERGE) $(srcdir) $< $(@) -x -u -c .intltool-merge-cache

GENPOT = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --pot

MSGMERGE = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE)-tips --dist


$(srcdir)/$(GETTEXT_PACKAGE)-tips.pot: $(tips_in_files)
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

validate: update-po gimp-tips.xml
	( xmllint=`which xmllint`; \
	  if test x$$xmllint != x && test -x $$xmllint; then \
	    $$xmllint --noout --valid gimp-tips.xml || \
	      ( echo "* gimp-tips.xml INVALID *"; exit 1; ) \
	  else \
	    echo "Can't find xmllint to validate gimp-tips.xml; proceed with fingers crossed..."; \
	  fi )

dist-hook: validate
