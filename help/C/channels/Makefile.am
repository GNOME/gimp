## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/channels

helpdata_DATA = \
        channel_to_selection.html \
        channels.html \
        delete_channel.html \
        duplicate_channel.html \
        index.html \
        lower_channel.html \
        raise_channel.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(helpdatadir)/dialogs
	$(LN_S) ../dialogs/channels $(helpdatadir)/dialogs
