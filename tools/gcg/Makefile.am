noinst_PROGRAMS = gcg

CPP=cpp -nostdinc -I /usr/share/gcg -I /usr/local/share/gcg -I .

CFLAGS=-g -Wall -W "-DCPP=\"$(CPP)\""
INCLUDES		= @GLIB_CFLAGS@
YFLAGS			= -d -v
LFLAGS			= -d 
LIBS			= @GLIB_LIBS@

lexer.c: $(srcdir)/lexer.l
	@rm -f $@
	$(LEX) $(LFLAGS) -t $< >$@

parser.c parser.h: $(srcdir)/parser.y
	$(YACC) $(YFLAGS) $<
	test -f y.tab.c && mv -f y.tab.c parser.c
	test -f y.tab.h && mv -f y.tab.h parser.h

MOSTLYCLEANFILES	= parser.output y.output
DISTCLEANFILES		= parser.c parser.h lexer.c
EXTRA_DIST		= parser.y lexer.l

BUILT_SOURCES		= lexer.c parser.c parser.h

gcg_SOURCES = \
	gcg.c \
	parser.c \
	lexer.c \
	output.c \
	pnode.c \
	db.c \
	output_enum.c \
	output_flags.c \
	output_object.c \
	fp.c \
	marshall.c \
	parser.h

gcg_DEPENDS = parser.h
