/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "base/tile-manager.h"

#include "tools/gimpperspectivetool.h"
#include "tools/gimptransformtool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-preview.h"
#include "gimpdisplayshell-transform.h"


#define INT_MULT(a,b,t)     ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c) + 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))


#define MAX_SUB_COLS 6     /* number of columns and  */
#define MAX_SUB_ROWS 6     /* rows to use in perspective preview subdivision */

/*  local function prototypes  */

static void    gimp_display_shell_draw_quad          (GimpDrawable    *texture,
                                                      cairo_t         *cr,
                                                      GimpChannel     *mask,
                                                      gint             mask_offx,
                                                      gint             mask_offy,
                                                      gint            *x,
                                                      gint            *y,
                                                      gfloat          *u,
                                                      gfloat          *v,
                                                      guchar           opacity);
static void    gimp_display_shell_draw_tri           (GimpDrawable    *texture,
                                                      cairo_t         *cr,
                                                      cairo_surface_t *area,
                                                      gint             area_offx,
                                                      gint             area_offy,
                                                      GimpChannel     *mask,
                                                      gint             mask_offx,
                                                      gint             mask_offy,
                                                      gint            *x,
                                                      gint            *y,
                                                      gfloat          *u,
                                                      gfloat          *v,
                                                      guchar           opacity);
static void    gimp_display_shell_draw_tri_row       (GimpDrawable    *texture,
                                                      cairo_t         *cr,
                                                      cairo_surface_t *area,
                                                      gint             area_offx,
                                                      gint             area_offy,
                                                      gint             x1,
                                                      gfloat           u1,
                                                      gfloat           v1,
                                                      gint             x2,
                                                      gfloat           u2,
                                                      gfloat           v2,
                                                      gint             y,
                                                      guchar           opacity);
static void    gimp_display_shell_draw_tri_row_mask  (GimpDrawable    *texture,
                                                      cairo_t         *cr,
                                                      cairo_surface_t *area,
                                                      gint             area_offx,
                                                      gint             area_offy,
                                                      GimpChannel     *mask,
                                                      gint             mask_offx,
                                                      gint             mask_offy,
                                                      gint             x1,
                                                      gfloat           u1,
                                                      gfloat           v1,
                                                      gint             x2,
                                                      gfloat           u2,
                                                      gfloat           v2,
                                                      gint             y,
                                                      guchar           opacity);
static void    gimp_display_shell_trace_tri_edge     (gint            *dest,
                                                      gint             x1,
                                                      gint             y1,
                                                      gint             x2,
                                                      gint             y2);

/*  public functions  */

/**
 * gimp_display_shell_preview_transform:
 * @shell: the #GimpDisplayShell
 *
 * If the active tool as reported by tool_manager_get_active() is a
 * #GimpTransformTool and the tool has a valid drawable, and the tool
 * has use_grid true (which, incidentally, is not the same thing as
 * the preview type), and the area of the transformed preview is
 * convex, then proceed with drawing the preview.
 *
 * The preview area is divided into 1 or more quadrilaterals, and
 * drawn with gimp_display_shell_draw_quad(), which in turn breaks it
 * down into 2 triangles, and draws row by row. If the tool is the
 * Perspective tool, then more small quadrilaterals are used to
 * compensate for the little rectangles not being the same size. In
 * other words, all the transform tools are affine transformations
 * except perspective, so approximate it with a few subdivisions.
 **/
void
gimp_display_shell_preview_transform (GimpDisplayShell *shell,
                                      cairo_t          *cr)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;
  gdouble            z1, z2, z3, z4;

  GimpChannel *mask;
  gint         mask_x1, mask_y1;
  gint         mask_x2, mask_y2;
  gint         mask_offx, mask_offy;

  gint         columns, rows;
  gint         j, k, sub;

   /* x and y get filled with the screen coordinates of each corner of
    * each quadrilateral subdivision of the transformed area. u and v
    * are the corresponding points in the mask
    */
  gfloat       du, dv, dx, dy;
  gint         x[MAX_SUB_COLS * MAX_SUB_ROWS][4],
               y[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  gfloat       u[MAX_SUB_COLS * MAX_SUB_ROWS][4],
               v[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  guchar       opacity = 255;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  if (! gimp_display_shell_get_show_transform (shell) || ! shell->canvas)
    return;

  tool = tool_manager_get_active (shell->display->gimp);

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

  if (! ((z1 * z2 > 0) && (z3 * z4 > 0)))
    return;

  /* take opacity from the tool options */
  {
    gdouble value;

    g_object_get (gimp_tool_get_options (tool),
                  "preview-opacity", &value,
                  NULL);

    opacity = value * 255.999;
  }

  mask = NULL;
  mask_offx = mask_offy = 0;

  if (gimp_item_mask_bounds (GIMP_ITEM (tool->drawable),
                             &mask_x1, &mask_y1,
                             &mask_x2, &mask_y2))
    {
      mask = gimp_image_get_mask (gimp_display_get_image (shell->display));

      gimp_item_get_offset (GIMP_ITEM (tool->drawable),
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

#define CALC_VERTEX(col, row, sub, index)                       \
  {                                                             \
    gdouble tx1, ty1;                                           \
    gdouble tx2, ty2;                                           \
                                                                \
    u[sub][index] = tr_tool->x1 + (dx * (col + (index & 1)));   \
    v[sub][index] = tr_tool->y1 + (dy * (row + (index >> 1)));  \
                                                                \
    gimp_matrix3_transform_point (&tr_tool->transform,          \
                                  u[sub][index], v[sub][index], \
                                  &tx1, &ty1);                  \
                                                                \
    gimp_display_shell_transform_xy_f (shell,                   \
                                       tx1, ty1,                \
                                       &tx2, &ty2,              \
                                       FALSE);                  \
    x[sub][index] = (gint) tx2;                                 \
    y[sub][index] = (gint) ty2;                                 \
                                                                \
    u[sub][index] = mask_x1 + (du * (col + (index & 1)));       \
    v[sub][index] = mask_y1 + (dv * (row + (index >> 1)));      \
  }

#define COPY_VERTEX(subdest, idest, subsrc, isrc) \
  x[subdest][idest] = x[subsrc][isrc];            \
  y[subdest][idest] = y[subsrc][isrc];            \
  u[subdest][idest] = u[subsrc][isrc];            \
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
    gimp_display_shell_draw_quad (tool->drawable, cr,
                                  mask, mask_offx, mask_offy,
                                  x[j], y[j], u[j], v[j],
                                  opacity);
}


/*  private functions  */

/**
 * gimp_display_shell_draw_quad:
 * @texture:   the #GimpDrawable to be previewed
 * @cr:        the #cairo_t to draw to
 * @mask:      a #GimpChannel
 * @opacity:   the opacity of the preview
 *
 * Take a quadrilateral, divide it into two triangles, and draw those
 * with gimp_display_shell_draw_tri().
 **/
static void
gimp_display_shell_draw_quad (GimpDrawable *texture,
                              cairo_t      *cr,
                              GimpChannel  *mask,
                              gint          mask_offx,
                              gint          mask_offy,
                              gint         *x,
                              gint         *y,
                              gfloat       *u,
                              gfloat       *v,
                              guchar        opacity)
{
  gint    x2[3], y2[3];
  gfloat  u2[3], v2[3];
  gint    minx, maxx, miny, maxy; /* screen bounds of the quad        */
  gdouble clip_x1, clip_y1, clip_x2, clip_y2;
  gint    c;

  x2[0] = x[3];  y2[0] = y[3];  u2[0] = u[3];  v2[0] = v[3];
  x2[1] = x[2];  y2[1] = y[2];  u2[1] = u[2];  v2[1] = v[2];
  x2[2] = x[1];  y2[2] = y[1];  u2[2] = u[1];  v2[2] = v[1];

   /* Allocate a box around the quad to compute preview data into
    * and fill it with the original window contents.
    */

  cairo_clip_extents (cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

  /* find bounds of quad in order to only grab as much of dest as needed */

  minx = maxx = x[0];
  miny = maxy = y[0];
  for (c = 1; c < 4; c++)
    {
      if (x[c] < minx) minx = x[c];
      else if (x[c] > maxx) maxx = x[c];

      if (y[c] < miny) miny = y[c];
      else if (y[c] > maxy) maxy = y[c];
    }
  if (minx < clip_x1) minx = clip_x1;
  if (miny < clip_y1) miny = clip_y1;
  if (maxx > clip_x2) maxx = clip_x2;
  if (maxy > clip_y2) maxy = clip_y2;

  if (minx <= maxx && miny <= maxy)
    {
      cairo_surface_t *area;

      area = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                         maxx - minx + 1,
                                         maxy - miny + 1);

      g_return_if_fail (area != NULL);

      gimp_display_shell_draw_tri (texture, cr, area, minx, miny,
                                   mask, mask_offx, mask_offy,
                                   x, y, u, v, opacity);
      gimp_display_shell_draw_tri (texture, cr, area, minx, miny,
                                   mask, mask_offx, mask_offy,
                                   x2, y2, u2, v2, opacity);

      cairo_surface_destroy (area);
    }
}

/**
 * gimp_display_shell_draw_tri:
 * @texture:   the thing being transformed
 * @cr:        the #cairo_t to draw to
 * @area:      has prefetched pixel data of dest
 * @area_offx: x coordinate of area in dest
 * @area_offy: y coordinate of area in dest
 * @x:         Array of the three x coords of triangle
 * @y:         Array of the three y coords of triangle
 *
 * This draws a triangle onto dest by breaking it down into pixel rows, and
 * then calling gimp_display_shell_draw_tri_row() and
 * gimp_display_shell_draw_tri_row_mask() to do the actual pixel changing.
 **/
static void
gimp_display_shell_draw_tri (GimpDrawable    *texture,
                             cairo_t         *cr,
                             cairo_surface_t *area,
                             gint             area_offx,
                             gint             area_offy,
                             GimpChannel     *mask,
                             gint             mask_offx,
                             gint             mask_offy,
                             gint            *x,
                             gint            *y,
                             gfloat          *u, /* texture coords */
                             gfloat          *v, /* 0.0 ... tex width, height */
                             guchar           opacity)
{
  gdouble      clip_x1, clip_y1, clip_x2, clip_y2;
  gint         j, k;
  gint         ry;
  gint        *l_edge, *r_edge;    /* arrays holding x-coords of edge pixels */
  gint        *left,   *right;     /* temp pointers into l_edge and r_edge  */
  gfloat       dul, dvl, dur, dvr; /* left and right texture coord deltas  */
  gfloat       u_l, v_l, u_r, v_r; /* left and right texture coord pairs  */

  g_return_if_fail (GIMP_IS_DRAWABLE (texture));
  g_return_if_fail (area != NULL);

  g_return_if_fail (x != NULL && y != NULL && u != NULL && v != NULL);

  left = right = NULL;
  dul = dvl = dur = dvr = 0;
  u_l = v_l = u_r = v_r = 0;

  cairo_clip_extents (cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

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

      if (mask)
        for (ry = y[0]; ry < y[1]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_display_shell_draw_tri_row_mask (texture, cr,
                                                    area, area_offx, area_offy,
                                                    mask, mask_offx, mask_offy,
                                                    *left, u_l, v_l,
                                                    *right, u_r, v_r,
                                                    ry,
                                                    opacity);
            left ++;      right ++;
            u_l += dul;   v_l += dvl;
            u_r += dur;   v_r += dvr;
          }
      else
        for (ry = y[0]; ry < y[1]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_display_shell_draw_tri_row (texture, cr,
                                               area, area_offx, area_offy,
                                               *left, u_l, v_l,
                                               *right, u_r, v_r,
                                               ry,
                                               opacity);
            left ++;      right ++;
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

      if (mask)
        for (ry = y[1]; ry < y[2]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_display_shell_draw_tri_row_mask (texture, cr,
                                                    area, area_offx, area_offy,
                                                    mask, mask_offx, mask_offy,
                                                    *left,  u_l, v_l,
                                                    *right, u_r, v_r,
                                                    ry,
                                                    opacity);
            left ++;      right ++;
            u_l += dul;   v_l += dvl;
            u_r += dur;   v_r += dvr;
          }
      else
        for (ry = y[1]; ry < y[2]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_display_shell_draw_tri_row (texture, cr,
                                               area, area_offx, area_offy,
                                               *left,  u_l, v_l,
                                               *right, u_r, v_r,
                                               ry,
                                               opacity);
            left ++;      right ++;
            u_l += dul;   v_l += dvl;
            u_r += dur;   v_r += dvr;
          }
    }

  g_free (l_edge);
  g_free (r_edge);
}

/**
 * gimp_display_shell_draw_tri_row:
 * @texture: the thing being transformed
 * @cr:      the #cairo_t to draw to
 * @area:    has prefetched pixel data of dest
 *
 * Called from gimp_display_shell_draw_tri(), this draws a single row of a
 * triangle onto dest when there is not a mask. The run (x1,y) to (x2,y) in
 * dest corresponds to the run (u1,v1) to (u2,v2) in texture.
 **/
static void
gimp_display_shell_draw_tri_row (GimpDrawable    *texture,
                                 cairo_t         *cr,
                                 cairo_surface_t *area,
                                 gint             area_offx,
                                 gint             area_offy,
                                 gint             x1,
                                 gfloat           u1,
                                 gfloat           v1,
                                 gint             x2,
                                 gfloat           u2,
                                 gfloat           v2,
                                 gint             y,
                                 guchar           opacity)
{
  TileManager  *tiles;     /* used to get the source texture colors   */
  guchar       *pptr;      /* points into the pixels of a row of area */
  gfloat        u, v;
  gfloat        du, dv;
  gint          dx;
  guchar        pixel[4];
  const guchar *cmap;
  gint          offset;

  if (x2 == x1)
    return;

  g_return_if_fail (GIMP_IS_DRAWABLE (texture));
  g_return_if_fail (area != NULL);
  g_return_if_fail (cairo_image_surface_get_format (area) == CAIRO_FORMAT_ARGB32);

  /* make sure the pixel run goes in the positive direction */
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
  if (x1 < area_offx)
    {
      u += du * (area_offx - x1);
      v += dv * (area_offx - x1);
      x1 = area_offx;
    }
  else if (x1 > area_offx + cairo_image_surface_get_width (area) - 1)
    {
      return;
    }

  if (x2 < area_offx)
    {
      return;
    }
  else if (x2 > area_offx + cairo_image_surface_get_width (area) - 1)
    {
      x2 = area_offx + cairo_image_surface_get_width (area) - 1;
    }

  dx = x2 - x1;
  if (! dx)
    return;

  pptr = (cairo_image_surface_get_data (area)
          + (y - area_offy) * cairo_image_surface_get_stride (area)
          + (x1 - area_offx) * 4);

  tiles = gimp_drawable_get_tiles (texture);

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel[0] + pixel[0] + pixel[0];

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       cmap[offset + 0],
                                       cmap[offset + 1],
                                       cmap[offset + 2],
                                       opacity);

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          offset = pixel[0] + pixel[0] + pixel[0];

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       cmap[offset + 0],
                                       cmap[offset + 1],
                                       cmap[offset + 2],
                                       INT_MULT (opacity, pixel[1], tmp));

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       pixel[0],
                                       pixel[0],
                                       pixel[0],
                                       opacity);

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       pixel[0],
                                       pixel[0],
                                       pixel[0],
                                       INT_MULT (opacity, pixel[1], tmp));

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
      while (dx--)
        {
          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       pixel[0],
                                       pixel[1],
                                       pixel[2],
                                       opacity);

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    case GIMP_RGBA_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);

          GIMP_CAIRO_ARGB32_SET_PIXEL (pptr,
                                       pixel[0],
                                       pixel[1],
                                       pixel[2],
                                       INT_MULT (opacity, pixel[3], tmp));

          pptr += 4;

          u += du;
          v += dv;
        }
      break;

    default:
      return;
    }

  cairo_surface_mark_dirty (area);

  cairo_set_source_surface (cr, area, area_offx, area_offy);
  cairo_rectangle (cr, x1, y, x2 - x1, 1);
  cairo_fill (cr);
}

/**
 * gimp_display_shell_draw_tri_row_mask:
 *
 * Called from gimp_display_shell_draw_tri(), this draws a single row of a
 * triangle onto dest, when there is a mask.
 **/
static void
gimp_display_shell_draw_tri_row_mask (GimpDrawable    *texture,
                                      cairo_t         *cr,
                                      cairo_surface_t *area,
                                      gint             area_offx,
                                      gint             area_offy,
                                      GimpChannel     *mask,
                                      gint             mask_offx,
                                      gint             mask_offy,
                                      gint             x1,
                                      gfloat           u1,
                                      gfloat           v1,
                                      gint             x2,
                                      gfloat           u2,
                                      gfloat           v2,
                                      gint             y,
                                      guchar           opacity)
{

  TileManager  *tiles, *masktiles; /* used to get the source texture colors */
  guchar       *pptr;              /* points into the pixels of area        */
  gfloat        u, v;
  gfloat        mu, mv;
  gfloat        du, dv;
  gint          dx;
  guchar        pixel[4];
  guchar        maskval;
  const guchar *cmap;
  gint          offset;

  if (x2 == x1)
    return;

  g_return_if_fail (GIMP_IS_DRAWABLE (texture));
  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (area != NULL);
  g_return_if_fail (cairo_image_surface_get_format (area) == CAIRO_FORMAT_ARGB32);

  /* make sure the pixel run goes in the positive direction */
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
  if (x1 < area_offx)
    {
      u += du * (area_offx - x1);
      v += dv * (area_offx - x1);
      x1 = area_offx;
    }
  else if (x1 > area_offx + cairo_image_surface_get_width (area) - 1)
    {
      return;
    }

  if (x2 < area_offx)
    {
      return;
    }
  else if (x2 > area_offx + cairo_image_surface_get_width (area) - 1)
    {
      x2 = area_offx + cairo_image_surface_get_width (area) - 1;
    }

  dx = x2 - x1;
  if (! dx)
    return;

  mu = u + mask_offx;
  mv = v + mask_offy;

  pptr = (cairo_image_surface_get_data (area)
          + (y - area_offy) * cairo_image_surface_get_stride (area)
          + (x1 - area_offx) * 4);

  tiles     = gimp_drawable_get_tiles (texture);
  masktiles = gimp_drawable_get_tiles (GIMP_DRAWABLE (mask));

  switch (gimp_drawable_type (texture))
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_drawable_get_colormap (texture);

      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          offset = pixel[0] + pixel[0] + pixel[0];

          pptr[0] = cmap[offset + 0];
          pptr[1] = cmap[offset + 1];
          pptr[2] = cmap[offset + 2];
          pptr[3] = INT_MULT (opacity, maskval, tmp);

          pptr += 4;

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
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          offset = pixel[0] + pixel[0] + pixel[0];

          pptr[0] = cmap[offset + 0];
          pptr[1] = cmap[offset + 1];
          pptr[2] = cmap[offset + 2];
          pptr[3] = INT_MULT3 (opacity, maskval, pixel[1], tmp);

          pptr += 4;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAY_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr[0] = pixel[0];
          pptr[1] = pixel[0];
          pptr[2] = pixel[0];
          pptr[3] = INT_MULT (opacity, maskval, tmp);

          pptr += 4;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr[0] = pixel[0];
          pptr[1] = pixel[0];
          pptr[2] = pixel[0];
          pptr[3] = INT_MULT3 (opacity, maskval, pixel[1], tmp);

          pptr += 4;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_RGB_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr[0] = pixel[0];
          pptr[1] = pixel[1];
          pptr[2] = pixel[2];
          pptr[3] = INT_MULT (opacity, maskval, tmp);

          pptr += 4;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    case GIMP_RGBA_IMAGE:
      while (dx--)
        {
          register gulong tmp;

          read_pixel_data_1 (tiles, (gint) u, (gint) v, pixel);
          read_pixel_data_1 (masktiles, (gint) mu, (gint) mv, &maskval);

          pptr[0] = pixel[0];
          pptr[1] = pixel[1];
          pptr[2] = pixel[2];
          pptr[3] = INT_MULT3 (opacity, maskval, pixel[3], tmp);

          pptr += 4;

          u += du;
          v += dv;
          mu += du;
          mv += dv;
        }
      break;

    default:
      return;
    }

  cairo_surface_mark_dirty (area);

  cairo_set_source_surface (cr, area, area_offx, area_offy);
  cairo_rectangle (cr, x1, y, x2 - x1, 1);
  cairo_fill (cr);
}

/**
 * gimp_display_shell_trace_tri_edge:
 * @dest: x coordinates are placed in this array
 *
 * Find the x coordinates for a line that runs from (x1,y1) to (x2,y2),
 * corresponding to the y coordinates y1 to y2-1. So
 * dest must be large enough to hold y2-y1 values.
 **/
static void
gimp_display_shell_trace_tri_edge (gint *dest,
                                   gint  x1,
                                   gint  y1,
                                   gint  x2,
                                   gint  y2)
{
  const gint  dy = y2 - y1;
  gint        dx;
  gint        xdir;
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
