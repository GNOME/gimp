/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppolar.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp-cairo.h"

#include "gimppolar.h"


enum
{
  PROP_0,
  PROP_ANGLE,
  PROP_RADIUS
};

typedef enum
{
  POLAR_TARGET_NONE   = 0,
  POLAR_TARGET_CIRCLE = 1 << 0
} PolarTarget;


struct _GimpPolarPrivate
{
  gdouble      angle;
  gdouble      radius;

  PolarTarget  target;
};


static void        gimp_polar_set_property         (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void        gimp_polar_get_property         (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static gboolean    gimp_polar_draw                 (GtkWidget          *widget,
                                                    cairo_t            *cr);
static gboolean    gimp_polar_button_press_event   (GtkWidget          *widget,
                                                    GdkEventButton     *bevent);
static gboolean    gimp_polar_motion_notify_event  (GtkWidget          *widget,
                                                    GdkEventMotion     *mevent);

static void        gimp_polar_reset_target         (GimpCircle         *circle);

static void        gimp_polar_set_target           (GimpPolar           *polar,
                                                    PolarTarget          target);

static void        gimp_polar_draw_circle          (cairo_t            *cr,
                                                    gint                size,
                                                    gdouble             angle,
                                                    gdouble             radius,
                                                    PolarTarget         highlight);

static gdouble     gimp_polar_normalize_angle      (gdouble             angle);
static gdouble     gimp_polar_get_angle_distance   (gdouble             alpha,
                                                    gdouble             beta);


G_DEFINE_TYPE (GimpPolar, gimp_polar, GIMP_TYPE_CIRCLE)

#define parent_class gimp_polar_parent_class


static void
gimp_polar_class_init (GimpPolarClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS (klass);
  GimpCircleClass *circle_class = GIMP_CIRCLE_CLASS (klass);

  object_class->get_property         = gimp_polar_get_property;
  object_class->set_property         = gimp_polar_set_property;

  widget_class->draw                 = gimp_polar_draw;
  widget_class->button_press_event   = gimp_polar_button_press_event;
  widget_class->motion_notify_event  = gimp_polar_motion_notify_event;

  circle_class->reset_target         = gimp_polar_reset_target;

  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("angle",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, 0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RADIUS,
                                   g_param_spec_double ("radius",
                                                        NULL, NULL,
                                                        0.0, 1.0, 0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpPolarPrivate));
}

static void
gimp_polar_init (GimpPolar *polar)
{
  polar->priv = G_TYPE_INSTANCE_GET_PRIVATE (polar,
                                             GIMP_TYPE_POLAR,
                                             GimpPolarPrivate);
}

static void
gimp_polar_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpPolar *polar = GIMP_POLAR (object);

  switch (property_id)
    {
    case PROP_ANGLE:
      polar->priv->angle = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (polar));
      break;

    case PROP_RADIUS:
      polar->priv->radius = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (polar));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_polar_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpPolar *polar = GIMP_POLAR (object);

  switch (property_id)
    {
    case PROP_ANGLE:
      g_value_set_double (value, polar->priv->angle);
      break;

    case PROP_RADIUS:
      g_value_set_double (value, polar->priv->radius);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_polar_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  GimpPolar     *polar = GIMP_POLAR (widget);
  GtkAllocation  allocation;
  gint           size;

  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  g_object_get (widget,
                "size", &size,
                NULL);

  gtk_widget_get_allocation (widget, &allocation);

  cairo_save (cr);

  cairo_translate (cr,
                   (allocation.width  - size) / 2.0,
                   (allocation.height - size) / 2.0);

  gimp_polar_draw_circle (cr, size,
                          polar->priv->angle, polar->priv->radius,
                          polar->priv->target);

  cairo_restore (cr);

  return FALSE;
}

static gboolean
gimp_polar_button_press_event (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  GimpPolar *polar = GIMP_POLAR (widget);

  if (bevent->type == GDK_BUTTON_PRESS &&
      bevent->button == 1              &&
      polar->priv->target != POLAR_TARGET_NONE)
    {
      gdouble angle;
      gdouble radius;

      GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

      angle = _gimp_circle_get_angle_and_distance (GIMP_CIRCLE (polar),
                                                   bevent->x, bevent->y,
                                                   &radius);
      radius = MIN (radius, 1.0);

      g_object_set (polar,
                    "angle",  angle,
                    "radius", radius,
                    NULL);
    }

  return FALSE;
}

static gboolean
gimp_polar_motion_notify_event (GtkWidget      *widget,
                                GdkEventMotion *mevent)
{
  GimpPolar *polar = GIMP_POLAR (widget);
  gdouble    angle;
  gdouble    radius;

  angle = _gimp_circle_get_angle_and_distance (GIMP_CIRCLE (polar),
                                               mevent->x, mevent->y,
                                               &radius);

  if (_gimp_circle_has_grab (GIMP_CIRCLE (polar)))
    {
      radius = MIN (radius, 1.0);

      g_object_set (polar,
                    "angle",  angle,
                    "radius", radius,
                    NULL);
    }
  else
    {
      PolarTarget target;
      gdouble     dist_angle;
      gdouble     dist_radius;

      dist_angle  = gimp_polar_get_angle_distance (polar->priv->angle, angle);
      dist_radius = ABS (polar->priv->radius - radius);

      if ((radius < 0.2 && polar->priv->radius < 0.2) ||
          (dist_angle  < (G_PI / 12) && dist_radius < 0.2))
        {
          target = POLAR_TARGET_CIRCLE;
        }
      else
        {
          target = POLAR_TARGET_NONE;
        }

      gimp_polar_set_target (polar, target);
    }

  gdk_event_request_motions (mevent);

  return FALSE;
}

static void
gimp_polar_reset_target (GimpCircle *circle)
{
  gimp_polar_set_target (GIMP_POLAR (circle), POLAR_TARGET_NONE);
}


/*  public functions  */

GtkWidget *
gimp_polar_new (void)
{
  return g_object_new (GIMP_TYPE_POLAR, NULL);
}


/*  private functions  */

static void
gimp_polar_set_target (GimpPolar   *polar,
                       PolarTarget  target)
{
  if (target != polar->priv->target)
    {
      polar->priv->target = target;
      gtk_widget_queue_draw (GTK_WIDGET (polar));
    }
}

static void
gimp_polar_draw_circle (cairo_t     *cr,
                        gint         size,
                        gdouble      angle,
                        gdouble      radius,
                        PolarTarget  highlight)
{
  gdouble r = size / 2.0 - 2.0; /* half the broad line with and half a px */
  gdouble x = r + r * radius * cos (angle);
  gdouble y = r - r * radius * sin (angle);

  cairo_save (cr);

  cairo_translate (cr, 2.0, 2.0); /* half the broad line width and half a px*/

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  cairo_arc (cr, x, y, 3, 0, 2 * G_PI);

  if (highlight == POLAR_TARGET_NONE)
    {
      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke (cr);
    }
  else
    {
      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.8);
      cairo_stroke (cr);
    }

  cairo_restore (cr);
}

static gdouble
gimp_polar_normalize_angle (gdouble angle)
{
  if (angle < 0)
    return angle + 2 * G_PI;
  else if (angle > 2 * G_PI)
    return angle - 2 * G_PI;
  else
    return angle;
}

static gdouble
gimp_polar_get_angle_distance (gdouble alpha,
                               gdouble beta)
{
  return ABS (MIN (gimp_polar_normalize_angle (alpha - beta),
                   2 * G_PI - gimp_polar_normalize_angle (alpha - beta)));
}
