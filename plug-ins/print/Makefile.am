## Process this file with automake to produce Makefile.in


## When updating gimp-print, do not forget to update 
## GIMP_PRINT_VERSION and GIMP_PRINT_RELEASE_DATE 
## in the toplevel configure.in !!


libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = escputil-1.3

print_SOURCES = @STRIP_BEGIN@ \
	print-canon.c		\
	print-dither.c		\
	print-escp2.c		\
	print-intl.h		\
	print-pcl.c		\
	print-ps.c		\
	print-util.c		\
	print-weave.c		\
	print.h			\
	quickmatrix257.h	\
	ran.367.179.h		\
	print-image-gimp.c	\
	print.c			\
	gimp_color_window.c	\
	gimp_main_window.c	\
	print_gimp.h		\
@STRIP_END@

escputil_1_3_SOURCES = escputil.c

man_MANS = escputil-1.3.1

EXTRA_DIST = print-printers.c

AM_CPPFLAGS = @STRIP_BEGIN@ \
        -DLOCALEDIR=\""$(localedir)"\"			\
	-DVERSION=\"$(GIMP_PRINT_VERSION)\"		\
	-DRELEASE_DATE=\"$(GIMP_PRINT_RELEASE_DATE)\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@

escputil_1_3_LDADD =

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f escputil \
	&& $(LN_S) escputil-1.3 escputil
endif

install-data-hook:
if DEFAULT_BINARY
	@list='$(man_MANS)'; \
	for i in $$list; do \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`; \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`; \
	  f="$$n.$$s"; \
	  echo "cd $(DESTDIR)$(mandir)/man$$s"; \
	  cd $(DESTDIR)$(mandir)/man$$s; \
	  echo "rm -f $$f"; \
	  rm -f $$f; \
	  echo "ln -s $$i $$f"; \
	  ln -s $$i $$f; \
	done
endif

uninstall-local:
if DEFAULT_BINARY
	@echo " rm -f $(DESTDIR)$(bindir)/escputil"; \
	rm -f $(DESTDIR)$(bindir)/escputil
	@list='$(man_MANS)'; \
	for i in $$list; do \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`; \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`; \
	  f="$$n.$$s"; \
	  echo " rm -f $(DESTDIR)$(mandir)/man$$s/$$f"; \
	  rm -f $(DESTDIR)$(mandir)/man$$s/$$f; \
	done
endif
