## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
pagecurl_RC = pagecurl.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = pagecurl

pagecurl_sources = pagecurl.c

pagecurl_built_sources = pagecurl-icons.h
pagecurl.c: $(pagecurl_built_sources)

pagecurl_SOURCES = \
	$(pagecurl_built_sources) $(pagecurl_sources)


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
	$(pagecurl_RC)


PAGECURL_IMAGES = \
	curl0.png	\
	curl1.png	\
	curl2.png	\
	curl3.png	\
	curl4.png	\
	curl5.png	\
	curl6.png	\
	curl7.png

EXTRA_DIST = $(PAGECURL_IMAGES)

CLEANFILES = $(pagecurl_built_sources) pagecurl-icons.list

pagecurl-icons.list: $(PAGECURL_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(PAGECURL_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/pagecurl-icons.h: pagecurl-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat pagecurl-icons.list` > $(@F)
