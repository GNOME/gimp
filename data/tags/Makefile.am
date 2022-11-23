## Makefile.am for ligma/data/tags

tagsdatadir = $(ligmadatadir)/tags

tagsdata_in_files = ligma-tags-default.xml.in

tagsdata_data_files = $(tagsdata_in_files:.xml.in=.xml)

tagsdata_DATA =	$(tagsdata_data_files)

EXTRA_DIST = \
	$(tagsdata_DATA)	\
	$(tagsdata_in_files)	\
	ligma-tags.dtd

DISTCLEANFILES = $(tagsdata_data_files)

itsdir = $(datadir)/gettext/its
its_DATA = its/ligma-tags.loc its/ligma-tags.its

EXTRA_DIST += $(its_DATA)


ligma-tags-default.xml: ligma-tags-default.xml.in $(wildcard $(top_srcdir)/po-tags/*.po)
	$(AM_V_GEN) GETTEXTDATADIRS=$(abs_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-tags -o $@


validate: ligma-tags-default.xml
if HAVE_XMLLINT
	@$(XMLLINT) --noout --path $(srcdir) --valid $< || \
	  ( echo "* ligma-tags-default.xml INVALID *"; exit 1; )
endif

dist-hook: validate
