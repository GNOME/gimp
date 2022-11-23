# Introspectable sources for libligmamath
#
libligmamath_introspectable_headers = \
	../libligmamath/ligmamathtypes.h	\
	../libligmamath/ligmamatrix.h		\
	../libligmamath/ligmavector.h

libligmamath_introspectable = \
	../libligmamath/ligmamatrix.c		\
	../libligmamath/ligmavector.c		\
	$(libligmamath_introspectable_headers)
