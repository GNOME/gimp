# Introspectable sources for libgimpthumb

libgimpthumb_introspectable_headers =	\
	../libgimpthumb/gimpthumb-enums.h	\
	../libgimpthumb/gimpthumb-error.h	\
	../libgimpthumb/gimpthumb-types.h	\
	../libgimpthumb/gimpthumb-utils.h	\
	../libgimpthumb/gimpthumbnail.h

libgimpthumb_introspectable =	\
	../libgimpthumb/gimpthumb-error.c	\
	../libgimpthumb/gimpthumb-utils.c	\
	../libgimpthumb/gimpthumbnail.c	\
	$(libgimpthumb_introspectable_headers)
