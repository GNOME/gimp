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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpcurve.h"
#include "gimpcurve-load.h"
#include "gimpcurve-save.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CURVE_TYPE
};


/*  local function prototypes  */

static void       gimp_curve_finalize         (GObject       *object);
static void       gimp_curve_set_property     (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);
static void       gimp_curve_get_property     (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);

static gint64     gimp_curve_get_memsize      (GimpObject    *object,
                                               gint64        *gui_size);

static void       gimp_curve_get_preview_size (GimpViewable  *viewable,
                                               gint           size,
                                               gboolean       popup,
                                               gboolean       dot_for_dot,
                                               gint          *width,
                                               gint          *height);
static gboolean   gimp_curve_get_popup_size   (GimpViewable  *viewable,
                                               gint           width,
                                               gint           height,
                                               gboolean       dot_for_dot,
                                               gint          *popup_width,
                                               gint          *popup_height);
static TempBuf  * gimp_curve_get_new_preview  (GimpViewable  *viewable,
                                               GimpContext   *context,
                                               gint           width,
                                               gint           height);
static gchar    * gimp_curve_get_description  (GimpViewable  *viewable,
                                               gchar        **tooltip);
static gchar    * gimp_curve_get_extension    (GimpData      *data);
static GimpData * gimp_curve_duplicate        (GimpData      *data);

static void       gimp_curve_plot             (GimpCurve     *curve,
                                               gint           p1,
                                               gint           p2,
                                               gint           p3,
                                               gint           p4);


G_DEFINE_TYPE (GimpCurve, gimp_curve, GIMP_TYPE_DATA)

#define parent_class gimp_curve_parent_class


static void
gimp_curve_class_init (GimpCurveClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize           = gimp_curve_finalize;
  object_class->set_property       = gimp_curve_set_property;
  object_class->get_property       = gimp_curve_get_property;

  gimp_object_class->get_memsize   = gimp_curve_get_memsize;

  viewable_class->default_stock_id = "FIXME";
  viewable_class->get_preview_size = gimp_curve_get_preview_size;
  viewable_class->get_popup_size   = gimp_curve_get_popup_size;
  viewable_class->get_new_preview  = gimp_curve_get_new_preview;
  viewable_class->get_description  = gimp_curve_get_description;

  data_class->save                 = gimp_curve_save;
  data_class->get_extension        = gimp_curve_get_extension;
  data_class->duplicate            = gimp_curve_duplicate;

  g_object_class_install_property (object_class, PROP_CURVE_TYPE,
                                   g_param_spec_enum ("curve-type", NULL, NULL,
                                                      GIMP_TYPE_CURVE_TYPE,
                                                      GIMP_CURVE_SMOOTH,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_curve_init (GimpCurve *curve)
{
  gimp_curve_reset (curve, TRUE);
}

static void
gimp_curve_finalize (GObject *object)
{
  GimpCurve *curve = GIMP_CURVE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curve_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpCurve *curve = GIMP_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      gimp_curve_set_curve_type (curve, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpCurve *curve = GIMP_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      g_value_set_enum (value, curve->curve_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_curve_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpCurve *curve   = GIMP_CURVE (object);
  gint64     memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_curve_get_preview_size (GimpViewable *viewable,
                             gint          size,
                             gboolean      popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  *width  = size;
  *height = size;
}

static gboolean
gimp_curve_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  GimpCurve *curve = GIMP_CURVE (viewable);

  *popup_width  = width  * 2;
  *popup_height = height * 2;

  return TRUE;
}

static TempBuf *
gimp_curve_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpCurve *curve = GIMP_CURVE (viewable);

  return NULL;
}

static gchar *
gimp_curve_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpCurve *curve = GIMP_CURVE (viewable);

  return g_strdup_printf ("%s", GIMP_OBJECT (curve)->name);
}


static gchar *
gimp_curve_get_extension (GimpData *data)
{
  return GIMP_CURVE_FILE_EXTENSION;
}

static GimpData *
gimp_curve_duplicate (GimpData *data)
{
  GimpCurve *curve = GIMP_CURVE (data);
  GimpCurve *new;

  new = g_object_new (GIMP_TYPE_CURVE,
                      "curve-type", curve->curve_type,
                      NULL);

  return GIMP_DATA (new);
}


/*  public functions  */

GimpData *
gimp_curve_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (GIMP_TYPE_CURVE,
                       "name", name,
                       NULL);
}

GimpData *
gimp_curve_get_standard (void)
{
  static GimpData *standard_curve = NULL;

  if (! standard_curve)
    {
      standard_curve = gimp_curve_new ("Standard");

      standard_curve->dirty = FALSE;
      gimp_data_make_internal (standard_curve);

      g_object_ref (standard_curve);
    }

  return standard_curve;
}

void
gimp_curve_reset (GimpCurve *curve,
                  gboolean   reset_type)
{
  gint i;

  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (reset_type)
    curve->curve_type = GIMP_CURVE_SMOOTH;

  for (i = 0; i < 256; i++)
    curve->curve[i] = i;

  for (i = 0; i < GIMP_CURVE_NUM_POINTS; i++)
    {
      curve->points[i][0] = -1;
      curve->points[i][1] = -1;
    }

  curve->points[0][0]  = 0;
  curve->points[0][1]  = 0;
  curve->points[GIMP_CURVE_NUM_POINTS - 1][0] = 255;
  curve->points[GIMP_CURVE_NUM_POINTS - 1][1] = 255;

  g_object_freeze_notify (G_OBJECT (curve));

  /* g_object_notify (G_OBJECT (curve), "points"); */
  /* g_object_notify (G_OBJECT (curve), "curve"); */
  g_object_notify (G_OBJECT (curve), "curve-type");

  g_object_thaw_notify (G_OBJECT (curve));
}

void
gimp_curve_set_curve_type (GimpCurve     *curve,
                           GimpCurveType  curve_type)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->curve_type != curve_type)
    {
      curve->curve_type = curve_type;

      if (curve_type == GIMP_CURVE_SMOOTH)
        {
          gint   i;
          gint32 index;

          /*  pick representative points from the curve and make them
           *  control points
           */
          for (i = 0; i <= 8; i++)
            {
              index = CLAMP0255 (i * 32);

              curve->points[i * 2][0] = index;
              curve->points[i * 2][1] = curve->curve[index];
            }
        }

      gimp_curve_calculate (curve);

      g_object_notify (G_OBJECT (curve), "curve-type");
      gimp_data_dirty (GIMP_DATA (curve));
    }
}

GimpCurveType
gimp_curve_get_curve_type (GimpCurve *curve)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), GIMP_CURVE_SMOOTH);

  return curve->curve_type;
}

#define MIN_DISTANCE 8

gint
gimp_curve_get_closest_point (GimpCurve *curve,
                              gint       x)
{
  gint closest_point = 0;
  gint distance      = G_MAXINT;
  gint i;

  g_return_val_if_fail (GIMP_IS_CURVE (curve), 0);

  for (i = 0; i < GIMP_CURVE_NUM_POINTS; i++)
    {
      if (curve->points[i][0] != -1)
        if (abs (x - curve->points[i][0]) < distance)
          {
            distance = abs (x - curve->points[i][0]);
            closest_point = i;
          }
    }

  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  return closest_point;
}

void
gimp_curve_set_point (GimpCurve *curve,
                      gint       point,
                      gint       x,
                      gint       y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->curve_type == GIMP_CURVE_FREE)
    return;

  curve->points[point][0] = x;
  curve->points[point][1] = y;

  gimp_curve_calculate (curve);
}

void
gimp_curve_move_point (GimpCurve *curve,
                       gint       point,
                       gint       y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->curve_type == GIMP_CURVE_FREE)
    return;

  curve->points[point][1] = y;

  gimp_curve_calculate (curve);
}

void
gimp_curve_get_uchar (GimpCurve *curve,
                      guchar    *dest_array)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (dest_array != NULL);

  memcpy (dest_array, curve->curve, 256);
}


/*  private functions  */

void
gimp_curve_calculate (GimpCurve *curve)
{
  gint i;
  gint points[GIMP_CURVE_NUM_POINTS];
  gint num_pts;
  gint p1, p2, p3, p4;

  switch (curve->curve_type)
    {
    case GIMP_CURVE_SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < GIMP_CURVE_NUM_POINTS; i++)
        if (curve->points[i][0] != -1)
          points[num_pts++] = i;

      /*  Initialize boundary curve points */
      if (num_pts != 0)
        {
          for (i = 0; i < curve->points[points[0]][0]; i++)
            curve->curve[i] = curve->points[points[0]][1];

          for (i = curve->points[points[num_pts - 1]][0]; i < 256; i++)
            curve->curve[i] = curve->points[points[num_pts - 1]][1];
        }

      for (i = 0; i < num_pts - 1; i++)
        {
          p1 = points[MAX (i - 1, 0)];
          p2 = points[i];
          p3 = points[i + 1];
          p4 = points[MIN (i + 2, num_pts - 1)];

          gimp_curve_plot (curve, p1, p2, p3, p4);
        }

      /* ensure that the control points are used exactly */
      for (i = 0; i < num_pts; i++)
        {
          gint x = curve->points[points[i]][0];
          gint y = curve->points[points[i]][1];

          curve->curve[x] = y;
        }
      break;

    case GIMP_CURVE_FREE:
      break;
    }
}

/*
 * This function calculates the curve values between the control points
 * p2 and p3, taking the potentially existing neighbors p1 and p4 into
 * account.
 *
 * This function uses a cubic bezier curve for the individual segments and
 * calculates the necessary intermediate control points depending on the
 * neighbor curve control points.
 */
static void
gimp_curve_plot (GimpCurve *curve,
                 gint       p1,
                 gint       p2,
                 gint       p3,
                 gint       p4)
{
  gint    i;
  gdouble x0, x3;
  gdouble y0, y1, y2, y3;
  gdouble dx, dy;
  gdouble y, t;
  gdouble slope;

  /* the outer control points for the bezier curve. */
  x0 = curve->points[p2][0];
  y0 = curve->points[p2][1];
  x3 = curve->points[p3][0];
  y3 = curve->points[p3][1];

  /*
   * the x values of the inner control points are fixed at
   * x1 = 1/3*x0 + 2/3*x3   and  x2 = 2/3*x0 + 1/3*x3
   * this ensures that the x values increase linearily with the
   * parameter t and enables us to skip the calculation of the x
   * values altogehter - just calculate y(t) evenly spaced.
   */

  dx = x3 - x0;
  dy = y3 - y0;

  g_return_if_fail (dx > 0);

  if (p1 == p2 && p3 == p4)
    {
      /* No information about the neighbors,
       * calculate y1 and y2 to get a straight line
       */
      y1 = y0 + dy / 3.0;
      y2 = y0 + dy * 2.0 / 3.0;
    }
  else if (p1 == p2 && p3 != p4)
    {
      /* only the right neighbor is available. Make the tangent at the
       * right endpoint parallel to the line between the left endpoint
       * and the right neighbor. Then point the tangent at the left towards
       * the control handle of the right tangent, to ensure that the curve
       * does not have an inflection point.
       */
      slope = (curve->points[p4][1] - y0) /
              (curve->points[p4][0] - x0);

      y2 = y3 - slope * dx / 3.0;
      y1 = y0 + (y2 - y0) / 2.0;
    }
  else if (p1 != p2 && p3 == p4)
    {
      /* see previous case */
      slope = (y3 - curve->points[p1][1]) /
              (x3 - curve->points[p1][0]);

      y1 = y0 + slope * dx / 3.0;
      y2 = y3 + (y1 - y3) / 2.0;
    }
  else /* (p1 != p2 && p3 != p4) */
    {
      /* Both neighbors are available. Make the tangents at the endpoints
       * parallel to the line between the opposite endpoint and the adjacent
       * neighbor.
       */
      slope = (y3 - curve->points[p1][1]) /
              (x3 - curve->points[p1][0]);

      y1 = y0 + slope * dx / 3.0;

      slope = (curve->points[p4][1] - y0) /
              (curve->points[p4][0] - x0);

      y2 = y3 - slope * dx / 3.0;
    }

  /*
   * finally calculate the y(t) values for the given bezier values. We can
   * use homogenously distributed values for t, since x(t) increases linearily.
   */
  for (i = 0; i <= dx; i++)
    {
      t = i / dx;
      y =     y0 * (1-t) * (1-t) * (1-t) +
          3 * y1 * (1-t) * (1-t) * t     +
          3 * y2 * (1-t) * t     * t     +
              y3 * t     * t     * t;

      curve->curve[ROUND(x0) + i] = CLAMP0255 (ROUND (y));
    }
}
