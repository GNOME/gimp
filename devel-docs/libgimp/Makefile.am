## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=libgimp

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libgimp-docs.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(top_srcdir)/libgimp

CFLAGS=`gimptool --cflags`
LDFLAGS=`gimptool --libs`

HTML_DIR=$(gimpdatadir)/devel-docs/html

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

tmpl_sources = 				\
	tmpl/color_display.sgml		\
	tmpl/color_selector.sgml	\
	tmpl/gimp.sgml			\
	tmpl/gimpchainbutton.sgml	\
	tmpl/gimpcolorbutton.sgml	\
	tmpl/gimpcolorspace.sgml	\
	tmpl/gimpcompat.sgml		\
	tmpl/gimpdialog.sgml		\
	tmpl/gimpenums.sgml		\
	tmpl/gimpenv.sgml		\
	tmpl/gimpexport.sgml		\
	tmpl/gimpfeatures.sgml		\
	tmpl/gimpfileselection.sgml	\
	tmpl/gimphelpui.sgml		\
	tmpl/gimplimits.sgml		\
	tmpl/gimpmath.sgml		\
	tmpl/gimpmatrix.sgml		\
	tmpl/gimpmenu.sgml		\
	tmpl/gimpmodule.sgml		\
	tmpl/gimppatheditor.sgml	\
	tmpl/gimpprotocol.sgml		\
	tmpl/gimpsizeentry.sgml		\
	tmpl/gimpui.sgml		\
	tmpl/gimpunit.sgml		\
	tmpl/gimpunitmenu.sgml		\
	tmpl/gimpwidgets.sgml		\
	tmpl/gimpwire.sgml		\
	tmpl/gserialize.sgml		\
	tmpl/libgimp-unused.sgml	\
	tmpl/parasite.sgml		\
	tmpl/parasiteF.sgml		\
	tmpl/parasiteP.sgml		\
	tmpl/parasiteio.sgml

libgimp_docdir = $(HTML_DIR)

EXTRA_DIST =				\
	libgimp.hierarchy		\
	libgimp.types			\
	libgimp-include.c		\
	libgimp-decl.txt		\
	libgimp-sections.txt

if ENABLE_GTK_DOC
html/book1.html: sgml/libgimp-doc.bottom
	$(MAKE) html
else
html/book1.html:
endif

sgml/libgimp-doc.bottom: $(tmpl_sources)
	$(MAKE) sgml

scan:
	-(cd $(srcdir) \
	&& env CFLAGS="$(CFLAGS) libgimp-include.c" LDFLAGS=$(LDFLAGS) \
		gtkdoc-scanobj --module=$(DOC_MODULE) \
	&& gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) \
		--ignore-headers="gimpintl.h libgimp-intl.h stdplugins-intl.h" )

templates: scan
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	cd $(srcdir) \
	&& gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

html:
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	-cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

clean-local:
	rm -f *~ *.bak *.signals *-unused.txt

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



































