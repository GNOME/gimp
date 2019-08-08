# Introspectable sources for libgimpmath
#
libgimpmath_introspectable_headers = \
	../libgimpmath/gimpmathtypes.h	\
	../libgimpmath/gimpmatrix.h		\
	../libgimpmath/gimpvector.h

libgimpmath_introspectable = \
	../libgimpmath/gimpmatrix.c		\
	../libgimpmath/gimpvector.c		\
	$(libgimpmath_introspectable_headers)
