# Introspectable sources for libgimpcolor

libgimpcolor_introspectable_headers = \
	../libgimpcolor/gimpcolortypes.h		\
	../libgimpcolor/gimpadaptivesupersample.h	\
	../libgimpcolor/gimpbilinear.h		\
	../libgimpcolor/gimpcairo.h			\
	../libgimpcolor/gimpcmyk.h			\
	../libgimpcolor/gimpcolormanaged.h		\
	../libgimpcolor/gimpcolorprofile.h		\
	../libgimpcolor/gimpcolorspace.h		\
	../libgimpcolor/gimpcolortransform.h		\
	../libgimpcolor/gimphsl.h			\
	../libgimpcolor/gimphsv.h			\
	../libgimpcolor/gimppixbuf.h			\
	../libgimpcolor/gimprgb.h

libgimpcolor_introspectable = \
	../libgimpcolor/gimpadaptivesupersample.c	\
	../libgimpcolor/gimpbilinear.c		\
	../libgimpcolor/gimpcairo.c			\
	../libgimpcolor/gimpcmyk.c			\
	../libgimpcolor/gimpcolormanaged.c		\
	../libgimpcolor/gimpcolorprofile.c		\
	../libgimpcolor/gimpcolorspace.c		\
	../libgimpcolor/gimpcolortransform.c		\
	../libgimpcolor/gimphsl.c			\
	../libgimpcolor/gimphsv.c			\
	../libgimpcolor/gimppixbuf.c			\
	../libgimpcolor/gimprgb.c			\
	../libgimpcolor/gimprgb-parse.c		\
	$(libgimpcolor_introspectable_headers)
