/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacircle.c
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

#include "ligmacircle.h"


enum
{
  PROP_0,
  PROP_SIZE,
  PROP_BORDER_WIDTH,
  PROP_BACKGROUND
};


struct _LigmaCirclePrivate
{
  gint                  size;
  gint                  border_width;
  LigmaCircleBackground  background;

  GdkWindow            *event_window;
  cairo_surface_t      *surface;
  gboolean              has_grab;
  gboolean              in_widget;
};


static void        ligma_circle_dispose              (GObject              *object);
static void        ligma_circle_set_property         (GObject              *object,
                                                     guint                 property_id,
                                                     const GValue         *value,
                                                     GParamSpec           *pspec);
static void        ligma_circle_get_property         (GObject              *object,
                                                     guint                 property_id,
                                                     GValue               *value,
                                                     GParamSpec           *pspec);

static void        ligma_circle_realize              (GtkWidget            *widget);
static void        ligma_circle_unrealize            (GtkWidget            *widget);
static void        ligma_circle_map                  (GtkWidget            *widget);
static void        ligma_circle_unmap                (GtkWidget            *widget);
static void        ligma_circle_get_preferred_width  (GtkWidget            *widget,
                                                     gint                 *minimum_width,
                                                     gint                 *natural_width);
static void        ligma_circle_get_preferred_height (GtkWidget            *widget,
                                                     gint                 *minimum_height,
                                                     gint                 *natural_height);
static void        ligma_circle_size_allocate        (GtkWidget            *widget,
                                                     GtkAllocation        *allocation);
static gboolean    ligma_circle_draw                 (GtkWidget            *widget,
                                                     cairo_t              *cr);
static gboolean    ligma_circle_button_press_event   (GtkWidget            *widget,
                                                     GdkEventButton       *bevent);
static gboolean    ligma_circle_button_release_event (GtkWidget            *widget,
                                                     GdkEventButton       *bevent);
static gboolean    ligma_circle_enter_notify_event   (GtkWidget            *widget,
                                                     GdkEventCrossing     *event);
static gboolean    ligma_circle_leave_notify_event   (GtkWidget            *widget,
                                                     GdkEventCrossing     *event);

static void        ligma_circle_real_reset_target    (LigmaCircle           *circle);

static void        ligma_circle_background_hsv       (gdouble               angle,
                                                     gdouble               distance,
                                                     guchar               *rgb);

static void        ligma_circle_draw_background      (LigmaCircle           *circle,
                                                     cairo_t              *cr,
                                                     gint                  size,
                                                     LigmaCircleBackground  background);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCircle, ligma_circle, GTK_TYPE_WIDGET)

#define parent_class ligma_circle_parent_class


static void
ligma_circle_class_init (LigmaCircleClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = ligma_circle_dispose;
  object_class->get_property         = ligma_circle_get_property;
  object_class->set_property         = ligma_circle_set_property;

  widget_class->realize              = ligma_circle_realize;
  widget_class->unrealize            = ligma_circle_unrealize;
  widget_class->map                  = ligma_circle_map;
  widget_class->unmap                = ligma_circle_unmap;
  widget_class->get_preferred_width  = ligma_circle_get_preferred_width;
  widget_class->get_preferred_height = ligma_circle_get_preferred_height;
  widget_class->size_allocate        = ligma_circle_size_allocate;
  widget_class->draw                 = ligma_circle_draw;
  widget_class->button_press_event   = ligma_circle_button_press_event;
  widget_class->button_release_event = ligma_circle_button_release_event;
  widget_class->enter_notify_event   = ligma_circle_enter_notify_event;
  widget_class->leave_notify_event   = ligma_circle_leave_notify_event;

  klass->reset_target                = ligma_circle_real_reset_target;

  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     NULL, NULL,
                                                     32, 1024, 96,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BORDER_WIDTH,
                                   g_param_spec_int ("border-width",
                                                     NULL, NULL,
                                                     0, 64, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BACKGROUND,
                                   g_param_spec_enum ("background",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_CIRCLE_BACKGROUND,
                                                      LIGMA_CIRCLE_BACKGROUND_HSV,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
ligma_circle_init (LigmaCircle *circle)
{
  circle->priv = ligma_circle_get_instance_private (circle);

  gtk_widget_set_has_window (GTK_WIDGET (circle), FALSE);
  gtk_widget_add_events (GTK_WIDGET (circle),
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_ENTER_NOTIFY_MASK   |
                         GDK_LEAVE_NOTIFY_MASK);
}

static void
ligma_circle_dispose (GObject *object)
{
  LigmaCircle *circle = LIGMA_CIRCLE (object);

  g_clear_pointer (&circle->priv->surface, cairo_surface_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_circle_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaCircle *circle = LIGMA_CIRCLE (object);

  switch (property_id)
    {
    case PROP_SIZE:
      circle->priv->size = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (circle));
      break;

    case PROP_BORDER_WIDTH:
      circle->priv->border_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (circle));
      break;

    case PROP_BACKGROUND:
      circle->priv->background = g_value_get_enum (value);
      g_clear_pointer (&circle->priv->surface, cairo_surface_destroy);
      gtk_widget_queue_draw (GTK_WIDGET (circle));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_circle_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaCircle *circle = LIGMA_CIRCLE (object);

  switch (property_id)
    {
    case PROP_SIZE:
      g_value_set_int (value, circle->priv->size);
      break;

    case PROP_BORDER_WIDTH:
      g_value_set_int (value, circle->priv->border_width);
      break;

    case PROP_BACKGROUND:
      g_value_set_enum (value, circle->priv->background);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_circle_realize (GtkWidget *widget)
{
  LigmaCircle    *circle = LIGMA_CIRCLE (widget);
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

  circle->priv->event_window = gdk_window_new (gtk_widget_get_window (widget),
                                               &attributes, attributes_mask);
  gdk_window_set_user_data (circle->priv->event_window, circle);
}

static void
ligma_circle_unrealize (GtkWidget *widget)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  if (circle->priv->event_window)
    {
      gdk_window_set_user_data (circle->priv->event_window, NULL);
      gdk_window_destroy (circle->priv->event_window);
      circle->priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
ligma_circle_map (GtkWidget *widget)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (circle->priv->event_window)
    gdk_window_show (circle->priv->event_window);
}

static void
ligma_circle_unmap (GtkWidget *widget)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  if (circle->priv->has_grab)
    {
      gtk_grab_remove (widget);
      circle->priv->has_grab = FALSE;
    }

  if (circle->priv->event_window)
    gdk_window_hide (circle->priv->event_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_circle_get_preferred_width (GtkWidget *widget,
                                 gint      *minimum_width,
                                 gint      *natural_width)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  *minimum_width = *natural_width = 2 * circle->priv->border_width + circle->priv->size;
}

static void
ligma_circle_get_preferred_height (GtkWidget *widget,
                                  gint      *minimum_height,
                                  gint      *natural_height)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  *minimum_height = *natural_height = 2 * circle->priv->border_width + circle->priv->size;
}

static void
ligma_circle_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (circle->priv->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  g_clear_pointer (&circle->priv->surface, cairo_surface_destroy);
}

static gboolean
ligma_circle_draw (GtkWidget *widget,
                  cairo_t   *cr)
{
  LigmaCircle    *circle = LIGMA_CIRCLE (widget);
  GtkAllocation  allocation;
  gint           size = circle->priv->size;

  gtk_widget_get_allocation (widget, &allocation);

  cairo_save (cr);

  cairo_translate (cr,
                   (allocation.width  - size) / 2,
                   (allocation.height - size) / 2);

  ligma_circle_draw_background (circle, cr, size, circle->priv->background);

  cairo_restore (cr);

  return FALSE;
}

static gboolean
ligma_circle_button_press_event (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  if (bevent->type == GDK_BUTTON_PRESS &&
      bevent->button == 1)
    {
      gtk_grab_add (widget);
      circle->priv->has_grab = TRUE;
    }

  return FALSE;
}

static gboolean
ligma_circle_button_release_event (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);
      circle->priv->has_grab = FALSE;

      if (! circle->priv->in_widget)
        LIGMA_CIRCLE_GET_CLASS (circle)->reset_target (circle);
    }

  return FALSE;
}

static gboolean
ligma_circle_enter_notify_event (GtkWidget        *widget,
                                GdkEventCrossing *event)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  circle->priv->in_widget = TRUE;

  return FALSE;
}

static gboolean
ligma_circle_leave_notify_event (GtkWidget        *widget,
                                GdkEventCrossing *event)
{
  LigmaCircle *circle = LIGMA_CIRCLE (widget);

  circle->priv->in_widget = FALSE;

  if (! circle->priv->has_grab)
    LIGMA_CIRCLE_GET_CLASS (circle)->reset_target (circle);

  return FALSE;
}

static void
ligma_circle_real_reset_target (LigmaCircle *circle)
{
}


/*  public functions  */

GtkWidget *
ligma_circle_new (void)
{
  return g_object_new (LIGMA_TYPE_CIRCLE, NULL);
}


/*  protected functions  */

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

gboolean
_ligma_circle_has_grab (LigmaCircle *circle)
{
  g_return_val_if_fail (LIGMA_IS_CIRCLE (circle), FALSE);

  return circle->priv->has_grab;
}

gdouble
_ligma_circle_get_angle_and_distance (LigmaCircle *circle,
                                     gdouble     event_x,
                                     gdouble     event_y,
                                     gdouble    *distance)
{
  GtkAllocation allocation;
  gdouble       center_x;
  gdouble       center_y;

  g_return_val_if_fail (LIGMA_IS_CIRCLE (circle), 0.0);

  gtk_widget_get_allocation (GTK_WIDGET (circle), &allocation);

  center_x = allocation.width  / 2.0;
  center_y = allocation.height / 2.0;

  return get_angle_and_distance (center_x, center_y, circle->priv->size / 2.0,
                                 event_x, event_y,
                                 distance);
}


/*  private functions  */

static void
ligma_circle_background_hsv (gdouble  angle,
                            gdouble  distance,
                            guchar  *rgb)
{
  LigmaHSV hsv;
  LigmaRGB color;

  ligma_hsv_set (&hsv,
                angle / (2.0 * G_PI),
                distance,
                1 - sqrt (distance) / 4 /* it just looks nicer this way */);

  ligma_hsv_to_rgb (&hsv, &color);

  ligma_rgb_get_uchar (&color, rgb, rgb + 1, rgb + 2);
}

static void
ligma_circle_draw_background (LigmaCircle           *circle,
                             cairo_t              *cr,
                             gint                  size,
                             LigmaCircleBackground  background)
{
  cairo_save (cr);

  if (background == LIGMA_CIRCLE_BACKGROUND_PLAIN)
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
      if (! circle->priv->surface)
        {
          guchar *data;
          gint    stride;
          gint    x, y;

          circle->priv->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                              size, size);

          data   = cairo_image_surface_get_data (circle->priv->surface);
          stride = cairo_image_surface_get_stride (circle->priv->surface);

          for (y = 0; y < size; y++)
            {
              for (x = 0; x < size; x++)
                {
                  gdouble angle;
                  gdouble distance;
                  guchar  rgb[3] = { 0, };

                  angle = get_angle_and_distance (size / 2.0, size / 2.0,
                                                  size / 2.0,
                                                  x, y,
                                                  &distance);

                  switch (background)
                    {
                    case LIGMA_CIRCLE_BACKGROUND_HSV:
                      ligma_circle_background_hsv (angle, distance, rgb);
                      break;

                    default:
                      break;
                    }

                  LIGMA_CAIRO_ARGB32_SET_PIXEL (data + y * stride + x * 4,
                                               rgb[0], rgb[1], rgb[2], 255);
                }
            }

          cairo_surface_mark_dirty (circle->priv->surface);
        }

      cairo_set_source_surface (cr, circle->priv->surface, 0.0, 0.0);

      cairo_arc (cr, size / 2.0, size / 2.0, size / 2.0, 0.0, 2 * G_PI);
      cairo_clip (cr);

      cairo_paint (cr);
    }

  cairo_restore (cr);
}
