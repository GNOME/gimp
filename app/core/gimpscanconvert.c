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
#include <libart_lgpl/art_svp_intersect.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimpscanconvert.h"


struct _GimpScanConvert
{
  gdouble      ratio_xy;

  /* stuff necessary for the _add_polygons API...  :-/  */
  gboolean     got_first;
  gboolean     need_closing;
  GimpVector2  first;
  GimpVector2  prev;

  gboolean     have_open;
  guint        num_nodes;
  ArtVpath    *vpath;

  ArtSVP      *svp;         /* Sorted vector path
                               (extension no longer possible)          */

  /* stuff necessary for the rendering callback */
  guchar      *buf;
  gint         rowstride;
  gint         x0, x1;
  gboolean     antialias;
};

/* private functions */

static void   gimp_scan_convert_finish           (GimpScanConvert *sc);
static void   gimp_scan_convert_close_add_points (GimpScanConvert *sc);

static void   gimp_scan_convert_render_callback (gpointer          user_data,
                                                 gint              y,
                                                 gint              start_value,
                                                 ArtSVPRenderAAStep *steps,
                                                 gint              n_steps);

/*  public functions  */

GimpScanConvert *
gimp_scan_convert_new (void)
{
  GimpScanConvert *sc;

  sc = g_new0 (GimpScanConvert, 1);

  sc->ratio_xy  = 1.0;

  return sc;
}

void
gimp_scan_convert_free (GimpScanConvert *sc)
{
  g_return_if_fail (sc != NULL);

  if (sc->vpath)
    art_free (sc->vpath);
  if (sc->svp)
    art_svp_free (sc->svp);

  g_free (sc);
}

/* set the Pixel-Ratio (width / height) for the pixels.
 */
void
gimp_scan_convert_set_pixel_ratio (GimpScanConvert *sc,
                                   gdouble          ratio_xy)
{
  g_return_if_fail (sc != NULL);

  /* we only need the relative resolution */
  sc->ratio_xy = ratio_xy;
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
  gint  i;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (points != NULL);
  g_return_if_fail (n_points > 0);
  g_return_if_fail (sc->svp == NULL);

  /* We need an extra nodes to end the path */
  sc->vpath = art_renew (sc->vpath, ArtVpath, sc->num_nodes + n_points + 1);

  if (sc->num_nodes == 0 || new_polygon)
    {
      if (sc->need_closing)
        gimp_scan_convert_close_add_points (sc);

      sc->got_first = FALSE;
    }

  /* We have to compress multiple identical coordinates */

  for (i = 0; i < n_points; i++)
    {
      if (sc->got_first == FALSE ||
          sc->prev.x != points[i].x || sc->prev.y != points[i].y)
        {
          sc->vpath[sc->num_nodes].code = ((! sc->got_first) || new_polygon) ?
                                           ART_MOVETO : ART_LINETO;
          sc->vpath[sc->num_nodes].x = points[i].x;
          sc->vpath[sc->num_nodes].y = points[i].y;
          sc->num_nodes++;
          sc->prev = points[i];

          if (!sc->got_first)
            {
              sc->got_first = TRUE;
              sc->first = points[i];
            }
        }
    }

  sc->need_closing = TRUE;

  sc->vpath[sc->num_nodes].code = ART_END;
  sc->vpath[sc->num_nodes].x = 0.0;
  sc->vpath[sc->num_nodes].y = 0.0;
}


static void
gimp_scan_convert_close_add_points (GimpScanConvert *sc)
{
  if (sc->need_closing &&
      (sc->prev.x != sc->first.x || sc->prev.y != sc->first.y))
    {
      sc->vpath = art_renew (sc->vpath, ArtVpath, sc->num_nodes + 2);
      sc->vpath[sc->num_nodes].code = ART_LINETO;
      sc->vpath[sc->num_nodes].x = sc->first.x;
      sc->vpath[sc->num_nodes].y = sc->first.y;
      sc->num_nodes++;
      sc->vpath[sc->num_nodes].code = ART_END;
      sc->vpath[sc->num_nodes].x = 0.0;
      sc->vpath[sc->num_nodes].y = 0.0;
    }

  sc->need_closing = FALSE;
}


/* Add a polygon with "npoints" "points" that may be open or closed.
 * It is not recommended to mix gimp_scan_convert_add_polyline with
 * gimp_scan_convert_add_points.
 *
 * Please note that you should use gimp_scan_convert_stroke() if you
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

  if (sc->need_closing)
    gimp_scan_convert_close_add_points (sc);

  if (!closed)
    sc->have_open = TRUE;

  /* make sure that we have enough space for the nodes */
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

  /* close the polyline when needed */
  if (closed && (prev.x != points[0].x ||
                 prev.y != points[0].y))
    {
      sc->vpath[sc->num_nodes].x = points[0].x;
      sc->vpath[sc->num_nodes].y = points[0].y;
      sc->vpath[sc->num_nodes].code = ART_LINETO;
      sc->num_nodes++;
    }

  sc->vpath[sc->num_nodes].code = ART_END;
  sc->vpath[sc->num_nodes].x = 0.0;
  sc->vpath[sc->num_nodes].y = 0.0;

  /* If someone wants to mix this function with _add_points ()
   * try to do something reasonable...
   */

  sc->got_first = FALSE;
}



/* Stroke the content of a GimpScanConvert. The next
 * gimp_scan_convert_render() will result in the outline of the polygon
 * defined with the commands above.
 *
 * You cannot add additional polygons after this command.
 */
void
gimp_scan_convert_stroke (GimpScanConvert *sc,
                          gdouble          width,
                          GimpJoinStyle    join,
                          GimpCapStyle     cap,
                          gdouble          miter,
                          gdouble          dash_offset,
                          GArray          *dash_info)
{
  ArtSVP                *stroke;
  ArtPathStrokeJoinType  artjoin = 0;
  ArtPathStrokeCapType   artcap  = 0;

  g_return_if_fail (sc->svp == NULL);

  if (sc->need_closing)
    gimp_scan_convert_close_add_points (sc);

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

  if (sc->ratio_xy != 1.0)
    {
      gint i;

      for (i = 0; i < sc->num_nodes; i++)
        sc->vpath[i].x *= sc->ratio_xy;
    }

  if (dash_info && dash_info->len >= 2)
    {
      ArtVpath     *dash_vpath;
      ArtVpathDash  dash;
      gdouble      *dashes;
      gint          i;

      dash.offset = dash_offset * MAX (width, 1.0);

      dashes = g_new (gdouble, dash_info->len);

      for (i=0; i < dash_info->len ; i++)
        dashes[i] = MAX (width, 1.0) * g_array_index (dash_info, gdouble, i);

      dash.n_dash = dash_info->len;
      dash.dash = dashes;

      /* correct 0.0 in the first element (starts with a gap) */

      if (dash.dash[0] == 0.0)
        {
          gdouble first;

          first = dash.dash[1];

          /* shift the pattern to really starts with a dash and
           * use the offset to skip into it.
           */
          for (i=0; i < dash_info->len - 2; i++)
            {
              dash.dash[i] = dash.dash[i+2];
              dash.offset += dash.dash[i];
            }

          if (dash_info->len % 2 == 1)
            {
              dash.dash[dash_info->len - 2] = first;
              dash.n_dash --;
            }
          else
            {
              if (dash_info->len < 3)
                {
                  /* empty stroke */
                  art_free (sc->vpath);
                  sc->vpath = NULL;
                }
              else
                {
                  dash.dash [dash_info->len - 3] += first;
                  dash.n_dash -= 2;
                }
            }
        }

      /* correct odd number of dash specifiers */

      if (dash.n_dash % 2 == 1)
        {
          gdouble last;

          last = dash.dash[dash.n_dash - 1];
          dash.dash[0] += last;
          dash.offset += last;
          dash.n_dash --;
        }


      if (sc->vpath)
        {
          dash_vpath = art_vpath_dash (sc->vpath, &dash);
          art_free (sc->vpath);
          sc->vpath = dash_vpath;
        }

      g_free (dashes);
    }

  if (sc->vpath)
    {
      stroke = art_svp_vpath_stroke (sc->vpath, artjoin, artcap,
                                     width, miter, 0.2);

      if (sc->ratio_xy != 1.0)
        {
          ArtSVPSeg *segment;
          ArtPoint  *point;
          gint       i, j;

          for (i = 0; i < stroke->n_segs; i++)
            {
              segment = stroke->segs + i;
              segment->bbox.x0 /= sc->ratio_xy;
              segment->bbox.x1 /= sc->ratio_xy;

              for (j=0; j < segment->n_points  ; j++)
                {
                  point = segment->points + j;
                  point->x /= sc->ratio_xy;
                }
            }
        }

      sc->svp = stroke;
    }
}


/* This is a more low level version. Expects a tile manager of depth 1.
 *
 * You cannot add additional polygons after this command.
 */
void
gimp_scan_convert_render (GimpScanConvert *sc,
                          TileManager     *tile_manager,
                          gint             off_x,
                          gint             off_y,
                          gboolean         antialias)
{
  PixelRegion  maskPR;
  gpointer     pr;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (tile_manager != NULL);

  gimp_scan_convert_finish (sc);

  if (!sc->svp)
    return;

  pixel_region_init (&maskPR, tile_manager, 0, 0,
                     tile_manager_width (tile_manager),
                     tile_manager_height (tile_manager),
                     TRUE);

  g_return_if_fail (maskPR.bytes == 1);

  sc->antialias = antialias;

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      sc->buf = maskPR.data;
      sc->rowstride = maskPR.rowstride;
      sc->x0 = off_x + maskPR.x;
      sc->x1 = off_x + maskPR.x + maskPR.w;

      art_svp_render_aa (sc->svp,
                         sc->x0,
                         off_y + maskPR.y,
                         sc->x1,
                         off_y + maskPR.y + maskPR.h,
                         gimp_scan_convert_render_callback, sc);

    }
}


/* private function to convert the vpath to a svp when not using
 * gimp_scan_convert_stroke
 */
static void
gimp_scan_convert_finish (GimpScanConvert *sc)
{
  ArtSVP       *svp, *svp2;
  ArtSvpWriter *swr;

  /* return gracefully on empty path */
  if (!sc->vpath)
    return;

  if (sc->need_closing)
    gimp_scan_convert_close_add_points (sc);

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

  svp = art_svp_from_vpath (sc->vpath);

  swr = art_svp_writer_rewind_new (ART_WIND_RULE_ODDEVEN);
  art_svp_intersector (svp, swr);

  svp2 = art_svp_writer_rewind_reap (swr); /* this also frees swr */

  art_svp_free (svp);

  sc->svp = svp2;
}


/*
 * private function to render libart SVPRenderAASteps into the pixel region
 *
 * A function pretty similiar to this could be used to implement a
 * lookup table for the values (just change VALUE_TO_PIXEL).
 *
 * from the libart documentation:
 *
 * The value 0x8000 represents 0% coverage by the polygon, while
 * 0xff8000 represents 100% coverage. This format is designed so that
 * >> 16 results in a standard 0x00..0xff value range, with nice
 * rounding.
 */

static void
gimp_scan_convert_render_callback (gpointer            user_data,
                                   gint                y,
                                   gint                start_value,
                                   ArtSVPRenderAAStep *steps,
                                   gint                n_steps)
{
  GimpScanConvert *sc        = user_data;
  gint             cur_value = start_value;
  gint             k, run_x0, run_x1;

#define VALUE_TO_PIXEL(x) (sc->antialias ? \
                           ((x) >> 16)   : \
                           (((x) & (1 << 23) ? 255 : 0)))

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;

      if (run_x1 > sc->x0)
        memset (sc->buf,
                VALUE_TO_PIXEL (cur_value),
                run_x1 - sc->x0);

      for (k = 0; k < n_steps - 1; k++)
        {
          cur_value += steps[k].delta;

          run_x0 = run_x1;
          run_x1 = steps[k + 1].x;

          if (run_x1 > run_x0)
            memset (sc->buf + run_x0 - sc->x0,
                    VALUE_TO_PIXEL (cur_value),
                    run_x1 - run_x0);
        }

      cur_value += steps[k].delta;

      if (sc->x1 > run_x1)
        memset (sc->buf + run_x1 - sc->x0,
                VALUE_TO_PIXEL (cur_value),
                sc->x1 - run_x1);
    }
  else
    {
      memset (sc->buf,
              VALUE_TO_PIXEL (cur_value),
              sc->x1 - sc->x0);
    }

  sc->buf += sc->rowstride;

#undef VALUE_TO_PIXEL
}

