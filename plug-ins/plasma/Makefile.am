## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = plasma

plasma_SOURCES = \
	plasma.c   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la		\
	$(GTK_LIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la	

plasma_DEPENDENCIES = $(DEPS)

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
