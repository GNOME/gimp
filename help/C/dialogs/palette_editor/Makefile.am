## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/palette_editor

helpdata_DATA = \
        import_palette.html \
        index.html \
        merge_palette.html \
        new_palette.html \
        palette_editor.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
