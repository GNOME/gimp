## Process this file with automake to produce Makefile.in

STOCK_IMAGES = @STRIP_BEGIN@ \
	anchor.xpm		\
	delete.xpm		\
	duplicate.xpm		\
	edit.xpm		\
	lower.xpm		\
	new.xpm			\
	paste.xpm		\
	paste-as-new.xpm	\
	paste-into.xpm		\
	raise.xpm		\
	refresh.xpm		\
	topath.xpm		\
	toselection.xpm		\
	zoom_in.xpm		\
	zoom_out.xpm		\
@STRIP_END@

STOCK_VARIABLES = @STRIP_BEGIN@ \
	stock_anchor_button		$(srcdir)/anchor.xpm		\
	stock_delete_button		$(srcdir)/delete.xpm		\
	stock_duplicate_button		$(srcdir)/duplicate.xpm		\
	stock_edit_button		$(srcdir)/edit.xpm		\
	stock_lower_button		$(srcdir)/lower.xpm		\
	stock_new_button		$(srcdir)/new.xpm		\
	stock_paste_button		$(srcdir)/paste.xpm		\
	stock_paste_as_new_button	$(srcdir)/paste-as-new.xpm	\
	stock_paste_into_button		$(srcdir)/paste-into.xpm	\
	stock_raise_button		$(srcdir)/raise.xpm		\
	stock_refresh_button		$(srcdir)/refresh.xpm		\
	stock_to_path_button		$(srcdir)/topath.xpm		\
	stock_to_selection_button	$(srcdir)/toselection.xpm	\
	stock_zoom_in_button		$(srcdir)/zoom_in.xpm		\
	stock_zoom_out_button		$(srcdir)/zoom_out.xpm		\
@STRIP_END@

EXTRA_IMAGES = @STRIP_BEGIN@ \
	chain.xpm		\
	default.xpm		\
	eek.xpm			\
	eye.xpm			\
	first.xpm		\
	folder.xpm		\
	last.xpm		\
	linked.xpm		\
	locked.xbm		\
	navbutton.xpm		\
	next.xpm		\
	no.xpm			\
	path.xbm		\
	penadd.xpm		\
	pendel.xpm		\
	penedit.xpm		\
	pennorm.xpm		\
	penstroke.xpm		\
	play.xpm		\
	prev.xpm		\
	qmasksel.xpm		\
	qmasknosel.xpm		\
	question.xpm		\
	swap.xpm		\
	update.xpm      	\
	wilber.xpm		\
	wilber2.xpm		\
	wilber3.xpm		\
	yes.xpm			\
@STRIP_END@

EXTRA_DIST = $(STOCK_IMAGES) $(EXTRA_IMAGES)

noinst_DATA = gimp-stock-pixbufs.h
CLEANFILES += $(noinst_DATA)

gimp-stock-pixbufs.h: $(STOCK_IMAGES)
	gdk-pixbuf-csource --raw --build-list $(STOCK_VARIABLES) >$(srcdir)/gimp-stock-pixbufs.h

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
