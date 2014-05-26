/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdial.c
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

#include "gimpdial.h"


#define SEGMENT_FRACTION 0.3


enum
{
  PROP_0,
  PROP_DRAW_BETA,
  PROP_ALPHA,
  PROP_BETA,
  PROP_CLOCKWISE
};

typedef enum
{
  DIAL_TARGET_ALPHA,
  DIAL_TARGET_BETA,
  DIAL_TARGET_BOTH
} DialTarget;


struct _GimpDialPrivate
{
  gdouble     alpha;
  gdouble     beta;
  gboolean    clockwise;
  gboolean    draw_beta;

  DialTarget  target;
  gdouble     last_angle;
  gboolean    has_grab;
};


static void        gimp_dial_dispose              (GObject            *object);
static void        gimp_dial_set_property         (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void        gimp_dial_get_property         (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);

static void        gimp_dial_unmap                (GtkWidget          *widget);
static gboolean    gimp_dial_expose_event         (GtkWidget          *widget,
                                                   GdkEventExpose     *event);
static gboolean    gimp_dial_button_press_event   (GtkWidget          *widget,
                                                   GdkEventButton     *bevent);
static gboolean    gimp_dial_button_release_event (GtkWidget          *widget,
                                                   GdkEventButton     *bevent);
static gboolean    gimp_dial_motion_notify_event  (GtkWidget          *widget,
                                                   GdkEventMotion     *mevent);

static void        gimp_dial_draw_arrows          (cairo_t            *cr,
                                                   gint                size,
                                                   gdouble             alpha,
                                                   gdouble             beta,
                                                   gboolean            clockwise,
                                                   gboolean            draw_beta);


G_DEFINE_TYPE (GimpDial, gimp_dial, GIMP_TYPE_CIRCLE)

#define parent_class gimp_dial_parent_class


static void
gimp_dial_class_init (GimpDialClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = gimp_dial_dispose;
  object_class->get_property         = gimp_dial_get_property;
  object_class->set_property         = gimp_dial_set_property;

  widget_class->unmap                = gimp_dial_unmap;
  widget_class->expose_event         = gimp_dial_expose_event;
  widget_class->button_press_event   = gimp_dial_button_press_event;
  widget_class->button_release_event = gimp_dial_button_release_event;
  widget_class->motion_notify_event  = gimp_dial_motion_notify_event;

  g_object_class_install_property (object_class, PROP_ALPHA,
                                   g_param_spec_double ("alpha",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, 0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BETA,
                                   g_param_spec_double ("beta",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, G_PI,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CLOCKWISE,
                                   g_param_spec_boolean ("clockwise",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DRAW_BETA,
                                   g_param_spec_boolean ("draw-beta",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpDialPrivate));
}

static void
gimp_dial_init (GimpDial *dial)
{
  dial->priv = G_TYPE_INSTANCE_GET_PRIVATE (dial,
                                            GIMP_TYPE_DIAL,
                                            GimpDialPrivate);

  gtk_widget_add_events (GTK_WIDGET (dial),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK);
}

static void
gimp_dial_dispose (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dial_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpDial *dial = GIMP_DIAL (object);

  switch (property_id)
    {
    case PROP_ALPHA:
      dial->priv->alpha = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

    case PROP_BETA:
      dial->priv->beta = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

    case PROP_CLOCKWISE:
      dial->priv->clockwise = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

    case PROP_DRAW_BETA:
      dial->priv->draw_beta = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dial_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpDial *dial = GIMP_DIAL (object);

  switch (property_id)
    {
    case PROP_ALPHA:
      g_value_set_double (value, dial->priv->alpha);
      break;

    case PROP_BETA:
      g_value_set_double (value, dial->priv->beta);
      break;

    case PROP_CLOCKWISE:
      g_value_set_boolean (value, dial->priv->clockwise);
      break;

    case PROP_DRAW_BETA:
      g_value_set_boolean (value, dial->priv->draw_beta);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dial_unmap (GtkWidget *widget)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (dial->priv->has_grab)
    {
      gtk_grab_remove (widget);
      dial->priv->has_grab = FALSE;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean
gimp_dial_expose_event (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpDial *dial = GIMP_DIAL (widget);

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  if (gtk_widget_is_drawable (widget))
    {
      GtkAllocation  allocation;
      gint           size;
      cairo_t       *cr;

      g_object_get (widget,
                    "size", &size,
                    NULL);

      cr = gdk_cairo_create (event->window);
      gdk_cairo_region (cr, event->region);
      cairo_clip (cr);

      gtk_widget_get_allocation (widget, &allocation);

      cairo_translate (cr,
                       allocation.x + (allocation.width  - size) / 2,
                       allocation.y + (allocation.height - size) / 2);

      gimp_dial_draw_arrows (cr, size,
                             dial->priv->alpha, dial->priv->beta,
                             dial->priv->clockwise,
                             dial->priv->draw_beta);

      cairo_destroy (cr);
    }

  return FALSE;
}

static gdouble
normalize_angle (gdouble angle)
{
  if (angle < 0)
    return angle + 2 * G_PI;
  else if (angle > 2 * G_PI)
    return angle - 2 * G_PI;
  else
    return angle;
}

static gdouble
get_angle_distance (gdouble alpha,
                    gdouble beta)
{
  return ABS (MIN (normalize_angle (alpha - beta),
                   2 * G_PI - normalize_angle (alpha - beta)));
}

static gdouble
get_angle_and_distance (gdouble  center_x,
                        gdouble  center_y,
                        gdouble  radius,
                        gdouble  x,
                        gdouble  y,
                        gdouble *distance)
{
  gdouble angle = atan2 (center_y - y,
                         x - center_x);

  if (angle < 0)
    angle += 2 * G_PI;

  if (distance)
    *distance = sqrt ((SQR (x - center_x) +
                       SQR (y - center_y)) / SQR (radius));

  return angle;
}

static gboolean
gimp_dial_button_press_event (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (bevent->type == GDK_BUTTON_PRESS &&
      bevent->button == 1)
    {
      GtkAllocation allocation;
      gint          size;
      gdouble       center_x;
      gdouble       center_y;
      gdouble       angle;
      gdouble       distance;

      g_object_get (widget,
                    "size", &size,
                    NULL);

      gtk_grab_add (widget);
      dial->priv->has_grab = TRUE;

      gtk_widget_get_allocation (widget, &allocation);

      center_x = allocation.width  / 2.0;
      center_y = allocation.height / 2.0;

      angle = get_angle_and_distance (center_x, center_y, size / 2.0,
                                      bevent->x, bevent->y,
                                      &distance);
      dial->priv->last_angle = angle;

      if (dial->priv->draw_beta       &&
          distance > SEGMENT_FRACTION &&
          MIN (get_angle_distance (dial->priv->alpha, angle),
               get_angle_distance (dial->priv->beta,  angle)) < G_PI / 12)
        {
          if (get_angle_distance (dial->priv->alpha, angle) <
              get_angle_distance (dial->priv->beta,  angle))
            {
              dial->priv->target = DIAL_TARGET_ALPHA;
              g_object_set (dial, "alpha", angle, NULL);
            }
          else
            {
              dial->priv->target = DIAL_TARGET_BETA;
              g_object_set (dial, "beta", angle, NULL);
            }
        }
      else
        {
          dial->priv->target = DIAL_TARGET_BOTH;
        }
    }

  return FALSE;
}

static gboolean
gimp_dial_button_release_event (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);
      dial->priv->has_grab = FALSE;
    }

  return FALSE;
}

static gboolean
gimp_dial_motion_notify_event (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (dial->priv->has_grab)
    {
      GtkAllocation  allocation;
      gdouble        center_x;
      gdouble        center_y;
      gdouble        angle;
      gdouble        delta;

      gtk_widget_get_allocation (widget, &allocation);

      center_x = allocation.width  / 2.0;
      center_y = allocation.height / 2.0;

      angle = get_angle_and_distance (center_x, center_y, 1.0,
                                      mevent->x, mevent->y,
                                      NULL);

      delta = angle - dial->priv->last_angle;
      dial->priv->last_angle = angle;

      if (delta != 0.0)
        {
          switch (dial->priv->target)
            {
            case DIAL_TARGET_ALPHA:
              g_object_set (dial, "alpha", angle, NULL);
              break;

            case DIAL_TARGET_BETA:
              g_object_set (dial, "beta", angle, NULL);
              break;

            case DIAL_TARGET_BOTH:
              g_object_set (dial,
                            "alpha", normalize_angle (dial->priv->alpha + delta),
                            "beta",  normalize_angle (dial->priv->beta  + delta),
                            NULL);
              break;

            default:
              break;
            }
        }
    }

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_dial_new (void)
{
  return g_object_new (GIMP_TYPE_DIAL, NULL);
}


/*  private functions  */

static void
gimp_dial_draw_arrows (cairo_t  *cr,
                       gint      size,
                       gdouble   alpha,
                       gdouble   beta,
                       gboolean  clockwise,
                       gboolean  draw_beta)
{
  gint radius    = size / 2.0 - 1.5;
  gint direction = clockwise ? -1 : 1;

#define REL 0.8
#define DEL 0.1

  cairo_save (cr);

  cairo_translate (cr, 1.5, 1.5); /* half the broad line width */

  cairo_move_to (cr, radius, radius);
  cairo_line_to (cr,
                 ROUND (radius + radius * cos (alpha)),
                 ROUND (radius - radius * sin (alpha)));

  cairo_move_to (cr,
                 radius + radius * cos (alpha),
                 radius - radius * sin (alpha));
  cairo_line_to (cr,
                 ROUND (radius + radius * REL * cos (alpha - DEL)),
                 ROUND (radius - radius * REL * sin (alpha - DEL)));

  cairo_move_to (cr,
                 radius + radius * cos (alpha),
                 radius - radius * sin (alpha));
  cairo_line_to (cr,
                 ROUND (radius + radius * REL * cos (alpha + DEL)),
                 ROUND (radius - radius * REL * sin (alpha + DEL)));

  if (draw_beta)
    {
      gint    segment_dist;
      gint    tick;
      gdouble slice;

      cairo_move_to (cr, radius, radius);
      cairo_line_to (cr,
                     ROUND (radius + radius * cos (beta)),
                     ROUND (radius - radius * sin (beta)));

      cairo_move_to (cr,
                     radius + radius * cos (beta),
                     radius - radius * sin (beta));
      cairo_line_to (cr,
                     ROUND (radius + radius * REL * cos (beta - DEL)),
                     ROUND (radius - radius * REL * sin (beta - DEL)));

      cairo_move_to (cr,
                     radius + radius * cos (beta),
                     radius - radius * sin (beta));
      cairo_line_to (cr,
                     ROUND (radius + radius * REL * cos (beta + DEL)),
                     ROUND (radius - radius * REL * sin (beta + DEL)));

      segment_dist = radius * SEGMENT_FRACTION;
      tick         = MIN (10, segment_dist);

      cairo_move_to (cr,
                     radius + segment_dist * cos (beta),
                     radius - segment_dist * sin (beta));
      cairo_line_to (cr,
                     ROUND (radius + segment_dist * cos (beta) +
                            direction * tick * sin (beta)),
                     ROUND (radius - segment_dist * sin(beta) +
                            direction * tick * cos (beta)));

      cairo_new_sub_path (cr);

      if (clockwise)
        slice = -normalize_angle (alpha - beta);
      else
        slice = normalize_angle (beta - alpha);

      gimp_cairo_add_arc (cr, radius, radius, segment_dist,
                          alpha, slice);
    }

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  cairo_restore (cr);
}
