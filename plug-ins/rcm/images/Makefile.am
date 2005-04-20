## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	rcm-360.png	\
	rcm-a-b.png	\
	rcm-ccw.png	\
	rcm-cw.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = rcm-stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/rcm-stock-pixbufs.h: stock-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat stock-icons.list` > $(@F)
