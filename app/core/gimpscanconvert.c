/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include "scan_convert.h"
#include "libgimp/gimpmath.h"


#ifdef DEBUG
#define TRC(x) printf x
#else
#define TRC(x)
#endif


/* Reveal our private structure */
struct ScanConverterPrivate
{
    guint    width;
    guint    height;
    GSList **scanlines;		/* array of height*antialias scanlines */

    guint    antialias;		/* how much to oversample by */

    /* record the first and last points so we can close the curve */
    gboolean got_first;
    ScanConvertPoint first;
    gboolean got_last;
    ScanConvertPoint last;
};



/* Local helper routines to scan convert the polygon */

static GSList *
insert_into_sorted_list (GSList *list,
			 int     x)
{
  GSList *orig = list;
  GSList *rest;

  if (!list)
    return g_slist_prepend (list, GINT_TO_POINTER (x));

  while (list)
    {
      rest = g_slist_next (list);
      if (x < GPOINTER_TO_INT (list->data))
	{
	  rest = g_slist_prepend (rest, list->data);
	  list->next = rest;
	  list->data = GINT_TO_POINTER (x);
	  return orig;
	}
      else if (!rest)
	{
	  g_slist_append (list, GINT_TO_POINTER (x));
	  return orig;
	}
      list = g_slist_next (list);
    }

  return orig;
}


static void
convert_segment (ScanConverter *sc,
		 int      x1,
		 int      y1,
		 int      x2,
		 int      y2)
{
    int ydiff, y, tmp;
    gint width;
    gint height;
    GSList **scanlines;
    float xinc, xstart;

    /* pre-calculate invariant commonly used values */
    width = sc->width * sc->antialias;
    height = sc->height * sc->antialias;
    scanlines = sc->scanlines;    

    x1 = CLAMP (x1, 0, width - 1);
    y1 = CLAMP (y1, 0, height - 1);
    x2 = CLAMP (x2, 0, width - 1);
    y2 = CLAMP (y2, 0, height - 1);

    if (y1 > y2)
    {
	tmp = y2; y2 = y1; y1 = tmp;
	tmp = x2; x2 = x1; x1 = tmp;
    }

    ydiff = (y2 - y1);

    if (ydiff)
    {
	xinc = (float) (x2 - x1) / (float) ydiff;
	xstart = x1 + 0.5 * xinc;
	for (y = y1 ; y < y2; y++)
	{
	  scanlines[y] = insert_into_sorted_list (scanlines[y], ROUND (xstart));
	  xstart += xinc;
	}
    }
    else
    {
	/* horizontal line */
	scanlines[y1] = insert_into_sorted_list (scanlines[y1], ROUND (x1));
	scanlines[y1] = insert_into_sorted_list (scanlines[y1], ROUND (x2));
    }
}



/**************************************************************/
/* Exported functions */


/* Create a new scan conversion context */
ScanConverter *
scan_converter_new (guint width, guint height, guint antialias)
{
    ScanConverter *sc;

    g_return_val_if_fail (width > 0, NULL);
    g_return_val_if_fail (height > 0, NULL);
    g_return_val_if_fail (antialias > 0, NULL);

    sc = g_new0 (ScanConverter, 1);

    sc->antialias = antialias;
    sc->width  = width;
    sc->height = height;
    sc->scanlines = g_new0 (GSList *, height*antialias);

    return sc;
}


/* Add "npoints" from "pointlist" to the polygon currently being
 * described by "scan_converter".  */
void
scan_converter_add_points (ScanConverter *sc,
			   guint npoints,
			   ScanConvertPoint *pointlist)
{
    int i;
    guint antialias;

    g_return_if_fail (sc != NULL);
    g_return_if_fail (pointlist != NULL);

    antialias = sc->antialias;

    if (!sc->got_first && npoints > 0)
    {
	sc->got_first = TRUE;
	sc->first = pointlist[0];
    }

    /* link from previous point */
    if (sc->got_last && npoints > 0)
    {
	TRC (("|| %g,%g -> %g,%g\n",
	      sc->last.x, sc->last.y,
	      pointlist[0].x, pointlist[0].y));
	convert_segment (sc,
			 (int)sc->last.x * antialias,
			 (int)sc->last.y * antialias,
			 (int)pointlist[0].x * antialias,
			 (int)pointlist[0].y * antialias);
    }

    for (i = 0; i < (npoints - 1); i++)
    {
	convert_segment (sc,
			 (int) pointlist[i].x * antialias,
			 (int) pointlist[i].y * antialias,
			 (int) pointlist[i + 1].x * antialias,
			 (int) pointlist[i + 1].y * antialias);
    }

    TRC (("[] %g,%g -> %g,%g\n",
	  pointlist[0].x, pointlist[0].y,
	  pointlist[npoints-1].x, pointlist[npoints-1].y));

    if (npoints > 0)
    {
	sc->got_last = TRUE;
	sc->last = pointlist[npoints - 1];
    }
}



/* Scan convert the polygon described by the list of points passed to
 * scan_convert_add_points, and return a channel with a bits set if
 * they fall within the polygon defined.  The polygon is filled
 * according to the even-odd rule.  The polygon is closed by
 * joining the final point to the initial point. */
Channel *
scan_converter_to_channel (ScanConverter *sc,
			   GimpImage *gimage)
{
    Channel *mask;
    GSList *list;
    PixelRegion maskPR;
    guint widtha;
    guint heighta;
    guint antialias;
    guint antialias2;
    unsigned char *buf, *b;
    int * vals, val;
    int x, x2, w;
    int i, j;

    antialias = sc->antialias;
    antialias2 = antialias * antialias;

    /*  do we need to close the polygon? */
    if (sc->got_first && sc->got_last &&
	(sc->first.x != sc->last.x || sc->first.y != sc->last.y))
    {
	convert_segment (sc,
			 (int) sc->last.x * antialias,
			 (int) sc->last.y * antialias,
			 (int) sc->first.x * antialias,
			 (int) sc->first.y * antialias);
    }

    mask = channel_new_mask (gimage, sc->width, sc->height);

    buf = g_new0 (unsigned char, sc->width);
    widtha  = sc->width * antialias;
    heighta = sc->height * antialias;
    /* allocate value array  */
    vals = g_new (int, widtha);

    /* dump scanlines */
    for(i=0; i<heighta; i++)
    {
	list = sc->scanlines[i];
	TRC (("%03d: ", i));
	while (list)
	{
	    TRC (("%3d ", GPOINTER_TO_INT (list->data)));
	    list = g_slist_next (list);
	}
	TRC (("\n"));
    }

    pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 0, 0, 
		       drawable_width (GIMP_DRAWABLE(mask)), 
		       drawable_height (GIMP_DRAWABLE(mask)), TRUE);

    for (i = 0; i < heighta; i++)
    {
	list = sc->scanlines[i];

	/*  zero the vals array  */
	if (!(i % antialias))
	    memset (vals, 0, widtha * sizeof (int));

	while (list)
	{
	    x = GPOINTER_TO_INT (list->data);
	    list = g_slist_next (list);
	    if (!list)
	    {
		g_message ("Cannot properly scanline convert polygon!\n");
	    }
	    else
	    {
		/*  bounds checking  */
		x = CLAMP (x, 0, widtha);
		x2 = CLAMP (GPOINTER_TO_INT (list->data), 0, widtha);

		w = x2 - x;

		if (w > 0)
		{
		    if (antialias == 1)
		    {
			channel_add_segment (mask, x, i, w, 255);
		    }
		    else
		    {
			for (j = 0; j < w; j++)
			    vals[j + x] += 255;
		    }
		}
		list = g_slist_next (list);
	    }
	}

	if (antialias != 1 && !((i+1) % antialias))
	{
	    b = buf;
	    for (j = 0; j < widtha; j += antialias)
	    {
		val = 0;
		for (x = 0; x < antialias; x++)
		    val += vals[j + x];

	      *b++ = (unsigned char) (val / antialias2);
	    }

	  pixel_region_set_row (&maskPR, 0, (i / antialias), sc->width, buf);
	}
    }

    g_free (vals);
    g_free (buf);

    return mask;
}


void
scan_converter_free (ScanConverter *sc)
{
    g_free (sc->scanlines);
    g_free (sc);
}


/* End of scan_convert.c */
