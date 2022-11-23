/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadial.c
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma-cairo.h"

#include "ligmadial.h"


#define SEGMENT_FRACTION 0.3

/* round n to the nearest multiple of m */
#define SNAP(n, m) (RINT((n) / (m)) * (m))

enum
{
  PROP_0,
  PROP_DRAW_BETA,
  PROP_ALPHA,
  PROP_BETA,
  PROP_CLOCKWISE_ANGLES,
  PROP_CLOCKWISE_DELTA
};

typedef enum
{
  DIAL_TARGET_NONE  = 0,
  DIAL_TARGET_ALPHA = 1 << 0,
  DIAL_TARGET_BETA  = 1 << 1,
  DIAL_TARGET_BOTH  = DIAL_TARGET_ALPHA | DIAL_TARGET_BETA
} DialTarget;


struct _LigmaDialPrivate
{
  gdouble     alpha;
  gdouble     beta;
  gboolean    clockwise_angles;
  gboolean    clockwise_delta;
  gboolean    draw_beta;

  DialTarget  target;
  gdouble     last_angle;
};


static void        ligma_dial_set_property         (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void        ligma_dial_get_property         (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);

static gboolean    ligma_dial_draw                 (GtkWidget          *widget,
                                                   cairo_t            *cr);
static gboolean    ligma_dial_button_press_event   (GtkWidget          *widget,
                                                   GdkEventButton     *bevent);
static gboolean    ligma_dial_motion_notify_event  (GtkWidget          *widget,
                                                   GdkEventMotion     *mevent);

static void        ligma_dial_reset_target         (LigmaCircle         *circle);

static void        ligma_dial_set_target           (LigmaDial           *dial,
                                                   DialTarget          target);

static void        ligma_dial_draw_arrows          (cairo_t            *cr,
                                                   gint                size,
                                                   gdouble             alpha,
                                                   gdouble             beta,
                                                   gboolean            clockwise_delta,
                                                   DialTarget          highlight,
                                                   gboolean            draw_beta);

static gdouble     ligma_dial_normalize_angle      (gdouble             angle);
static gdouble     ligma_dial_get_angle_distance   (gdouble             alpha,
                                                   gdouble             beta);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDial, ligma_dial, LIGMA_TYPE_CIRCLE)

#define parent_class ligma_dial_parent_class


static void
ligma_dial_class_init (LigmaDialClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaCircleClass *circle_class = LIGMA_CIRCLE_CLASS (klass);

  object_class->get_property         = ligma_dial_get_property;
  object_class->set_property         = ligma_dial_set_property;

  widget_class->draw                 = ligma_dial_draw;
  widget_class->button_press_event   = ligma_dial_button_press_event;
  widget_class->motion_notify_event  = ligma_dial_motion_notify_event;

  circle_class->reset_target         = ligma_dial_reset_target;

  g_object_class_install_property (object_class, PROP_ALPHA,
                                   g_param_spec_double ("alpha",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, 0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BETA,
                                   g_param_spec_double ("beta",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, G_PI,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CLOCKWISE_ANGLES,
                                   g_param_spec_boolean ("clockwise-angles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CLOCKWISE_DELTA,
                                   g_param_spec_boolean ("clockwise-delta",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DRAW_BETA,
                                   g_param_spec_boolean ("draw-beta",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_dial_init (LigmaDial *dial)
{
  dial->priv = ligma_dial_get_instance_private (dial);
}

static void
ligma_dial_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaDial *dial = LIGMA_DIAL (object);

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

    case PROP_CLOCKWISE_ANGLES:
      dial->priv->clockwise_angles = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

    case PROP_CLOCKWISE_DELTA:
      dial->priv->clockwise_delta = g_value_get_boolean (value);
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
ligma_dial_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaDial *dial = LIGMA_DIAL (object);

  switch (property_id)
    {
    case PROP_ALPHA:
      g_value_set_double (value, dial->priv->alpha);
      break;

    case PROP_BETA:
      g_value_set_double (value, dial->priv->beta);
      break;

    case PROP_CLOCKWISE_ANGLES:
      g_value_set_boolean (value, dial->priv->clockwise_angles);
      break;

    case PROP_CLOCKWISE_DELTA:
      g_value_set_boolean (value, dial->priv->clockwise_delta);
      break;

    case PROP_DRAW_BETA:
      g_value_set_boolean (value, dial->priv->draw_beta);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_dial_draw (GtkWidget *widget,
                cairo_t   *cr)
{
  LigmaDial      *dial  = LIGMA_DIAL (widget);
  GtkAllocation  allocation;
  gint           size;
  gdouble        alpha = dial->priv->alpha;
  gdouble        beta  = dial->priv->beta;

  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  g_object_get (widget,
                "size", &size,
                NULL);

  gtk_widget_get_allocation (widget, &allocation);

  if (dial->priv->clockwise_angles)
    {
      alpha = -alpha;
      beta  = -beta;
    }

  cairo_save (cr);

  cairo_translate (cr,
                   (allocation.width  - size) / 2.0,
                   (allocation.height - size) / 2.0);

  ligma_dial_draw_arrows (cr, size,
                         alpha, beta,
                         dial->priv->clockwise_delta,
                         dial->priv->target,
                         dial->priv->draw_beta);

  cairo_restore (cr);

  return FALSE;
}

static gboolean
ligma_dial_button_press_event (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  LigmaDial *dial = LIGMA_DIAL (widget);

  if (bevent->type == GDK_BUTTON_PRESS &&
      bevent->button == 1              &&
      dial->priv->target != DIAL_TARGET_NONE)
    {
      gdouble angle;

      GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

      angle = _ligma_circle_get_angle_and_distance (LIGMA_CIRCLE (dial),
                                                   bevent->x, bevent->y,
                                                   NULL);

      if (dial->priv->clockwise_angles && angle)
        angle = 2.0 * G_PI - angle;

      if (bevent->state & GDK_SHIFT_MASK)
        angle = SNAP (angle, G_PI / 12.0);

      dial->priv->last_angle = angle;

      switch (dial->priv->target)
        {
        case DIAL_TARGET_ALPHA:
          g_object_set (dial, "alpha", angle, NULL);
          break;

        case DIAL_TARGET_BETA:
          g_object_set (dial, "beta", angle, NULL);
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static gboolean
ligma_dial_motion_notify_event (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  LigmaDial *dial = LIGMA_DIAL (widget);
  gdouble   angle;
  gdouble   distance;

  angle = _ligma_circle_get_angle_and_distance (LIGMA_CIRCLE (dial),
                                               mevent->x, mevent->y,
                                               &distance);

  if (dial->priv->clockwise_angles && angle)
    angle = 2.0 * G_PI - angle;

  if (_ligma_circle_has_grab (LIGMA_CIRCLE (dial)))
    {
      gdouble delta;

      if (mevent->state & GDK_SHIFT_MASK)
        angle = SNAP (angle, G_PI / 12.0);

      delta = angle - dial->priv->last_angle;
      dial->priv->last_angle = angle;

      if (delta != 0.0)
        {
          gdouble alpha = dial->priv->alpha;

          switch (dial->priv->target)
            {
            case DIAL_TARGET_ALPHA:
              g_object_set (dial, "alpha", angle, NULL);
              break;

            case DIAL_TARGET_BETA:
              g_object_set (dial, "beta", angle, NULL);
              break;

            case DIAL_TARGET_BOTH:
              /* snap both by the alpha value */
              if (mevent->state & GDK_SHIFT_MASK)
                delta = SNAP (alpha + delta, G_PI / 12.0) - alpha;

              g_object_set (dial,
                            "alpha", ligma_dial_normalize_angle (dial->priv->alpha + delta),
                            "beta",  ligma_dial_normalize_angle (dial->priv->beta  + delta),
                            NULL);
              break;

            default:
              break;
            }
        }
    }
  else
    {
      DialTarget target;
      gdouble    dist_alpha;
      gdouble    dist_beta;

      dist_alpha = ligma_dial_get_angle_distance (dial->priv->alpha, angle);
      dist_beta  = ligma_dial_get_angle_distance (dial->priv->beta, angle);

      if (dial->priv->draw_beta       &&
          distance > SEGMENT_FRACTION &&
          MIN (dist_alpha, dist_beta) < G_PI / 12)
        {
          if (dist_alpha < dist_beta)
            {
              target = DIAL_TARGET_ALPHA;
            }
          else
            {
              target = DIAL_TARGET_BETA;
            }
        }
      else
        {
          target = DIAL_TARGET_BOTH;
        }

      ligma_dial_set_target (dial, target);
    }

  gdk_event_request_motions (mevent);

  return FALSE;
}

static void
ligma_dial_reset_target (LigmaCircle *circle)
{
  ligma_dial_set_target (LIGMA_DIAL (circle), DIAL_TARGET_NONE);
}


/*  public functions  */

GtkWidget *
ligma_dial_new (void)
{
  return g_object_new (LIGMA_TYPE_DIAL, NULL);
}


/*  private functions  */

static void
ligma_dial_set_target (LigmaDial   *dial,
                      DialTarget  target)
{
  if (target != dial->priv->target)
    {
      dial->priv->target = target;
      gtk_widget_queue_draw (GTK_WIDGET (dial));
    }
}

static void
ligma_dial_draw_arrow (cairo_t *cr,
                      gdouble  radius,
                      gdouble  angle)
{
#define REL 0.8
#define DEL 0.1

  cairo_move_to (cr, radius, radius);
  cairo_line_to (cr,
                 radius + radius * cos (angle),
                 radius - radius * sin (angle));

  cairo_move_to (cr,
                 radius + radius * cos (angle),
                 radius - radius * sin (angle));
  cairo_line_to (cr,
                 radius + radius * REL * cos (angle - DEL),
                 radius - radius * REL * sin (angle - DEL));

  cairo_move_to (cr,
                 radius + radius * cos (angle),
                 radius - radius * sin (angle));
  cairo_line_to (cr,
                 radius + radius * REL * cos (angle + DEL),
                 radius - radius * REL * sin (angle + DEL));
}

static void
ligma_dial_draw_segment (cairo_t  *cr,
                        gdouble   radius,
                        gdouble   alpha,
                        gdouble   beta,
                        gboolean  clockwise_delta)
{
  gint    direction = clockwise_delta ? -1 : 1;
  gint    segment_dist;
  gint    tick;
  gdouble slice;

  segment_dist = radius * SEGMENT_FRACTION;
  tick         = MIN (10, segment_dist);

  cairo_move_to (cr,
                 radius + segment_dist * cos (beta),
                 radius - segment_dist * sin (beta));
  cairo_line_to (cr,
                 radius + segment_dist * cos (beta) +
                 direction * tick * sin (beta),
                 radius - segment_dist * sin (beta) +
                 direction * tick * cos (beta));

  cairo_new_sub_path (cr);

  if (clockwise_delta)
    slice = -ligma_dial_normalize_angle (alpha - beta);
  else
    slice = ligma_dial_normalize_angle (beta - alpha);

  ligma_cairo_arc (cr, radius, radius, segment_dist,
                  alpha, slice);
}

static void
ligma_dial_draw_arrows (cairo_t    *cr,
                       gint        size,
                       gdouble     alpha,
                       gdouble     beta,
                       gboolean    clockwise_delta,
                       DialTarget  highlight,
                       gboolean    draw_beta)
{
  gdouble radius = size / 2.0 - 2.0; /* half the broad line with and half a px */

  cairo_save (cr);

  cairo_translate (cr, 2.0, 2.0); /* half the broad line width and half a px*/

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  if (highlight != DIAL_TARGET_BOTH)
    {
      if (! (highlight & DIAL_TARGET_ALPHA))
        ligma_dial_draw_arrow (cr, radius, alpha);

      if (draw_beta)
        {
          if (! (highlight & DIAL_TARGET_BETA))
            ligma_dial_draw_arrow (cr, radius, beta);

          if ((highlight & DIAL_TARGET_BOTH) != DIAL_TARGET_BOTH)
            ligma_dial_draw_segment (cr, radius, alpha, beta, clockwise_delta);
        }

      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke (cr);
    }

  if (highlight != DIAL_TARGET_NONE)
    {
      if (highlight & DIAL_TARGET_ALPHA)
        ligma_dial_draw_arrow (cr, radius, alpha);

      if (draw_beta)
        {
          if (highlight & DIAL_TARGET_BETA)
            ligma_dial_draw_arrow (cr, radius, beta);

          if ((highlight & DIAL_TARGET_BOTH) == DIAL_TARGET_BOTH)
            ligma_dial_draw_segment (cr, radius, alpha, beta, clockwise_delta);
        }

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
ligma_dial_normalize_angle (gdouble angle)
{
  if (angle < 0)
    return angle + 2 * G_PI;
  else if (angle > 2 * G_PI)
    return angle - 2 * G_PI;
  else
    return angle;
}

static gdouble
ligma_dial_get_angle_distance (gdouble alpha,
                              gdouble beta)
{
  return ABS (MIN (ligma_dial_normalize_angle (alpha - beta),
                   2 * G_PI - ligma_dial_normalize_angle (alpha - beta)));
}
