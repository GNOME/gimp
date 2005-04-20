## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	stock-bezier.png	\
	stock-circle.png	\
	stock-copy-object.png	\
	stock-curve.png		\
	stock-delete-object.png	\
	stock-ellipse.png	\
	stock-line.png		\
	stock-move-object.png	\
	stock-move-point.png	\
	stock-polygon.png	\
	stock-rectangle.png	\
	stock-select-object.png \
	stock-show-all.png      \
	stock-spiral.png	\
	stock-star.png		\
	stock-logo.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = gfig-stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/gfig-stock-pixbufs.h: stock-icons.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat stock-icons.list` > $(@F)
