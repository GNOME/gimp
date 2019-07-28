# Introspectable sources for libgimpthumb

libgimpthumb_introspectable_headers =	\
	$(top_srcdir)/libgimpthumb/gimpthumb-enums.h	\
	$(top_srcdir)/libgimpthumb/gimpthumb-error.h	\
	$(top_srcdir)/libgimpthumb/gimpthumb-types.h	\
	$(top_srcdir)/libgimpthumb/gimpthumb-utils.h	\
	$(top_srcdir)/libgimpthumb/gimpthumbnail.h

libgimpthumb_introspectable =	\
	$(top_srcdir)/libgimpthumb/gimpthumb-error.c	\
	$(top_srcdir)/libgimpthumb/gimpthumb-utils.c	\
	$(top_srcdir)/libgimpthumb/gimpthumbnail.c	\
	$(libgimpthumb_introspectable_headers)
