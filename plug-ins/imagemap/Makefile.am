## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
imagemap_RC = imagemap.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = images

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = imagemap

EXTRA_DIST = \
	imap_cern.l  	\
	imap_csim.l  	\
	imap_ncsa.l	\
	imap_cern.y  	\
	imap_csim.y  	\
	imap_ncsa.y

imagemap_SOURCES = \
	imap_about.c			\
	imap_about.h			\
	imap_browse.c			\
	imap_browse.h			\
	imap_cern_lex.c			\
	imap_cern_parse.c		\
	imap_cern_parse.h		\
	imap_circle.c			\
	imap_circle.h			\
	imap_cmd_clear.c		\
	imap_cmd_copy.c			\
	imap_cmd_copy_object.c		\
	imap_cmd_create.c		\
	imap_cmd_cut.c			\
	imap_cmd_cut_object.c		\
	imap_cmd_delete.c		\
	imap_cmd_delete_point.c		\
	imap_cmd_edit_object.c		\
	imap_cmd_gimp_guides.c		\
	imap_cmd_guides.c		\
	imap_cmd_insert_point.c		\
	imap_cmd_move.c			\
	imap_cmd_move_down.c		\
	imap_cmd_move_sash.c		\
	imap_cmd_move_selected.c	\
	imap_cmd_move_to_front.c	\
	imap_cmd_move_up.c		\
	imap_cmd_object_down.c		\
	imap_cmd_object_move.c		\
	imap_cmd_object_up.c		\
	imap_cmd_paste.c		\
	imap_cmd_select.c		\
	imap_cmd_select_all.c		\
	imap_cmd_select_next.c		\
	imap_cmd_select_prev.c		\
	imap_cmd_select_region.c	\
	imap_cmd_send_to_back.c		\
	imap_cmd_unselect.c		\
	imap_cmd_unselect_all.c		\
	imap_command.c			\
	imap_command.h			\
	imap_commands.h			\
	imap_csim_lex.c			\
	imap_csim_parse.c		\
	imap_csim_parse.h		\
	imap_default_dialog.c		\
	imap_default_dialog.h		\
	imap_edit_area_info.c		\
	imap_edit_area_info.h		\
	imap_file.c			\
	imap_file.h			\
	imap_grid.c			\
	imap_grid.h			\
	imap_main.c			\
	imap_main.h			\
	imap_menu.c			\
	imap_menu.h			\
	imap_menu_funcs.c		\
	imap_menu_funcs.h		\
	imap_misc.c			\
	imap_misc.h			\
	imap_mru.c			\
	imap_mru.h			\
	imap_ncsa_lex.c			\
	imap_ncsa_parse.c		\
	imap_ncsa_parse.h		\
	imap_object.c			\
	imap_object.h			\
	imap_object_popup.c		\
	imap_object_popup.h		\
	imap_polygon.c			\
	imap_polygon.h			\
	imap_preferences.c		\
	imap_preferences.h		\
	imap_preview.c			\
	imap_preview.h			\
	imap_rectangle.c		\
	imap_rectangle.h		\
	imap_selection.c		\
	imap_selection.h		\
	imap_settings.c			\
	imap_settings.h			\
	imap_source.c			\
	imap_source.h			\
	imap_stock.c			\
	imap_stock.h			\
	imap_statusbar.c		\
	imap_statusbar.h		\
	imap_string.c			\
	imap_string.h			\
	imap_table.c			\
	imap_table.h			\
	imap_taglist.c			\
	imap_taglist.h

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(imagemap)

CLEANFILES = y.tab.c y.tab.h

## The following rules are not necessary for most users.  They are
## only used by the maintainers who modify the symbols and grammar
## that are used for parsing the map files.  These rules are very
## specific and a test for flex and bison in configure.ac would not be
## appropriate in most cases, so the Makefile rules are included here.
## In addition, the default rules provided by automake would not be
## sufficient because the source and target files have different base
## names and because of the non-standard prefix used in the output
## code (cern_, csim_, ncsa_).

## Require flex because the standard lex does not support the -P option.
LEX=flex
YACC=bison -y

REBUILD_FILES = \
	imap_cern_lex.c.rebuild \
	imap_csim_lex.c.rebuild \
	imap_ncsa_lex.c.rebuild \
	imap_cern_parse.c.rebuild \
	imap_csim_parse.c.rebuild \
	imap_ncsa_parse.c.rebuild \
	imap_cern_parse.h.rebuild \
	imap_csim_parse.h.rebuild \
	imap_ncsa_parse.h.rebuild

rebuild-parsers: $(REBUILD_FILES)
	@list='$(REBUILD_FILES)'; for p in $$list; do \
	  newfile="`echo $$p | sed -e 's|.rebuild||'`"; \
	  cp $$p $(srcdir)/$$newfile; \
	done

imap_cern_lex.c.rebuild: imap_cern.l
	@$(RM) $@
	$(LEX) $(LFLAGS) -Pcern_ -i -t $< > $@
imap_csim_lex.c.rebuild: imap_csim.l
	@$(RM) $@
	$(LEX) $(LFLAGS) -Pcsim_ -i -t $< > $@
imap_ncsa_lex.c.rebuild: imap_ncsa.l
	@$(RM) $@
	$(LEX) $(LFLAGS) -Pncsa_ -i -t $< > $@

imap_cern_parse.c.rebuild: imap_cern.y
	$(YACC) $(YFLAGS) -d -p cern_ $<
	mv -f y.tab.c $@
imap_csim_parse.c.rebuild: imap_csim.y
	$(YACC) $(YFLAGS) -d -p csim_ $<
	mv -f y.tab.c $@
imap_ncsa_parse.c.rebuild: imap_ncsa.y
	$(YACC) $(YFLAGS) -d -p ncsa_ $<
	mv -f y.tab.c $@

imap_cern_parse.h.rebuild: imap_cern.y
	$(YACC) $(YFLAGS) -d -p cern_ $<
	mv -f y.tab.h $@
imap_csim_parse.h.rebuild: imap_csim.y
	$(YACC) $(YFLAGS) -d -p csim_ $<
	mv -f y.tab.h $@
imap_ncsa_parse.h.rebuild: imap_ncsa.y
	$(YACC) $(YFLAGS) -d -p ncsa_ $<
	mv -f y.tab.h $@
