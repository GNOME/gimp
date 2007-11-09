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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpcurve.h"
#include "core/gimpmarshal.h"

#include "gimpcurveview.h"


enum
{
  PROP_0,
  PROP_GRID_ROWS,
  PROP_GRID_COLUMNS
};


static void       gimp_curve_view_finalize       (GObject          *object);
static void       gimp_curve_view_dispose        (GObject          *object);
static void       gimp_curve_view_set_property   (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       gimp_curve_view_get_property   (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static gboolean   gimp_curve_view_expose         (GtkWidget        *widget,
                                                  GdkEventExpose   *event);
static gboolean   gimp_curve_view_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean   gimp_curve_view_button_release (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean   gimp_curve_view_motion_notify  (GtkWidget        *widget,
                                                  GdkEventMotion   *bevent);
static gboolean   gimp_curve_view_leave_notify   (GtkWidget        *widget,
                                                  GdkEventCrossing *cevent);
static gboolean   gimp_curve_view_key_press      (GtkWidget        *widget,
                                                  GdkEventKey      *kevent);


G_DEFINE_TYPE (GimpCurveView, gimp_curve_view,
               GIMP_TYPE_HISTOGRAM_VIEW)

#define parent_class gimp_curve_view_parent_class


static void
gimp_curve_view_class_init (GimpCurveViewClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass         *widget_class = GTK_WIDGET_CLASS (klass);
  GimpHistogramViewClass *hist_class   = GIMP_HISTOGRAM_VIEW_CLASS (klass);

  object_class->finalize             = gimp_curve_view_finalize;
  object_class->dispose              = gimp_curve_view_dispose;
  object_class->set_property         = gimp_curve_view_set_property;
  object_class->get_property         = gimp_curve_view_get_property;

  widget_class->expose_event         = gimp_curve_view_expose;
  widget_class->button_press_event   = gimp_curve_view_button_press;
  widget_class->button_release_event = gimp_curve_view_button_release;
  widget_class->motion_notify_event  = gimp_curve_view_motion_notify;
  widget_class->leave_notify_event   = gimp_curve_view_leave_notify;
  widget_class->key_press_event      = gimp_curve_view_key_press;

  hist_class->light_histogram        = TRUE;

  g_object_class_install_property (object_class, PROP_GRID_ROWS,
                                   g_param_spec_int ("grid-rows", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_GRID_COLUMNS,
                                   g_param_spec_int ("grid-columns", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_curve_view_init (GimpCurveView *view)
{
  view->curve       = NULL;
  view->selected    = 0;
  view->last        = 0;
  view->cursor_type = -1;
  view->xpos        = -1;
  view->cursor_x    = -1;
  view->cursor_y    = -1;

  GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);

  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK      |
                         GDK_LEAVE_NOTIFY_MASK);
}

static void
gimp_curve_view_finalize (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  if (view->xpos_layout)
    {
      g_object_unref (view->xpos_layout);
      view->xpos_layout = NULL;
    }

  if (view->cursor_layout)
    {
      g_object_unref (view->cursor_layout);
      view->cursor_layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curve_view_dispose (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  gimp_curve_view_set_curve (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_curve_view_set_property   (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GRID_ROWS:
      view->grid_rows = g_value_get_int (value);
      break;
    case PROP_GRID_COLUMNS:
      view->grid_columns = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GRID_ROWS:
      g_value_set_int (value, view->grid_rows);
      break;
    case PROP_GRID_COLUMNS:
      g_value_set_int (value, view->grid_columns);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_curve_view_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GtkStyle      *style = widget->style;
  cairo_t       *cr;
  gint           border;
  gint           width, height;
  gint           x, y;
  gint           i;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  if (! view->curve)
    return FALSE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;

  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  cr = gdk_cairo_create (widget->window);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1);

  gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);

  /*  Draw the grid lines  */
  for (i = 1; i < view->grid_rows; i++)
    {
      gint y = i * (height / view->grid_rows);

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }


  for (i = 1; i < view->grid_columns; i++)
    {
      gint x = i * (width / view->grid_columns);

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  cairo_stroke (cr);

  /*  Draw the curve  */
  gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

  x = 0;
  y = 255 - view->curve->curve[x];

  cairo_move_to (cr,
                 border + (gdouble) width  * x / 256.0,
                 border + (gdouble) height * y / 256.0);

  for (i = 0; i < 256; i++)
    {
      x = i;
      y = 255 - view->curve->curve[x];

      cairo_line_to (cr,
                     border + (gdouble) width  * x / 256.0,
                     border + (gdouble) height * y / 256.0);
    }

  cairo_stroke (cr);

  if (view->curve->curve_type == GIMP_CURVE_SMOOTH)
    {
      /*  Draw the points  */
      for (i = 0; i < GIMP_CURVE_NUM_POINTS; i++)
        {
          x = view->curve->points[i][0];
          if (x < 0)
            continue;

          y = 255 - view->curve->points[i][1];

          cairo_move_to (cr,
                         border + (gdouble) width  * x / 256.0,
                         border + (gdouble) height * y / 256.0);
          cairo_arc (cr,
                     border + (gdouble) width  * x / 256.0,
                     border + (gdouble) height * y / 256.0,
                     border,
                     0, 2 * G_PI);

          if (i == view->selected)
            {
              cairo_fill (cr);

              gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_NORMAL]);

              cairo_arc (cr,
                         border + (gdouble) width  * x / 256.0,
                         border + (gdouble) height * y / 256.0,
                         border - 2,
                         0, 2 * G_PI);

              cairo_fill (cr);

              gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
            }
        }

      cairo_fill (cr);
    }

  if (view->xpos >= 0)
    {
      gchar buf[32];

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

      /* draw the color line */
      cairo_move_to (cr,
                     border + ROUND ((gdouble) width * view->xpos / 256.0),
                     border);
      cairo_line_to (cr,
                     border + ROUND ((gdouble) width * view->xpos / 256.0),
                     border + height - 1);
      cairo_stroke (cr);

      /* and xpos indicator */
      g_snprintf (buf, sizeof (buf), "x:%d", view->xpos);

      if (! view->xpos_layout)
        view->xpos_layout = gtk_widget_create_pango_layout (widget, buf);
      else
        pango_layout_set_text (view->xpos_layout, buf, -1);

      pango_layout_get_pixel_size (view->xpos_layout, &x, &y);

      if (view->xpos < 127)
        x = border;
      else
        x = -(x + border);

      cairo_move_to (cr,
                     border + (gdouble) width * view->xpos / 256.0 + x,
                     border + height - border - y);
      pango_cairo_show_layout (cr, view->xpos_layout);
      cairo_fill (cr);
    }

  if (view->cursor_x >= 0 && view->cursor_x <= 255 &&
      view->cursor_y >= 0 && view->cursor_y <= 255)
    {
      gchar buf[32];
      gint  x, y;
      gint  w, h;

      if (! view->cursor_layout)
        {
          view->cursor_layout = gtk_widget_create_pango_layout (widget,
                                                                "x:888 y:888");
          pango_layout_get_pixel_extents (view->cursor_layout,
                                          NULL, &view->cursor_rect);
        }

      x = border * 2 + 3;
      y = border * 2 + 3;
      w = view->cursor_rect.width;
      h = view->cursor_rect.height;

      cairo_push_group (cr);

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
      cairo_rectangle (cr, x, y, w + 1, h + 1);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 6);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      cairo_stroke (cr);

      g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
                  view->cursor_x, 255 - view->cursor_y);
      pango_layout_set_text (view->cursor_layout, buf, -1);

      gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_NORMAL]);

      cairo_move_to (cr, x, y);
      pango_cairo_show_layout (cr, view->cursor_layout);
      cairo_fill (cr);

      cairo_pop_group_to_source (cr);
      cairo_paint_with_alpha (cr, 0.6);
    }

  cairo_destroy (cr);

  return FALSE;
}

static void
set_cursor (GimpCurveView *view,
            GdkCursorType  new_cursor)
{
  if (new_cursor != view->cursor_type)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (view));
      GdkCursor  *cursor  = gdk_cursor_new_for_display (display, new_cursor);

      gdk_window_set_cursor (GTK_WIDGET (view)->window, cursor);
      gdk_cursor_unref (cursor);

      view->cursor_type = new_cursor;
    }
}

static gboolean
gimp_curve_view_button_press (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve = view->curve;
  gint           border;
  gint           width, height;
  gint           x, y;
  gint           closest_point;
  gint           i;

  if (! curve || bevent->button != 1)
    return TRUE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  x = ROUND (((gdouble) (bevent->x - border) / (gdouble) width)  * 255.0);
  y = ROUND (((gdouble) (bevent->y - border) / (gdouble) height) * 255.0);

  x = CLAMP0255 (x);
  y = CLAMP0255 (y);

  closest_point = gimp_curve_get_closest_point (curve, x);

  view->grabbed = TRUE;

  set_cursor (view, GDK_TCROSS);

  switch (curve->curve_type)
    {
    case GIMP_CURVE_SMOOTH:
      /*  determine the leftmost and rightmost points  */
      view->leftmost = -1;
      for (i = closest_point - 1; i >= 0; i--)
        if (curve->points[i][0] != -1)
          {
            view->leftmost = curve->points[i][0];
            break;
          }

      view->rightmost = 256;
      for (i = closest_point + 1; i < GIMP_CURVE_NUM_POINTS; i++)
        if (curve->points[i][0] != -1)
          {
            view->rightmost = curve->points[i][0];
            break;
          }

      gimp_curve_view_set_selected (view, closest_point);

      gimp_curve_set_point (curve, view->selected, x, 255 - y);
      break;

    case GIMP_CURVE_FREE:
      gimp_curve_view_set_selected (view, x);
      view->last = y;

      gimp_curve_set_curve (curve, x, 255 - y);
      break;
    }

  if (! GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  return TRUE;
}

static gboolean
gimp_curve_view_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  if (bevent->button != 1)
    return TRUE;

  view->grabbed = FALSE;

  set_cursor (view, GDK_FLEUR);

  return TRUE;
}

static gboolean
gimp_curve_view_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  GimpCurveView  *view       = GIMP_CURVE_VIEW (widget);
  GimpCurve      *curve      = view->curve;
  GimpCursorType  new_cursor = GDK_X_CURSOR;
  gint            border;
  gint            width, height;
  gint            x, y;
  gint            closest_point;
  gint            i;

  if (! curve)
    return TRUE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  x = ROUND (((gdouble) (mevent->x - border) / (gdouble) width)  * 255.0);
  y = ROUND (((gdouble) (mevent->y - border) / (gdouble) height) * 255.0);

  x = CLAMP0255 (x);
  y = CLAMP0255 (y);

  closest_point = gimp_curve_get_closest_point (curve, x);

  switch (curve->curve_type)
    {
    case GIMP_CURVE_SMOOTH:
      if (! view->grabbed) /*  If no point is grabbed...  */
        {
          if (curve->points[closest_point][0] != -1)
            new_cursor = GDK_FLEUR;
          else
            new_cursor = GDK_TCROSS;
        }
      else /*  Else, drag the grabbed point  */
        {
          new_cursor = GDK_TCROSS;

          gimp_data_freeze (GIMP_DATA (curve));

          gimp_curve_set_point (curve, view->selected, -1, -1);

          if (x > view->leftmost && x < view->rightmost)
            {
              closest_point = (x + 8) / 16;
              if (curve->points[closest_point][0] == -1)
                gimp_curve_view_set_selected (view, closest_point);

              gimp_curve_set_point (curve, view->selected, x, 255 - y);
            }

          gimp_data_thaw (GIMP_DATA (curve));
        }
      break;

    case GIMP_CURVE_FREE:
      if (view->grabbed)
        {
          gint x1, x2, y1, y2;

          if (view->selected > x)
            {
              x1 = x;
              x2 = view->selected;
              y1 = y;
              y2 = view->last;
            }
          else
            {
              x1 = view->selected;
              x2 = x;
              y1 = view->last;
              y2 = y;
            }

          if (x2 != x1)
            {
              gimp_data_freeze (GIMP_DATA (curve));

              for (i = x1; i <= x2; i++)
                gimp_curve_set_curve (curve, i,
                                      255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1)));

              gimp_data_thaw (GIMP_DATA (curve));
            }
          else
            {
              gimp_curve_set_curve (curve, x, 255 - y);
            }

          gimp_curve_view_set_selected (view, x);
          view->last = y;
        }

      if (mevent->state & GDK_BUTTON1_MASK)
        new_cursor = GDK_TCROSS;
      else
        new_cursor = GDK_PENCIL;

      break;
    }

  set_cursor (view, new_cursor);

  gimp_curve_view_set_cursor (view, x, y);

  return TRUE;
}

static gboolean
gimp_curve_view_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *cevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  gimp_curve_view_set_cursor (view, -1, -1);

  return TRUE;
}

static gboolean
gimp_curve_view_key_press (GtkWidget   *widget,
                           GdkEventKey *kevent)
{
  GimpCurveView *view   = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve  = view->curve;
  gint           i      = view->selected;
  gint           y      = curve->points[i][1];
  gboolean       retval = FALSE;

  if (view->grabbed || ! curve || curve->curve_type == GIMP_CURVE_FREE)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_Left:
      for (i = i - 1; i >= 0 && ! retval; i--)
        {
          if (curve->points[i][0] != -1)
            {
              gimp_curve_view_set_selected (view, i);

              retval = TRUE;
            }
        }
      break;

    case GDK_Right:
      for (i = i + 1; i < GIMP_CURVE_NUM_POINTS && ! retval; i++)
        {
          if (curve->points[i][0] != -1)
            {
              gimp_curve_view_set_selected (view, i);

              retval = TRUE;
            }
        }
      break;

    case GDK_Up:
      if (y < 255)
        {
          y = y + (kevent->state & GDK_SHIFT_MASK ? 16 : 1);

          gimp_curve_move_point (curve, i, CLAMP0255 (y));

          retval = TRUE;
        }
      break;

    case GDK_Down:
      if (y > 0)
        {
          y = y - (kevent->state & GDK_SHIFT_MASK ? 16 : 1);

          gimp_curve_move_point (curve, i, CLAMP0255 (y));

          retval = TRUE;
        }
      break;

    default:
      break;
    }

  if (retval)
    set_cursor (view, GDK_TCROSS);

  return retval;
}

GtkWidget *
gimp_curve_view_new (void)
{
  GtkWidget *view = g_object_new (GIMP_TYPE_CURVE_VIEW, NULL);

  gtk_widget_add_events (view,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK);

  return view;
}

static void
gimp_curve_view_curve_dirty (GimpCurve     *curve,
                             GimpCurveView *view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_curve (GimpCurveView *view,
                           GimpCurve     *curve)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));
  g_return_if_fail (curve == NULL || GIMP_IS_CURVE (curve));

  if (view->curve == curve)
    return;

  if (view->curve)
    {
      g_signal_handlers_disconnect_by_func (view->curve,
                                            gimp_curve_view_curve_dirty,
                                            view);
      g_object_unref (view->curve);
    }

  view->curve = curve;

  if (view->curve)
    {
      g_object_ref (view->curve);
      g_signal_connect (view->curve, "dirty",
                        G_CALLBACK (gimp_curve_view_curve_dirty),
                        view);
    }
}

GimpCurve *
gimp_curve_view_get_curve (GimpCurveView *view)
{
  g_return_val_if_fail (GIMP_IS_CURVE_VIEW (view), NULL);

  return view->curve;
}

void
gimp_curve_view_set_selected (GimpCurveView *view,
                              gint           selected)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->selected = selected;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_xpos (GimpCurveView *view,
                          gint           x)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->xpos = x;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_cursor (GimpCurveView *view,
                            gint           x,
                            gint           y)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->cursor_x = x;
  view->cursor_y = y;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}
