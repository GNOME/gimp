## Process this file with automake to produce Makefile.in


## When updating gimp-print, do not forget to update 
## GIMP_PRINT_VERSION and GIMP_PRINT_RELEASE_DATE 
## in the toplevel configure.in !!


libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = print-printers.c

libexec_PROGRAMS = print

bin_PROGRAMS = escputil-1.2

print_SOURCES = \
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
	print_gimp.h

escputil_1_2_SOURCES = escputil.c

man_MANS = escputil-1.2.1

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"		\
	-DVERSION=\"$(GIMP_PRINT_VERSION)\"	\
	-DRELEASE_DATE=\"$(GIMP_PRINT_RELEASE_DATE)\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

escputil_1_2_LDADD =

if DEFAULT_BINARY
install-exec-hook:
	cd $(DESTDIR)$(bindir) \
	&& rm -f escputil \
	&& $(LN_S) escputil-1.2 escputil

install-data-hook:
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

uninstall-local:
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

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
