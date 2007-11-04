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

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpcurve.h"
#include "core/gimpmarshal.h"

#include "gimpcurveview.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_SCALE,
  PROP_BORDER_WIDTH,
  PROP_SUBDIVISIONS
};


static void       gimp_curve_view_finalize       (GObject        *object);
static void       gimp_curve_view_dispose        (GObject        *object);
static void       gimp_curve_view_set_property   (GObject        *object,
                                                  guint           property_id,
                                                  const GValue   *value,
                                                  GParamSpec     *pspec);
static void       gimp_curve_view_get_property   (GObject        *object,
                                                  guint           property_id,
                                                  GValue         *value,
                                                  GParamSpec     *pspec);

static gboolean   gimp_curve_view_expose         (GtkWidget      *widget,
                                                  GdkEventExpose *event);
static gboolean   gimp_curve_view_button_press   (GtkWidget      *widget,
                                                  GdkEventButton *bevent);
static gboolean   gimp_curve_view_button_release (GtkWidget      *widget,
                                                  GdkEventButton *bevent);
static gboolean   gimp_curve_view_motion_notify  (GtkWidget      *widget,
                                                  GdkEventMotion *bevent);


G_DEFINE_TYPE (GimpCurveView, gimp_curve_view,
               GIMP_TYPE_HISTOGRAM_VIEW)

#define parent_class gimp_curve_view_parent_class


static void
gimp_curve_view_class_init (GimpCurveViewClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass         *widget_class = GTK_WIDGET_CLASS (klass);
  GimpHistogramViewClass *hist_class   = GIMP_HISTOGRAM_VIEW_CLASS (klass);

  object_class->finalize     = gimp_curve_view_finalize;
  object_class->dispose      = gimp_curve_view_dispose;
  object_class->get_property = gimp_curve_view_get_property;
  object_class->set_property = gimp_curve_view_set_property;

  widget_class->expose_event         = gimp_curve_view_expose;
  widget_class->button_press_event   = gimp_curve_view_button_press;
  widget_class->button_release_event = gimp_curve_view_button_release;
  widget_class->motion_notify_event  = gimp_curve_view_motion_notify;

  hist_class->light_histogram = TRUE;

#if 0
  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("curve-channel",
                                                      NULL, NULL,
                                                      GIMP_TYPE_CURVE_CHANNEL,
                                                      GIMP_CURVE_VALUE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SCALE,
                                   g_param_spec_enum ("curve-scale",
                                                      NULL, NULL,
                                                      GIMP_TYPE_CURVE_SCALE,
                                                      GIMP_CURVE_SCALE_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BORDER_WIDTH,
                                   g_param_spec_int ("border-width", NULL, NULL,
                                                     0, 32, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUBDIVISIONS,
                                   g_param_spec_int ("subdivisions",
                                                     NULL, NULL,
                                                     1, 64, 5,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
#endif
}

static void
gimp_curve_view_init (GimpCurveView *view)
{
  view->curve     = NULL;
  view->selected  = 0;
  view->xpos      = -1;
  view->cursor_x  = -1;
  view->cursor_y  = -1;

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
gimp_curve_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_curve_view_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);
  gint            x, y, i;
  gint            border;
  gint            width, height;
  GdkPoint        points[256];
  GdkGC          *graph_gc;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  if (! view->curve)
    return FALSE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  /*  Draw the grid lines  */
  for (i = 1; i < 4; i++)
    {
      gdk_draw_line (widget->window,
                     widget->style->dark_gc[GTK_STATE_NORMAL],
                     border,
                     border + i * (height / 4),
                     border + width - 1,
                     border + i * (height / 4));
      gdk_draw_line (widget->window,
                     widget->style->dark_gc[GTK_STATE_NORMAL],
                     border + i * (width / 4),
                     border,
                     border + i * (width / 4),
                     border + height - 1);
    }

  /*  Draw the curve  */
  graph_gc = widget->style->text_gc[GTK_STATE_NORMAL];

  for (i = 0; i < 256; i++)
    {
      x = i;
      y = 255 - view->curve->curve[x];

      points[i].x = border + ROUND ((gdouble) width  * x / 256.0);
      points[i].y = border + ROUND ((gdouble) height * y / 256.0);
    }

  gdk_draw_lines (widget->window, graph_gc, points, 256);

  if (view->curve->curve_type == GIMP_CURVE_SMOOTH)
    {
      /*  Draw the points  */
      for (i = 0; i < GIMP_CURVE_NUM_POINTS; i++)
        {
          x = view->curve->points[i][0];
          if (x < 0)
            continue;

          y = 255 - view->curve->points[i][1];

          gdk_draw_arc (widget->window, graph_gc,
                        TRUE,
                        ROUND ((gdouble) width  * x / 256.0),
                        ROUND ((gdouble) height * y / 256.0),
                        border * 2, border * 2, 0, 23040);

          if (i == view->selected)
            gdk_draw_arc (widget->window,
                          widget->style->base_gc[GTK_STATE_NORMAL],
                          TRUE,
                          ROUND ((gdouble) width  * x / 256.0) + 2,
                          ROUND ((gdouble) height * y / 256.0) + 2,
                          (border - 2) * 2, (border - 2) * 2, 0, 23040);
        }
    }

  if (view->xpos >= 0)
    {
      gchar buf[32];

      /* draw the color line */
      gdk_draw_line (widget->window,
                     widget->style->text_gc[GTK_STATE_NORMAL],
                     border +
                     ROUND ((gdouble) width * view->xpos / 256.0),
                     border,
                     border +
                     ROUND ((gdouble) width * view->xpos / 256.0),
                     height + border - 1);

      /* and xpos indicator */
      g_snprintf (buf, sizeof (buf), "x:%d", view->xpos);

      if (! view->xpos_layout)
        view->xpos_layout = gtk_widget_create_pango_layout (widget, buf);
      else
        pango_layout_set_text (view->xpos_layout, buf, -1);

      pango_layout_get_pixel_size (view->xpos_layout, &x, &y);

      if ((view->xpos + border) < 127)
        x = border + 4;
      else
        x = -(x + 2);

      gdk_draw_layout (widget->window,
                       widget->style->text_gc[GTK_STATE_NORMAL],
                       border +
                       ROUND ((gdouble) width * view->xpos / 256.0 + x),
                       height - y - 2,
                       view->xpos_layout);
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

      x = border * 2 + 2;
      y = border * 2 + 2;
      w = view->cursor_rect.width  + 4;
      h = view->cursor_rect.height + 4;

      gdk_draw_rectangle (widget->window,
                          widget->style->base_gc[GTK_STATE_ACTIVE],
                          TRUE,
                          x, y, w + 1, h + 1);
      gdk_draw_rectangle (widget->window,
                          widget->style->text_gc[GTK_STATE_NORMAL],
                          FALSE,
                          x, y, w, h);

      g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
                  view->cursor_x, 255 - view->cursor_y);
      pango_layout_set_text (view->cursor_layout, buf, 11);

      gdk_draw_layout (widget->window,
                       widget->style->text_gc[GTK_STATE_ACTIVE],
                       x + 2, y + 2,
                       view->cursor_layout);
    }

  return FALSE;
}

static gboolean
gimp_curve_view_button_press (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);
}

static gboolean
gimp_curve_view_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, bevent);
}

static gboolean
gimp_curve_view_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  return GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, mevent);
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
gimp_curve_view_curve_notify (GimpCurve        *curve,
                              const GParamSpec *pspec,
                              GimpCurveView    *view)
{
  if (! strcmp (pspec->name, "curve-type") ||
      ! strcmp (pspec->name, "points")     ||
      ! strcmp (pspec->name, "curve"))
    {
      gtk_widget_queue_draw (GTK_WIDGET (view));
    }
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
                                            gimp_curve_view_curve_notify,
                                            view);
      g_object_unref (view->curve);
    }

  view->curve = curve;

  if (view->curve)
    {
      g_object_ref (view->curve);
      g_signal_connect (view->curve, "notify",
                        G_CALLBACK (gimp_curve_view_curve_notify),
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
gimp_curve_view_set_cusor (GimpCurveView *view,
                           gint           x,
                           gint           y)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->cursor_x = x;
  view->cursor_y = y;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}
