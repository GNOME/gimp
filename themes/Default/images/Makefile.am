## Process this file with automake to produce Makefile.in

SUBDIRS = tools

themedatadir = $(gimpdatadir)/themes/Default/images

STOCK_IMAGES = @STRIP_BEGIN@ \
	stock-button-anchor.png		\
	stock-button-delete.png		\
	stock-button-duplicate.png	\
	stock-button-edit.png		\
	stock-button-eye.png		\
	stock-button-linked.png		\
	stock-button-lower.png		\
	stock-button-new.png		\
	stock-button-paste.png		\
	stock-button-paste-as-new.png	\
	stock-button-paste-into.png	\
	stock-button-stroke.png		\
	stock-button-raise.png		\
	stock-button-refresh.png	\
	stock-button-to-path.png	\
	stock-button-to-selection.png	\
@STRIP_END@

themedata_DATA = $(STOCK_IMAGES)

EXTRA_DIST = $(themedata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
