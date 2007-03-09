## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	stock-circle.png	\
	stock-coord.png		\
	stock-dimension.png	\
	stock-java.png		\
	stock-polygon.png	\
	stock-rectangle.png	\
	stock-to-back.png	\
	stock-to-front.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = imap-stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/imap-stock-pixbufs.h: stock-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat stock-icons.list` > $(@F)
