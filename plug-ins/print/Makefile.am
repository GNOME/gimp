## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = print

print_SOURCES = \
	print-escp2.c	\
	print-pcl.c	\
	print-ps.c	\
	print-util.c	\
	print.c		\
	print.h   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"	\
	@LP_DEF@			\
	@LPSTAT_DEF@			\
	@LPR_DEF@			\
	@LPC_DEF@

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
