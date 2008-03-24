## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	stock-intensity-ambient-high.png	\
	stock-intensity-ambient-low.png		\
	stock-intensity-diffuse-high.png	\
	stock-intensity-diffuse-low.png		\
	stock-reflectivity-diffuse-high.png	\
	stock-reflectivity-diffuse-low.png	\
	stock-reflectivity-specular-high.png	\
	stock-reflectivity-specular-low.png	\
	stock-reflectivity-highlight-high.png	\
	stock-reflectivity-highlight-low.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/stock-pixbufs.h: stock-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat stock-icons.list` > $(@F)
