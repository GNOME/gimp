/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpregioniterator.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "gimp.h"
#include "gimpregioniterator.h"


/**
 * SECTION: gimpregioniterator
 * @title: gimpregioniterator
 * @short_description: Functions to traverse a pixel regions.
 *
 * The GimpRgnIterator functions provide a variety of common ways to
 * traverse a PixelRegion, using a pre-defined function pointer per
 * pixel.
 **/


struct _GimpRgnIterator
{
  GimpDrawable *drawable;
  gint          x1;
  gint          y1;
  gint          x2;
  gint          y2;
};


static void  gimp_rgn_iterator_iter_single (GimpRgnIterator    *iter,
                                            GimpPixelRgn       *srcPR,
                                            GimpRgnFuncSrc      func,
                                            gpointer            data);
static void  gimp_rgn_render_row           (const guchar       *src,
                                            guchar             *dest,
                                            gint                col,
                                            gint                bpp,
                                            GimpRgnFunc2        func,
                                            gpointer            data);
static void  gimp_rgn_render_region        (const GimpPixelRgn *srcPR,
                                            const GimpPixelRgn *destPR,
                                            GimpRgnFunc2        func,
                                            gpointer            data);


/**
 * gimp_rgn_iterator_new:
 * @drawable: a #GimpDrawable
 * @unused:   ignored
 *
 * Creates a new #GimpRgnIterator for @drawable. The #GimpRunMode
 * parameter is ignored. Use gimp_rgn_iterator_free() to free this
 * iterator.
 *
 * Return value: a newly allocated #GimpRgnIterator.
 **/
GimpRgnIterator *
gimp_rgn_iterator_new (GimpDrawable *drawable,
                       GimpRunMode   unused)
{
  GimpRgnIterator *iter;

  g_return_val_if_fail (drawable != NULL, NULL);

  iter = g_slice_new (GimpRgnIterator);

  iter->drawable = drawable;

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &iter->x1, &iter->y1,
                             &iter->x2, &iter->y2);

  return iter;
}

/**
 * gimp_rgn_iterator_free:
 * @iter: a #GimpRgnIterator
 *
 * Frees the resources allocated for @iter.
 **/
void
gimp_rgn_iterator_free (GimpRgnIterator *iter)
{
  g_return_if_fail (iter != NULL);

  g_slice_free (GimpRgnIterator, iter);
}

void
gimp_rgn_iterator_src (GimpRgnIterator *iter,
                       GimpRgnFuncSrc   func,
                       gpointer         data)
{
  GimpPixelRgn srcPR;

  g_return_if_fail (iter != NULL);

  gimp_pixel_rgn_init (&srcPR, iter->drawable,
                       iter->x1, iter->y1,
                       iter->x2 - iter->x1, iter->y2 - iter->y1,
                       FALSE, FALSE);
  gimp_rgn_iterator_iter_single (iter, &srcPR, func, data);
}

void
gimp_rgn_iterator_src_dest (GimpRgnIterator    *iter,
                            GimpRgnFuncSrcDest  func,
                            gpointer            data)
{
  GimpPixelRgn  srcPR, destPR;
  gint          x1, y1, x2, y2;
  gint          bpp;
  gint          count;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;

  g_return_if_fail (iter != NULL);

  x1 = iter->x1;
  y1 = iter->y1;
  x2 = iter->x2;
  y2 = iter->y2;

  total_area  = (x2 - x1) * (y2 - y1);
  area_so_far = 0;

  gimp_pixel_rgn_init (&srcPR, iter->drawable, x1, y1, x2 - x1, y2 - y1,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, iter->drawable, x1, y1, x2 - x1, y2 - y1,
                       TRUE, TRUE);

  bpp = srcPR.bpp;

  for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR), count = 0;
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr), count++)
    {
      const guchar *src  = srcPR.data;
      guchar       *dest = destPR.data;
      gint          y;

      for (y = srcPR.y; y < srcPR.y + srcPR.h; y++)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          x;

          for (x = srcPR.x; x < srcPR.x + srcPR.w; x++)
            {
              func (x, y, s, d, bpp, data);

              s += bpp;
              d += bpp;
            }

          src  += srcPR.rowstride;
          dest += destPR.rowstride;
        }

      area_so_far += srcPR.w * srcPR.h;

      if ((count % 16) == 0)
        gimp_progress_update ((gdouble) area_so_far / (gdouble) total_area);
    }

  gimp_drawable_flush (iter->drawable);
  gimp_drawable_merge_shadow (iter->drawable->drawable_id, TRUE);
  gimp_drawable_update (iter->drawable->drawable_id,
                        x1, y1, x2 - x1, y2 - y1);
}

void
gimp_rgn_iterator_dest (GimpRgnIterator *iter,
                        GimpRgnFuncDest  func,
                        gpointer         data)
{
  GimpPixelRgn destPR;

  g_return_if_fail (iter != NULL);

  gimp_pixel_rgn_init (&destPR, iter->drawable,
                       iter->x1, iter->y1,
                       iter->x2 - iter->x1, iter->y2 - iter->y1,
                       TRUE, TRUE);
  gimp_rgn_iterator_iter_single (iter, &destPR, (GimpRgnFuncSrc) func, data);

  /*  update the processed region  */
  gimp_drawable_flush (iter->drawable);
  gimp_drawable_merge_shadow (iter->drawable->drawable_id, TRUE);
  gimp_drawable_update (iter->drawable->drawable_id,
                        iter->x1, iter->y1,
                        iter->x2 - iter->x1, iter->y2 - iter->y1);
}


void
gimp_rgn_iterate1 (GimpDrawable *drawable,
                   GimpRunMode   unused,
                   GimpRgnFunc1  func,
                   gpointer      data)
{
  GimpPixelRgn  srcPR;
  gint          x1, y1, x2, y2;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;
  gint          count;

  g_return_if_fail (drawable != NULL);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  total_area  = (x2 - x1) * (y2 - y1);
  area_so_far = 0;

  if (total_area <= 0)
    return;

  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &srcPR), count = 0;
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr), count++)
    {
      const guchar *src = srcPR.data;
      gint          y;

      for (y = 0; y < srcPR.h; y++)
        {
          const guchar *s = src;
          gint          x;

          for (x = 0; x < srcPR.w; x++)
            {
              func (s, srcPR.bpp, data);
              s += srcPR.bpp;
            }

          src += srcPR.rowstride;
        }

      area_so_far += srcPR.w * srcPR.h;

      if ((count % 16) == 0)
        gimp_progress_update ((gdouble) area_so_far / (gdouble) total_area);
    }
}

void
gimp_rgn_iterate2 (GimpDrawable *drawable,
                   GimpRunMode   unused,
                   GimpRgnFunc2  func,
                   gpointer      data)
{
  GimpPixelRgn  srcPR, destPR;
  gint          x1, y1, x2, y2;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;
  gint          count;

  g_return_if_fail (drawable != NULL);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  total_area  = (x2 - x1) * (y2 - y1);
  area_so_far = 0;

  if (total_area <= 0)
    return;

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
                       TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR), count = 0;
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr), count++)
    {
      gimp_rgn_render_region (&srcPR, &destPR, func, data);

      area_so_far += srcPR.w * srcPR.h;

      if ((count % 16) == 0)
        gimp_progress_update ((gdouble) area_so_far / (gdouble) total_area);
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

static void
gimp_rgn_iterator_iter_single (GimpRgnIterator *iter,
                               GimpPixelRgn    *srcPR,
                               GimpRgnFuncSrc   func,
                               gpointer         data)
{
  gpointer  pr;
  gint      total_area;
  gint      area_so_far;
  gint      count;

  total_area  = (iter->x2 - iter->x1) * (iter->y2 - iter->y1);
  area_so_far = 0;

  for (pr = gimp_pixel_rgns_register (1, srcPR), count = 0;
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr), count++)
    {
      const guchar *src = srcPR->data;
      gint          y;

      for (y = srcPR->y; y < srcPR->y + srcPR->h; y++)
        {
          const guchar *s = src;
          gint          x;

          for (x = srcPR->x; x < srcPR->x + srcPR->w; x++)
            {
              func (x, y, s, srcPR->bpp, data);
              s += srcPR->bpp;
            }

          src += srcPR->rowstride;
        }

      area_so_far += srcPR->w * srcPR->h;

      if ((count % 16) == 0)
        gimp_progress_update ((gdouble) area_so_far / (gdouble) total_area);
    }
}

static void
gimp_rgn_render_row (const guchar *src,
                     guchar       *dest,
                     gint          col,    /* row width in pixels */
                     gint          bpp,
                     GimpRgnFunc2  func,
                     gpointer      data)
{
  while (col--)
    {
      func (src, dest, bpp, data);

      src  += bpp;
      dest += bpp;
    }
}

static void
gimp_rgn_render_region (const GimpPixelRgn *srcPR,
                        const GimpPixelRgn *destPR,
                        GimpRgnFunc2        func,
                        gpointer            data)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          row;

  for (row = 0; row < srcPR->h; row++)
    {
      gimp_rgn_render_row (src, dest, srcPR->w, srcPR->bpp, func, data);

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}
