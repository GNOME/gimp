# Introspectable sources for libgimpconfig

libgimpconfig_introspectable_headers =	\
	../libgimpconfig/gimpconfigenums.h		\
	../libgimpconfig/gimpconfigtypes.h		\
	../libgimpconfig/gimpconfig-iface.h		\
	../libgimpconfig/gimpconfig-deserialize.h	\
	../libgimpconfig/gimpconfig-error.h		\
	../libgimpconfig/gimpconfig-params.h		\
	../libgimpconfig/gimpconfig-path.h		\
	../libgimpconfig/gimpconfig-register.h		\
	../libgimpconfig/gimpconfig-serialize.h		\
	../libgimpconfig/gimpconfig-utils.h		\
	../libgimpconfig/gimpconfigwriter.h		\
	../libgimpconfig/gimpscanner.h			\
	../libgimpconfig/gimpcolorconfig.h

libgimpconfig_introspectable =	\
	../libgimpconfig/gimpconfig-iface.c		\
	../libgimpconfig/gimpconfig-deserialize.c	\
	../libgimpconfig/gimpconfig-error.c		\
	../libgimpconfig/gimpconfig-path.c		\
	../libgimpconfig/gimpconfig-params.c		\
	../libgimpconfig/gimpconfig-register.c		\
	../libgimpconfig/gimpconfig-serialize.c		\
	../libgimpconfig/gimpconfig-utils.c		\
	../libgimpconfig/gimpconfigwriter.c		\
	../libgimpconfig/gimpscanner.c			\
	../libgimpconfig/gimpcolorconfig.c		\
	$(libgimpconfig_introspectable_headers)
