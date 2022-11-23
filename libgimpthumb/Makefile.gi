# Introspectable sources for libligmathumb

libligmathumb_introspectable_headers =	\
	../libligmathumb/ligmathumb-enums.h	\
	../libligmathumb/ligmathumb-error.h	\
	../libligmathumb/ligmathumb-types.h	\
	../libligmathumb/ligmathumb-utils.h	\
	../libligmathumb/ligmathumbnail.h

libligmathumb_introspectable =	\
	../libligmathumb/ligmathumb-error.c	\
	../libligmathumb/ligmathumb-utils.c	\
	../libligmathumb/ligmathumbnail.c	\
	$(libligmathumb_introspectable_headers)
