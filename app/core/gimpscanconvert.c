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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include <libart_lgpl/libart.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"

#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpscanconvert.h"


struct _GimpScanConvert
{
  guint        width;
  guint        height;

  guint        antialias;   /* how much to oversample by */
                            /* currently only used as boolean value */

  /* record the first and last points so we can close the current polygon. */
  gboolean     got_first;
  GimpVector2  first;
  GimpVector2  prev;
  gboolean     got_last;
  GimpVector2  last;

  guint        num_nodes;
  ArtVpath    *vpath;
};


/*  public functions  */

GimpScanConvert *
gimp_scan_convert_new (guint width,
                       guint height,
                       guint antialias)
{
  GimpScanConvert *sc;

  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (antialias > 0, NULL);

  sc = g_new0 (GimpScanConvert, 1);

  sc->antialias = antialias;
  sc->width     = width;
  sc->height    = height;

  sc->num_nodes = 0;
  sc->vpath     = NULL;

  return sc;
}

void
gimp_scan_convert_free (GimpScanConvert *sc)
{
  art_free (sc->vpath);
  g_free (sc);
}

/* Add "n_points" from "points" to the polygon currently being
 * described by "scan_converter".
 */
void
gimp_scan_convert_add_points (GimpScanConvert *sc,
                              guint            n_points,
                              GimpVector2     *points,
                              gboolean         new_polygon)
{
  gint  i;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (points != NULL);

  /* We need up to three extra nodes later to close and finish the path */
  sc->vpath = art_renew (sc->vpath, ArtVpath, sc->num_nodes + n_points + 3);

  /* if we need to start a new polygon check, if we need to close the
   * previous.
   */
  if (new_polygon)
    {
      /* close the last polygon if necessary */
      if (sc->got_first && sc->got_last &&
          (sc->first.x != sc->last.x || sc->first.y != sc->last.y))
        {
          sc->vpath[sc->num_nodes].code = ART_LINETO;
          sc->vpath[sc->num_nodes].x    = sc->first.x;
          sc->vpath[sc->num_nodes].y    = sc->first.y;
          sc->num_nodes++;
        }
      sc->got_first = FALSE;
      sc->got_last = FALSE;
    }

  if (!sc->got_first && n_points > 0)
    {
      sc->got_first = TRUE;
      sc->first = points[0];
      sc->vpath[sc->num_nodes].code =
          (sc->num_nodes == 0 || new_polygon) ?
                ART_MOVETO : ART_LINETO;
      sc->vpath[sc->num_nodes].x = points[0].x;
      sc->vpath[sc->num_nodes].y = points[0].y;
      sc->num_nodes++;
      sc->prev  = points[0];
    }

  for (i = 1; i < n_points; i++)
    {
      if (sc->prev.x != points[i].x || sc->prev.y != points[i].y)
        {
          sc->vpath[sc->num_nodes].code = ART_LINETO;
          sc->vpath[sc->num_nodes].x = points[i].x;
          sc->vpath[sc->num_nodes].y = points[i].y;
          sc->num_nodes++;
          sc->prev = points[i];
        }

    }

  if (n_points > 0)
    {
      sc->got_last = TRUE;
      sc->last = points[n_points - 1];
    }
}


/* Scan convert the polygon described by the list of points passed to
 * scan_convert_add_points, and return a channel with a bits set if
 * they fall within the polygon defined.  The polygon is filled
 * according to the even-odd rule.  The polygon is closed by
 * joining the final point to the initial point.
 */
GimpChannel *
gimp_scan_convert_to_channel (GimpScanConvert *sc,
                              GimpImage       *gimage)
{
  GimpChannel *mask;
  PixelRegion  maskPR;
  gint         i, j;

  gpointer     pr;  
  ArtVpath    *pert_vpath;
  ArtSVP      *svp, *svp2, *svp3;
  guchar      *dest, *d;

  /*  do we need to close the polygon? */
  if (sc->got_first && sc->got_last &&
      (sc->first.x != sc->last.x || sc->first.y != sc->last.y))
    {
      sc->vpath[sc->num_nodes].code = ART_LINETO;
      sc->vpath[sc->num_nodes].x    = sc->first.x;
      sc->vpath[sc->num_nodes].y    = sc->first.y;
      sc->num_nodes++;
    }

  mask = gimp_channel_new_mask (gimage, sc->width, sc->height);

  sc->vpath[sc->num_nodes].code = ART_END;
  sc->vpath[sc->num_nodes].x    = sc->first.x;
  sc->vpath[sc->num_nodes].y    = sc->first.y;
  sc->num_nodes++;

  /*
   * Current Libart (2.3.8) recommends a slight random distorsion
   * of the path, because art_svp_uncross and art_svp_rewind_uncrossed
   * are not yet numerically stable. It is actually possible to construct
   * worst case scenarios. The slight perturbation should not have any
   * visible effect.
   */

  /* Debug output of libart path
  for (i=0; i < sc->num_nodes ; i++)
    {
      g_printerr ("X: %f, Y: %f, Type: %d\n", sc->vpath[i].x, sc->vpath[i].y,
                                              sc->vpath[i].code );
    }
  */
  
  pert_vpath = art_vpath_perturb (sc->vpath);

  svp  = art_svp_from_vpath (pert_vpath);
  svp2 = art_svp_uncross (svp);
  svp3 = art_svp_rewind_uncrossed (svp2, ART_WIND_RULE_ODDEVEN);

  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)),
                     0, 0, 
                     gimp_item_width (GIMP_ITEM (mask)), 
                     gimp_item_height (GIMP_ITEM (mask)),
                     TRUE);

  g_return_val_if_fail (maskPR.bytes == 1, NULL);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      art_gray_svp_aa (svp3, maskPR.x, maskPR.y,
                       maskPR.x + maskPR.w, maskPR.y + maskPR.h,
                       maskPR.data, maskPR.rowstride);
      if (!sc->antialias)
        {
          /*
           * Ok, the user didn't want to have antialiasing, so just
           * remove the results from lots of CPU-Power...
           */
          dest = maskPR.data;

          for (j = 0; j < maskPR.h; j++)
            {
              d = dest;
              for (i = 0; i < maskPR.w; i++)
                {
                  d[0] = (d[0] >= 127) ? 255 : 0;
                  d += maskPR.bytes;
                }
              dest += maskPR.rowstride;
            }
        }
    }

  art_free (svp3);
  art_free (svp2);
  art_free (svp);
  art_free (pert_vpath);

  mask->bounds_known = FALSE;

  return mask;
}
