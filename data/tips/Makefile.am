## Makefile.am for ligma/data/tips

tipsdatadir = $(ligmadatadir)/tips

tipsdata_in_files = ligma-tips.xml.in

tipsdata_data_files = $(tipsdata_in_files:.xml.in=.xml)

tipsdata_DATA =	$(tipsdata_data_files)

EXTRA_DIST = \
	$(tipsdata_DATA)	\
	$(tipsdata_in_files)	\
        ligma-tips.dtd		\
        fortunes.xsl

DISTCLEANFILES = $(tipsdata_data_files)

itsdir = $(datadir)/gettext/its
its_DATA = its/ligma-tips.loc its/ligma-tips.its

EXTRA_DIST += $(its_DATA)


ligma-tips.xml: ligma-tips.xml.in $(wildcard $(top_srcdir)/po-tips/*.po)
	$(AM_V_GEN) GETTEXTDATADIRS=$(abs_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-tips -o $@


validate: ligma-tips.xml
if HAVE_XMLLINT
	@$(XMLLINT) --noout --path $(srcdir) --valid $< || \
	  ( echo "* ligma-tips.xml INVALID *"; exit 1; )
endif

fortunes: ligma-tips.xml fortunes.xsl
if HAVE_XSLTPROC
	$(XSLTPROC) --stringparam lang en fortunes.xsl $< > $(@) || rm -f $(@)
else
	@echo "xsltproc is needed to build fortunes"; exit 1;
endif


dist-hook: validate
