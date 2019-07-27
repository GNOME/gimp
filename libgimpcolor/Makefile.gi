# Introspectable sources for libgimpcolor

libgimpcolor_introspectable_headers = \
	$(top_srcdir)/libgimpcolor/gimpcolortypes.h		\
	$(top_srcdir)/libgimpcolor/gimpadaptivesupersample.h	\
	$(top_srcdir)/libgimpcolor/gimpbilinear.h		\
	$(top_srcdir)/libgimpcolor/gimpcairo.h			\
	$(top_srcdir)/libgimpcolor/gimpcmyk.h			\
	$(top_srcdir)/libgimpcolor/gimpcolormanaged.h		\
	$(top_srcdir)/libgimpcolor/gimpcolorprofile.h		\
	$(top_srcdir)/libgimpcolor/gimpcolorspace.h		\
	$(top_srcdir)/libgimpcolor/gimpcolortransform.h		\
	$(top_srcdir)/libgimpcolor/gimphsl.h			\
	$(top_srcdir)/libgimpcolor/gimphsv.h			\
	$(top_srcdir)/libgimpcolor/gimppixbuf.h			\
	$(top_srcdir)/libgimpcolor/gimprgb.h

libgimpcolor_introspectable = \
	$(top_srcdir)/libgimpcolor/gimpadaptivesupersample.c	\
	$(top_srcdir)/libgimpcolor/gimpbilinear.c		\
	$(top_srcdir)/libgimpcolor/gimpcairo.c			\
	$(top_srcdir)/libgimpcolor/gimpcmyk.c			\
	$(top_srcdir)/libgimpcolor/gimpcolormanaged.c		\
	$(top_srcdir)/libgimpcolor/gimpcolorprofile.c		\
	$(top_srcdir)/libgimpcolor/gimpcolorspace.c		\
	$(top_srcdir)/libgimpcolor/gimpcolortransform.c		\
	$(top_srcdir)/libgimpcolor/gimphsl.c			\
	$(top_srcdir)/libgimpcolor/gimphsv.c			\
	$(top_srcdir)/libgimpcolor/gimppixbuf.c			\
	$(top_srcdir)/libgimpcolor/gimprgb.c			\
	$(top_srcdir)/libgimpcolor/gimprgb-parse.c		\
	$(libgimpcolor_introspectable_headers)
