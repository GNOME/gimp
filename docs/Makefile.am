## Process this file with automake to produce Makefile.in

# Don't install any man-pages for now. They need to be updated and
# would clash with older gimp installations.

man_MANS = gimp-1.3.1 gimptool-1.3.1 gimprc-1.3.5 gimp-remote-1.3.1

EXTRA_DIST = \
	gimp-1.3.1.in			\
	gimptool-1.3.1.in		\
	gimp-remote-1.3.1.in		\
	gimprc-1.3.5.in			\
	Wilber.svg			\
	Wilber.xcf.gz			\
	Wilber.xcf.gz.README		\
	Wilber_Construction_Kit.xcf.gz	\
	cheat_sheet.txt			\
	gimp.txt			\
	keybindings.txt 		\
	quick_reference.ps 		\
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

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
