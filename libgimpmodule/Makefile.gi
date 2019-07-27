# Introspectable sources for libgimpmodule

libgimpmodule_introspectable_headers =	\
	$(top_srcdir)/libgimpmodule/gimpmoduletypes.h	\
	$(top_srcdir)/libgimpmodule/gimpmodule.h	\
	$(top_srcdir)/libgimpmodule/gimpmoduledb.h

libgimpmodule_introspectable =	\
	$(top_srcdir)/libgimpmodule/gimpmodule.c	\
	$(top_srcdir)/libgimpmodule/gimpmoduledb.c	\
	$(libgimpmodule_introspectable_headers)
