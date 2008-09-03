## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	color-rotate-360.png	\
	color-rotate-a-b.png	\
	color-rotate-ccw.png	\
	color-rotate-cw.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = color-rotate-stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/color-rotate-stock-pixbufs.h: stock-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat stock-icons.list` > $(@F)
