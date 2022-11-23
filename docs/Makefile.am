## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	Wilber.xcf.gz			\
	Wilber.xcf.gz.README		\
	Wilber_Construction_Kit.xcf.gz

mans = \
	ligma-$(LIGMA_APP_VERSION).1	\
	ligmarc-$(LIGMA_APP_VERSION).5

man_MANS = \
	$(mans)				\
	ligmatool-$(LIGMA_TOOL_VERSION).1

default_binary_mans = $(mans)
if ENABLE_LIGMA_CONSOLE
default_binary_mans += ligma-console-$(LIGMA_APP_VERSION).1
endif

install-data-hook:
if ENABLE_LIGMA_CONSOLE
	cd $(DESTDIR)$(mandir)/man1; \
	rm -f ligma-console-$(LIGMA_APP_VERSION).1; \
	ln -s ligma-$(LIGMA_APP_VERSION).1 ligma-console-$(LIGMA_APP_VERSION).1
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
if ENABLE_LIGMA_CONSOLE
	rm -f $(DESTDIR)$(mandir)/man1/ligma-console-$(LIGMA_APP_VERSION).1
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
