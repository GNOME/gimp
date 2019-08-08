# Introspectable sources for libgimpmodule

libgimpmodule_introspectable_headers =	\
	../libgimpmodule/gimpmoduletypes.h	\
	../libgimpmodule/gimpmodule.h	\
	../libgimpmodule/gimpmoduledb.h

libgimpmodule_introspectable =	\
	../libgimpmodule/gimpmodule.c	\
	../libgimpmodule/gimpmoduledb.c	\
	$(libgimpmodule_introspectable_headers)
