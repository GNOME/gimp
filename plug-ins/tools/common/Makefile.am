libexecdir = $(gimpplugindir)/tool-plug-ins

INCLUDES = \
	-I$(top_srcdir)		  \
	-I$(top_srcdir)/app/      \
	-I$(top_srcdir)/app/tools \
	$(GTK_CFLAGS)		  \
	-I$(includedir)

libexec_LTLIBRARIES = libgimpbrushselecttool.la


###BEGIN EVIL KLUDGE

install-exec-hook:
	list='$(libexec_LTLIBRARIES)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(libexecdir)/$$p; \
	  rm -f `echo $(DESTDIR)$(libexecdir)/$$p | sed -e "s/\\.la/.a/"`; \
	done

###END EVIL KLUDGE	


libgimpbrushselecttool_la_SOURCES = \
	gimpbrushselecttool.c 	\
	gimpbrushselecttool.h

libgimpbrushselecttool_la_LDFLAGS = -avoid-version -module
libgimpbrushselecttool_la_LIBADD = $(GTK_LIBS)

