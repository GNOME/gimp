INCLUDES = 		\
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	`pkg-config --cflags gdk-2.0` -I$(top_srcdir)

LDADDS = $(GTK_LIBS) $(top_builddir)/libgimp/libgimpui-2.0.la $(top_builddir)/libgimpwidgets/libgimpwidgets-2.0.la

noinst_PROGRAMS = 		\
	doc-shooter

doc_shooter_SOURCES=	\
	shadow.c	\
	shadow.h	\
	shooter.c	\
	widgets.c	\
	widgets.h

doc_shooter_LDADD = $(LDADDS)

clean-local:
	rm -f *.png
