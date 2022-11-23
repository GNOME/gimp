# Introspectable sources for libligmaconfig

libligmaconfig_introspectable_headers =	\
	../libligmaconfig/ligmaconfigenums.h		\
	../libligmaconfig/ligmaconfigtypes.h		\
	../libligmaconfig/ligmaconfig-iface.h		\
	../libligmaconfig/ligmaconfig-deserialize.h	\
	../libligmaconfig/ligmaconfig-error.h		\
	../libligmaconfig/ligmaconfig-params.h		\
	../libligmaconfig/ligmaconfig-path.h		\
	../libligmaconfig/ligmaconfig-register.h		\
	../libligmaconfig/ligmaconfig-serialize.h		\
	../libligmaconfig/ligmaconfig-utils.h		\
	../libligmaconfig/ligmaconfigwriter.h		\
	../libligmaconfig/ligmascanner.h			\
	../libligmaconfig/ligmacolorconfig.h

libligmaconfig_introspectable =	\
	../libligmaconfig/ligmaconfig-iface.c		\
	../libligmaconfig/ligmaconfig-deserialize.c	\
	../libligmaconfig/ligmaconfig-error.c		\
	../libligmaconfig/ligmaconfig-path.c		\
	../libligmaconfig/ligmaconfig-params.c		\
	../libligmaconfig/ligmaconfig-register.c		\
	../libligmaconfig/ligmaconfig-serialize.c		\
	../libligmaconfig/ligmaconfig-utils.c		\
	../libligmaconfig/ligmaconfigwriter.c		\
	../libligmaconfig/ligmascanner.c			\
	../libligmaconfig/ligmacolorconfig.c		\
	$(libligmaconfig_introspectable_headers)
