## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/channels

helpdata_DATA = \
        channels.html \
        edit_channel_attributes.html \
        index.html \
        new_channel.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
