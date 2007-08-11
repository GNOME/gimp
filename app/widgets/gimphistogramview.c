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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"

#include "core/gimpmarshal.h"

#include "gimphistogramview.h"


#define MIN_WIDTH  64
#define MIN_HEIGHT 64

enum
{
  RANGE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_SCALE,
  PROP_BORDER_WIDTH,
  PROP_SUBDIVISIONS
};


static void  gimp_histogram_view_set_property      (GObject        *object,
                                                    guint           property_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void  gimp_histogram_view_get_property      (GObject        *object,
                                                    guint           property_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);
static void  gimp_histogram_view_size_request      (GtkWidget      *widget,
                                                    GtkRequisition *requisition);
static gboolean gimp_histogram_view_expose         (GtkWidget      *widget,
                                                    GdkEventExpose *event);
static gboolean gimp_histogram_view_button_press   (GtkWidget      *widget,
                                                    GdkEventButton *bevent);
static gboolean gimp_histogram_view_button_release (GtkWidget      *widget,
                                                    GdkEventButton *bevent);
static gboolean gimp_histogram_view_motion_notify  (GtkWidget      *widget,
                                                    GdkEventMotion *bevent);

static void     gimp_histogram_view_draw_spike (GimpHistogramView    *view,
                                                GimpHistogramChannel  channel,
                                                GdkGC                *gc,
                                                gint                  x,
                                                gint                  i,
                                                gint                  j,
                                                gdouble               max,
                                                gint                  height,
                                                gint                  border);


G_DEFINE_TYPE (GimpHistogramView, gimp_histogram_view,
               GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_histogram_view_parent_class

static guint histogram_view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_histogram_view_class_init (GimpHistogramViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  histogram_view_signals[RANGE_CHANGED] =
    g_signal_new ("range-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpHistogramViewClass, range_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->get_property = gimp_histogram_view_get_property;
  object_class->set_property = gimp_histogram_view_set_property;

  widget_class->size_request         = gimp_histogram_view_size_request;
  widget_class->expose_event         = gimp_histogram_view_expose;
  widget_class->button_press_event   = gimp_histogram_view_button_press;
  widget_class->button_release_event = gimp_histogram_view_button_release;
  widget_class->motion_notify_event  = gimp_histogram_view_motion_notify;

  klass->range_changed = NULL;

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("histogram-channel",
                                                      NULL, NULL,
                                                      GIMP_TYPE_HISTOGRAM_CHANNEL,
                                                      GIMP_HISTOGRAM_VALUE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SCALE,
                                   g_param_spec_enum ("histogram-scale",
                                                      NULL, NULL,
                                                      GIMP_TYPE_HISTOGRAM_SCALE,
                                                      GIMP_HISTOGRAM_SCALE_LINEAR,
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
}

static void
gimp_histogram_view_init (GimpHistogramView *view)
{
  view->histogram = NULL;
  view->start     = 0;
  view->end       = 255;
}

static void
gimp_histogram_view_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      view->channel = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;
    case PROP_SCALE:
      view->scale = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;
    case PROP_BORDER_WIDTH:
      view->border_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (view));
      break;
    case PROP_SUBDIVISIONS:
      view->subdivisions = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_view_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, view->channel);
      break;
    case PROP_SCALE:
      g_value_set_enum (value, view->scale);
      break;
    case PROP_BORDER_WIDTH:
      g_value_set_int (value, view->border_width);
      break;
    case PROP_SUBDIVISIONS:
      g_value_set_int (value, view->subdivisions);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_view_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  requisition->width  = MIN_WIDTH  + 2 * view->border_width;
  requisition->height = MIN_HEIGHT + 2 * view->border_width;
}

static gdouble
gimp_histogram_view_get_maximum (GimpHistogramView    *view,
                                 GimpHistogramChannel  channel)
{
  gdouble max = gimp_histogram_get_maximum (view->histogram, channel);

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
      if (max > 0.0)
        max = log (max);
      else
        max = 1.0;
      break;
    }

  return max;
}

static gboolean
gimp_histogram_view_expose (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);
  gint               x;
  gint               x1, x2;
  gint               border;
  gint               width, height;
  gdouble            max;
  gint               xstop;
  GdkGC             *gc_in;
  GdkGC             *gc_out;
  GdkGC             *rgb_gc[3]  = { NULL, NULL, NULL };

  if (! view->histogram)
    return FALSE;

  border = view->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  x1 = CLAMP (MIN (view->start, view->end), 0, 255);
  x2 = CLAMP (MAX (view->start, view->end), 0, 255);

  gdk_draw_rectangle (widget->window,
                      widget->style->base_gc[GTK_STATE_NORMAL], TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  /*  Draw the outer border  */
  gdk_draw_rectangle (widget->window,
                      widget->style->dark_gc[GTK_STATE_NORMAL], FALSE,
                      border, border,
                      width - 1, height - 1);

  max = gimp_histogram_view_get_maximum (view, view->channel);

  gc_in  = (view->light_histogram ?
            widget->style->mid_gc[GTK_STATE_SELECTED] :
            widget->style->text_gc[GTK_STATE_SELECTED]);

  gc_out = (view->light_histogram ?
            widget->style->mid_gc[GTK_STATE_NORMAL] :
            widget->style->text_gc[GTK_STATE_NORMAL]);

  if (view->channel == GIMP_HISTOGRAM_RGB)
    {
      GdkGCValues  values;
      GdkColor     color;

      values.function = GDK_OR;

      for (x = 0; x < 3; x++)
        {
          rgb_gc[x] = gdk_gc_new_with_values (widget->window,
                                              &values, GDK_GC_FUNCTION);

          color.red   = (x == 0 ? 0xFFFF : 0x0);
          color.green = (x == 1 ? 0xFFFF : 0x0);
          color.blue  = (x == 2 ? 0xFFFF : 0x0);

          gdk_gc_set_rgb_fg_color (rgb_gc[x], &color);
        }
    }

  xstop = 1;
  for (x = 0; x < width; x++)
    {
      gboolean  in_selection = FALSE;

      gint  i = (x * 256) / width;
      gint  j = ((x + 1) * 256) / width;

      if (! (x1 == 0 && x2 == 255))
        {
          gint k = i;

          do
            in_selection |= (x1 <= k && k <= x2);
          while (++k < j);
        }

      if (view->subdivisions > 1 && x >= (xstop * width / view->subdivisions))
        {
          gdk_draw_line (widget->window,
                         widget->style->dark_gc[GTK_STATE_NORMAL],
                         x + border, border,
                         x + border, border + height - 1);
          xstop++;
        }
      else if (in_selection)
        {
          gdk_draw_line (widget->window,
                         widget->style->base_gc[GTK_STATE_SELECTED],
                         x + border, border,
                         x + border, border + height - 1);
        }

      if (view->channel == GIMP_HISTOGRAM_RGB)
        {
          gint c;

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c,
                                            widget->style->black_gc,
                                            x, i, j, max, height, border);

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c,
                                            rgb_gc[c],
                                            x, i, j, max, height, border);
        }

      gimp_histogram_view_draw_spike (view, view->channel,
                                      in_selection ? gc_in : gc_out,
                                      x, i, j, max, height, border);
    }

  if (view->channel == GIMP_HISTOGRAM_RGB)
    {
      for (x = 0; x < 3; x++)
        g_object_unref (rgb_gc[x]);
    }

  return FALSE;
}

static void
gimp_histogram_view_draw_spike (GimpHistogramView    *view,
                                GimpHistogramChannel  channel,
                                GdkGC                *gc,
                                gint                  x,
                                gint                  i,
                                gint                  j,
                                gdouble               max,
                                gint                  height,
                                gint                  border)
{
  gdouble  value = 0.0;
  gint     y;

  do
    {
      gdouble v = gimp_histogram_get_value (view->histogram, channel, i++);

      if (v > value)
        value = v;
    }
  while (i < j);

  if (value <= 0.0)
    return;

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      y = (gint) (((height - 2) * value) / max);
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
      y = (gint) (((height - 2) * log (value)) / max);
      break;

    default:
      y = 0;
      break;
    }

  gdk_draw_line (GTK_WIDGET (view)->window, gc,
                 x + border, height + border - 1,
                 x + border, height + border - y - 1);
}

static gboolean
gimp_histogram_view_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      gint width;

      gdk_pointer_grab (widget->window, FALSE,
                        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK,
                        NULL, NULL, bevent->time);

      width = widget->allocation.width - 2 * view->border_width;

      view->start = CLAMP ((((bevent->x - view->border_width) * 256) / width),
                           0, 255);
      view->end   = view->start;

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}

static gboolean
gimp_histogram_view_button_release (GtkWidget      *widget,
                                    GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->button == 1)
    {
      gint start, end;

      gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (view)),
                                  bevent->time);

      start = view->start;
      end   = view->end;

      view->start = MIN (start, end);
      view->end   = MAX (start, end);

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }

  return TRUE;
}

static gboolean
gimp_histogram_view_motion_notify (GtkWidget      *widget,
                                   GdkEventMotion *mevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);
  gint               width;

  width = widget->allocation.width - 2 * view->border_width;

  view->start = CLAMP ((((mevent->x - view->border_width) * 256) / width),
                       0, 255);

  gtk_widget_queue_draw (widget);

  return TRUE;
}

GtkWidget *
gimp_histogram_view_new (gboolean range)
{
  GtkWidget *view = g_object_new (GIMP_TYPE_HISTOGRAM_VIEW, NULL);

  if (range)
    gtk_widget_add_events (view,
                           GDK_BUTTON_PRESS_MASK   |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_BUTTON1_MOTION_MASK);

  return view;
}

void
gimp_histogram_view_set_histogram (GimpHistogramView *view,
                                   GimpHistogram     *histogram)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (view->histogram != histogram)
    {
      view->histogram = histogram;

      if (histogram && view->channel >= gimp_histogram_n_channels (histogram))
        gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

GimpHistogram *
gimp_histogram_view_get_histogram (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->histogram;
}

void
gimp_histogram_view_set_channel (GimpHistogramView    *view,
                                 GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  g_object_set (view, "histogram-channel", channel, NULL);
}

GimpHistogramChannel
gimp_histogram_view_get_channel (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), 0);

  return view->channel;
}

void
gimp_histogram_view_set_scale (GimpHistogramView  *view,
                               GimpHistogramScale  scale)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  g_object_set (view, "histogram-scale", scale, NULL);
}

GimpHistogramScale
gimp_histogram_view_get_scale (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), 0);

  return view->scale;
}

void
gimp_histogram_view_set_range (GimpHistogramView *view,
                               gint               start,
                               gint               end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  view->start = MIN (start, end);
  view->end   = MAX (start, end);

  gtk_widget_queue_draw (GTK_WIDGET (view));

  g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                 view->start, view->end);
}

void
gimp_histogram_view_get_range (GimpHistogramView *view,
                               gint              *start,
                               gint              *end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (start) *start = view->start;
  if (end)   *end   = view->end;
}
