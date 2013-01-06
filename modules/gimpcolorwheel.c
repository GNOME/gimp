/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
 *          Michael Natterer <mitch@gimp.org> (ported back to GIMP)
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include <gegl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgimpconfig/gimpconfig.h>
#include <libgimpcolor/gimpcolor.h>
#include <libgimpmath/gimpmath.h>
#include <libgimpwidgets/gimpwidgets.h>

#include "gimpcolorwheel.h"


/* Default ring fraction */
#define DEFAULT_FRACTION 0.1

/* Default width/height */
#define DEFAULT_SIZE 100

/* Default ring width */
#define DEFAULT_RING_WIDTH 10


/* Dragging modes */
typedef enum
{
  DRAG_NONE,
  DRAG_H,
  DRAG_SV
} DragMode;

/* Private part of the GimpColorWheel structure */
typedef struct
{
  /* Color value */
  gdouble h;
  gdouble s;
  gdouble v;

  /* ring_width is this fraction of size */
  gdouble ring_fraction;

  /* Size and ring width */
  gint size;
  gint ring_width;

  /* Window for capturing events */
  GdkWindow *window;

  /* Dragging mode */
  DragMode mode;

  guint focus_on_ring : 1;

  GimpColorConfig    *config;
  GimpColorTransform *transform;
} GimpColorWheelPrivate;

enum
{
  CHANGED,
  MOVE,
  LAST_SIGNAL
};


static void     gimp_color_wheel_dispose              (GObject            *object);

static void     gimp_color_wheel_map                  (GtkWidget          *widget);
static void     gimp_color_wheel_unmap                (GtkWidget          *widget);
static void     gimp_color_wheel_realize              (GtkWidget          *widget);
static void     gimp_color_wheel_unrealize            (GtkWidget          *widget);
static void     gimp_color_wheel_get_preferred_width  (GtkWidget          *widget,
                                                       gint               *minimum,
                                                       gint               *natural);
static void     gimp_color_wheel_get_preferred_height (GtkWidget          *widget,
                                                       gint               *minimum,
                                                       gint               *natural);
static void     gimp_color_wheel_size_allocate        (GtkWidget          *widget,
                                                       GtkAllocation      *allocation);
static gboolean gimp_color_wheel_button_press         (GtkWidget          *widget,
                                                       GdkEventButton     *event);
static gboolean gimp_color_wheel_button_release       (GtkWidget          *widget,
                                                       GdkEventButton     *event);
static gboolean gimp_color_wheel_motion               (GtkWidget          *widget,
                                                       GdkEventMotion     *event);
static gboolean gimp_color_wheel_draw                 (GtkWidget          *widget,
                                                       cairo_t            *cr);
static gboolean gimp_color_wheel_grab_broken          (GtkWidget          *widget,
                                                       GdkEventGrabBroken *event);
static gboolean gimp_color_wheel_focus                (GtkWidget          *widget,
                                                       GtkDirectionType    direction);
static void     gimp_color_wheel_move                 (GimpColorWheel     *wheel,
                                                       GtkDirectionType    dir);

static void     gimp_color_wheel_create_transform     (GimpColorWheel     *wheel);
static void     gimp_color_wheel_destroy_transform    (GimpColorWheel     *wheel);


static guint wheel_signals[LAST_SIGNAL];

G_DEFINE_DYNAMIC_TYPE (GimpColorWheel, gimp_color_wheel, GTK_TYPE_WIDGET)

#define parent_class gimp_color_wheel_parent_class


void
color_wheel_register_type (GTypeModule *module)
{
  gimp_color_wheel_register_type (module);
}

static void
gimp_color_wheel_class_init (GimpColorWheelClass *class)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (class);
  GimpColorWheelClass *wheel_class  = GIMP_COLOR_WHEEL_CLASS (class);
  GtkBindingSet       *binding_set;

  object_class->dispose              = gimp_color_wheel_dispose;

  widget_class->map                  = gimp_color_wheel_map;
  widget_class->unmap                = gimp_color_wheel_unmap;
  widget_class->realize              = gimp_color_wheel_realize;
  widget_class->unrealize            = gimp_color_wheel_unrealize;
  widget_class->get_preferred_width  = gimp_color_wheel_get_preferred_width;
  widget_class->get_preferred_height = gimp_color_wheel_get_preferred_height;
  widget_class->size_allocate        = gimp_color_wheel_size_allocate;
  widget_class->button_press_event   = gimp_color_wheel_button_press;
  widget_class->button_release_event = gimp_color_wheel_button_release;
  widget_class->motion_notify_event  = gimp_color_wheel_motion;
  widget_class->draw                 = gimp_color_wheel_draw;
  widget_class->focus                = gimp_color_wheel_focus;
  widget_class->grab_broken_event    = gimp_color_wheel_grab_broken;

  wheel_class->move                  = gimp_color_wheel_move;

  wheel_signals[CHANGED] =
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorWheelClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  wheel_signals[MOVE] =
    g_signal_new ("move",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpColorWheelClass, move),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_DIRECTION_TYPE);

  binding_set = gtk_binding_set_by_class (class);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Up, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_UP);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Down, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_DOWN);


  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_RIGHT);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Right, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_RIGHT);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_LEFT);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Left, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_LEFT);

  g_type_class_add_private (object_class, sizeof (GimpColorWheelPrivate));
}

static void
gimp_color_wheel_class_finalize (GimpColorWheelClass *klass)
{
}

static void
gimp_color_wheel_init (GimpColorWheel *wheel)
{
  GimpColorWheelPrivate *priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (wheel, GIMP_TYPE_COLOR_WHEEL,
                                      GimpColorWheelPrivate);

  wheel->priv = priv;

  gtk_widget_set_has_window (GTK_WIDGET (wheel), FALSE);
  gtk_widget_set_can_focus (GTK_WIDGET (wheel), TRUE);

  priv->ring_fraction = DEFAULT_FRACTION;
  priv->size          = DEFAULT_SIZE;
  priv->ring_width    = DEFAULT_RING_WIDTH;

  gimp_widget_track_monitor (GTK_WIDGET (wheel),
                             G_CALLBACK (gimp_color_wheel_destroy_transform),
                             NULL);
}

static void
gimp_color_wheel_dispose (GObject *object)
{
  GimpColorWheel *wheel = GIMP_COLOR_WHEEL (object);

  gimp_color_wheel_set_color_config (wheel, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_wheel_map (GtkWidget *widget)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  gdk_window_show (priv->window);
}

static void
gimp_color_wheel_unmap (GtkWidget *widget)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;

  gdk_window_hide (priv->window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_color_wheel_realize (GtkWidget *widget)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  GtkAllocation          allocation;
  GdkWindowAttr          attr;
  gint                   attr_mask;
  GdkWindow             *parent_window;

  gtk_widget_get_allocation (widget, &allocation);

  gtk_widget_set_realized (widget, TRUE);

  attr.window_type = GDK_WINDOW_CHILD;
  attr.x           = allocation.x;
  attr.y           = allocation.y;
  attr.width       = allocation.width;
  attr.height      = allocation.height;
  attr.wclass      = GDK_INPUT_ONLY;
  attr.event_mask  = (gtk_widget_get_events (widget) |
                      GDK_KEY_PRESS_MASK      |
                      GDK_BUTTON_PRESS_MASK   |
                      GDK_BUTTON_RELEASE_MASK |
                      GDK_POINTER_MOTION_MASK |
                      GDK_ENTER_NOTIFY_MASK   |
                      GDK_LEAVE_NOTIFY_MASK);

  attr_mask = GDK_WA_X | GDK_WA_Y;

  parent_window = gtk_widget_get_parent_window (widget);

  gtk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  priv->window = gdk_window_new (parent_window, &attr, attr_mask);
  gdk_window_set_user_data (priv->window, wheel);

  gtk_widget_style_attach (widget);
}

static void
gimp_color_wheel_unrealize (GtkWidget *widget)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;

  gdk_window_set_user_data (priv->window, NULL);
  gdk_window_destroy (priv->window);
  priv->window = NULL;

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_color_wheel_get_preferred_width (GtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  gint focus_width;
  gint focus_pad;

  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  *minimum = *natural = DEFAULT_SIZE + 2 * (focus_width + focus_pad);
}

static void
gimp_color_wheel_get_preferred_height (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  gint focus_width;
  gint focus_pad;

  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  *minimum = *natural = DEFAULT_SIZE + 2 * (focus_width + focus_pad);
}

static void
gimp_color_wheel_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  gint                   focus_width;
  gint                   focus_pad;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding",    &focus_pad,
                        NULL);

  priv->size = MIN (allocation->width  - 2 * (focus_width + focus_pad),
                    allocation->height - 2 * (focus_width + focus_pad));

  priv->ring_width = priv->size * priv->ring_fraction;

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (priv->window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);
}


/* Utility functions */

/* Converts from HSV to RGB */
static void
hsv_to_rgb (gdouble *h,
            gdouble *s,
            gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
   /* *v = *v; -- heh */
    }
  else
    {
      hue = *h * 6.0;
      saturation = *s;
      value = *v;

      if (hue == 6.0)
        hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
        {
        case 0:
          *h = value;
          *s = t;
          *v = p;
          break;

        case 1:
          *h = q;
          *s = value;
          *v = p;
          break;

        case 2:
          *h = p;
          *s = value;
          *v = t;
          break;

        case 3:
          *h = p;
          *s = q;
          *v = value;
          break;

        case 4:
          *h = t;
          *s = p;
          *v = value;
          break;

        case 5:
          *h = value;
          *s = p;
          *v = q;
          break;

        default:
          g_assert_not_reached ();
        }
    }
}

/* Computes the vertices of the saturation/value triangle */
static void
compute_triangle (GimpColorWheel *wheel,
                  gint           *hx,
                  gint           *hy,
                  gint           *sx,
                  gint           *sy,
                  gint           *vx,
                  gint           *vy)
{
  GimpColorWheelPrivate *priv = wheel->priv;
  GtkAllocation          allocation;
  gdouble                center_x;
  gdouble                center_y;
  gdouble                inner, outer;
  gdouble                angle;

  gtk_widget_get_allocation (GTK_WIDGET (wheel), &allocation);

  center_x = allocation.width / 2.0;
  center_y = allocation.height / 2.0;

  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;
  angle = priv->h * 2.0 * G_PI;

  *hx = floor (center_x + cos (angle) * inner + 0.5);
  *hy = floor (center_y - sin (angle) * inner + 0.5);
  *sx = floor (center_x + cos (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *sy = floor (center_y - sin (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *vx = floor (center_x + cos (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
  *vy = floor (center_y - sin (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
}

/* Computes whether a point is inside the hue ring */
static gboolean
is_in_ring (GimpColorWheel *wheel,
            gdouble         x,
            gdouble         y)
{
  GimpColorWheelPrivate *priv = wheel->priv;
  GtkAllocation          allocation;
  gdouble                dx, dy, dist;
  gdouble                center_x;
  gdouble                center_y;
  gdouble                inner, outer;

  gtk_widget_get_allocation (GTK_WIDGET (wheel), &allocation);

  center_x = allocation.width / 2.0;
  center_y = allocation.height / 2.0;

  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;

  dx = x - center_x;
  dy = center_y - y;
  dist = dx * dx + dy * dy;

  return (dist >= inner * inner && dist <= outer * outer);
}

/* Computes a saturation/value pair based on the mouse coordinates */
static void
compute_sv (GimpColorWheel *wheel,
            gdouble         x,
            gdouble         y,
            gdouble        *s,
            gdouble        *v)
{
  GtkAllocation allocation;
  gint          ihx, ihy, isx, isy, ivx, ivy;
  gdouble       hx, hy, sx, sy, vx, vy;
  gdouble       center_x;
  gdouble       center_y;

  gtk_widget_get_allocation (GTK_WIDGET (wheel), &allocation);

  compute_triangle (wheel, &ihx, &ihy, &isx, &isy, &ivx, &ivy);

  center_x = allocation.width / 2.0;
  center_y = allocation.height / 2.0;

  hx = ihx - center_x;
  hy = center_y - ihy;
  sx = isx - center_x;
  sy = center_y - isy;
  vx = ivx - center_x;
  vy = center_y - ivy;
  x -= center_x;
  y = center_y - y;

  if (vx * (x - sx) + vy * (y - sy) < 0.0)
    {
      *s = 1.0;
      *v = (((x - sx) * (hx - sx) + (y - sy) * (hy-sy))
            / ((hx - sx) * (hx - sx) + (hy - sy) * (hy - sy)));

      if (*v < 0.0)
        *v = 0.0;
      else if (*v > 1.0)
        *v = 1.0;
    }
  else if (hx * (x - sx) + hy * (y - sy) < 0.0)
    {
      *s = 0.0;
      *v = (((x - sx) * (vx - sx) + (y - sy) * (vy - sy))
            / ((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy)));

      if (*v < 0.0)
        *v = 0.0;
      else if (*v > 1.0)
        *v = 1.0;
    }
  else if (sx * (x - hx) + sy * (y - hy) < 0.0)
    {
      *v = 1.0;
      *s = (((x - vx) * (hx - vx) + (y - vy) * (hy - vy)) /
            ((hx - vx) * (hx - vx) + (hy - vy) * (hy - vy)));

      if (*s < 0.0)
        *s = 0.0;
      else if (*s > 1.0)
        *s = 1.0;
    }
  else
    {
      *v = (((x - sx) * (hy - vy) - (y - sy) * (hx - vx))
            / ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx)));

      if (*v<= 0.0)
        {
          *v = 0.0;
          *s = 0.0;
        }
      else
        {
          if (*v > 1.0)
            *v = 1.0;

          if (fabs (hy - vy) < fabs (hx - vx))
            *s = (x - sx - *v * (vx - sx)) / (*v * (hx - vx));
          else
            *s = (y - sy - *v * (vy - sy)) / (*v * (hy - vy));

          if (*s < 0.0)
            *s = 0.0;
          else if (*s > 1.0)
            *s = 1.0;
        }
    }
}

/* Computes whether a point is inside the saturation/value triangle */
static gboolean
is_in_triangle (GimpColorWheel *wheel,
                gdouble         x,
                gdouble         y)
{
  gint    hx, hy, sx, sy, vx, vy;
  gdouble det, s, v;

  compute_triangle (wheel, &hx, &hy, &sx, &sy, &vx, &vy);

  det = (vx - sx) * (hy - sy) - (vy - sy) * (hx - sx);

  s = ((x - sx) * (hy - sy) - (y - sy) * (hx - sx)) / det;
  v = ((vx - sx) * (y - sy) - (vy - sy) * (x - sx)) / det;

  return (s >= 0.0 && v >= 0.0 && s + v <= 1.0);
}

/* Computes a value based on the mouse coordinates */
static double
compute_v (GimpColorWheel *wheel,
           gdouble         x,
           gdouble         y)
{
  GtkAllocation allocation;
  gdouble       center_x;
  gdouble       center_y;
  gdouble       dx, dy;
  gdouble       angle;

  gtk_widget_get_allocation (GTK_WIDGET (wheel), &allocation);

  center_x = allocation.width / 2.0;
  center_y = allocation.height / 2.0;

  dx = x - center_x;
  dy = center_y - y;

  angle = atan2 (dy, dx);
  if (angle < 0.0)
    angle += 2.0 * G_PI;

  return angle / (2.0 * G_PI);
}

static void
set_cross_grab (GimpColorWheel *wheel,
                guint32         time)
{
  GimpColorWheelPrivate *priv = wheel->priv;
  GdkCursor             *cursor;

  cursor =
    gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (wheel)),
                                GDK_CROSSHAIR);

  gdk_pointer_grab (priv->window, FALSE,
                    GDK_POINTER_MOTION_MASK      |
                    GDK_POINTER_MOTION_HINT_MASK |
                    GDK_BUTTON_RELEASE_MASK,
                    NULL, cursor, time);
  g_object_unref (cursor);
}

static gboolean
gimp_color_wheel_grab_broken (GtkWidget          *widget,
                              GdkEventGrabBroken *event)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;

  priv->mode = DRAG_NONE;

  return TRUE;
}

static gboolean
gimp_color_wheel_button_press (GtkWidget      *widget,
                               GdkEventButton *event)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  gdouble                x, y;

  if (priv->mode != DRAG_NONE || event->button != 1)
    return FALSE;

  x = event->x;
  y = event->y;

  if (is_in_ring (wheel, x, y))
    {
      priv->mode = DRAG_H;
      set_cross_grab (wheel, event->time);

      gimp_color_wheel_set_color (wheel,
                                  compute_v (wheel, x, y),
                                  priv->s,
                                  priv->v);

      gtk_widget_grab_focus (widget);
      priv->focus_on_ring = TRUE;

      return TRUE;
    }

  if (is_in_triangle (wheel, x, y))
    {
      gdouble s, v;

      priv->mode = DRAG_SV;
      set_cross_grab (wheel, event->time);

      compute_sv (wheel, x, y, &s, &v);
      gimp_color_wheel_set_color (wheel, priv->h, s, v);

      gtk_widget_grab_focus (widget);
      priv->focus_on_ring = FALSE;

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_color_wheel_button_release (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  DragMode               mode;
  gdouble                x, y;

  if (priv->mode == DRAG_NONE || event->button != 1)
    return FALSE;

  /* Set the drag mode to DRAG_NONE so that signal handlers for "catched"
   * can see that this is the final color state.
   */

  mode = priv->mode;
  priv->mode = DRAG_NONE;

  x = event->x;
  y = event->y;

  if (mode == DRAG_H)
    {
      gimp_color_wheel_set_color (wheel,
                                  compute_v (wheel, x, y), priv->s, priv->v);
    }
  else if (mode == DRAG_SV)
    {
      gdouble s, v;

      compute_sv (wheel, x, y, &s, &v);
      gimp_color_wheel_set_color (wheel, priv->h, s, v);
    }
  else
    g_assert_not_reached ();

  gdk_display_pointer_ungrab (gdk_window_get_display (event->window),
                              event->time);

  return TRUE;
}

static gboolean
gimp_color_wheel_motion (GtkWidget      *widget,
                         GdkEventMotion *event)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  gdouble                x, y;

  if (priv->mode == DRAG_NONE)
    return FALSE;

  gdk_event_request_motions (event);
  x = event->x;
  y = event->y;

  if (priv->mode == DRAG_H)
    {
      gimp_color_wheel_set_color (wheel,
                                  compute_v (wheel, x, y), priv->s, priv->v);
      return TRUE;
    }
  else if (priv->mode == DRAG_SV)
    {
      gdouble s, v;

      compute_sv (wheel, x, y, &s, &v);
      gimp_color_wheel_set_color (wheel, priv->h, s, v);
      return TRUE;
    }

  g_assert_not_reached ();

  return FALSE;
}


/* Redrawing */

/* Paints the hue ring */
static void
paint_ring (GimpColorWheel *wheel,
            cairo_t        *cr)
{
  GtkWidget             *widget = GTK_WIDGET (wheel);
  GimpColorWheelPrivate *priv   = wheel->priv;
  gint                   width, height;
  gint                   xx, yy;
  gdouble                dx, dy, dist;
  gdouble                center_x;
  gdouble                center_y;
  gdouble                inner, outer;
  guint32               *buf, *p;
  gdouble                angle;
  gdouble                hue;
  gdouble                r, g, b;
  cairo_surface_t       *source;
  cairo_t               *source_cr;
  gint                   stride;

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  center_x = width  / 2.0;
  center_y = height / 2.0;

  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;

  /* Create an image initialized with the ring colors */

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
  buf = g_new (guint32, height * stride / 4);

  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;

      dy = -(yy - center_y);

      for (xx = 0; xx < width; xx++)
        {
          dx = xx - center_x;

          dist = dx * dx + dy * dy;
          if (dist < ((inner-1) * (inner-1)) || dist > ((outer+1) * (outer+1)))
            {
              *p++ = 0;
              continue;
            }

          angle = atan2 (dy, dx);
          if (angle < 0.0)
            angle += 2.0 * G_PI;

          hue = angle / (2.0 * G_PI);

          r = hue;
          g = 1.0;
          b = 1.0;
          hsv_to_rgb (&r, &g, &b);

          *p++ = (((int)floor (r * 255 + 0.5) << 16) |
                  ((int)floor (g * 255 + 0.5) << 8) |
                  (int)floor (b * 255 + 0.5));
        }
    }

  if (priv->transform)
    {
      const Babl *format = babl_format ("cairo-RGB24");
      guchar     *b      = (guchar *) buf;
      gint        i;

      for (i = 0; i < height; i++)
        {
          gimp_color_transform_process_pixels (priv->transform,
                                               format, b,
                                               format, b,
                                               width);

          b  += stride;
        }
    }

  source = cairo_image_surface_create_for_data ((guchar *) buf,
                                                CAIRO_FORMAT_RGB24,
                                                width, height, stride);

  /* Now draw the value marker onto the source image, so that it
   * will get properly clipped at the edges of the ring
   */
  source_cr = cairo_create (source);

  r = priv->h;
  g = 1.0;
  b = 1.0;
  hsv_to_rgb (&r, &g, &b);

  if (GIMP_RGB_LUMINANCE (r, g, b) > 0.5)
    cairo_set_source_rgb (source_cr, 0.0, 0.0, 0.0);
  else
    cairo_set_source_rgb (source_cr, 1.0, 1.0, 1.0);

  cairo_move_to (source_cr, center_x, center_y);
  cairo_line_to (source_cr,
                 center_x + cos (priv->h * 2.0 * G_PI) * priv->size / 2,
                 center_y - sin (priv->h * 2.0 * G_PI) * priv->size / 2);
  cairo_stroke (source_cr);
  cairo_destroy (source_cr);

  /* Draw the ring using the source image */

  cairo_save (cr);

  cairo_set_source_surface (cr, source, 0, 0);
  cairo_surface_destroy (source);

  cairo_set_line_width (cr, priv->ring_width);
  cairo_new_path (cr);
  cairo_arc (cr,
             center_x, center_y,
             priv->size / 2.0 - priv->ring_width / 2.0,
             0, 2 * G_PI);
  cairo_stroke (cr);

  cairo_restore (cr);

  g_free (buf);
}

/* Converts an HSV triplet to an integer RGB triplet */
static void
get_color (gdouble  h,
           gdouble  s,
           gdouble  v,
           gint    *r,
           gint    *g,
           gint    *b)
{
  hsv_to_rgb (&h, &s, &v);

  *r = floor (h * 255 + 0.5);
  *g = floor (s * 255 + 0.5);
  *b = floor (v * 255 + 0.5);
}

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#define LERP(a, b, v1, v2, i) (((v2) - (v1) != 0)                                       \
                               ? ((a) + ((b) - (a)) * ((i) - (v1)) / ((v2) - (v1)))     \
                               : (a))

/* Number of pixels we extend out from the edges when creating
 * color source to avoid artifacts
 */
#define PAD 3

/* Paints the HSV triangle */
static void
paint_triangle (GimpColorWheel *wheel,
                cairo_t        *cr,
                gboolean        draw_focus)
{
  GtkWidget             *widget = GTK_WIDGET (wheel);
  GimpColorWheelPrivate *priv   = wheel->priv;
  gint                   hx, hy, sx, sy, vx, vy; /* HSV vertices */
  gint                   x1, y1, r1, g1, b1; /* First vertex in scanline order */
  gint                   x2, y2, r2, g2, b2; /* Second vertex */
  gint                   x3, y3, r3, g3, b3; /* Third vertex */
  gint                   t;
  guint32               *buf, *p, c;
  gint                   xl, xr, rl, rr, gl, gr, bl, br; /* Scanline data */
  gint                   xx, yy;
  gint                   x_interp, y_interp;
  gint                   x_start, x_end;
  cairo_surface_t       *source;
  gdouble                r, g, b;
  gint                   stride;
  gint                   width, height;
  GtkStyleContext       *context;

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  /* Compute triangle's vertices */

  compute_triangle (wheel, &hx, &hy, &sx, &sy, &vx, &vy);

  x1 = hx;
  y1 = hy;
  get_color (priv->h, 1.0, 1.0, &r1, &g1, &b1);

  x2 = sx;
  y2 = sy;
  get_color (priv->h, 1.0, 0.0, &r2, &g2, &b2);

  x3 = vx;
  y3 = vy;
  get_color (priv->h, 0.0, 1.0, &r3, &g3, &b3);

  if (y2 > y3)
    {
      SWAP (x2, x3, t);
      SWAP (y2, y3, t);
      SWAP (r2, r3, t);
      SWAP (g2, g3, t);
      SWAP (b2, b3, t);
    }

  if (y1 > y3)
    {
      SWAP (x1, x3, t);
      SWAP (y1, y3, t);
      SWAP (r1, r3, t);
      SWAP (g1, g3, t);
      SWAP (b1, b3, t);
    }

  if (y1 > y2)
    {
      SWAP (x1, x2, t);
      SWAP (y1, y2, t);
      SWAP (r1, r2, t);
      SWAP (g1, g2, t);
      SWAP (b1, b2, t);
    }

  /* Shade the triangle */

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
  buf = g_new (guint32, height * stride / 4);

  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;

      if (yy >= y1 - PAD && yy < y3 + PAD)
        {
          y_interp = CLAMP (yy, y1, y3);

          if (y_interp < y2)
            {
              xl = LERP (x1, x2, y1, y2, y_interp);

              rl = LERP (r1, r2, y1, y2, y_interp);
              gl = LERP (g1, g2, y1, y2, y_interp);
              bl = LERP (b1, b2, y1, y2, y_interp);
            }
          else
            {
              xl = LERP (x2, x3, y2, y3, y_interp);

              rl = LERP (r2, r3, y2, y3, y_interp);
              gl = LERP (g2, g3, y2, y3, y_interp);
              bl = LERP (b2, b3, y2, y3, y_interp);
            }

          xr = LERP (x1, x3, y1, y3, y_interp);

          rr = LERP (r1, r3, y1, y3, y_interp);
          gr = LERP (g1, g3, y1, y3, y_interp);
          br = LERP (b1, b3, y1, y3, y_interp);

          if (xl > xr)
            {
              SWAP (xl, xr, t);
              SWAP (rl, rr, t);
              SWAP (gl, gr, t);
              SWAP (bl, br, t);
            }

          x_start = MAX (xl - PAD, 0);
          x_end = MIN (xr + PAD, width);
          x_start = MIN (x_start, x_end);

          c = (rl << 16) | (gl << 8) | bl;

          for (xx = 0; xx < x_start; xx++)
            *p++ = c;

          for (; xx < x_end; xx++)
            {
              x_interp = CLAMP (xx, xl, xr);

              *p++ = ((LERP (rl, rr, xl, xr, x_interp) << 16) |
                      (LERP (gl, gr, xl, xr, x_interp) << 8) |
                      LERP (bl, br, xl, xr, x_interp));
            }

          c = (rr << 16) | (gr << 8) | br;

          for (; xx < width; xx++)
            *p++ = c;
        }
    }

  if (priv->transform)
    {
      const Babl *format = babl_format ("cairo-RGB24");
      guchar     *b      = (guchar *) buf;
      gint        i;

      for (i = 0; i < height; i++)
        {
          gimp_color_transform_process_pixels (priv->transform,
                                               format, b,
                                               format, b,
                                               width);

          b  += stride;
        }
    }

  source = cairo_image_surface_create_for_data ((guchar *) buf,
                                                CAIRO_FORMAT_RGB24,
                                                width, height, stride);

  /* Draw a triangle with the image as a source */

  cairo_set_source_surface (cr, source, 0, 0);
  cairo_surface_destroy (source);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_line_to (cr, x3, y3);
  cairo_close_path (cr);
  cairo_fill (cr);

  g_free (buf);

  /* Draw value marker */

  xx = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  yy = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);

  r = priv->h;
  g = priv->s;
  b = priv->v;
  hsv_to_rgb (&r, &g, &b);

  context = gtk_widget_get_style_context (widget);

  gtk_style_context_save (context);

  if (GIMP_RGB_LUMINANCE (r, g, b) > 0.5)
    {
      gtk_style_context_add_class (context, "light-area-focus");
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    }
  else
    {
      gtk_style_context_add_class (context, "dark-area-focus");
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    }

#define RADIUS 4
#define FOCUS_RADIUS 6

  cairo_new_path (cr);
  cairo_arc (cr, xx, yy, RADIUS, 0, 2 * G_PI);
  cairo_stroke (cr);

  /* Draw focus outline */

  if (draw_focus && ! priv->focus_on_ring)
    {
      gint focus_width;
      gint focus_pad;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus_width,
                            "focus-padding",    &focus_pad,
                            NULL);

      gtk_render_focus (context, cr,
                        xx - FOCUS_RADIUS - focus_width - focus_pad,
                        yy - FOCUS_RADIUS - focus_width - focus_pad,
                        2 * (FOCUS_RADIUS + focus_width + focus_pad),
                        2 * (FOCUS_RADIUS + focus_width + focus_pad));
    }

  gtk_style_context_restore (context);
}

static gboolean
gimp_color_wheel_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;
  gboolean               draw_focus;

  draw_focus = gtk_widget_has_visible_focus (widget);

  if (! priv->transform)
    gimp_color_wheel_create_transform (wheel);

  paint_ring (wheel, cr);
  paint_triangle (wheel, cr, draw_focus);

  if (draw_focus && priv->focus_on_ring)
    {
      GtkStyleContext *context = gtk_widget_get_style_context (widget);

      gtk_render_focus (context, cr, 0, 0,
                        gtk_widget_get_allocated_width (widget),
                        gtk_widget_get_allocated_height (widget));
    }

  return FALSE;
}

static gboolean
gimp_color_wheel_focus (GtkWidget        *widget,
                        GtkDirectionType  dir)
{
  GimpColorWheel        *wheel = GIMP_COLOR_WHEEL (widget);
  GimpColorWheelPrivate *priv  = wheel->priv;

  if (!gtk_widget_has_focus (widget))
    {
      if (dir == GTK_DIR_TAB_BACKWARD)
        priv->focus_on_ring = FALSE;
      else
        priv->focus_on_ring = TRUE;

      gtk_widget_grab_focus (widget);
      return TRUE;
    }

  switch (dir)
    {
    case GTK_DIR_UP:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case GTK_DIR_DOWN:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;

    case GTK_DIR_LEFT:
    case GTK_DIR_TAB_BACKWARD:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case GTK_DIR_RIGHT:
    case GTK_DIR_TAB_FORWARD:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;
    }

  gtk_widget_queue_draw (widget);

  return TRUE;
}

/**
 * gimp_color_wheel_new:
 *
 * Creates a new HSV color selector.
 *
 * Return value: A newly-created HSV color selector.
 *
 * Since: 2.10
 */
GtkWidget*
gimp_color_wheel_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_WHEEL, NULL);
}

/**
 * gimp_color_wheel_set_color:
 * @hsv: An HSV color selector
 * @h: Hue
 * @s: Saturation
 * @v: Value
 *
 * Sets the current color in an HSV color selector.
 * Color component values must be in the [0.0, 1.0] range.
 *
 * Since: 2.10
 */
void
gimp_color_wheel_set_color (GimpColorWheel *wheel,
                            gdouble         h,
                            gdouble         s,
                            gdouble         v)
{
  GimpColorWheelPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_WHEEL (wheel));
  g_return_if_fail (h >= 0.0 && h <= 1.0);
  g_return_if_fail (s >= 0.0 && s <= 1.0);
  g_return_if_fail (v >= 0.0 && v <= 1.0);

  priv = wheel->priv;

  priv->h = h;
  priv->s = s;
  priv->v = v;

  g_signal_emit (wheel, wheel_signals[CHANGED], 0);

  gtk_widget_queue_draw (GTK_WIDGET (wheel));
}

/**
 * gimp_color_wheel_get_color:
 * @hsv: An HSV color selector
 * @h: (out): Return value for the hue
 * @s: (out): Return value for the saturation
 * @v: (out): Return value for the value
 *
 * Queries the current color in an HSV color selector.
 * Returned values will be in the [0.0, 1.0] range.
 *
 * Since: 2.10
 */
void
gimp_color_wheel_get_color (GimpColorWheel *wheel,
                            gdouble        *h,
                            gdouble        *s,
                            gdouble        *v)
{
  GimpColorWheelPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_WHEEL (wheel));

  priv = wheel->priv;

  if (h) *h = priv->h;
  if (s) *s = priv->s;
  if (v) *v = priv->v;
}

/**
 * gimp_color_wheel_set_ring_fraction:
 * @ring: A wheel color selector
 * @fraction: Ring fraction
 *
 * Sets the ring fraction of a wheel color selector.
 *
 * Since: 2.10
 */
void
gimp_color_wheel_set_ring_fraction (GimpColorWheel *hsv,
                                    gdouble         fraction)
{
  GimpColorWheelPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_WHEEL (hsv));

  priv = hsv->priv;

  priv->ring_fraction = CLAMP (fraction, 0.01, 0.99);

  gtk_widget_queue_draw (GTK_WIDGET (hsv));
}

/**
 * gimp_color_wheel_get_ring_fraction:
 * @ring: A wheel color selector
 *
 * Returns value: The ring fraction of the wheel color selector.
 *
 * Since: 2.10
 */
gdouble
gimp_color_wheel_get_ring_fraction (GimpColorWheel *wheel)
{
  GimpColorWheelPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_WHEEL (wheel), DEFAULT_FRACTION);

  priv = wheel->priv;

  return priv->ring_fraction;
}

/**
 * gimp_color_wheel_set_color_config:
 * @wheel:  a #GimpColorWheel widget.
 * @config: a #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color wheel.
 *
 * Since: 2.10
 */
void
gimp_color_wheel_set_color_config (GimpColorWheel  *wheel,
                                   GimpColorConfig *config)
{
  GimpColorWheelPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_WHEEL (wheel));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  priv = wheel->priv;

  if (config != priv->config)
    {
      if (priv->config)
        {
          g_signal_handlers_disconnect_by_func (priv->config,
                                                gimp_color_wheel_destroy_transform,
                                                wheel);
          g_object_unref (priv->config);

          gimp_color_wheel_destroy_transform (wheel);
        }

      priv->config = config;

      if (priv->config)
        {
          g_object_ref (priv->config);

          g_signal_connect_swapped (priv->config, "notify",
                                    G_CALLBACK (gimp_color_wheel_destroy_transform),
                                    wheel);
        }
    }
}

/**
 * gimp_color_wheel_is_adjusting:
 * @hsv: A #GimpColorWheel
 *
 * An HSV color selector can be said to be adjusting if multiple rapid
 * changes are being made to its value, for example, when the user is
 * adjusting the value with the mouse. This function queries whether
 * the HSV color selector is being adjusted or not.
 *
 * Return value: %TRUE if clients can ignore changes to the color value,
 *     since they may be transitory, or %FALSE if they should consider
 *     the color value status to be final.
 *
 * Since: 2.10
 */
gboolean
gimp_color_wheel_is_adjusting (GimpColorWheel *wheel)
{
  GimpColorWheelPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_WHEEL (wheel), FALSE);

  priv = wheel->priv;

  return priv->mode != DRAG_NONE;
}

static void
gimp_color_wheel_move (GimpColorWheel   *wheel,
                       GtkDirectionType  dir)
{
  GimpColorWheelPrivate *priv = wheel->priv;
  gdouble                hue, sat, val;
  gint                   hx, hy, sx, sy, vx, vy; /* HSV vertices */
  gint                   x, y; /* position in triangle */

  hue = priv->h;
  sat = priv->s;
  val = priv->v;

  compute_triangle (wheel, &hx, &hy, &sx, &sy, &vx, &vy);

  x = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  y = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);

#define HUE_DELTA 0.002
  switch (dir)
    {
    case GTK_DIR_UP:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          y -= 1;
          compute_sv (wheel, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_DOWN:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA;
      else
        {
          y += 1;
          compute_sv (wheel, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_LEFT:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          x -= 1;
          compute_sv (wheel, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_RIGHT:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA
          ;
      else
        {
          x += 1;
          compute_sv (wheel, x, y, &sat, &val);
        }
      break;

    default:
      /* we don't care about the tab directions */
      break;
    }

  /* Wrap */
  if (hue < 0.0)
    hue = 1.0;
  else if (hue > 1.0)
    hue = 0.0;

  gimp_color_wheel_set_color (wheel, hue, sat, val);
}

static void
gimp_color_wheel_create_transform (GimpColorWheel *wheel)
{
  GimpColorWheelPrivate *priv = wheel->priv;

  if (priv->config)
    {
      static GimpColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      priv->transform = gimp_widget_get_color_transform (GTK_WIDGET (wheel),
                                                         priv->config,
                                                         profile,
                                                         format,
                                                         format);
    }
}

static void
gimp_color_wheel_destroy_transform (GimpColorWheel *wheel)
{
  GimpColorWheelPrivate *priv = wheel->priv;

  if (priv->transform)
    {
      g_object_unref (priv->transform);
      priv->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (wheel));
}
