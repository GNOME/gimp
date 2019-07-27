# Introspectable sources for libgimpconfig

libgimpconfig_introspectable_headers =	\
	$(top_srcdir)/libgimpconfig/gimpconfigenums.h		\
	$(top_srcdir)/libgimpconfig/gimpconfigtypes.h		\
	$(top_srcdir)/libgimpconfig/gimpconfig-iface.h		\
	$(top_srcdir)/libgimpconfig/gimpconfig-deserialize.h	\
	$(top_srcdir)/libgimpconfig/gimpconfig-error.h		\
	$(top_srcdir)/libgimpconfig/gimpconfig-params.h		\
	$(top_srcdir)/libgimpconfig/gimpconfig-path.h		\
	$(top_srcdir)/libgimpconfig/gimpconfig-serialize.h	\
	$(top_srcdir)/libgimpconfig/gimpconfig-utils.h		\
	$(top_srcdir)/libgimpconfig/gimpconfigwriter.h		\
	$(top_srcdir)/libgimpconfig/gimpscanner.h		\
	$(top_srcdir)/libgimpconfig/gimpcolorconfig.h

libgimpconfig_introspectable =	\
	$(top_srcdir)/libgimpconfig/gimpconfig-iface.c		\
	$(top_srcdir)/libgimpconfig/gimpconfig-deserialize.c	\
	$(top_srcdir)/libgimpconfig/gimpconfig-error.c		\
	$(top_srcdir)/libgimpconfig/gimpconfig-path.c		\
	$(top_srcdir)/libgimpconfig/gimpconfig-serialize.c	\
	$(top_srcdir)/libgimpconfig/gimpconfig-utils.c		\
	$(top_srcdir)/libgimpconfig/gimpconfigwriter.c		\
	$(top_srcdir)/libgimpconfig/gimpscanner.c		\
	$(top_srcdir)/libgimpconfig/gimpcolorconfig.c		\
	$(libgimpconfig_introspectable_headers)
