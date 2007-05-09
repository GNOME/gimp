/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimpchannel.h"

#include "base/tile-manager.h"

#include "tools/gimpperspectivetool.h"
#include "tools/gimptransformtool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-preview.h"
#include "gimpdisplayshell-transform.h"


#define MAX_SUB_COLS 6     /* number of columns and  */
#define MAX_SUB_ROWS 6     /* rows to use in perspective preview subdivision */

/*  local function prototypes  */

static void    gimp_display_shell_draw_quad          (GimpDrawable *texture,
                                                      GdkDrawable  *dest,
                                                      GimpChannel  *mask,
                                                      gint          mask_offx,
                                                      gint          mask_offy,
                                                      gint         *x,
                                                      gint         *y,
                                                      gfloat       *u,
                                                      gfloat       *v);
static void    gimp_display_shell_draw_tri           (GimpDrawable *texture,
                                                      GdkDrawable  *dest,
                                                      GimpChannel  *mask,
                                                      gint          mask_offx,
                                                      gint          mask_offy,
                                                      gint         *x,
                                                      gint         *y,
                                                      gfloat       *u,
                                                      gfloat       *v);
static void    gimp_display_shell_draw_tri_row       (GimpDrawable *texture,
                                                      GdkDrawable  *dest,
                                                      GdkPixbuf    *row,
                                                      gint          x1,
                                                      gfloat        u1,
                                                      gfloat        v1,
                                                      gint          x2,
                                                      gfloat        u2,
                                                      gfloat        v2,
                                                      gint          y);
static void    gimp_display_shell_draw_tri_row_mask  (GimpDrawable *texture,
                                                      GdkDrawable  *dest,
                                                      GdkPixbuf    *row,
                                                      GimpChannel  *mask,
                                                      gint          mask_offx,
                                                      gint          mask_offy,
                                                      gint          x1,
                                                      gfloat        u1,
                                                      gfloat        v1,
                                                      gint          x2,
                                                      gfloat        u2,
                                                      gfloat        v2,
                                                      gint          y);
static void    gimp_display_shell_trace_tri_edge     (gint         *dest,
                                                      gint          x1,
                                                      gint          y1,
                                                      gint          x2,
                                                      gint          y2);

/*  public functions  */

void
gimp_display_shell_preview_transform (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (gimp_display_shell_get_show_transform (shell))
    {
      GimpTool          *tool;
      GimpTransformTool *tr_tool;
      gdouble            z1, z2, z3, z4;

      tool = tool_manager_get_active (shell->display->image->gimp);

      if (! GIMP_IS_TRANSFORM_TOOL (tool) ||
          ! GIMP_IS_DRAWABLE (tool->drawable))
        return;

      tr_tool = GIMP_TRANSFORM_TOOL (tool);

      if (! tr_tool->use_grid)
        return;

      z1 = ((tr_tool->tx2 - tr_tool->tx1) * (tr_tool->ty4 - tr_tool->ty1) -
            (tr_tool->tx4 - tr_tool->tx1) * (tr_tool->ty2 - tr_tool->ty1));
      z2 = ((tr_tool->tx4 - tr_tool->tx1) * (tr_tool->ty3 - tr_tool->ty1) -
            (tr_tool->tx3 - tr_tool->tx1) * (tr_tool->ty4 - tr_tool->ty1));
      z3 = ((tr_tool->tx4 - tr_tool->tx2) * (tr_tool->ty3 - tr_tool->ty2) -
            (tr_tool->tx3 - tr_tool->tx2) * (tr_tool->ty4 - tr_tool->ty2));
      z4 = ((tr_tool->tx3 - tr_tool->tx2) * (tr_tool->ty1 - tr_tool->ty2) -
            (tr_tool->tx1 - tr_tool->tx2) * (tr_tool->ty3 - tr_tool->ty2));

      /* only draw convex polygons */

      if ((z1 * z2 > 0) && (z3 * z4 > 0))
        {
          GimpChannel *mask;
          gint         mask_x1, mask_y1;
          gint         mask_x2, mask_y2;
          gint         mask_offx, mask_offy;

          gint         columns, rows;
          gint         j, k, sub;

          gfloat       du, dv, dx, dy;
          gint         x[MAX_SUB_COLS * MAX_SUB_ROWS][4],
                       y[MAX_SUB_COLS * MAX_SUB_ROWS][4];
          gfloat       u[MAX_SUB_COLS * MAX_SUB_ROWS][4],
                       v[MAX_SUB_COLS * MAX_SUB_ROWS][4];

          mask = NULL;
          mask_offx = mask_offy = 0;

          if (gimp_drawable_mask_bounds (tool->drawable,
                                         &mask_x1, &mask_y1,
                                         &mask_x2, &mask_y2))
            {
              mask = gimp_image_get_mask (shell->display->image);

              gimp_item_offsets (GIMP_ITEM (tool->drawable),
                                 &mask_offx, &mask_offy);
            }

          if (GIMP_IS_PERSPECTIVE_TOOL (tr_tool))
            {
              /* approximate perspective transform by subdivision
               *
               * increase number of columns/rows to increase precision
               */
              columns = MAX_SUB_COLS;
              rows    = MAX_SUB_ROWS;
            }
          else
            {
              /*  for affine transforms subdivision has no effect
               */
              columns = 1;
              rows    = 1;
            }

          dx = (tr_tool->x2 - tr_tool->x1) / ((gfloat) columns);
          dy = (tr_tool->y2 - tr_tool->y1) / ((gfloat) rows);

          du = (mask_x2 - mask_x1) / ((gfloat) columns);
          dv = (mask_y2 - mask_y1) / ((gfloat) rows);

#define CALC_VERTEX(col, row, sub, index) \
          { \
            gdouble tx1, ty1; \
            gdouble tx2, ty2; \
\
            u[sub][index] = tr_tool->x1 + (dx * (col + (index & 1))); \
            v[sub][index] = tr_tool->y1 + (dy * (row + (index >> 1))); \
\
            gimp_matrix3_transform_point (&tr_tool->transform, \
                                          u[sub][index], v[sub][index], \
                                          &tx1, &ty1); \
\
            gimp_display_shell_transform_xy_f (shell, \
                                               tx1, ty1, \
                                               &tx2, &ty2, \
                                               FALSE); \
            x[sub][index] = (gint) tx2; \
            y[sub][index] = (gint) ty2; \
\
            u[sub][index] = mask_x1 + (du * (col + (index & 1))); \
            v[sub][index] = mask_y1 + (dv * (row + (index >> 1))); \
          }

#define COPY_VERTEX(subdest, idest, subsrc, isrc) \
          x[subdest][idest] = x[subsrc][isrc]; \
          y[subdest][idest] = y[subsrc][isrc]; \
          u[subdest][idest] = u[subsrc][isrc]; \
          v[subdest][idest] = v[subsrc][isrc];

          /*
           * upper left corner subdivision: calculate all vertices
           */

          for (j = 0; j < 4; j++)
            {
              CALC_VERTEX (0, 0, 0, j);
            }

          /*
           * top row subdivisions: calculate only right side vertices
           */

          for (j = 1; j < columns; j++)
            {
              COPY_VERTEX (j, 0, j - 1, 1);
              CALC_VERTEX (j, 0, j, 1);
              COPY_VERTEX (j, 2, j - 1, 3);
              CALC_VERTEX (j, 0, j, 3);
            }

          /*
           * left column subdivisions: calculate only bottom side vertices
           */

          for (j = 1, sub = columns; j < rows; j++, sub += columns)
            {
              COPY_VERTEX (sub, 0, sub - columns, 2);
              COPY_VERTEX (sub, 1, sub - columns, 3);
              CALC_VERTEX (0, j, sub, 2);
              CALC_VERTEX (0, j, sub, 3);
            }

          /*
           * the rest: calculate only the bottom-right vertex
           */

          for (j = 1, sub = columns; j < rows; j++)
            {
              sub++;

              for (k = 1; k < columns; k++, sub++)
                {
                  COPY_VERTEX (sub, 0, sub - 1, 1);
                  COPY_VERTEX (sub, 1, sub - columns, 3);
                  COPY_VERTEX (sub, 2, sub - 1, 3);
                  CALC_VERTEX (k, j, sub, 3);
                }
            }

#undef CALC_VERTEX
#undef COPY_VERTEX

          k = columns * rows;
          for (j = 0; j < k; j++)
            gimp_display_shell_draw_quad (tool->drawable,
                            GDK_DRAWABLE (GTK_WIDGET (shell->canvas)->window),
                            mask, mask_offx, mask_offy,
                            x[j], y[j], u[j], v[j]);

        }
    }
}


/*  private functions  */

static void
gimp_display_shell_draw_quad (GimpDrawable *texture,
                              GdkDrawable  *dest,
                              GimpChannel  *mask,
                              gint          mask_offx,
                              gint          mask_offy,
                              gint         *x,
                              gint         *y,
                              gfloat       *u,
                              gfloat       *v)
{
  gint   x2[3], y2[3];
  gfloat u2[3], v2[3];

  x2[0] = x[3];  y2[0] = y[3];  u2[0] = u[3];  v2[0] = v[3];
  x2[1] = x[2];  y2[1] = y[2];  u2[1] = u[2];  v2[1] = v[2];
  x2[2] = x[1];  y2[2] = y[1];  u2[2] = u[1];  v2[2] = v[1];

  gimp_display_shell_draw_tri (texture, dest, mask, mask_offx, mask_offy,
                               x, y, u, v);
  gimp_display_shell_draw_tri (texture, dest, mask, mask_offx, mask_offy,
                               x2, y2, u2, v2);
}

static void
gimp_display_shell_draw_tri (GimpDrawable *texture,
                             GdkDrawable  *dest,
                             GimpChannel  *mask,
                             gint          mask_offx,
                             gint          mask_offy,
                             gint         *x,
                             gint         *y,
                             gfloat       *u, /* texture coords */
                             gfloat       *v) /* 0.0 ... tex width, height */
{
  GdkPixbuf   *row;
  gint         dwidth, dheight;    /* clip boundary */
  gint         j, k;
  gint         ry;
  gint        *l_edge, *r_edge;    /* arrays holding x-coords of edge pixels */
  gint        *left,   *right;
  gfloat       dul, dvl, dur, dvr; /* left and right texture coord deltas */
  gfloat       u_l, v_l, u_r, v_r; /* left and right texture coord pairs */

  if (! GIMP_IS_DRAWABLE (texture) ||
      ! GDK_IS_DRAWABLE (dest))
    return;

  g_return_if_fail (x != NULL && y != NULL && u != NULL && v != NULL);

  gdk_drawable_get_size (dest, &dwidth, &dheight);

  if (dwidth > 0 && dheight > 0)
    row = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                          mask ? TRUE : gimp_drawable_has_alpha (texture),
                          8, dwidth, 1);
  else
    return;

  g_return_if_fail (row != NULL);

  left = right = NULL;
  dul = dvl = dur = dvr = 0;
  u_l = v_l = u_r = v_r = 0;

  /* sort vertices in order of y-coordinate */

  for (j = 0; j < 3; j++)
    for (k = j + 1; k < 3; k++)
      if (y[k] < y[j])
        {
          gint tmp;
          gfloat ftmp;

          tmp  = y[k];  y[k] = y[j];  y[j] = tmp;
          tmp  = x[k];  x[k] = x[j];  x[j] = tmp;
          ftmp = u[k];  u[k] = u[j];  u[j] = ftmp;
          ftmp = v[k];  v[k] = v[j];  v[j] = ftmp;
        }

  if (y[2] == y[0])
    return;

  l_edge = g_new (gint, y[2] - y[0]);
  r_edge = g_new (gint, y[2] - y[0]);

  /* draw the triangle */

  gimp_display_shell_trace_tri_edge (l_edge, x[0], y[0], x[2], y[2]);

  left = l_edge;
  dul  = (u[2] - u[0]) / (y[2] - y[0]);
  dvl  = (v[2] - v[0]) / (y[2] - y[0]);
  u_l  = u[0];
  v_l  = v[0];

  if (y[0] != y[1])
    {
      gimp_display_shell_trace_tri_edge (r_edge, x[0], y[0], x[1], y[1]);

      right = r_edge;
      dur   = (u[1] - u[0]) / (y[1] - y[0]);
      dvr   = (v[1] - v[0]) / (y[1] - y[0]);
      u_r   = u[0];
      v_r   = v[0];

      for (ry = y[0]; ry < y[1]; ry++)
        {
          if (ry >= 0 && ry < dheight)
            {
              if (mask)
                gimp_display_shell_draw_tri_row_mask
                                                (texture, dest, row,
                                                 mask, mask_offx, mask_offy,
                                                 *left, u_l, v_l,
                                                 *right, u_r, v_r,
                                                 ry);
              else
                gimp_display_shell_draw_tri_row (texture, dest, row,
                                                 *left, u_l, v_l,
                                                 *right, u_r, v_r,
                                                 ry);
            }

          left++;       right++;
          u_l += dul;   v_l += dvl;
          u_r += dur;   v_r += dvr;
        }
    }

  if (y[1] != y[2])
    {
      gimp_display_shell_trace_tri_edge (r_edge, x[1], y[1], x[2], y[2]);

      right = r_edge;
      dur   = (u[2] - u[1]) / (y[2] - y[1]);
      dvr   = (v[2] - v[1]) / (y[2] - y[1]);
      u_r   = u[1];
      v_r   = v[1];

      for (ry = y[1]; ry < y[2]; ry++)
        {
          if (ry >= 0 && ry < dheight)
            {
              if (mask)
                gimp_display_shell_draw_tri_row_mask
                                                (texture, dest, row,
                                                 mask, mask_offx, mask_offy,
                                                 *left,  u_l, v_l,
                                                 *right, u_r, v_r,
                                                 ry);
              else
                gimp_display_shell_draw_tri_row (texture, dest, row,
                                                 *left,  u_l, v_l,
                                                 *right, u_r, v_r,
                                                 ry);
            }

          left++;       right++;
          u_l += dul;   v_l += dvl;
          u_r += dur;   v_r += dvr;
        }
    }

  g_free (l_edge);
  g_free (r_edge);

  g_object_unref (row);
}

static void
gimp_display_shell_draw_tri_row (GimpDrawable *texture,
                                 GdkDrawable  *dest,
                                 GdkPixbuf    *row,
                                 gint          x1,
                                 gfloat        u1,
                                 gfloat        v1,
                                 gint          x2,
                                 gfloat        u2,
                                 gfloat        v2,
                                 gint          y)
{
  TileManager  *tiles;
  guchar       *pptr;
  guchar        bytes;
  gfloat        u, v;
  gfloat        du, dv;
  gint          dx;
  guchar        pixel[4];
  const guchar *cmap;
  gint          offset;

  if (x2 == x1)
    return;

  g_return_if_fail (GIMP_IS_DRAWABLE (texture));
  g_return_if_fail (GDK_IS_DRAWABLE (dest));
  g_return_if_fail (GDK_IS_PIXBUF (row));
  g_return_if_fail (gdk_pixbuf_get_bits_per_sample (row) == 8);
  g_return_if_fail (gdk_pixbuf_get_colorspace (row) == GDK_COLORSPACE_RGB);
  g_return_if_fail (gdk_pixbuf_get_has_alpha (row) ==
                    gimp_drawable_has_alpha (texture));

  bytes = gdk_pixbuf_get_n_channels (row);
  pptr  = gdk_pixbuf_get_pixels (row);
  tiles = gimp_drawable_get_tiles (texture);

  if (x1 > x2)
    {
      gint   tmp;
      gfloat ftmp;

      tmp  = x2;  x2 = x1;  x1 = tmp;
      ftmp = u2;  u2 = u1;  u1 = ftmp;
      ftmp = v2;  v2 = v1;  v1 = ftmp;
    }

  u = u1;
  v = v1;
  du = (u2 - u1) / (x2 - x1);
  dv = (v2 - v1) / (x2 - x1);

  /* don't calculate unseen pixels */
  if (x1 < 0)
    {
      u += du * (0 - x1);
      v += dv * (0 - x1);
      x1 = 0;
    }
  else if (x1 > gdk_pixbuf_get_width (row))
    {
      return;
    }

  if (x2 < 0)
    {
      return;
    }
  else if (x2 > gdk_pixbuf_get_width (row))
    {
      x2 = gdk_pixbuf_get_width (row);
    }

  dx = x2 - x1;

  if (! dx)
    return;

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel[0] + pixel[0] + pixel[0];

          *pptr++ = cmap[offset];
          *pptr++ = cmap[offset + 1];
          *pptr++ = cmap[offset + 2];

          u += du;
          v += dv;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel[0] + pixel[0] + pixel[0];

          *pptr++ = cmap[offset];
          *pptr++ = cmap[offset + 1];
          *pptr++ = cmap[offset + 2];
          *pptr++ = pixel[1];

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = pixel[0];

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = pixel[1];

          u += du;
          v += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pptr);

          pptr += bytes;
          u += du;
          v += dv;
        }
      break;

    default:
      return;
    }

  gdk_draw_pixbuf (dest, NULL, row, 0, 0, x1, y, x2 - x1, 1,
                   GDK_RGB_DITHER_NONE, 0, 0);
}

static void
gimp_display_shell_draw_tri_row_mask (GimpDrawable *texture,
                                      GdkDrawable  *dest,
                                      GdkPixbuf    *row,
                                      GimpChannel  *mask,
                                      gint          mask_offx,
                                      gint          mask_offy,
                                      gint          x1,
                                      gfloat        u1,
                                      gfloat        v1,
                                      gint          x2,
                                      gfloat        u2,
                                      gfloat        v2,
                                      gint          y)
{
  TileManager  *tiles, *masktiles;
  guchar       *pptr;
  guchar        bytes, alpha;
  gfloat        u, v;
  gfloat        mu, mv;
  gfloat        du, dv;
  gint          dx;
  guchar        pixel[4], maskval;
  const guchar *cmap;
  gint          offset;

  if (x2 == x1)
    return;

  g_return_if_fail (GIMP_IS_DRAWABLE (texture));
  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GDK_IS_DRAWABLE (dest));
  g_return_if_fail (GDK_IS_PIXBUF (row));
  g_return_if_fail (gdk_pixbuf_get_bits_per_sample (row) == 8);
  g_return_if_fail (gdk_pixbuf_get_colorspace (row) == GDK_COLORSPACE_RGB);

  bytes     = gdk_pixbuf_get_n_channels (row);
  alpha     = bytes - 1;
  pptr      = gdk_pixbuf_get_pixels (row);
  tiles     = gimp_drawable_get_tiles (texture);
  masktiles = gimp_drawable_get_tiles (GIMP_DRAWABLE (mask));

  if (x1 > x2)
    {
      gint   tmp;
      gfloat ftmp;

      tmp  = x2;  x2 = x1;  x1 = tmp;
      ftmp = u2;  u2 = u1;  u1 = ftmp;
      ftmp = v2;  v2 = v1;  v1 = ftmp;
    }

  u = u1;
  v = v1;
  du = (u2 - u1) / (x2 - x1);
  dv = (v2 - v1) / (x2 - x1);

  /* don't calculate unseen pixels */
  if (x1 < 0)
    {
      u += du * (0 - x1);
      v += dv * (0 - x1);
      x1 = 0;
    }
  else if (x1 > gdk_pixbuf_get_width (row))
    {
      return;
    }

  if (x2 < 0)
    {
      return;
    }
  else if (x2 > gdk_pixbuf_get_width (row))
    {
      x2 = gdk_pixbuf_get_width (row);
    }

  dx = x2 - x1;

  if (! dx)
    return;

  mu = u + mask_offx;
  mv = v + mask_offy;

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, pptr + alpha);

          offset = pixel[0] + pixel[0] + pixel[0];

          *pptr++ = cmap[offset];
          *pptr++ = cmap[offset + 1];
          *pptr++ = cmap[offset + 2];

          pptr ++;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          offset = pixel[0] + pixel[0] + pixel[0];

          *pptr++ = cmap[offset];
          *pptr++ = cmap[offset + 1];
          *pptr++ = cmap[offset + 2];
          *pptr++ = ((gint) maskval * pixel[1]) >> 8;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, pptr + alpha);

          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = pixel[0];

          pptr ++;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = pixel[0];
          *pptr++ = ((gint) maskval * pixel[1]) >> 8;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pptr);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, pptr + alpha);

          pptr += bytes;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_RGBA_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pptr);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr[alpha] = ((gint) maskval * pptr[alpha]) >> 8;

          pptr += bytes;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    default:
      return;
    }

  gdk_draw_pixbuf (dest, NULL, row, 0, 0, x1, y, x2 - x1, 1,
                   GDK_RGB_DITHER_NONE, 0, 0);
}

static void
gimp_display_shell_trace_tri_edge (gint *dest,
                                   gint  x1,
                                   gint  y1,
                                   gint  x2,
                                   gint  y2)
{
  const gint  dy = y2 - y1;
  gint        dx;
  gchar       xdir;
  gint        errorterm;
  gint        b;
  gint       *dptr;

  if (dy <= 0)
    return;

  g_return_if_fail (dest != NULL);

  b         = 0;
  errorterm = 0;
  dptr      = dest;

  if (x2 < x1)
    {
      dx = x1 - x2;
      xdir = -1;
    }
  else
    {
      dx = x2 - x1;
      xdir = 1;
    }

  if (dx >= dy)
    {
      b = dy;
      while (b --)
        {
          *dptr = x1;
          errorterm += dx;

          while (errorterm > dy)
            {
              x1 += xdir;
              errorterm -= dy;
            }

          dptr ++;
        }
    }
  else if (dy >= dx)
    {
      b = dy;
      while (b --)
        {
          *dptr = x1;
          errorterm += dx;

          if (errorterm > dy)
            {
              x1 += xdir;
              errorterm -= dy;
            }

          dptr ++;
        }
    }
  else if (dx == 0)
    {
      b = dy;
      while (b --)
        {
          *dptr = x1;

          dptr ++;
        }
    }
  else /* dy == dx */
    {
      b = dy;
      while (b --)
        {
          *dptr = x1;
          x1 += xdir;

          dptr ++;
        }
    }
}
