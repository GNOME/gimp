## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/image/view

helpdata_DATA = \
        dot_for_dot.html \
        index.html \
        new_view.html \
        shrink_wrap.html \
        snap_to_guides.html \
        toggle_guides.html \
        toggle_rulers.html \
        toggle_selection.html \
        toggle_statusbar.html \
        zoom.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(helpdatadir)/dialogs
	ln -sf ../../dialogs $(helpdatadir)/dialogs
