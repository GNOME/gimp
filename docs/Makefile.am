## Process this file with automake to produce Makefile.in

man_MANS = \
	gimp-$(GIMP_APP_VERSION).1		\
	gimptool-$(GIMP_TOOL_VERSION).1		\
	gimprc-$(GIMP_APP_VERSION).5		\
	gimp-remote-$(GIMP_APP_VERSION).1

EXTRA_DIST = \
	gimp-$(GIMP_APP_VERSION).1.in		\
	gimprc-$(GIMP_APP_VERSION).5.in		\
	gimptool-$(GIMP_TOOL_VERSION).1.in	\
	gimp-remote-$(GIMP_API_VERSION).1.in	\
	Wilber.svg				\
	Wilber.xcf.gz				\
	Wilber.xcf.gz.README			\
	Wilber_Construction_Kit.xcf.gz		\
	gimp.txt				\
	keybindings.txt 			\
	quick_reference.ps 			\
	quick_reference.tar.gz

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
	@list='$(man_MANS)'; \
	for i in $$list; do \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`; \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`; \
	  f="$$n.$$s"; \
	  echo " rm -f $(DESTDIR)$(mandir)/man$$s/$$f"; \
	  rm -f $(DESTDIR)$(mandir)/man$$s/$$f; \
	done
endif
