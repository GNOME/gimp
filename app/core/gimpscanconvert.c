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
#include "base/tile-manager.h"

#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpscanconvert.h"


struct _GimpScanConvert
{
  guint        width;
  guint        height;

  gboolean     antialias;   /* do we want antialiasing? */

  /* record the first and last points so we can close the current polygon. */
  gboolean     got_first;
  gboolean     have_open;

  guint        num_nodes;
  ArtVpath    *vpath;

  ArtSVP      *svp;         /* Sorted vector path
                               (extension no longer possible)          */
};

/* Private functions */
static void gimp_scan_convert_finish (GimpScanConvert *sc);

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

  sc->antialias = (antialias != 0 ? TRUE : FALSE);
  sc->width     = width;
  sc->height    = height;

  sc->got_first = FALSE;
  sc->have_open = FALSE;

  sc->num_nodes = 0;
  sc->vpath     = NULL;
  sc->svp       = NULL;

  return sc;
}

void
gimp_scan_convert_free (GimpScanConvert *sc)
{
  art_free (sc->vpath);
  art_free (sc->svp);
  g_free (sc);
}

/* Add "n_points" from "points" to the polygon currently being
 * described by "scan_converter". DEPRECATED.
 */
void
gimp_scan_convert_add_points (GimpScanConvert *sc,
                              guint            n_points,
                              GimpVector2     *points,
                              gboolean         new_polygon)
{
  GimpVector2  prev;
  gint  i;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (points != NULL);
  g_return_if_fail (n_points > 0);
  g_return_if_fail (sc->svp == NULL);

  /* We need an extra nodes to end the path */
  sc->vpath = art_renew (sc->vpath, ArtVpath, sc->num_nodes + n_points + 2);

  sc->vpath[sc->num_nodes].code = ((! sc->got_first) || new_polygon) ?
                                        ART_MOVETO : ART_LINETO;
  sc->vpath[sc->num_nodes].x = points[0].x;
  sc->vpath[sc->num_nodes].y = points[0].y;
  sc->num_nodes++;
  sc->got_first = TRUE;
  prev  = points[0];

  for (i = 1; i < n_points; i++)
    {
      if (prev.x != points[i].x || prev.y != points[i].y)
        {
          sc->vpath[sc->num_nodes].code = ART_LINETO;
          sc->vpath[sc->num_nodes].x = points[i].x;
          sc->vpath[sc->num_nodes].y = points[i].y;
          sc->num_nodes++;
          prev = points[i];
        }
    }

  /* for some reason we need to duplicate the last node?? */

  sc->vpath[sc->num_nodes] = sc->vpath[sc->num_nodes - 1];
  sc->num_nodes++;
  sc->vpath[sc->num_nodes].code = ART_END;
}


/* Add a polygon with "npoints" "points" that may be open or closed.
 * It is not recommended to mix gimp_scan_convert_add_polyline with
 * gimp_scan_convert_add_points.
 *
 * Please note that if you should use gimp_scan_convert_stroke() if you
 * specify open polygons.
 */

void
gimp_scan_convert_add_polyline (GimpScanConvert *sc,
                                guint            n_points,
                                GimpVector2     *points,
                                gboolean         closed)
{
  GimpVector2  prev;
  gint i;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (points != NULL);
  g_return_if_fail (n_points > 0);
  g_return_if_fail (sc->svp == NULL);

  if (!closed)
    sc->have_open = TRUE;

  /* We need two extra nodes later to close the path. */
  sc->vpath = art_renew (sc->vpath, ArtVpath,
                         sc->num_nodes + n_points + 2);

  for (i = 0; i < n_points; i++)
    {
      /* compress multiple identical coordinates */
      if (i == 0 ||
          prev.x != points[i].x ||
          prev.y != points[i].y)
        {
          sc->vpath[sc->num_nodes].code = (i == 0 ? (closed ?
                                                     ART_MOVETO :
                                                     ART_MOVETO_OPEN) :
                                                    ART_LINETO);
          sc->vpath[sc->num_nodes].x = points[i].x;
          sc->vpath[sc->num_nodes].y = points[i].y;
          sc->num_nodes++;
          prev = points[i];
        }
    }

  /* for some reason we need to duplicate the last node?? */

  sc->vpath[sc->num_nodes] = sc->vpath[sc->num_nodes - 1];
  sc->num_nodes++;
  sc->vpath[sc->num_nodes].code = ART_END;

  /* If someone wants to mix this function with _add_points ()
   * try to do something reasonable...
   */

  sc->got_first = FALSE;
}



/* Stroke the content of a GimpScanConvert. The next
 * gimp_scan_convert_to_channel will result in the outline of the polygon
 * defined with the commands above.
 *
 * You cannot add additional polygons after this command.
 */
void
gimp_scan_convert_stroke (GimpScanConvert *sc, 
                          GimpJoinType     join,
                          GimpCapType      cap,
                          gdouble          width)
{
  ArtSVP                *stroke;
  ArtPathStrokeJoinType  artjoin;
  ArtPathStrokeCapType   artcap;

  g_return_if_fail (sc->svp == NULL);

  switch (join)
    {
    case GIMP_JOIN_MITER:
        artjoin = ART_PATH_STROKE_JOIN_MITER;
        break;
    case GIMP_JOIN_ROUND:
        artjoin = ART_PATH_STROKE_JOIN_ROUND;
        break;
    case GIMP_JOIN_BEVEL:
        artjoin = ART_PATH_STROKE_JOIN_BEVEL;
        break;
    }

  switch (cap)
    {
    case GIMP_CAP_BUTT:
        artcap = ART_PATH_STROKE_CAP_BUTT;
        break;
    case GIMP_CAP_ROUND:
        artcap = ART_PATH_STROKE_CAP_ROUND;
        break;
    case GIMP_CAP_SQUARE:
        artcap = ART_PATH_STROKE_CAP_SQUARE;
        break;
    }

  stroke = art_svp_vpath_stroke (sc->vpath, artjoin, artcap,
                                 width, 10.0, 1.0);
  
  sc->svp = stroke;
}

/* Return a new Channel according to the polygonal shapes defined with
 * the commands above.
 *
 * You cannot add additional polygons after this command.
 */
GimpChannel *
gimp_scan_convert_to_channel (GimpScanConvert *sc,
                              GimpImage       *gimage)
{
  GimpChannel *mask;

  mask = gimp_channel_new_mask (gimage, sc->width, sc->height);

  gimp_scan_convert_render (sc, gimp_drawable_data (GIMP_DRAWABLE (mask)));

  mask->bounds_known = FALSE;

  return mask;
}


/* This is a more low level version. Expects a tile manager of depth 1.
 *
 * You cannot add additional polygons after this command.
 */
void 
gimp_scan_convert_render (GimpScanConvert *sc,
                          TileManager     *tile_manager)
{
  PixelRegion  maskPR;
  gpointer     pr;  

  gint         i, j, x0, y0;
  guchar      *dest, *d;
  
  g_return_if_fail (sc != NULL);
  g_return_if_fail (tile_manager != NULL);
  
  gimp_scan_convert_finish (sc);

  tile_manager_get_offsets (tile_manager, &x0, &y0);

  pixel_region_init (&maskPR, tile_manager,
                     x0, y0, 
                     tile_manager_width (tile_manager),
                     tile_manager_height (tile_manager),
                     TRUE);

  g_return_if_fail (maskPR.bytes == 1);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      art_gray_svp_aa (sc->svp, maskPR.x, maskPR.y,
                       maskPR.x + maskPR.w, maskPR.y + maskPR.h,
                       maskPR.data, maskPR.rowstride);
      if (sc->antialias == FALSE)
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
}


/* private function to convert the vpath to a svp when not using
 * gimp_scan_convert_stroke
 */
static void 
gimp_scan_convert_finish (GimpScanConvert *sc)
{
  ArtVpath    *pert_vpath;
  ArtSVP      *svp, *svp2;

  g_return_if_fail (sc->vpath != NULL);

  /* gimp_scan_convert_stroke (sc, GIMP_JOIN_MITER, GIMP_CAP_BUTT, 15.0);
   */
  
  if (sc->svp)
    return;   /* We already have a valid SVP */

  /* Debug output of libart path */
  /* {
   *   gint i;
   *   for (i=0; i < sc->num_nodes + 1; i++)
   *     {
   *       g_printerr ("X: %f, Y: %f, Type: %d\n", sc->vpath[i].x,
   *                                               sc->vpath[i].y,
   *                                               sc->vpath[i].code );
   *     }
   * }
   */

  if (sc->have_open)
    {
      gint i;
      for (i = 0; i < sc->num_nodes; i++)
        if (sc->vpath[i].code == ART_MOVETO_OPEN)
          {
            g_printerr ("Fixing ART_MOVETO_OPEN - result might be incorrect\n");
            sc->vpath[i].code = ART_MOVETO;
          }
    }

  /*
   * Current Libart (2.3.8) recommends a slight random distorsion
   * of the path, because art_svp_uncross and art_svp_rewind_uncrossed
   * are not yet numerically stable. It is actually possible to construct
   * worst case scenarios. The slight perturbation should not have any
   * visible effect.
   */

  pert_vpath = art_vpath_perturb (sc->vpath);

  svp  = art_svp_from_vpath (pert_vpath);
  art_free (pert_vpath);

  svp2 = art_svp_uncross (svp);
  art_free (svp);
  
  svp = art_svp_rewind_uncrossed (svp2, ART_WIND_RULE_ODDEVEN);
  art_free (svp2);

  sc->svp = svp;
}

