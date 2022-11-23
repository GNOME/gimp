/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "core/ligmahistogram.h"
#include "core/ligmamarshal.h"

#include "ligmahistogramview.h"
#include "ligmawidgets-utils.h"


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


static void     ligma_histogram_view_dispose              (GObject        *object);
static void     ligma_histogram_view_set_property         (GObject        *object,
                                                          guint           property_id,
                                                          const GValue   *value,
                                                          GParamSpec     *pspec);
static void     ligma_histogram_view_get_property         (GObject        *object,
                                                          guint           property_id,
                                                          GValue         *value,
                                                          GParamSpec     *pspec);

static void     ligma_histogram_view_get_preferred_width  (GtkWidget      *widget,
                                                          gint           *minimum_width,
                                                          gint           *natural_width);
static void     ligma_histogram_view_get_preferred_height (GtkWidget      *widget,
                                                          gint           *minimum_height,
                                                          gint           *natural_height);
static gboolean ligma_histogram_view_draw                 (GtkWidget      *widget,
                                                          cairo_t        *cr);
static gboolean ligma_histogram_view_button_press         (GtkWidget      *widget,
                                                          GdkEventButton *bevent);
static gboolean ligma_histogram_view_button_release       (GtkWidget      *widget,
                                                          GdkEventButton *bevent);
static gboolean ligma_histogram_view_motion_notify        (GtkWidget      *widget,
                                                          GdkEventMotion *bevent);

static void     ligma_histogram_view_notify      (LigmaHistogram        *histogram,
                                                 const GParamSpec     *pspec,
                                                 LigmaHistogramView    *view);
static void     ligma_histogram_view_update_bins (LigmaHistogramView    *view);

static void     ligma_histogram_view_draw_spike  (LigmaHistogramView    *view,
                                                 LigmaHistogramChannel  channel,
                                                 cairo_t              *cr,
                                                 const GdkRGBA        *fg_color,
                                                 cairo_operator_t      fg_operator,
                                                 const GdkRGBA        *bg_color,
                                                 gint                  x,
                                                 gint                  i,
                                                 gint                  j,
                                                 gdouble               max,
                                                 gdouble               bg_max,
                                                 gint                  height,
                                                 gint                  border);


G_DEFINE_TYPE (LigmaHistogramView, ligma_histogram_view,
               GTK_TYPE_DRAWING_AREA)

#define parent_class ligma_histogram_view_parent_class

static guint histogram_view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_histogram_view_class_init (LigmaHistogramViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  histogram_view_signals[RANGE_CHANGED] =
    g_signal_new ("range-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaHistogramViewClass, range_changed),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->dispose              = ligma_histogram_view_dispose;
  object_class->get_property         = ligma_histogram_view_get_property;
  object_class->set_property         = ligma_histogram_view_set_property;

  widget_class->get_preferred_width  = ligma_histogram_view_get_preferred_width;
  widget_class->get_preferred_height = ligma_histogram_view_get_preferred_height;
  widget_class->draw                 = ligma_histogram_view_draw;
  widget_class->button_press_event   = ligma_histogram_view_button_press;
  widget_class->button_release_event = ligma_histogram_view_button_release;
  widget_class->motion_notify_event  = ligma_histogram_view_motion_notify;

  klass->range_changed               = NULL;

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("histogram-channel",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_HISTOGRAM_CHANNEL,
                                                      LIGMA_HISTOGRAM_VALUE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SCALE,
                                   g_param_spec_enum ("histogram-scale",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_HISTOGRAM_SCALE,
                                                      LIGMA_HISTOGRAM_SCALE_LINEAR,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BORDER_WIDTH,
                                   g_param_spec_int ("border-width", NULL, NULL,
                                                     0, 32, 1,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUBDIVISIONS,
                                   g_param_spec_int ("subdivisions",
                                                     NULL, NULL,
                                                     1, 64, 5,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
ligma_histogram_view_init (LigmaHistogramView *view)
{
  view->histogram    = NULL;
  view->bg_histogram = NULL;
  view->n_bins       = 256;
  view->start        = 0;
  view->end          = 255;
}

static void
ligma_histogram_view_dispose (GObject *object)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (object);

  ligma_histogram_view_set_histogram (view, NULL);
  ligma_histogram_view_set_background (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_histogram_view_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (object);

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
ligma_histogram_view_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (object);

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
ligma_histogram_view_get_preferred_width (GtkWidget *widget,
                                         gint      *minimum_width,
                                         gint      *natural_width)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (widget);

  *minimum_width = *natural_width = MIN_WIDTH + 2 * view->border_width;
}

static void
ligma_histogram_view_get_preferred_height (GtkWidget *widget,
                                          gint      *minimum_height,
                                          gint      *natural_height)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (widget);

  *minimum_height = *natural_height = MIN_HEIGHT + 2 * view->border_width;
}

static gdouble
ligma_histogram_view_get_maximum (LigmaHistogramView    *view,
                                 LigmaHistogram        *histogram,
                                 LigmaHistogramChannel  channel)
{
  gdouble max = ligma_histogram_get_maximum (histogram, channel);

  switch (view->scale)
    {
    case LIGMA_HISTOGRAM_SCALE_LINEAR:
      break;

    case LIGMA_HISTOGRAM_SCALE_LOGARITHMIC:
      if (max > 0.0)
        max = log (max);
      else
        max = 1.0;
      break;
    }

  return max;
}

static gboolean
ligma_histogram_view_draw (GtkWidget *widget,
                          cairo_t   *cr)
{
  LigmaHistogramView *view  = LIGMA_HISTOGRAM_VIEW (widget);
  GtkStyleContext   *style = gtk_widget_get_style_context (widget);
  GtkAllocation      allocation;
  gint               x;
  gint               x1, x2;
  gint               border;
  gint               width, height;
  gdouble            max    = 0.0;
  gdouble            bg_max = 0.0;
  gint               xstop;
  GdkRGBA            grid_color;
  GdkRGBA            color_in;
  GdkRGBA            color_out;
  GdkRGBA            bg_color_in;
  GdkRGBA            bg_color_out;
  GdkRGBA            rgb_color[3];

  gtk_widget_get_allocation (widget, &allocation);

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, "view");

  gtk_render_background (style, cr, 0, 0,
                         allocation.width, allocation.height);

  border = view->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_translate (cr, 0.5, 0.5);

  /*  Draw the outer border  */
  gtk_style_context_add_class (style, "grid");
  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &grid_color);
  gtk_style_context_remove_class (style, "grid");

  gdk_cairo_set_source_rgba (cr, &grid_color);
  cairo_rectangle (cr, border, border, width - 1, height - 1);
  cairo_stroke (cr);

  if (! view->histogram && ! view->bg_histogram)
    {
      gtk_style_context_restore (style);
      return FALSE;
    }

  x1 = CLAMP (MIN (view->start, view->end), 0, view->n_bins - 1);
  x2 = CLAMP (MAX (view->start, view->end), 0, view->n_bins - 1);

  if (view->histogram)
    max = ligma_histogram_view_get_maximum (view, view->histogram,
                                           view->channel);

  if (view->bg_histogram)
    bg_max = ligma_histogram_view_get_maximum (view, view->bg_histogram,
                                              view->channel);

  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &color_out);
  bg_color_out       = color_out;
  bg_color_out.alpha = 0.5;

  gtk_style_context_save (style);
  gtk_style_context_set_state (style, GTK_STATE_FLAG_SELECTED);
  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &color_in);
  bg_color_in       = color_in;
  bg_color_in.alpha = 0.5;
  gtk_style_context_restore (style);

  if (view->channel == LIGMA_HISTOGRAM_RGB)
    {
      for (x = 0; x < 3; x++)
        {
          rgb_color[x].red   = (x == 0 ? 1.0 : 0.0);
          rgb_color[x].green = (x == 1 ? 1.0 : 0.0);
          rgb_color[x].blue  = (x == 2 ? 1.0 : 0.0);
          rgb_color[x].alpha = 1.0;
        }
    }

  xstop = 1;
  for (x = 0; x < width; x++)
    {
      gboolean  in_selection = FALSE;

      gint  i = (x * view->n_bins) / width;
      gint  j = ((x + 1) * view->n_bins) / width;

      if (! (x1 == 0 && x2 == (view->n_bins - 1)))
        {
          gint k = i;

          do
            in_selection |= (x1 <= k && k <= x2);
          while (++k < j);
        }

      if (view->subdivisions > 1 && x >= (xstop * width / view->subdivisions))
        {
          gdk_cairo_set_source_rgba (cr, &grid_color);

          cairo_move_to (cr, x + border, border);
          cairo_line_to (cr, x + border, border + height - 1);
          cairo_stroke (cr);

          xstop++;
        }
      else if (in_selection)
        {
          gtk_style_context_save (style);
          gtk_style_context_set_state (style, GTK_STATE_FLAG_SELECTED);

          gtk_render_background (style, cr,
                                 x + border, border,
                                 1,          height - 1);

          gtk_style_context_restore (style);
        }

      if (view->channel == LIGMA_HISTOGRAM_RGB)
        {
          GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
          gint    c;

          for (c = 0; c < 3; c++)
            ligma_histogram_view_draw_spike (view, LIGMA_HISTOGRAM_RED + c, cr,
                                            &black,
                                            CAIRO_OPERATOR_OVER,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          for (c = 0; c < 3; c++)
            ligma_histogram_view_draw_spike (view, LIGMA_HISTOGRAM_RED + c, cr,
                                            &rgb_color[c],
                                            CAIRO_OPERATOR_ADD,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          ligma_histogram_view_draw_spike (view, view->channel, cr,
                                          in_selection ? &color_in : &color_out,
                                          CAIRO_OPERATOR_OVER,
                                          NULL,
                                          x, i, j, max, bg_max, height, border);
        }
      else
        {
          ligma_histogram_view_draw_spike (view, view->channel, cr,
                                          in_selection ? &color_in : &color_out,
                                          CAIRO_OPERATOR_OVER,
                                          in_selection ? &bg_color_in : &bg_color_out,
                                          x, i, j, max, bg_max, height, border);
        }
    }

  gtk_style_context_restore (style);

  return FALSE;
}

static void
ligma_histogram_view_draw_spike (LigmaHistogramView    *view,
                                LigmaHistogramChannel  channel,
                                cairo_t              *cr,
                                const GdkRGBA        *fg_color,
                                cairo_operator_t      fg_operator,
                                const GdkRGBA        *bg_color,
                                gint                  x,
                                gint                  i,
                                gint                  j,
                                gdouble               max,
                                gdouble               bg_max,
                                gint                  height,
                                gint                  border)
{
  gdouble value    = 0.0;
  gdouble bg_value = 0.0;
  gint    y;
  gint    bg_y;

  if (view->histogram)
    {
      gint ii = i;

      do
        {
          gdouble v = ligma_histogram_get_value (view->histogram,
                                                channel, ii++);

          if (v > value)
            value = v;
        }
      while (ii < j);
    }

  if (bg_color && view->bg_histogram)
    {
      gint ii = i;

      do
        {
          gdouble v = ligma_histogram_get_value (view->bg_histogram,
                                                channel, ii++);

          if (v > bg_value)
            bg_value = v;
        }
      while (ii < j);
    }

  if (value <= 0.0 && bg_value <= 0.0)
    return;

  switch (view->scale)
    {
    case LIGMA_HISTOGRAM_SCALE_LINEAR:
      y    = (gint) (((height - 2) * value)    / max);
      bg_y = (gint) (((height - 2) * bg_value) / bg_max);
      break;

    case LIGMA_HISTOGRAM_SCALE_LOGARITHMIC:
      y    = (gint) (((height - 2) * log (value))    / max);
      bg_y = (gint) (((height - 2) * log (bg_value)) / bg_max);
      break;

    default:
      y    = 0;
      bg_y = 0;
      break;
    }

  if (bg_color)
    {
      gdk_cairo_set_source_rgba (cr, bg_color);

      cairo_move_to (cr, x + border, height + border - 1);
      cairo_line_to (cr, x + border, height + border - bg_y - 1);

      cairo_stroke (cr);
    }

  cairo_set_operator (cr, fg_operator);

  gdk_cairo_set_source_rgba (cr, fg_color);

  cairo_move_to (cr, x + border, height + border - 1);
  cairo_line_to (cr, x + border, height + border - y - 1);

  cairo_stroke (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
}

static gboolean
ligma_histogram_view_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (widget);

  if (! view->grab_seat &&
      bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      GdkSeat       *seat = gdk_event_get_seat ((GdkEvent *) bevent);
      GtkAllocation  allocation;
      gint           width;

      if (gdk_seat_grab (seat, gtk_widget_get_window (widget),
                         GDK_SEAT_CAPABILITY_ALL, FALSE,
                         NULL, (GdkEvent *) bevent,
                         NULL, NULL) != GDK_GRAB_SUCCESS)
        {
          return TRUE;
        }

      view->grab_seat = seat;

      gtk_widget_get_allocation (widget, &allocation);

      width = allocation.width - 2 * view->border_width;

      view->start = CLAMP (((bevent->x - view->border_width) * view->n_bins) / width,
                           0, view->n_bins - 1);
      view->end   = view->start;

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}

static gboolean
ligma_histogram_view_button_release (GtkWidget      *widget,
                                    GdkEventButton *bevent)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (widget);

  if (gdk_event_get_seat ((GdkEvent *) bevent) == view->grab_seat &&
      bevent->button == 1)
    {
      gint start, end;

      gdk_seat_ungrab (view->grab_seat);
      view->grab_seat = NULL;

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
ligma_histogram_view_motion_notify (GtkWidget      *widget,
                                   GdkEventMotion *mevent)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_VIEW (widget);
  GtkAllocation      allocation;
  gint               width;

  if (gdk_event_get_seat ((GdkEvent *) mevent) == view->grab_seat)
    {
      gtk_widget_get_allocation (widget, &allocation);

      width = allocation.width - 2 * view->border_width;

      view->start = CLAMP (((mevent->x - view->border_width) * view->n_bins) / width,
                           0, view->n_bins - 1);

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}


/*  public functions  */

GtkWidget *
ligma_histogram_view_new (gboolean range)
{
  GtkWidget *view = g_object_new (LIGMA_TYPE_HISTOGRAM_VIEW, NULL);

  if (range)
    gtk_widget_add_events (view,
                           GDK_BUTTON_PRESS_MASK   |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_BUTTON1_MOTION_MASK);

  return view;
}

void
ligma_histogram_view_set_histogram (LigmaHistogramView *view,
                                   LigmaHistogram     *histogram)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->bg_histogram == NULL ||
                    ligma_histogram_n_components (view->bg_histogram) ==
                    ligma_histogram_n_components (histogram));
#endif

  if (view->histogram != histogram)
    {
      if (view->histogram)
        {
          g_signal_handlers_disconnect_by_func (view->histogram,
                                                ligma_histogram_view_notify,
                                                view);
          g_object_unref (view->histogram);
        }

      view->histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          g_signal_connect (histogram, "notify",
                            G_CALLBACK (ligma_histogram_view_notify),
                            view);

          if (! ligma_histogram_has_channel (histogram, view->channel))
            ligma_histogram_view_set_channel (view, LIGMA_HISTOGRAM_VALUE);
        }

      ligma_histogram_view_update_bins (view);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

LigmaHistogram *
ligma_histogram_view_get_histogram (LigmaHistogramView *view)
{
  g_return_val_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view), NULL);

  return view->histogram;
}

void
ligma_histogram_view_set_background (LigmaHistogramView *view,
                                    LigmaHistogram     *histogram)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->histogram == NULL ||
                    ligma_histogram_n_components (view->histogram) ==
                    ligma_histogram_n_components (histogram));
#endif

  if (view->bg_histogram != histogram)
    {
      if (view->bg_histogram)
        {
          g_signal_handlers_disconnect_by_func (view->bg_histogram,
                                                ligma_histogram_view_notify,
                                                view);
          g_object_unref (view->bg_histogram);
        }

      view->bg_histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          g_signal_connect (histogram, "notify",
                            G_CALLBACK (ligma_histogram_view_notify),
                            view);

          if (! ligma_histogram_has_channel (histogram, view->channel))
            ligma_histogram_view_set_channel (view, LIGMA_HISTOGRAM_VALUE);
        }

      ligma_histogram_view_update_bins (view);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

LigmaHistogram *
ligma_histogram_view_get_background (LigmaHistogramView *view)
{
  g_return_val_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view), NULL);

  return view->bg_histogram;
}

void
ligma_histogram_view_set_channel (LigmaHistogramView    *view,
                                 LigmaHistogramChannel  channel)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));

  if (channel != view->channel)
    g_object_set (view, "histogram-channel", channel, NULL);
}

LigmaHistogramChannel
ligma_histogram_view_get_channel (LigmaHistogramView *view)
{
  g_return_val_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view), 0);

  return view->channel;
}

void
ligma_histogram_view_set_scale (LigmaHistogramView  *view,
                               LigmaHistogramScale  scale)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));

  if (scale != view->scale)
    g_object_set (view, "histogram-scale", scale, NULL);
}

LigmaHistogramScale
ligma_histogram_view_get_scale (LigmaHistogramView *view)
{
  g_return_val_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view), 0);

  return view->scale;
}

void
ligma_histogram_view_set_range (LigmaHistogramView *view,
                               gint               start,
                               gint               end)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));

  if (view->start != MIN (start, end) ||
      view->end   != MAX (start, end))
    {
      view->start = MIN (start, end);
      view->end   = MAX (start, end);

      gtk_widget_queue_draw (GTK_WIDGET (view));

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }
}

void
ligma_histogram_view_get_range (LigmaHistogramView *view,
                               gint              *start,
                               gint              *end)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_VIEW (view));

  if (start) *start = view->start;
  if (end)   *end   = view->end;
}


/*  private functions  */

static void
ligma_histogram_view_notify (LigmaHistogram     *histogram,
                            const GParamSpec  *pspec,
                            LigmaHistogramView *view)
{
  if (! strcmp (pspec->name, "n-bins"))
    {
      ligma_histogram_view_update_bins (view);
    }
  else
    {
      gtk_widget_queue_draw (GTK_WIDGET (view));
    }
}

static void
ligma_histogram_view_update_bins (LigmaHistogramView *view)
{
  gint new_bins = 0;

  if (view->histogram)
    new_bins = ligma_histogram_n_bins (view->histogram);
  else if (view->bg_histogram)
    new_bins = ligma_histogram_n_bins (view->bg_histogram);

  if (new_bins > 0 && new_bins != view->n_bins)
    {
      view->start = MIN (ROUND ((gdouble) view->start *
                                new_bins / view->n_bins),
                         new_bins - 1);
      view->end   = MAX (ROUND ((gdouble) (view->end + 1) *
                                new_bins / view->n_bins) - 1,
                         0);

      view->n_bins = new_bins;

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }
}
