/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             

/* Experimental: comment-out the following #define if a memcpy() call is
   slower than compiler-optimized memory copies for transfers of approx.
   64-256 bytes.

   FYI this #define is a win on Linux486/libc5.  Unbenchmarked on other
   architectures.  --adam */
#define MEMCPY_IS_NICE

#ifdef MEMCPY_IS_NICE
#include <string.h>
#endif

#include <stdarg.h>
#include "gimp.h"





#define TILE_WIDTH     _gimp_tile_width
#define TILE_HEIGHT    _gimp_tile_height
#define BOUNDS(a,x,y)  ((a < x) ? x : ((a > y) ? y : a))


typedef struct _GPixelRgnHolder    GPixelRgnHolder;
typedef struct _GPixelRgnIterator  GPixelRgnIterator;

struct _GPixelRgnHolder
{
  GPixelRgn *pr;
  guchar *original_data;
  int startx;
  int starty;
  int count;
};

struct _GPixelRgnIterator
{
  GSList *pixel_regions;
  int region_width;
  int region_height;
  int portion_width;
  int portion_height;
  int process_count;
};


static int      gimp_get_portion_width    (GPixelRgnIterator *pri);
static int      gimp_get_portion_height   (GPixelRgnIterator *pri);
static gpointer gimp_pixel_rgns_configure (GPixelRgnIterator *pri);
static void     gimp_pixel_rgn_configure  (GPixelRgnHolder   *prh,
					   GPixelRgnIterator *pri);


extern gint _gimp_tile_width;
extern gint _gimp_tile_height;


void
gimp_pixel_rgn_init (GPixelRgn *pr,
		     GDrawable *drawable,
		     int       x,
		     int       y,
		     int       width,
		     int       height,
		     int       dirty,
		     int       shadow)
{
  pr->data = NULL;
  pr->drawable = drawable;
  pr->bpp = drawable->bpp;
  pr->rowstride = pr->bpp * TILE_WIDTH;
  pr->x = x;
  pr->y = y;
  pr->w = width;
  pr->h = height;
  pr->dirty = dirty;
  pr->shadow = shadow;
}

void
gimp_pixel_rgn_resize (GPixelRgn *pr,
		       int       x,
		       int       y,
		       int       width,
		       int       height)
{
  if (pr->data != NULL)
    pr->data += ((y - pr->y) * pr->rowstride +
		 (x - pr->x) * pr->bpp);

  pr->x = x;
  pr->y = y;
  pr->w = width;
  pr->h = height;
}

void
gimp_pixel_rgn_get_pixel (GPixelRgn *pr,
			  guchar   *buf,
			  int       x,
			  int       y)
{
  GTile *tile;
  guchar *tile_data;
  int b;

  tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
  gimp_tile_ref (tile);

  tile_data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));

  for (b = 0; b < tile->bpp; b++)
    *buf++ = *tile_data++;

  gimp_tile_unref (tile, FALSE);
}

void
gimp_pixel_rgn_get_row (GPixelRgn *pr,
			guchar   *buf,
			int       x,
			int       y,
			int       width)
{
  GTile *tile;
  guchar *tile_data;
  int bpp, inc, min;
  int end;
  int boundary;
#ifndef MEMCPY_IS_NICE
  int b;
#endif

  end = x + width;

  while (x < end)
    {
      tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
      gimp_tile_ref (tile);

      tile_data = tile->data + (int)tile->bpp * (int)(tile->ewidth * (int)(y % TILE_HEIGHT) + (x % TILE_WIDTH));
      boundary = x + (tile->ewidth - (x % TILE_WIDTH));
      bpp = tile->bpp;

#ifdef MEMCPY_IS_NICE
      memcpy ((void *)buf,
	      (const void *)tile_data,
	      inc = (bpp *
		     ( (min = MIN(end,boundary)) -x) ) );
      x = min;;
      buf += inc;
#else
      for ( ; x < end && x < boundary; x++)
	{
	  for (b = 0; b < tile->bpp; b++)
	    *buf++ = tile_data[b];
	  tile_data += bpp;
	}
#endif /* MEMCPY_IS_NICE */

      gimp_tile_unref (tile, FALSE);
    }
}

void
gimp_pixel_rgn_get_col (GPixelRgn *pr,
			guchar   *buf,
			int       x,
			int       y,
			int       height)
{
  GTile *tile;
  guchar *tile_data;
  int inc;
  int end;
  int boundary;
  int b;

  end = y + height;

  while (y < end)
    {
      tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
      gimp_tile_ref (tile);

      tile_data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
      boundary = y + (tile->eheight - (y % TILE_HEIGHT));
      inc = tile->bpp * tile->ewidth;

      for ( ; y < end && y < boundary; y++)
	{
	  for (b = 0; b < tile->bpp; b++)
	    *buf++ = tile_data[b];
	  tile_data += inc;
	}

      gimp_tile_unref (tile, FALSE);
    }
}

void
gimp_pixel_rgn_get_rect (GPixelRgn *pr,
			 guchar   *buf,
			 int       x,
			 int       y,
			 int       width,
			 int       height)
{
  GTile *tile;
  guchar *src, *dest;
  gulong bufstride;
  int xstart, ystart;
  int xend, yend;
  int xboundary;
  int yboundary;
  int xstep, ystep;
  int ty, bpp;
#ifndef MEMCPY_IS_NICE
  int b, tx;
#endif

  bpp = pr->bpp;
  bufstride = bpp * width;

  xstart = x;
  ystart = y;
  xend = x + width;
  yend = y + height;
  ystep = 0;

  while (y < yend)
    {
      x = xstart;
      while (x < xend)
	{
	  tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
	  gimp_tile_ref (tile);

	  xstep = tile->ewidth - (x % TILE_WIDTH);
	  ystep = tile->eheight - (y % TILE_HEIGHT);
	  xboundary = x + xstep;
	  yboundary = y + ystep;
	  xboundary = MIN (xboundary, xend);
	  yboundary = MIN (yboundary, yend);

	  for (ty = y; ty < yboundary; ty++)
	    {
	      src = tile->data + tile->bpp * (tile->ewidth * (ty % TILE_HEIGHT) + (x % TILE_WIDTH));
	      dest = buf + bufstride * (ty - ystart) + bpp * (x - xstart);

#ifdef MEMCPY_IS_NICE
	      memcpy ((void *)dest, (const void *)src, (xboundary-x)*bpp);  
#else   
	      for (tx = x; tx < xboundary; tx++)
		{
		  for (b = 0; b < bpp; b++)
		    *dest++ = *src++;
		}
#endif /* MEMCPY_IS_NICE */

	    }

	  gimp_tile_unref (tile, FALSE);
	  x += xstep;
	}

      y += ystep;
    }
}

void
gimp_pixel_rgn_set_pixel (GPixelRgn *pr,
			  guchar   *buf,
			  int       x,
			  int       y)
{
  GTile *tile;
  guchar *tile_data;
  int b;

  tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
  gimp_tile_ref (tile);

  tile_data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));

  for (b = 0; b < tile->bpp; b++)
    *tile_data++ = *buf++;

  gimp_tile_unref (tile, TRUE);
}

void
gimp_pixel_rgn_set_row (GPixelRgn *pr,
			guchar   *buf,
			int       x,
			int       y,
			int       width)
{
  GTile *tile;
  guchar *tile_data;
  int inc, min;
  int end;
  int boundary;
#ifndef MEMCPY_IS_NICE
  int b;
#endif

  end = x + width;

  while (x < end)
    {
      tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
      gimp_tile_ref (tile);

      tile_data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
      boundary = x + (tile->ewidth - (x % TILE_WIDTH));

#ifdef MEMCPY_IS_NICE
      memcpy ((void *)tile_data,
	      (const void *)buf,
	      inc = (tile->bpp *
		     ( (min = MIN(end,boundary)) -x) ) );
      x = min;
      buf += inc;
#else
      for ( ; x < end && x < boundary; x++)
	{
	  for (b = 0; b < tile->bpp; b++)
	    *tile_data++ = *buf++;
	}
#endif /* MEMCPY_IS_NICE */

      gimp_tile_unref (tile, TRUE);
    }
}

void
gimp_pixel_rgn_set_col (GPixelRgn *pr,
			guchar   *buf,
			int       x,
			int       y,
			int       height)
{
  GTile *tile;
  guchar *tile_data;
  int inc;
  int end;
  int boundary;
  int b;

  end = y + height;

  while (y < end)
    {
      tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
      gimp_tile_ref (tile);

      tile_data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
      boundary = y + (tile->eheight - (y % TILE_HEIGHT));
      inc = tile->bpp * tile->ewidth;

      for ( ; y < end && y < boundary; y++)
	{
	  for (b = 0; b < tile->bpp; b++)
	    tile_data[b] = *buf++;
	  tile_data += inc;
	}

      gimp_tile_unref (tile, TRUE);
    }
}

void
gimp_pixel_rgn_set_rect (GPixelRgn *pr,
			 guchar   *buf,
			 int       x,
			 int       y,
			 int       width,
			 int       height)
{
  GTile *tile;
  guchar *src, *dest;
  gulong bufstride;
  int xstart, ystart;
  int xend, yend;
  int xboundary;
  int yboundary;
  int xstep, ystep;
  int ty, bpp;
#ifndef MEMCPY_IS_NICE
  int b, tx;
#endif

  bpp = pr->bpp;
  bufstride = bpp * width;

  xstart = x;
  ystart = y;
  xend = x + width;
  yend = y + height;
  ystep = 0;

  while (y < yend)
    {
      x = xstart;
      while (x < xend)
	{
	  tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
	  gimp_tile_ref (tile);

	  xstep = tile->ewidth - (x % TILE_WIDTH);
	  ystep = tile->eheight - (y % TILE_HEIGHT);
	  xboundary = x + xstep;
	  yboundary = y + ystep;
	  xboundary = MIN (xboundary, xend);
	  yboundary = MIN (yboundary, yend);

	  for (ty = y; ty < yboundary; ty++)
	    {
	      src = buf + bufstride * (ty - ystart) + bpp * (x - xstart);
	      dest = tile->data + tile->bpp * (tile->ewidth * (ty % TILE_HEIGHT) + (x % TILE_WIDTH));

#ifdef MEMCPY_IS_NICE
	      memcpy ((void *)dest, (const void *)src, (xboundary-x)*bpp); 
#else
	      for (tx = x; tx < xboundary; tx++)
		{
		  for (b = 0; b < bpp; b++)
		    *dest++ = *src++;
		}
#endif /* MEMCPY_IS_NICE */
	    }

	  gimp_tile_unref (tile, TRUE);
	  x += xstep;
	}

      y += ystep;
    }
}

gpointer
gimp_pixel_rgns_register (int nrgns,
			  ...)
{
  GPixelRgn *pr;
  GPixelRgnHolder *prh;
  GPixelRgnIterator *pri;
  va_list ap;
  int found;

  pri = g_new (GPixelRgnIterator, 1);
  pri->pixel_regions = NULL;
  pri->process_count = 0;

  if (nrgns < 1)
    return NULL;

  va_start (ap, nrgns);

  found = FALSE;
  while (nrgns --)
    {
      pr = va_arg (ap, GPixelRgn *);
      prh = g_new (GPixelRgnHolder, 1);
      prh->pr = pr;

      if (pr != NULL)
	{
	  /*  If there is a defined value for data, make sure tiles is NULL  */
	  if (pr->data)
	    pr->drawable = NULL;
	  prh->original_data = pr->data;
	  prh->startx = pr->x;
	  prh->starty = pr->y;
	  prh->pr->process_count = 0;

	  if (!found)
	    {
	      found = TRUE;
	      pri->region_width = pr->w;
	      pri->region_height = pr->h;
	    }
	}

      /*  Add the pixel Rgn holder to the list  */
      pri->pixel_regions = g_slist_prepend (pri->pixel_regions, prh);
    }

  va_end (ap);

  return gimp_pixel_rgns_configure (pri);
}

gpointer
gimp_pixel_rgns_process (gpointer pri_ptr)
{
  GSList *list;
  GPixelRgnHolder *prh;
  GPixelRgnIterator *pri;

  pri = (GPixelRgnIterator*) pri_ptr;
  pri->process_count++;

  /*  Unref all referenced tiles and increment the offsets  */

  list = pri->pixel_regions;
  while (list)
    {
      prh = (GPixelRgnHolder*) list->data;
      list = list->next;

      if ((prh->pr != NULL) && (prh->pr->process_count != pri->process_count))
	{
	  /*  This eliminates the possibility of incrementing the
	   *  same region twice
	   */
	  prh->pr->process_count++;

	  /*  Unref the last referenced tile if the underlying region is a tile manager  */
	  if (prh->pr->drawable)
	    {
	      GTile *tile = gimp_drawable_get_tile2 (prh->pr->drawable, prh->pr->shadow,
						    prh->pr->x, prh->pr->y);
	      gimp_tile_unref (tile, prh->pr->dirty);
	    }

	  prh->pr->x += pri->portion_width;

	  if ((prh->pr->x - prh->startx) >= pri->region_width)
	    {
	      prh->pr->x = prh->startx;
	      prh->pr->y += pri->portion_height;
	    }
	}
    }

  return gimp_pixel_rgns_configure (pri);
}


static int
gimp_get_portion_width (GPixelRgnIterator *pri)
{
  GSList *list;
  GPixelRgnHolder *prh;
  int min_width = G_MAXINT;
  int width;

  /* Find the minimum width to the next vertical tile (in the case of a tile manager)
   * or to the end of the pixel region (in the case of no tile manager)
   */

  list = pri->pixel_regions;
  while (list)
    {
      prh = (GPixelRgnHolder *) list->data;

      if (prh->pr)
        {
          /* Check if we're past the point of no return  */
          if ((prh->pr->x - prh->startx) >= pri->region_width)
            return 0;

          if (prh->pr->drawable)
            {
              width = TILE_WIDTH - (prh->pr->x % TILE_WIDTH);
              width = BOUNDS (width, 0, (pri->region_width - (prh->pr->x - prh->startx)));
            }
          else
            width = (pri->region_width - (prh->pr->x - prh->startx));

          if (width < min_width)
            min_width = width;
        }

      list = list->next;
    }

  return min_width;
}

static int
gimp_get_portion_height (GPixelRgnIterator *pri)
{
  GSList *list;
  GPixelRgnHolder *prh;
  int min_height = G_MAXINT;
  int height;

  /* Find the minimum height to the next vertical tile (in the case of a tile manager)
   * or to the end of the pixel region (in the case of no tile manager)
   */

  list = pri->pixel_regions;
  while (list)
    {
      prh = (GPixelRgnHolder *) list->data;

      if (prh->pr)
        {
          /* Check if we're past the point of no return  */
          if ((prh->pr->y - prh->starty) >= pri->region_height)
            return 0;

          if (prh->pr->drawable)
            {
              height = TILE_HEIGHT - (prh->pr->y % TILE_HEIGHT);
              height = BOUNDS (height, 0, (pri->region_height - (prh->pr->y - prh->starty)));
            }
          else
            height = (pri->region_height - (prh->pr->y - prh->starty));

          if (height < min_height)
            min_height = height;
        }

      list = list->next;
    }

  return min_height;
}

static gpointer
gimp_pixel_rgns_configure (GPixelRgnIterator *pri)
{
  GPixelRgnHolder *prh;
  GSList *list;

  /*  Determine the portion width and height  */
  pri->portion_width = gimp_get_portion_width (pri);
  pri->portion_height = gimp_get_portion_height (pri);

  if ((pri->portion_width == 0) ||
      (pri->portion_height == 0))
    {
      /*  free the pixel regions list  */
      if (pri->pixel_regions)
        {
          list = pri->pixel_regions;
          while (list)
            {
              g_free (list->data);
              list = list->next;
            }
          g_slist_free (pri->pixel_regions);
          g_free (pri);
        }

      return NULL;
    }

  pri->process_count++;

  list = pri->pixel_regions;
  while (list)
    {
      prh = (GPixelRgnHolder *) list->data;

      if ((prh->pr != NULL) && (prh->pr->process_count != pri->process_count))
        {
          prh->pr->process_count++;
          gimp_pixel_rgn_configure (prh, pri);
        }

      list = list->next;
    }

  return pri;
}

static void
gimp_pixel_rgn_configure (GPixelRgnHolder   *prh,
			  GPixelRgnIterator *pri)
{
  /* Configure the rowstride and data pointer for the pixel region
   * based on the current offsets into the region and whether the
   * region is represented by a tile manager or not
   */
  if (prh->pr->drawable)
    {
      GTile *tile;
      int offx, offy;

      tile = gimp_drawable_get_tile2 (prh->pr->drawable, prh->pr->shadow, prh->pr->x, prh->pr->y);
      gimp_tile_ref (tile);

      offx = prh->pr->x % TILE_WIDTH;
      offy = prh->pr->y % TILE_HEIGHT;

      prh->pr->rowstride = tile->ewidth * prh->pr->bpp;
      prh->pr->data = tile->data + offy * prh->pr->rowstride + offx * prh->pr->bpp;
    }
  else
    {
      prh->pr->data = (prh->original_data +
		       (prh->pr->y - prh->starty) * prh->pr->rowstride +
		       (prh->pr->x - prh->startx) * prh->pr->bpp);
    }

  prh->pr->w = pri->portion_width;
  prh->pr->h = pri->portion_height;
}
