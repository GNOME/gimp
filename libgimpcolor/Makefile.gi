# Introspectable sources for libligmacolor

libligmacolor_introspectable_headers = \
	../libligmacolor/ligmacolortypes.h		\
	../libligmacolor/ligmaadaptivesupersample.h	\
	../libligmacolor/ligmabilinear.h		\
	../libligmacolor/ligmacairo.h			\
	../libligmacolor/ligmacmyk.h			\
	../libligmacolor/ligmacolormanaged.h		\
	../libligmacolor/ligmacolorprofile.h		\
	../libligmacolor/ligmacolorspace.h		\
	../libligmacolor/ligmacolortransform.h		\
	../libligmacolor/ligmahsl.h			\
	../libligmacolor/ligmahsv.h			\
	../libligmacolor/ligmapixbuf.h			\
	../libligmacolor/ligmargb.h

libligmacolor_introspectable = \
	../libligmacolor/ligmaadaptivesupersample.c	\
	../libligmacolor/ligmabilinear.c		\
	../libligmacolor/ligmacairo.c			\
	../libligmacolor/ligmacmyk.c			\
	../libligmacolor/ligmacolormanaged.c		\
	../libligmacolor/ligmacolorprofile.c		\
	../libligmacolor/ligmacolorspace.c		\
	../libligmacolor/ligmacolortransform.c		\
	../libligmacolor/ligmahsl.c			\
	../libligmacolor/ligmahsv.c			\
	../libligmacolor/ligmapixbuf.c			\
	../libligmacolor/ligmargb.c			\
	../libligmacolor/ligmargb-parse.c		\
	$(libligmacolor_introspectable_headers)
