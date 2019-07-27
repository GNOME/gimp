# Introspectable sources for libgimpmath
#
libgimpmath_introspectable_headers = \
	$(top_srcdir)/libgimpmath/gimpmath.h		\
	$(top_srcdir)/libgimpmath/gimpmathtypes.h	\
	$(top_srcdir)/libgimpmath/gimpmatrix.h		\
	$(top_srcdir)/libgimpmath/gimpvector.h

libgimpmath_introspectable = \
	$(top_srcdir)/libgimpmath/gimpmatrix.c		\
	$(top_srcdir)/libgimpmath/gimpvector.c		\
	$(libgimpmath_introspectable_headers)
