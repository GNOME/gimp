noinst_LIBRARIES = libregex.a

if HAVE_GLIBC_REGEX
libregex_a_SOURCES =
else
libregex_a_SOURCES = regex.c regex.h
endif

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(includedir)

AM_CPPFLAGS = \
	-DREGEX_MALLOC

EXTRA_DIST = \
	regex.c		\
	regex.h		\
	makefile.msc
