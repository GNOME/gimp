REF_VERSION = 1.1 
FULL_VERSION = $(REF_VERSION).1

CSS_FILES = style.css

HTML_FILES =                      \
	html/class-drawable.html      \
	html/class-layer.html         \
	html/class-channel.html       \
	html/class-image.html         \
	html/class-pixel-rgn.html     \
	html/class-pixel-fetcher.html \
	html/class-parasite.html      \
	html/index.html               \
	html/index.sgml


XML_FILES = 					      \
	reference/pygimp-drawable.xml     \
	reference/pygimp-channel.xml      \
	reference/pygimp-image.xml        \
	reference/pygimp-layer.xml        \
	reference/pygimp-pixel-rgn.xml    \
	reference/pygimp-pixel-fetcher.xml
	reference/pygimp-parasite.xml

XSL_FILES =			\
	xsl/common.xsl 		\
	xsl/devhelp.xsl		\
	xsl/html.xsl 		\
	xsl/pdf-style.xsl 	\
	xsl/pdf.xsl		\
	xsl/ref-html-style.xsl

# fixxref cannot be included in XSL_FILES, because automake is touching it
# so the date is newer then the files in HTML_FILES
FIXXREF = xsl/fixxref.py

TARGET_DIR = $(datadir)/gtk-doc/html/pygimp
CSSdir = $(TARGET_DIR)
CSS_DATA = html/$(CSS_FILES)


install-data-local:
	installfiles=`echo $(srcdir)/html/*`; \
	if test "$$installfiles" = '$(srcdir)/html/*'; \
	then echo '-- Nothing to install' ; \
	else \
	  $(mkinstalldirs) $(DESTDIR)$(TARGET_DIR); \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR) || :; \
	fi

uninstall-local:
	rm -f $(DESTDIR)$(TARGET_DIR)/*


XSLdir = $(datadir)/pygimp/xsl
XSL_DATA = $(XSL_FILES) $(FIXXREF)

EXTRA_DIST = 		\
	$(HTML_FILES) 	\
	$(XML_FILES) 	\
	$(XSL_FILES) 	\
	$(FIXXREF) 	\
	$(CSS_FILES)

reference/builddate.xml:
	@$(PYTHON) -c 'import datetime; print datetime.date.today()' > $@



BUILT_SOURCES = reference/builddate.xml $(HTML_FILES)
CLEANFILES = reference/builddate.xml $(HTML_FILES)


# Do not add a builddate.xml dependency here, because the generated html 
# documentation will be included in the tarball so xsltproc is not needed to 
# install the documentation. Adding a builddate.xml dependency will force 
# the documentation to be rebuilt when it doesn't need to be.
$(HTML_FILES): $(XML_FILES) $(XSL_FILES)
	@xsltproc --nonet --xinclude -o ./html/ \
		 --path $(BUILDDIR)/reference:$(srcdir)/reference \
                 --stringparam gtkdoc.bookname "pygimp" \
                 --stringparam gtkdoc.version ${REF_VERSION} \
                 $(srcdir)/xsl/ref-html-style.xsl \
                 $(srcdir)/reference/pygimp-ref.xml

