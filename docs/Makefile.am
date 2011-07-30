## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	Wilber.xcf.gz			\
	Wilber.xcf.gz.README		\
	Wilber_Construction_Kit.xcf.gz

mans = \
	gimp-$(GIMP_APP_VERSION).1	\
	gimprc-$(GIMP_APP_VERSION).5

man_MANS = \
	$(mans)				\
	gimptool-$(GIMP_TOOL_VERSION).1

default_binary_mans = $(mans)
if ENABLE_GIMP_CONSOLE
default_binary_mans += gimp-console-$(GIMP_APP_VERSION).1
endif

install-data-hook:
if ENABLE_GIMP_CONSOLE
	cd $(DESTDIR)$(mandir)/man1; \
	rm -f gimp-console-$(GIMP_APP_VERSION).1; \
	ln -s gimp-$(GIMP_APP_VERSION).1 gimp-console-$(GIMP_APP_VERSION).1
endif
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
if ENABLE_GIMP_CONSOLE
	rm -f $(DESTDIR)$(mandir)/man1/gimp-console-$(GIMP_APP_VERSION).1
endif
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
