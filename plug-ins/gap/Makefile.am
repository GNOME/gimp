## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

scriptdatadir = $(gimpdatadir)/scripts

SUBDIRS = iter_ALT

scriptdata_DATA = sel-to-anim-img.scm

EXTRA_DIST = README README_developers TESTPROT_iter_ALT $(scriptdata_DATA)

pluginlib_PROGRAMS = gap_plugins  gap_filter

gap_plugins_SOURCES = \
	gap_main.c		\
	gap_mov_exec.c		\
	gap_mov_exec.h		\
	gap_range_ops.c		\
	gap_range_ops.h		\
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_mov_dialog.c	\
	gap_mov_dialog.h	\
	gap_layer_copy.c	\
	gap_layer_copy.h 	\
	gap_split.c		\
	gap_split.h		\
	gap_resi_dialog.c	\
	gap_resi_dialog.h	\
	gap_mpege.c		\
	gap_mpege.h		\
	gap_mod_layer.c		\
	gap_mod_layer.h		\
	gap_filter_pdb.c	\
	gap_filter_pdb.h	\
	gap_filter_codegen.c	\
	gap_dbbrowser_utils.c	\
	gap_dbbrowser_utils.h	\
	gap_match.c		\
	gap_match.h		\
	gap_lib.c		\
	gap_lib.h		\
	gap_exchange_image.c	\
	gap_exchange_image.h	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	resize.c		\
	resize.h		\
	appenv.h		


gap_filter_SOURCES = \
	gap_filter_main.c	\
	gap_dbbrowser_utils.c	\
	gap_dbbrowser_utils.h	\
	gap_filter_foreach.c	\
	gap_filter_iterators.c	\
	gap_filter_iterators.h	\
	gap_filter_pdb.c	\
	gap_filter_pdb.h	\
	gap_filter_codegen.c	\
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_filter.h


INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

gap_plugins_DEPENDENCIES = $(DEPS)

gap_filter_DEPENDENCIES = $(DEPS)

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done





