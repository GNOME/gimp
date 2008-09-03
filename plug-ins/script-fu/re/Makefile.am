## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	-DUSE_INTERFACE=1	\
	-I$(srcdir)/..		\
	$(GLIB_CFLAGS)

noinst_LIBRARIES = libre.a

libre_a_SOURCES = \
	cclass.h	\
	cname.h		\
	debug.c		\
	debug.ih	\
	re.c		\
	re.h		\
	regcomp.c	\
	regcomp.ih	\
	regerror.c	\
	regerror.ih	\
	regex.h		\
	regex2.h	\
	regexec.c	\
	regfree.c	\
	split.c		\
	split.h		\
	utils.h

#engine.c and engine.ih are listed below rather than above since the
#file is #include'ed in the source files and not compiled separately.
EXTRA_DIST = \
	COPYRIGHT	\
	README		\
	README.1st	\
	WHATSNEW	\
	engine.c	\
	engine.ih	\
	mkh		\
	regex.001	\
	regex.3		\
	regex.7		\
	re.makefile	\
	re.scm
