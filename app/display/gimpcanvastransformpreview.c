/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastransformpreview.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display/display-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "gimpcanvas.h"
#include "gimpcanvastransformpreview.h"
#include "gimpdisplayshell.h"


#define INT_MULT(a,b,t)    ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
#define INT_MULT3(a,b,c,t) ((t) = (a) * (b) * (c) + 0x7F5B, \
                           ((((t) >> 7) + (t)) >> 16))

#define MAX_SUB_COLS       6 /* number of columns and  */
#define MAX_SUB_ROWS       6 /* rows to use in perspective preview subdivision */


enum
{
  PROP_0,
  PROP_DRAWABLE,
  PROP_TRANSFORM,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_OPACITY
};


typedef struct _GimpCanvasTransformPreviewPrivate GimpCanvasTransformPreviewPrivate;

struct _GimpCanvasTransformPreviewPrivate
{
  GimpDrawable      *drawable;
  GimpMatrix3        transform;
  gdouble            x1, y1;
  gdouble            x2, y2;
  gdouble            opacity;
};

#define GET_PRIVATE(transform_preview) \
        G_TYPE_INSTANCE_GET_PRIVATE (transform_preview, \
                                     GIMP_TYPE_CANVAS_TRANSFORM_PREVIEW, \
                                     GimpCanvasTransformPreviewPrivate)


/*  local function prototypes  */

static void             gimp_canvas_transform_preview_set_property (GObject        *object,
                                                                    guint           property_id,
                                                                    const GValue   *value,
                                                                    GParamSpec     *pspec);
static void             gimp_canvas_transform_preview_get_property (GObject        *object,
                                                                    guint           property_id,
                                                                    GValue         *value,
                                                                    GParamSpec     *pspec);

static void             gimp_canvas_transform_preview_draw         (GimpCanvasItem *item,
                                                                    cairo_t        *cr);
static cairo_region_t * gimp_canvas_transform_preview_get_extents  (GimpCanvasItem *item);

static void   gimp_canvas_transform_preview_draw_quad         (GimpDrawable    *texture,
                                                               cairo_t         *cr,
                                                               GimpChannel     *mask,
                                                               gint             mask_offx,
                                                               gint             mask_offy,
                                                               gint            *x,
                                                               gint            *y,
                                                               gfloat          *u,
                                                               gfloat          *v,
                                                               guchar           opacity);
static void   gimp_canvas_transform_preview_draw_tri          (GimpDrawable    *texture,
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
static void   gimp_canvas_transform_preview_draw_tri_row      (GimpDrawable    *texture,
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
static void   gimp_canvas_transform_preview_draw_tri_row_mask (GimpDrawable    *texture,
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
static void   gimp_canvas_transform_preview_trace_tri_edge    (gint            *dest,
                                                               gint             x1,
                                                               gint             y1,
                                                               gint             x2,
                                                               gint             y2);


G_DEFINE_TYPE (GimpCanvasTransformPreview, gimp_canvas_transform_preview,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_transform_preview_parent_class


static void
gimp_canvas_transform_preview_class_init (GimpCanvasTransformPreviewClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_transform_preview_set_property;
  object_class->get_property = gimp_canvas_transform_preview_get_property;

  item_class->draw           = gimp_canvas_transform_preview_draw;
  item_class->get_extents    = gimp_canvas_transform_preview_get_extents;

  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   gimp_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasTransformPreviewPrivate));
}

static void
gimp_canvas_transform_preview_init (GimpCanvasTransformPreview *transform_preview)
{
}

static void
gimp_canvas_transform_preview_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      private->drawable = g_value_get_object (value); /* don't ref */
      break;

    case PROP_TRANSFORM:
      {
        GimpMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          gimp_matrix3_identity (&private->transform);
      }
      break;

    case PROP_X1:
      private->x1 = g_value_get_double (value);
      break;

    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      break;

    case PROP_X2:
      private->x2 = g_value_get_double (value);
      break;

    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      break;

    case PROP_OPACITY:
      private->opacity = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_transform_preview_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, private->drawable);
      break;

    case PROP_TRANSFORM:
      g_value_set_boxed (value, &private->transform);
      break;

    case PROP_X1:
      g_value_set_double (value, private->x1);
      break;

    case PROP_Y1:
      g_value_set_double (value, private->y1);
      break;

    case PROP_X2:
      g_value_set_double (value, private->x2);
      break;

    case PROP_Y2:
      g_value_set_double (value, private->y2);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, private->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_canvas_transform_preview_transform (GimpCanvasItem        *item,
                                         cairo_rectangle_int_t *extents)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  gdouble                            tx1, ty1;
  gdouble                            tx2, ty2;
  gdouble                            tx3, ty3;
  gdouble                            tx4, ty4;

  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y1,
                                &tx1, &ty1);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y1,
                                &tx2, &ty2);
  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y2,
                                &tx3, &ty3);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y2,
                                &tx4, &ty4);

  if (! gimp_transform_polygon_is_convex (tx1, ty1,
                                          tx2, ty2,
                                          tx3, ty3,
                                          tx4, ty4))
    return FALSE;

  if (extents)
    {
      gdouble dx1, dy1;
      gdouble dx2, dy2;
      gdouble dx3, dy3;
      gdouble dx4, dy4;

      gimp_canvas_item_transform_xy_f (item,
                                       tx1, ty1,
                                       &dx1, &dy1);
      gimp_canvas_item_transform_xy_f (item,
                                       tx2, ty2,
                                       &dx2, &dy2);
      gimp_canvas_item_transform_xy_f (item,
                                       tx3, ty3,
                                       &dx3, &dy3);
      gimp_canvas_item_transform_xy_f (item,
                                       tx4, ty4,
                                       &dx4, &dy4);

      extents->x      = (gint) floor (MIN4 (dx1, dx2, dx3, dx4));
      extents->y      = (gint) floor (MIN4 (dy1, dy2, dy3, dy4));
      extents->width  = (gint) ceil (MAX4 (dx1, dx2, dx3, dx4));
      extents->height = (gint) ceil (MAX4 (dy1, dy2, dy3, dy4));

      extents->width  -= extents->x;
      extents->height -= extents->y;
    }

  return TRUE;
}

static void
gimp_canvas_transform_preview_draw (GimpCanvasItem *item,
                                    cairo_t        *cr)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  GimpChannel                       *mask;
  gint                               mask_x1, mask_y1;
  gint                               mask_x2, mask_y2;
  gint                               mask_offx, mask_offy;
  gint                               columns, rows;
  gint                               j, k, sub;

   /* x and y get filled with the screen coordinates of each corner of
    * each quadrilateral subdivision of the transformed area. u and v
    * are the corresponding points in the mask
    */
  gfloat                             du, dv, dx, dy;
  gint                               x[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  gint                               y[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  gfloat                             u[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  gfloat                             v[MAX_SUB_COLS * MAX_SUB_ROWS][4];
  guchar                             opacity;

  opacity = private->opacity * 255.999;

  /* only draw convex polygons */
  if (! gimp_canvas_transform_preview_transform (item, NULL))
    return;

  mask      = NULL;
  mask_offx = 0;
  mask_offy = 0;

  if (gimp_item_mask_bounds (GIMP_ITEM (private->drawable),
                             &mask_x1, &mask_y1,
                             &mask_x2, &mask_y2))
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (private->drawable));

      mask = gimp_image_get_mask (image);

      gimp_item_get_offset (GIMP_ITEM (private->drawable),
                            &mask_offx, &mask_offy);
    }

  if (! gimp_matrix3_is_affine (&private->transform))
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

  dx = (private->x2 - private->x1) / ((gfloat) columns);
  dy = (private->y2 - private->y1) / ((gfloat) rows);

  du = (mask_x2 - mask_x1) / ((gfloat) columns);
  dv = (mask_y2 - mask_y1) / ((gfloat) rows);

#define CALC_VERTEX(col, row, sub, index)                       \
  {                                                             \
    gdouble tx1, ty1;                                           \
    gdouble tx2, ty2;                                           \
                                                                \
    tx2 = private->x1 + (dx * (col + (index & 1)));             \
    ty2 = private->y1 + (dy * (row + (index >> 1)));            \
                                                                \
    gimp_matrix3_transform_point (&private->transform,          \
                                  tx2, ty2,                     \
                                  &tx1, &ty1);                  \
                                                                \
    gimp_canvas_item_transform_xy_f (item,                      \
                                     tx1, ty1,                  \
                                     &tx2, &ty2);               \
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
    gimp_canvas_transform_preview_draw_quad (private->drawable, cr,
                                             mask, mask_offx, mask_offy,
                                             x[j], y[j], u[j], v[j],
                                             opacity);
}

static cairo_region_t *
gimp_canvas_transform_preview_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;

  if (gimp_canvas_transform_preview_transform (item, &rectangle))
    return cairo_region_create_rectangle (&rectangle);

  return NULL;
}

GimpCanvasItem *
gimp_canvas_transform_preview_new (GimpDisplayShell  *shell,
                                   GimpDrawable      *drawable,
                                   const GimpMatrix3 *transform,
                                   gdouble            x1,
                                   gdouble            y1,
                                   gdouble            x2,
                                   gdouble            y2)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  return g_object_new (GIMP_TYPE_CANVAS_TRANSFORM_PREVIEW,
                       "shell",       shell,
                       "drawable",    drawable,
                       "transform",   transform,
                       "x1",          x1,
                       "y1",          y1,
                       "x2",          x2,
                       "y2",          y2,
                       NULL);
}


/*  private functions  */

/**
 * gimp_canvas_transform_preview_draw_quad:
 * @texture:   the #GimpDrawable to be previewed
 * @cr:        the #cairo_t to draw to
 * @mask:      a #GimpChannel
 * @opacity:   the opacity of the preview
 *
 * Take a quadrilateral, divide it into two triangles, and draw those
 * with gimp_canvas_transform_preview_draw_tri().
 **/
static void
gimp_canvas_transform_preview_draw_quad (GimpDrawable *texture,
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

      gimp_canvas_transform_preview_draw_tri (texture, cr, area, minx, miny,
                                              mask, mask_offx, mask_offy,
                                              x, y, u, v, opacity);
      gimp_canvas_transform_preview_draw_tri (texture, cr, area, minx, miny,
                                              mask, mask_offx, mask_offy,
                                              x2, y2, u2, v2, opacity);

      cairo_surface_destroy (area);
    }
}

/**
 * gimp_canvas_transform_preview_draw_tri:
 * @texture:   the thing being transformed
 * @cr:        the #cairo_t to draw to
 * @area:      has prefetched pixel data of dest
 * @area_offx: x coordinate of area in dest
 * @area_offy: y coordinate of area in dest
 * @x:         Array of the three x coords of triangle
 * @y:         Array of the three y coords of triangle
 *
 * This draws a triangle onto dest by breaking it down into pixel
 * rows, and then calling gimp_canvas_transform_preview_draw_tri_row()
 * and gimp_canvas_transform_preview_draw_tri_row_mask() to do the
 * actual pixel changing.
 **/
static void
gimp_canvas_transform_preview_draw_tri (GimpDrawable    *texture,
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

  cairo_clip_extents (cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

  /* sort vertices in order of y-coordinate */

  for (j = 0; j < 2; j++)
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

  gimp_canvas_transform_preview_trace_tri_edge (l_edge, x[0], y[0], x[2], y[2]);

  left = l_edge;
  dul  = (u[2] - u[0]) / (y[2] - y[0]);
  dvl  = (v[2] - v[0]) / (y[2] - y[0]);
  u_l  = u[0];
  v_l  = v[0];

  if (y[0] != y[1])
    {
      gimp_canvas_transform_preview_trace_tri_edge (r_edge, x[0], y[0], x[1], y[1]);

      right = r_edge;
      dur   = (u[1] - u[0]) / (y[1] - y[0]);
      dvr   = (v[1] - v[0]) / (y[1] - y[0]);
      u_r   = u[0];
      v_r   = v[0];

      if (mask)
        for (ry = y[0]; ry < y[1]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_canvas_transform_preview_draw_tri_row_mask (texture, cr,
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
              gimp_canvas_transform_preview_draw_tri_row (texture, cr,
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
      gimp_canvas_transform_preview_trace_tri_edge (r_edge, x[1], y[1], x[2], y[2]);

      right = r_edge;
      dur   = (u[2] - u[1]) / (y[2] - y[1]);
      dvr   = (v[2] - v[1]) / (y[2] - y[1]);
      u_r   = u[1];
      v_r   = v[1];

      if (mask)
        for (ry = y[1]; ry < y[2]; ry++)
          {
            if (ry >= clip_y1 && ry < clip_y2)
              gimp_canvas_transform_preview_draw_tri_row_mask (texture, cr,
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
              gimp_canvas_transform_preview_draw_tri_row (texture, cr,
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
 * gimp_canvas_transform_preview_draw_tri_row:
 * @texture: the thing being transformed
 * @cr:      the #cairo_t to draw to
 * @area:    has prefetched pixel data of dest
 *
 * Called from gimp_canvas_transform_preview_draw_tri(), this draws a
 * single row of a triangle onto dest when there is not a mask. The
 * run (x1,y) to (x2,y) in dest corresponds to the run (u1,v1) to
 * (u2,v2) in texture.
 **/
static void
gimp_canvas_transform_preview_draw_tri_row (GimpDrawable    *texture,
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
  GeglBuffer *buffer;
  const Babl *format;
  guchar     *buf;
  guchar     *b;
  guchar     *pptr;      /* points into the pixels of a row of area */
  gint        bpp;
  gfloat      u, v;
  gfloat      du, dv;
  gint        dx;
  gint        samples;

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

  cairo_surface_flush (area);
  pptr = (cairo_image_surface_get_data (area)
          + (y - area_offy) * cairo_image_surface_get_stride (area)
          + (x1 - area_offx) * 4);

  buffer = gimp_drawable_get_buffer (texture);

  format = gegl_buffer_get_format (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);
  buf    = g_alloca (bpp * dx);

  samples = dx;
  b       = buf;

  while (samples--)
    {
      gegl_buffer_sample (buffer, (gint) u, (gint) v, NULL, b,
                          format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      b += bpp;

      u += du;
      v += dv;
    }

  if (opacity < 255)
    {
      guchar *u8_buffer = g_alloca (4 * dx);
      guchar *u8_b;

      babl_process (babl_fish (format,
                               babl_format ("R'G'B'A u8")),
                    buf, u8_buffer, dx);

      samples = dx;
      u8_b    = u8_buffer;

      while (samples--)
        {
          register gulong tmp;

          u8_b[3] = INT_MULT (opacity, u8_b[3], tmp);

          u8_b += 4;
        }

      babl_process (babl_fish (babl_format ("R'G'B'A u8"),
                               babl_format ("cairo-ARGB32")),
                    u8_buffer, pptr, dx);
    }
  else
    {
      babl_process (babl_fish (format,
                               babl_format ("cairo-ARGB32")),
                    buf, pptr, dx);
    }

  cairo_surface_mark_dirty (area);

  cairo_set_source_surface (cr, area, area_offx, area_offy);
  cairo_rectangle (cr, x1, y, x2 - x1, 1);
  cairo_fill (cr);
}

/**
 * gimp_canvas_transform_preview_draw_tri_row_mask:
 *
 * Called from gimp_canvas_transform_preview_draw_tri(), this draws a
 * single row of a triangle onto dest, when there is a mask.
 **/
static void
gimp_canvas_transform_preview_draw_tri_row_mask (GimpDrawable    *texture,
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
  GeglBuffer *buffer;
  GeglBuffer *mask_buffer;
  const Babl *format;
  const Babl *mask_format;
  guchar     *buf;
  guchar     *mask_buf;
  guchar     *b;
  guchar     *mask_b;
  guchar     *pptr;              /* points into the pixels of area        */
  gint        bpp;
  gint        mask_bpp;
  gfloat      u, v;
  gfloat      mu, mv;
  gfloat      du, dv;
  gint        dx;
  gint        samples;

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

  cairo_surface_flush (area);
  pptr = (cairo_image_surface_get_data (area)
          + (y - area_offy) * cairo_image_surface_get_stride (area)
          + (x1 - area_offx) * 4);

  buffer      = gimp_drawable_get_buffer (texture);
  mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

  format      = gegl_buffer_get_format (buffer);
  mask_format = gegl_buffer_get_format (mask_buffer);
  bpp         = babl_format_get_bytes_per_pixel (format);
  mask_bpp    = babl_format_get_bytes_per_pixel (mask_format);
  buf         = g_alloca (bpp * dx);
  mask_buf    = g_alloca (mask_bpp * dx);

  samples = dx;
  b       = buf;
  mask_b  = mask_buf;

  while (samples--)
    {
      gegl_buffer_sample (buffer, (gint) u, (gint) v, NULL, b,
                          format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample (mask_buffer, (gint) mu, (gint) mv, NULL, mask_b,
                          mask_format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      b      += bpp;
      mask_b += mask_bpp;

      u += du;
      v += dv;
      mu += du;
      mv += dv;
    }

  {
    guchar *u8_buffer      = g_alloca (4 * dx);
    guchar *u8_mask_buffer = g_alloca (dx);
    guchar *u8_b;
    guchar *u8_mask_b;

    babl_process (babl_fish (format,
                             babl_format ("R'G'B'A u8")),
                  buf, u8_buffer, dx);
    babl_process (babl_fish (mask_format,
                             babl_format ("Y u8")),
                  mask_buf, u8_mask_buffer, dx);

    samples   = dx;
    u8_b      = u8_buffer;
    u8_mask_b = u8_mask_buffer;

    while (samples--)
      {
        register gulong tmp;

        u8_b[3] = INT_MULT3 (opacity, *u8_mask_b, u8_b[3], tmp);

        u8_b      += 4;
        u8_mask_b += 1;
      }

    babl_process (babl_fish (babl_format ("R'G'B'A u8"),
                             babl_format ("cairo-ARGB32")),
                  u8_buffer, pptr, dx);
  }

  cairo_surface_mark_dirty (area);

  cairo_set_source_surface (cr, area, area_offx, area_offy);
  cairo_rectangle (cr, x1, y, x2 - x1, 1);
  cairo_fill (cr);
}

/**
 * gimp_canvas_transform_preview_trace_tri_edge:
 * @dest: x coordinates are placed in this array
 *
 * Find the x coordinates for a line that runs from (x1,y1) to
 * (x2,y2), corresponding to the y coordinates y1 to y2-1. So dest
 * must be large enough to hold y2-y1 values.
 **/
static void
gimp_canvas_transform_preview_trace_tri_edge (gint *dest,
                                              gint  x1,
                                              gint  y1,
                                              gint  x2,
                                              gint  y2)
{
  const gint  dx   = x2 - x1;
  const gint  dy   = y2 - y1;
  gint        b    = dy;
  gint       *dptr = dest;
  gint        errorterm;

  if (dy <= 0)
    return;

  g_return_if_fail (dest != NULL);

  if (dx >= 0)
    {
      errorterm = 0;

      while (b--)
        {
          *dptr++ = x1;

          errorterm += dx;

          while (errorterm > dy)
            {
              x1++;
              errorterm -= dy;
            }
        }
    }
  else
    {
      errorterm = dy;

      while (b--)
        {
          while (errorterm > dy)
            {
              x1--;
              errorterm -= dy;
            }

          /* dx is negative here, so this is effectively an addition: */
          errorterm -= dx;

          *dptr++ = x1;
        }
    }
}
