## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=libgimp

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libgimp-docs.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(top_srcdir)/libgimp

CFLAGS=`gimptool --cflags`
LDFLAGS=`gimptool --libs`

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

libgimp_docdir = $(HTML_DIR)

EXTRA_DIST =				\
	$(DOC_MAIN_SGML_FILE) 		\
	$(DOC_MODULE).hierarchy		\
	$(DOC_MODULE).signals		\
	$(DOC_MODULE).types		\
	$(DOC_MODULE)-include.c		\
	$(DOC_MODULE)-decl.txt		\
	$(DOC_MODULE)-sections.txt

if ENABLE_GTK_DOC
html/index.html: sgml/libgimp-doc.bottom
	$(MAKE) html
else
html/index.html:
endif

sgml/libgimp-doc.bottom: $(tmpl_sources)
	$(MAKE) sgml

scan:
	-(cd $(srcdir) \
	&& env CFLAGS="$(CFLAGS) libgimp-include.c" CPPFLAGS="$(CPPFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		gtkdoc-scanobj --module=$(DOC_MODULE) \
	&& gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) \
		--ignore-headers="gimpintl.h libgimp-intl.h stdplugins-intl.h gserialize.h")

templates: scan
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	cd $(srcdir) \
	&& gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

html:
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	-cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

clean-local:
	rm -f *~ *.bak *-unused.txt

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt 

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	  echo '-- Fixing Crossreferences' ; \
	  gtkdoc-fixxref --module=$(DOC_MODULE) --html-dir=$(HTML_DIR)|| true; \
	fi)

dist-hook:
	mkdir $(distdir)/html
	mkdir $(distdir)/sgml
	mkdir $(distdir)/tmpl
	-cp $(srcdir)/html/*.html $(srcdir)/html/*.css $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/sgml/*.bottom $(srcdir)/sgml/*.top $(distdir)/sgml

.PHONY : html sgml templates scan

# devel-docs/libgimp/Makefile.am ends here



