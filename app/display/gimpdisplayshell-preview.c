/* The GIMP -- an image manipulation program
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

#include "display-types.h"
#include "tools/tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimpchannel.h"

#include "base/tile-manager.h"

#include "tools/gimptransformtool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-preview.h"
#include "gimpdisplayshell-transform.h"

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
static void    gimp_display_shell_draw_quad_row      (GimpDrawable *texture,
                                                      GdkDrawable  *dest,
                                                      GdkPixbuf    *row,
                                                      gint          x1,
                                                      gfloat        u1,
                                                      gfloat        v1,
                                                      gint          x2,
                                                      gfloat        u2,
                                                      gfloat        v2,
                                                      gint          y);
static void    gimp_display_shell_draw_quad_row_mask (GimpDrawable *texture,
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
static void    gimp_display_shell_trace_quad_edge    (gint         *dest,
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

      tool = tool_manager_get_active (shell->gdisp->gimage->gimp);

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
          gdouble      dx [4], dy [4];
          gint          x [4],  y [4];
          gfloat        u [4],  v [4];
          GimpChannel *mask;
          gint         mask_x1, mask_y1;
          gint         mask_x2, mask_y2;
          gint         mask_offx, mask_offy;
          gint         i;

          gimp_display_shell_transform_xy_f (shell, tr_tool->tx1, tr_tool->ty1,
                                             dx + 0, dy + 0, FALSE);
          gimp_display_shell_transform_xy_f (shell, tr_tool->tx2, tr_tool->ty2,
                                             dx + 1, dy + 1, FALSE);
          gimp_display_shell_transform_xy_f (shell, tr_tool->tx3, tr_tool->ty3,
                                             dx + 2, dy + 2, FALSE);
          gimp_display_shell_transform_xy_f (shell, tr_tool->tx4, tr_tool->ty4,
                                             dx + 3, dy + 3, FALSE);

          for (i = 0; i < 4; i++)
            {
              x [i] = (gint) dx [i];
              y [i] = (gint) dy [i];
            }

          mask = NULL;
          mask_offx = mask_offy = 0;

          if (gimp_drawable_mask_bounds (tool->drawable,
                                         &mask_x1, &mask_y1,
                                         &mask_x2, &mask_y2))
            {
              mask = gimp_image_get_mask (shell->gdisp->gimage);

              gimp_item_offsets (GIMP_ITEM (tool->drawable),
                                 &mask_offx, &mask_offy);
            }

          u [0] = mask_x1;  v [0] = mask_y1;
          u [1] = mask_x2;  v [1] = mask_y1;
          u [2] = mask_x1;  v [2] = mask_y2;
          u [3] = mask_x2;  v [3] = mask_y2;

          gimp_display_shell_draw_quad (tool->drawable,
                              GDK_DRAWABLE (GTK_WIDGET (shell->canvas)->window),
                              mask, mask_offx, mask_offy,
                              x, y, u, v);
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

  left = right = NULL;
  dul = dvl = dur = dvr = 0;
  u_l = v_l = u_r = v_r = 0;

  gdk_drawable_get_size (dest, &dwidth, &dheight);

  /* is the preview in the window? */
  {
    gboolean in_window_x, in_window_y;

    in_window_x = in_window_y = FALSE;
    for (j = 0; j < 4; j++)
      {
        if (x [j] >= 0)
          in_window_x = TRUE;
        if (y [j] >= 0)
          in_window_y = TRUE;
      }
    if (! (in_window_x && in_window_y))
      return;

    in_window_x = in_window_y = FALSE;
    for (j = 0; j < 4; j++)
      {
        if (x [j] < dwidth)
          in_window_x = TRUE;
        if (y [j] < dheight)
          in_window_y = TRUE;
      }
    if (! (in_window_x && in_window_y))
      return;
  }

  row = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                        mask ? TRUE : gimp_drawable_has_alpha (texture),
                        8, dwidth, 1);
  g_return_if_fail (row != NULL);

  /* sort vertices in order of y-coordinate */

  for (j = 0; j < 4; j++)
    for (k = j + 1; k < 4; k++)
      if (y [k] < y [j])
        {
          gint tmp;
          gfloat ftmp;

          tmp  = y [k];  y [k] = y [j];  y [j] = tmp;
          tmp  = x [k];  x [k] = x [j];  x [j] = tmp;
          ftmp = u [k];  u [k] = u [j];  u [j] = ftmp;
          ftmp = v [k];  v [k] = v [j];  v [j] = ftmp;
        }

  if (y [3] == y [0])
    return;

  l_edge = g_malloc ((y [3] - y [0]) * sizeof (gint));
  r_edge = g_malloc ((y [3] - y [0]) * sizeof (gint));

  /* draw the quad */

#define QUAD_TRACE_L_EDGE(a, b) \
      if (y [a] != y [b]) \
        { \
          gimp_display_shell_trace_quad_edge (l_edge, \
                                              x [a], y [a], \
                                              x [b], y [b]); \
          left = l_edge; \
          dul  = (u [b] - u [a]) / (y [b] - y [a]); \
          dvl  = (v [b] - v [a]) / (y [b] - y [a]); \
          u_l  = u [a]; \
          v_l  = v [a]; \
        }

#define QUAD_TRACE_R_EDGE(a, b) \
      if (y [a] != y [b]) \
        { \
          gimp_display_shell_trace_quad_edge (r_edge, \
                                              x [a], y [a], \
                                              x [b], y [b]); \
          right = r_edge; \
          dur   = (u [b] - u [a]) / (y [b] - y [a]); \
          dvr   = (v [b] - v [a]) / (y [b] - y [a]); \
          u_r   = u [a]; \
          v_r   = v [a]; \
        }

#define QUAD_DRAW_SECTION(a, b) \
      if (y [a] != y [b]) \
        for (ry = y [a]; ry < y [b]; ry++) \
          { \
            if (ry >= 0 && ry < dheight) \
              { \
                if (mask) \
                  gimp_display_shell_draw_quad_row_mask \
                                                    (texture, dest, row, \
                                                    mask, mask_offx, mask_offy,\
                                                    *left, u_l, v_l, \
                                                    *right, u_r, v_r, \
                                                    ry); \
                else \
                  gimp_display_shell_draw_quad_row (texture, dest, row, \
                                                    *left, u_l, v_l, \
                                                    *right, u_r, v_r, \
                                                    ry); \
              } \
            left ++;      right ++; \
            u_l += dul;   v_l += dvl; \
            u_r += dur;   v_r += dvr; \
          } \


  if ((((x [0] > x [1]) && (x [3] > x [2])) ||
       ((x [0] < x [1]) && (x [3] < x [2]))) &&
      (! (((x [0] > x [2]) && (x [0] < x [1]) && (x [3] < x [2])) ||
          ((x [0] < x [2]) && (x [0] > x [1]) && (x [3] > x [2])) ||
          ((x [3] > x [1]) && (x [3] < x [2]) && (x [0] < x [1])) ||
          ((x [3] < x [1]) && (x [3] > x [2]) && (x [0] > x [2])))))
    {
      /*
       *     v0
       *       |--__
       * _____ |....--_ v1 ____
       *       |       |
       *       |       |
       * _____ |.....__| ______
       *       |__---   v2
       *     v3
       */

      QUAD_TRACE_L_EDGE (0, 3);

      QUAD_TRACE_R_EDGE (0, 1);
      QUAD_DRAW_SECTION (0, 1);  /* top section */

      QUAD_TRACE_R_EDGE (1, 2);
      QUAD_DRAW_SECTION (1, 2);  /* middle section */

      QUAD_TRACE_R_EDGE (2, 3);
      QUAD_DRAW_SECTION (2, 3);  /* bottom section */
    }
  else
    {
      /*
       *           v0
       *           /-___
       * -------- /.....-- v1 ---
       *         /       /
       * ___ v2 /__...../________
       *           ---_/
       *               v3
       */

      QUAD_TRACE_L_EDGE (0, 2);

      QUAD_TRACE_R_EDGE (0, 1);
      QUAD_DRAW_SECTION (0, 1);  /* top section */

      QUAD_TRACE_R_EDGE (1, 3);

      QUAD_DRAW_SECTION (1, 2);  /* middle section */

      QUAD_TRACE_L_EDGE (2, 3);
      QUAD_DRAW_SECTION (2, 3);  /* bottom section */
    }

#undef QUAD_TRACE_L_EDGE
#undef QUAD_TRACE_R_EDGE
#undef QUAD_DRAW_SECTION

  g_object_unref (row);
  g_free (l_edge);
  g_free (r_edge);
}

static void
gimp_display_shell_draw_quad_row (GimpDrawable *texture,
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
  TileManager *tiles;
  guchar      *pptr;
  guchar       bytes;
  gfloat       u, v;
  gfloat       du, dv;
  gint         dx;
  guchar       pixel [4];
  guchar      *cmap;
  gint         offset;

  if (! (x2 - x1))
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
  tiles = gimp_drawable_data (texture);

  if (x1 > x2)
    {
      gint tmp;
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
    return;
  if (x2 < 0)
    return;
  else if (x2 > gdk_pixbuf_get_width (row))
    x2 = gdk_pixbuf_get_width (row);


  dx = x2 - x1;

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_cmap (texture);

      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel [0] + pixel [0] + pixel [0];

          *pptr++ = cmap [offset];
          *pptr++ = cmap [offset + 1];
          *pptr++ = cmap [offset + 2];

          u += du;
          v += dv;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      cmap = gimp_drawable_cmap (texture);

      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel [0] + pixel [0] + pixel [0];

          *pptr++ = cmap [offset];
          *pptr++ = cmap [offset + 1];
          *pptr++ = cmap [offset + 2];
          *pptr++ = pixel [1];

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = pixel [0];

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = pixel [1];

          u += du;
          v += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      while (dx --)
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
gimp_display_shell_draw_quad_row_mask (GimpDrawable *texture,
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
  TileManager *tiles, *masktiles;
  guchar      *pptr;
  guchar       bytes, alpha;
  gfloat       u, v;
  gfloat       mu, mv;
  gfloat       du, dv;
  gint         dx;
  guchar       pixel [4], maskval;
  guchar      *cmap;
  gint         offset;

  if (! (x2 - x1))
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
  tiles     = gimp_drawable_data (texture);
  masktiles = gimp_drawable_data (GIMP_DRAWABLE (mask));

  if (x1 > x2)
    {
      gint tmp;
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
    return;
  if (x2 < 0)
    return;
  else if (x2 > gdk_pixbuf_get_width (row))
    x2 = gdk_pixbuf_get_width (row);

  mu = u + mask_offx;
  mv = v + mask_offy;
  dx = x2 - x1;

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_cmap (texture);

      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, pptr + alpha);

          offset = pixel [0] + pixel [0] + pixel [0];

          *pptr++ = cmap [offset];
          *pptr++ = cmap [offset + 1];
          *pptr++ = cmap [offset + 2];

          pptr ++;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      cmap = gimp_drawable_cmap (texture);

      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          offset = pixel [0] + pixel [0] + pixel [0];

          *pptr++ = cmap [offset];
          *pptr++ = cmap [offset + 1];
          *pptr++ = cmap [offset + 2];
          *pptr++ = ((gint) maskval * pixel [1]) >> 8;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, pptr + alpha);

          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = pixel [0];

          pptr ++;
          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = pixel [0];
          *pptr++ = ((gint) maskval * pixel [1]) >> 8;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
      while (dx --)
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
      while (dx --)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pptr);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr [alpha] = ((gint) maskval * pptr [alpha]) >> 8;

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
gimp_display_shell_trace_quad_edge (gint *dest,
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

  if (dy == 0)
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

