## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = jpeg

jpeg_SOURCES = \
	jpeg.c

INCLUDES = \
	$(GTK_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la		\
	$(GTK_LIBS)						\
	@LIBJPEG_LIB@

DEPS = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la	

jpeg_DEPENDENCIES = $(DEPS)

.PHONY: files

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
