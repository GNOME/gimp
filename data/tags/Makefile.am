## Makefile.am for gimp/data/tags

tagsdatadir = $(gimpdatadir)/tags

tagsdata_in_files = gimp-tags-default.xml.in

tagsdata_data_files = $(tagsdata_in_files:.xml.in=.xml)

tagsdata_DATA =	$(tagsdata_data_files)

EXTRA_DIST = \
	$(tagsdata_DATA)	\
	$(tagsdata_in_files)	\
	gimp-tags.dtd

DISTCLEANFILES = $(tagsdata_data_files)

itsdir = $(datadir)/gettext/its
its_DATA = its/gimp-tags.loc its/gimp-tags.its

EXTRA_DIST += $(its_DATA)


gimp-tags-default.xml: gimp-tags-default.xml.in $(wildcard $(top_srcdir)/po-tags/*.po)
	$(AM_V_GEN) GETTEXT_DATA_DIRS=$(abs_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-tags -o $@


validate: gimp-tags-default.xml
if HAVE_XMLLINT
	@$(XMLLINT) --noout --path $(srcdir) --valid $< || \
	  ( echo "* gimp-tags-default.xml INVALID *"; exit 1; )
endif

dist-hook: validate
