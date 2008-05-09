/* GIMP - The GNU Image Manipulation Program
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

#include <cairo/cairo.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimpscanconvert.h"


struct _GimpScanConvert
{
  gdouble         ratio_xy;

  gboolean        clip;
  gint            clip_x;
  gint            clip_y;
  gint            clip_w;
  gint            clip_h;

  /* stroking options */
  gboolean        do_stroke;
  gdouble         width;
  GimpJoinStyle   join;
  GimpCapStyle    cap;
  gdouble         miter;
  gdouble         dash_offset;
  GArray         *dash_info;

  guint           num_nodes;
  GArray         *path_data;

  /* stuff necessary for the rendering callback */
  GimpChannelOps  op;
  guchar         *buf;
  gint            rowstride;
  gint            x0, x1;
  gboolean        antialias;
  gboolean        value;
};


/* private functions */

static void   gimp_scan_convert_render_internal  (GimpScanConvert *sc,
                                                  GimpChannelOps   op,
                                                  TileManager     *tile_manager,
                                                  gint             off_x,
                                                  gint             off_y,
                                                  gboolean         antialias,
                                                  guchar           value);

static gint   gimp_cairo_stride_for_width        (gint             width);


/*  public functions  */

/**
 * gimp_scan_convert_new:
 *
 * Create a new scan conversion context.
 *
 * Return value: a newly allocated #GimpScanConvert context.
 */
GimpScanConvert *
gimp_scan_convert_new (void)
{
  GimpScanConvert *sc = g_slice_new0 (GimpScanConvert);

  sc->path_data = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));
  sc->ratio_xy = 1.0;

  return sc;
}

/**
 * gimp_scan_convert_free:
 * @sc: a #GimpScanConvert context
 *
 * Frees the resources allocated for @sc.
 */
void
gimp_scan_convert_free (GimpScanConvert *sc)
{
  g_return_if_fail (sc != NULL);

  if (sc->path_data)
    g_array_free (sc->path_data, TRUE);

  g_slice_free (GimpScanConvert, sc);
}

/**
 * gimp_scan_convert_set_pixel_ratio:
 * @sc:       a #GimpScanConvert context
 * @ratio_xy: the aspect ratio of the major coordinate axes
 *
 * Sets the pixel aspect ratio.
 */
void
gimp_scan_convert_set_pixel_ratio (GimpScanConvert *sc,
                                   gdouble          ratio_xy)
{
  g_return_if_fail (sc != NULL);

  /* we only need the relative resolution */
  sc->ratio_xy = ratio_xy;
}

/**
 * gimp_scan_convert_set_clip_rectangle
 * @sc:     a #GimpScanConvert context
 * @x:      horizontal offset of clip rectangle
 * @y:      vertical offset of clip rectangle
 * @width:  width of clip rectangle
 * @height: height of clip rectangle
 *
 * Sets a clip rectangle on @sc. Subsequent render operations will be
 * restricted to this area.
 */
void
gimp_scan_convert_set_clip_rectangle (GimpScanConvert *sc,
                                      gint             x,
                                      gint             y,
                                      gint             width,
                                      gint             height)
{
  g_return_if_fail (sc != NULL);

  sc->clip   = TRUE;
  sc->clip_x = x;
  sc->clip_y = y;
  sc->clip_w = width;
  sc->clip_h = height;
}

/**
 * gimp_scan_convert_add_polyline:
 * @sc:       a #GimpScanConvert context
 * @n_points: number of points to add
 * @points:   array of points to add
 * @closed:   whether to close the polyline and make it a polygon
 *
 * Add a polyline with @n_points @points that may be open or closed.
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
  GimpVector2  prev = { 0.0, 0.0, };
  cairo_path_data_t  pd;
  gint         i;

  g_return_if_fail (sc != NULL);
  g_return_if_fail (points != NULL);
  g_return_if_fail (n_points > 0);

  for (i = 0; i < n_points; i++)
    {
      /* compress multiple identical coordinates */
      if (i == 0 ||
          prev.x != points[i].x ||
          prev.y != points[i].y)
        {
          pd.header.type = (i == 0) ? CAIRO_PATH_MOVE_TO : CAIRO_PATH_LINE_TO;
          pd.header.length = 2;
          sc->path_data = g_array_append_val (sc->path_data, pd);

          pd.point.x = points[i].x;
          pd.point.y = points[i].y;
          sc->path_data = g_array_append_val (sc->path_data, pd);
          sc->num_nodes++;
          prev = points[i];
        }
    }

  /* close the polyline when needed */
  if (closed)
    {
      pd.header.type = CAIRO_PATH_CLOSE_PATH;
      pd.header.length = 1;
      sc->path_data = g_array_append_val (sc->path_data, pd);
    }
}


/**
 * gimp_scan_convert_stroke:
 * @sc:          a #GimpScanConvert context
 * @width:       line width in pixels
 * @join:        how lines should be joined
 * @cap:         how to render the end of lines
 * @miter:       convert a mitered join to a bevelled join if the miter would
 *               extend to a distance of more than @miter times @width from
 *               the actual join point
 * @dash_offset: offset to apply on the dash pattern
 * @dash_info:   dash pattern or %NULL for a solid line
 *
 * Stroke the content of a GimpScanConvert. The next
 * gimp_scan_convert_render() will result in the outline of the
 * polygon defined with the commands above.
 *
 * You cannot add additional polygons after this command.
 *
 * Note that if you have nonstandard resolution, "width" gives the
 * width (in pixels) for a vertical stroke, i.e. use the X resolution
 * to calculate the width of a stroke when operating with real world
 * units.
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
  sc->do_stroke = TRUE;
  sc->width = width;
  sc->join  = join;
  sc->cap   = cap;
  sc->miter = miter;
  sc->dash_offset = dash_offset;
  if (dash_info && dash_info->len)
    g_printerr ("Dashing not yet implemented\n");

#if 0
  if (dash_info && dash_info->len >= 2)
    {
      ArtVpath     *dash_vpath;
      ArtVpathDash  dash;
      gdouble      *dashes;
      gint          i;

      dash.offset = dash_offset * MAX (width, 1.0);

      dashes = g_new (gdouble, dash_info->len);

      for (i = 0; i < dash_info->len ; i++)
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
          for (i = 0; i < dash_info->len - 2; i++)
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
#endif
}


/**
 * gimp_scan_convert_render:
 * @sc:           a #GimpScanConvert context
 * @tile_manager: the #TileManager to render to
 * @off_x:        horizontal offset into the @tile_manager
 * @off_y:        vertical offset into the @tile_manager
 * @antialias:    whether to apply antialiasiing
 *
 * Actually renders the @sc to a mask. This function expects a tile
 * manager of depth 1.
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
  g_return_if_fail (sc != NULL);
  g_return_if_fail (tile_manager != NULL);

  gimp_scan_convert_render_internal (sc, GIMP_CHANNEL_OP_REPLACE,
                                     tile_manager, off_x, off_y,
                                     antialias, 255);
}

/**
 * gimp_scan_convert_render_value:
 * @sc:           a #GimpScanConvert context
 * @tile_manager: the #TileManager to render to
 * @off_x:        horizontal offset into the @tile_manager
 * @off_y:        vertical offset into the @tile_manager
 * @value:        value to use for covered pixels
 *
 * A variant of gimp_scan_convert_render() that doesn't do
 * antialiasing but gives control over the value that should be used
 * for pixels covered by the scan conversion . Uncovered pixels are
 * set to zero.
 *
 * You cannot add additional polygons after this command.
 */
void
gimp_scan_convert_render_value (GimpScanConvert *sc,
                                TileManager     *tile_manager,
                                gint             off_x,
                                gint             off_y,
                                guchar           value)
{
  g_return_if_fail (sc != NULL);
  g_return_if_fail (tile_manager != NULL);

  gimp_scan_convert_render_internal (sc, GIMP_CHANNEL_OP_REPLACE,
                                     tile_manager, off_x, off_y,
                                     FALSE, value);
}

/**
 * gimp_scan_convert_compose:
 * @sc:           a #GimpScanConvert context
 * @tile_manager: the #TileManager to render to
 * @off_x:        horizontal offset into the @tile_manager
 * @off_y:        vertical offset into the @tile_manager
 *
 * This is a variant of gimp_scan_convert_render() that composes the
 * (aliased) scan conversion with the content of the @tile_manager.
 *
 * You cannot add additional polygons after this command.
 */
void
gimp_scan_convert_compose (GimpScanConvert *sc,
                           GimpChannelOps   op,
                           TileManager     *tile_manager,
                           gint             off_x,
                           gint             off_y)
{
  g_return_if_fail (sc != NULL);
  g_return_if_fail (tile_manager != NULL);

  gimp_scan_convert_render_internal (sc, op,
                                     tile_manager, off_x, off_y,
                                     FALSE, 255);
}

static void
gimp_scan_convert_render_internal (GimpScanConvert *sc,
                                   GimpChannelOps   op,
                                   TileManager     *tile_manager,
                                   gint             off_x,
                                   gint             off_y,
                                   gboolean         antialias,
                                   guchar           value)
{
  PixelRegion      maskPR;
  gpointer         pr;
  gint             x, y;
  gint             width, height;

  cairo_t         *cr;
  cairo_surface_t *surface;
  cairo_path_t     path;

  x = 0;
  y = 0;
  width  = tile_manager_width (tile_manager);
  height = tile_manager_height (tile_manager);

  if (sc->clip &&
      ! gimp_rectangle_intersect (x, y, width, height,
                                  sc->clip_x, sc->clip_y,
                                  sc->clip_w, sc->clip_h,
                                  &x, &y, &width, &height))
    return;

  pixel_region_init (&maskPR, tile_manager, x, y, width, height, TRUE);

  g_return_if_fail (maskPR.bytes == 1);

  sc->antialias = antialias;
  sc->value     = value;
  sc->op        = op;

  path.status   = CAIRO_STATUS_SUCCESS;
  path.data     = (cairo_path_data_t *) sc->path_data->data;
  path.num_data = sc->path_data->len;

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *tmp_buf = NULL;
      gint    stride;

      sc->buf       = maskPR.data;
      sc->rowstride = maskPR.rowstride;
      sc->x0        = off_x + maskPR.x;
      sc->x1        = off_x + maskPR.x + maskPR.w;

      stride = gimp_cairo_stride_for_width (maskPR.w);

      g_assert (stride > 0);

      if (maskPR.rowstride != stride)
        {
          tmp_buf = g_alloca (stride * maskPR.h);
          memset (tmp_buf, 0, stride * maskPR.h);
        }

      surface = cairo_image_surface_create_for_data (tmp_buf ?
                                                     tmp_buf : maskPR.data,
                                                     CAIRO_FORMAT_A8,
                                                     maskPR.w, maskPR.h,
                                                     stride);

      cairo_surface_set_device_offset (surface,
                                       -off_x - maskPR.x,
                                       -off_y - maskPR.y);
      cr = cairo_create (surface);
      cairo_set_source_rgb (cr, value / 255.0, value / 255.0, value / 255.0);
      cairo_append_path (cr, &path);
      cairo_set_antialias (cr,
                           sc->antialias ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE);
      if (sc->do_stroke)
        {
          cairo_set_line_cap (cr, sc->cap == GIMP_CAP_BUTT ? CAIRO_LINE_CAP_BUTT :
                                  sc->cap == GIMP_CAP_ROUND ? CAIRO_LINE_CAP_ROUND :
                                  CAIRO_LINE_CAP_SQUARE);
          cairo_set_line_join (cr, sc->join == GIMP_JOIN_MITER ? CAIRO_LINE_JOIN_MITER :
                                   sc->join == GIMP_JOIN_ROUND ? CAIRO_LINE_JOIN_ROUND :
                                   CAIRO_LINE_JOIN_BEVEL);

#ifdef __GNUC__
#warning  cairo_set_dash() still missing!
#endif

          cairo_set_line_width (cr, sc->width);
          cairo_scale (cr, 1.0, sc->ratio_xy);
          cairo_stroke (cr);
        }
      else
        {
          cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
          cairo_fill (cr);
        }

      cairo_surface_flush (surface);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);

      if (tmp_buf)
        {
          guchar       *dest = maskPR.data;
          const guchar *src  = tmp_buf;
          gint          i;

          for (i = 0; i < maskPR.h; i++)
            {
              memcpy (dest, src, maskPR.w);

              src  += stride;
              dest += maskPR.rowstride;
            }
        }
    }
}

static gint
gimp_cairo_stride_for_width (gint width)
{
#ifdef __GNUC__
#warning use cairo_format_stride_for_width() as soon as we depend on cairo 1.6
#endif
#if 0
  return cairo_format_stride_for_width (CAIRO_FORMAT_A8, width);
#endif

#define CAIRO_STRIDE_ALIGNMENT (sizeof (guint32))
#define CAIRO_STRIDE_FOR_WIDTH_BPP(w,bpp) \
   (((bpp)*(w)+7)/8 + CAIRO_STRIDE_ALIGNMENT-1) & ~(CAIRO_STRIDE_ALIGNMENT-1)

  return CAIRO_STRIDE_FOR_WIDTH_BPP (width, 8);
}

static inline void
compose (GimpChannelOps  op,
         guchar         *buf,
         guchar          value,
         gint            len)
{
  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
      if (value)
        memset (buf, value, len);
      break;
    case GIMP_CHANNEL_OP_SUBTRACT:
      if (value)
        memset (buf, 0, len);
      break;
    case GIMP_CHANNEL_OP_REPLACE:
      memset (buf, value, len);
      break;
    case GIMP_CHANNEL_OP_INTERSECT:
      do
        {
          if (*buf)
            *buf = value;
          buf++;
        }
      while (--len);
      break;
    }
}

