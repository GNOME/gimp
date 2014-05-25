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
  PROP_BORDER_WIDTH,
  PROP_BACKGROUND,
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
  gint                border_width;
  GimpDialBackground  background;

  gdouble             alpha;
  gdouble             beta;
  gboolean            clockwise;

  GdkWindow        *event_window;
  DialTarget        target;
  gdouble           last_angle;
  guint             has_grab : 1;
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

static void        gimp_dial_realize              (GtkWidget          *widget);
static void        gimp_dial_unrealize            (GtkWidget          *widget);
static void        gimp_dial_map                  (GtkWidget          *widget);
static void        gimp_dial_unmap                (GtkWidget          *widget);
static void        gimp_dial_size_request         (GtkWidget          *widget,
                                                   GtkRequisition     *requisition);
static void        gimp_dial_size_allocate        (GtkWidget          *widget,
                                                   GtkAllocation      *allocation);
static gboolean    gimp_dial_expose_event         (GtkWidget          *widget,
                                                   GdkEventExpose     *event);
static gboolean    gimp_dial_button_press_event   (GtkWidget          *widget,
                                                   GdkEventButton     *bevent);
static gboolean    gimp_dial_button_release_event (GtkWidget          *widget,
                                                   GdkEventButton     *bevent);
static gboolean    gimp_dial_motion_notify_event  (GtkWidget          *widget,
                                                   GdkEventMotion     *mevent);

static void        gimp_dial_background_hsv       (gdouble             angle,
                                                   gdouble             distance,
                                                   guchar             *rgb);

static void        gimp_dial_draw_background      (cairo_t            *cr,
                                                   gint                size,
                                                   GimpDialBackground  background);
static void        gimp_dial_draw_arrows          (cairo_t            *cr,
                                                   gint                size,
                                                   gdouble             alpha,
                                                   gdouble             beta,
                                                   gboolean            clockwise);


G_DEFINE_TYPE (GimpDial, gimp_dial, GTK_TYPE_WIDGET)

#define parent_class gimp_dial_parent_class


static void
gimp_dial_class_init (GimpDialClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = gimp_dial_dispose;
  object_class->get_property         = gimp_dial_get_property;
  object_class->set_property         = gimp_dial_set_property;

  widget_class->realize              = gimp_dial_realize;
  widget_class->unrealize            = gimp_dial_unrealize;
  widget_class->map                  = gimp_dial_map;
  widget_class->unmap                = gimp_dial_unmap;
  widget_class->size_request         = gimp_dial_size_request;
  widget_class->size_allocate        = gimp_dial_size_allocate;
  widget_class->expose_event         = gimp_dial_expose_event;
  widget_class->button_press_event   = gimp_dial_button_press_event;
  widget_class->button_release_event = gimp_dial_button_release_event;
  widget_class->motion_notify_event  = gimp_dial_motion_notify_event;

  g_object_class_install_property (object_class, PROP_BORDER_WIDTH,
                                   g_param_spec_int ("border-width",
                                                     NULL, NULL,
                                                     0, 64, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BACKGROUND,
                                   g_param_spec_enum ("background",
                                                      NULL, NULL,
                                                      GIMP_TYPE_DIAL_BACKGROUND,
                                                      GIMP_DIAL_BACKGROUND_HSV,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

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

  g_type_class_add_private (klass, sizeof (GimpDialPrivate));
}

static void
gimp_dial_init (GimpDial *dial)
{
  dial->priv = G_TYPE_INSTANCE_GET_PRIVATE (dial,
                                            GIMP_TYPE_DIAL,
                                            GimpDialPrivate);

  gtk_widget_set_has_window (GTK_WIDGET (dial), FALSE);
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
    case PROP_BORDER_WIDTH:
      dial->priv->border_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (dial));
      break;

    case PROP_BACKGROUND:
      dial->priv->background = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (dial));
      break;

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
    case PROP_BORDER_WIDTH:
      g_value_set_int (value, dial->priv->border_width);
      break;

    case PROP_BACKGROUND:
      g_value_set_enum (value, dial->priv->background);
      break;

    case PROP_ALPHA:
      g_value_set_double (value, dial->priv->alpha);
      break;

    case PROP_BETA:
      g_value_set_double (value, dial->priv->beta);
      break;

    case PROP_CLOCKWISE:
      g_value_set_boolean (value, dial->priv->clockwise);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dial_realize (GtkWidget *widget)
{
  GimpDial      *dial = GIMP_DIAL (widget);
  GtkAllocation  allocation;
  GdkWindowAttr  attributes;
  gint           attributes_mask;

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.wclass      = GDK_INPUT_ONLY;
  attributes.event_mask  = gtk_widget_get_events (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  dial->priv->event_window = gdk_window_new (gtk_widget_get_window (widget),
                                             &attributes, attributes_mask);
  gdk_window_set_user_data (dial->priv->event_window, dial);
}

static void
gimp_dial_unrealize (GtkWidget *widget)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (dial->priv->event_window)
    {
      gdk_window_set_user_data (dial->priv->event_window, NULL);
      gdk_window_destroy (dial->priv->event_window);
      dial->priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_dial_map (GtkWidget *widget)
{
  GimpDial *dial = GIMP_DIAL (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (dial->priv->event_window)
    gdk_window_show (dial->priv->event_window);
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

  if (dial->priv->event_window)
    gdk_window_hide (dial->priv->event_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_dial_size_request (GtkWidget      *widget,
                        GtkRequisition *requisition)
{
  GimpDial *dial = GIMP_DIAL (widget);

  requisition->width  = 2 * dial->priv->border_width + 96;
  requisition->height = 2 * dial->priv->border_width + 96;
}

static void
gimp_dial_size_allocate (GtkWidget     *widget,
                         GtkAllocation *allocation)
{
  GimpDial *dial = GIMP_DIAL (widget);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (dial->priv->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);
}

static gboolean
gimp_dial_expose_event (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpDial *dial = GIMP_DIAL (widget);

  if (gtk_widget_is_drawable (widget))
    {
      GtkAllocation  allocation;
      gint           border_width = dial->priv->border_width;
      cairo_t       *cr;
      gint           size;
      gint           x, y;

      gtk_widget_get_allocation (widget, &allocation);

      cr = gdk_cairo_create (event->window);
      gdk_cairo_region (cr, event->region);
      cairo_clip (cr);

      size = MIN (allocation.width, allocation.height) - 2 * border_width;

      x = (allocation.width  - 2 * border_width - size) / 2;
      y = (allocation.height - 2 * border_width - size) / 2;

      cairo_translate (cr,
                       allocation.x + border_width + x,
                       allocation.y + border_width + y);

      gimp_dial_draw_background (cr, size, dial->priv->background);
      gimp_dial_draw_arrows (cr, size,
                             dial->priv->alpha, dial->priv->beta,
                             dial->priv->clockwise);

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
      gdouble       center_x;
      gdouble       center_y;
      gint          size;
      gdouble       angle;
      gdouble       distance;

      gtk_grab_add (widget);
      dial->priv->has_grab = TRUE;

      gtk_widget_get_allocation (widget, &allocation);

      center_x = allocation.width  / 2.0;
      center_y = allocation.height / 2.0;

      size = MIN (allocation.width, allocation.height) - 2 * dial->priv->border_width;

      angle = get_angle_and_distance (center_x, center_y, size / 2.0,
                                      bevent->x, bevent->y,
                                      &distance);
      dial->priv->last_angle = angle;

      if (distance > SEGMENT_FRACTION &&
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
gimp_dial_background_hsv (gdouble  angle,
                          gdouble  distance,
                          guchar  *rgb)
{
  gdouble v = 1 - sqrt (distance) / 4; /* it just looks nicer this way */

  gimp_hsv_to_rgb4 (rgb, angle / (2.0 * G_PI), distance, v);
}

static void
gimp_dial_draw_background (cairo_t            *cr,
                           gint                size,
                           GimpDialBackground  background)
{
  cairo_save (cr);

  if (background == GIMP_DIAL_BACKGROUND_PLAIN)
    {
      cairo_arc (cr, size / 2.0, size / 2.0, size / 2.0 - 1.5, 0.0, 2 * G_PI);

      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke (cr);
    }
  else
    {
      cairo_surface_t *surface;
      guchar          *data;
      gint             stride;
      gint             x, y;

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, size, size);

      data   = cairo_image_surface_get_data (surface);
      stride = cairo_image_surface_get_stride (surface);

      for (y = 0; y < size; y++)
        {
          for (x = 0; x < size; x++)
            {
              gdouble angle;
              gdouble distance;
              guchar  rgb[3] = { 0, };

              angle = get_angle_and_distance (size / 2.0, size / 2.0, size / 2.0,
                                              x, y,
                                              &distance);

              switch (background)
                {
                case GIMP_DIAL_BACKGROUND_HSV:
                  gimp_dial_background_hsv (angle, distance, rgb);
                  break;

                default:
                  break;
                }

              GIMP_CAIRO_ARGB32_SET_PIXEL (data + y * stride + x * 4,
                                           rgb[0], rgb[1], rgb[2], 255);
            }
        }

      cairo_surface_mark_dirty (surface);
      cairo_set_source_surface (cr, surface, 0.0, 0.0);
      cairo_surface_destroy (surface);

      cairo_arc (cr, size / 2.0, size / 2.0, size / 2.0, 0.0, 2 * G_PI);
      cairo_clip (cr);

      cairo_paint (cr);
    }

  cairo_restore (cr);
}

static void
gimp_dial_draw_arrows (cairo_t  *cr,
                       gint      size,
                       gdouble   alpha,
                       gdouble   beta,
                       gboolean  clockwise)
{
  gint    radius    = size / 2.0 - 1.5;
  gint    direction = clockwise ? -1 : 1;
  gint    slice_dist;
  gdouble slice;

#define REL  0.8
#define DEL  0.1
#define TICK 10

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

  slice_dist = radius * SEGMENT_FRACTION;

  cairo_move_to (cr,
                 radius + slice_dist * cos (beta),
                 radius - slice_dist * sin (beta));
  cairo_line_to (cr,
                 ROUND (radius + slice_dist * cos (beta) +
                        direction * TICK * sin (beta)),
                 ROUND (radius - slice_dist * sin(beta) +
                        direction * TICK * cos (beta)));

  cairo_new_sub_path (cr);

  if (clockwise)
    slice = -normalize_angle (alpha - beta);
  else
    slice = normalize_angle (beta - alpha);

  gimp_cairo_add_arc (cr, radius, radius, slice_dist,
                      alpha, slice);

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  cairo_restore (cr);
}
