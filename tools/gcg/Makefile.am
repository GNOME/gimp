noinst_PROGRAMS = gcg

# CPP=cpp -nostdinc -I /usr/share/gcg -I /usr/local/share/gcg -I .

CPPFLAGS		= -DCPP_SEARCH=\"" -I $(datadir)/gcg -I ."\"
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
