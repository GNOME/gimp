noinst_PROGRAMS = gcg

# CPP=cpp -nostdinc -I /usr/share/gcg -I /usr/local/share/gcg -I .

EXTRA_DIST = \
	getopt.c	\
	makefile.msc	\
	makefile.cygwin \
	gh/gcg.gh	\
	gh/glib.gh	\
	gh/gtk.gh

#AM_CPPFLAGS		= -DCPP_SEARCH=\"" -I $(datadir)/gcg -I ."\"
AM_CPPFLAGS		= -DCPP_SEARCH=\"" -I ."\"
INCLUDES		= @GLIB_CFLAGS@
YFLAGS			= -d -v
LFLAGS			= -d 
LIBS			= @GLIB_LIBS@

gcg_SOURCES = \
	gcg.c		\
	parser.y	\
	lexer.l		\
	output.c	\
	pnode.c		\
	db.c		\
	output_enum.c	\
	output_flags.c	\
	output_object.c	\
	fp.c		\
	marshall.c

if FALSE
ex = ex
endif

SUBDIRS = $(ex)
