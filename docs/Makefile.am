## Process this file with automake to produce Makefile.in

default_binary_mans = \
	gimp-$(GIMP_APP_VERSION).1		\
	gimprc-$(GIMP_APP_VERSION).5		\
	gimp-remote-$(GIMP_APP_VERSION).1

man_MANS = \
	$(default_binary_mans)			\
	gimptool-$(GIMP_TOOL_VERSION).1

EXTRA_DIST = \
	Wilber.svg				\
	Wilber.xcf.gz				\
	Wilber.xcf.gz.README			\
	Wilber_Construction_Kit.xcf.gz		\
	keybindings.txt 			\
	quick_reference.ps 			\
	quick_reference.tar.gz

install-data-hook:
if DEFAULT_BINARY
	@list='$(default_binary_mans)'; \
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
	@list='$(default_binary_mans)'; \
	for i in $$list; do \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`; \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`; \
	  f="$$n.$$s"; \
	  echo " rm -f $(DESTDIR)$(mandir)/man$$s/$$f"; \
	  rm -f $(DESTDIR)$(mandir)/man$$s/$$f; \
	done
endif
