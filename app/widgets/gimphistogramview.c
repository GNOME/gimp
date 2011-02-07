/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimphistogram.h"
#include "core/gimpmarshal.h"

#include "gimphistogramview.h"
#include "gimpwidgets-utils.h"


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


static void     gimp_histogram_view_finalize             (GObject        *object);
static void     gimp_histogram_view_set_property         (GObject        *object,
                                                          guint           property_id,
                                                          const GValue   *value,
                                                          GParamSpec     *pspec);
static void     gimp_histogram_view_get_property         (GObject        *object,
                                                          guint           property_id,
                                                          GValue         *value,
                                                          GParamSpec     *pspec);

static void     gimp_histogram_view_get_preferred_width  (GtkWidget      *widget,
                                                          gint           *minimum_width,
                                                          gint           *natural_width);
static void     gimp_histogram_view_get_preferred_height (GtkWidget      *widget,
                                                          gint           *minimum_height,
                                                          gint           *natural_height);
static gboolean gimp_histogram_view_draw                 (GtkWidget      *widget,
                                                          cairo_t        *cr);
static gboolean gimp_histogram_view_button_press         (GtkWidget      *widget,
                                                          GdkEventButton *bevent);
static gboolean gimp_histogram_view_button_release       (GtkWidget      *widget,
                                                          GdkEventButton *bevent);
static gboolean gimp_histogram_view_motion_notify        (GtkWidget      *widget,
                                                          GdkEventMotion *bevent);

static void     gimp_histogram_view_notify      (GimpHistogram        *histogram,
                                                 const GParamSpec     *pspec,
                                                 GimpHistogramView    *view);
static void     gimp_histogram_view_update_bins (GimpHistogramView    *view);

static void     gimp_histogram_view_draw_spike  (GimpHistogramView    *view,
                                                 GimpHistogramChannel  channel,
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

  object_class->finalize             = gimp_histogram_view_finalize;
  object_class->get_property         = gimp_histogram_view_get_property;
  object_class->set_property         = gimp_histogram_view_set_property;

  widget_class->get_preferred_width  = gimp_histogram_view_get_preferred_width;
  widget_class->get_preferred_height = gimp_histogram_view_get_preferred_height;
  widget_class->draw                 = gimp_histogram_view_draw;
  widget_class->button_press_event   = gimp_histogram_view_button_press;
  widget_class->button_release_event = gimp_histogram_view_button_release;
  widget_class->motion_notify_event  = gimp_histogram_view_motion_notify;

  klass->range_changed               = NULL;

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

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("grid-color",
                                                               NULL, NULL,
                                                               GDK_TYPE_RGBA,
                                                               GIMP_PARAM_READABLE));
}

static void
gimp_histogram_view_init (GimpHistogramView *view)
{
  GtkCssProvider *css;

  view->histogram    = NULL;
  view->bg_histogram = NULL;
  view->n_bins       = 256;
  view->start        = 0;
  view->end          = 255;

  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css,
                                   "GimpHistogramView {\n"
                                   "  -GimpHistogramView-grid-color: darker (@bg_color);\n"
                                   "}\n", -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (view)),
                                  GTK_STYLE_PROVIDER (css),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
}

static void
gimp_histogram_view_finalize (GObject *object)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  if (view->histogram)
    {
      g_object_unref (view->histogram);
      view->histogram = NULL;
    }

  if (view->bg_histogram)
    {
      g_object_unref (view->bg_histogram);
      view->bg_histogram = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
gimp_histogram_view_get_preferred_width (GtkWidget *widget,
                                         gint      *minimum_width,
                                         gint      *natural_width)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  *minimum_width = *natural_width = MIN_WIDTH + 2 * view->border_width;
}

static void
gimp_histogram_view_get_preferred_height (GtkWidget *widget,
                                          gint      *minimum_height,
                                          gint      *natural_height)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  *minimum_height = *natural_height = MIN_HEIGHT + 2 * view->border_width;
}

static gdouble
gimp_histogram_view_get_maximum (GimpHistogramView    *view,
                                 GimpHistogram        *histogram,
                                 GimpHistogramChannel  channel)
{
  gdouble max = gimp_histogram_get_maximum (histogram, channel);

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

static void
gimp_gdk_rgba_shade (const GdkRGBA *color,
                     gdouble        factor,
                     GdkRGBA       *shaded)
{
  GtkSymbolicColor *in;
  GtkSymbolicColor *out;

  in = gtk_symbolic_color_new_literal (color);
  out = gtk_symbolic_color_new_shade (in, factor);

  gtk_symbolic_color_resolve (out, NULL, shaded);

  gtk_symbolic_color_unref (out);
  gtk_symbolic_color_unref (in);
}

static void
gimp_gdk_rgba_mix (const GdkRGBA *color1,
                   const GdkRGBA *color2,
                   gdouble        factor,
                   GdkRGBA       *mixed)
{
  GtkSymbolicColor *in1;
  GtkSymbolicColor *in2;
  GtkSymbolicColor *out;

  in1 = gtk_symbolic_color_new_literal (color1);
  in2 = gtk_symbolic_color_new_literal (color2);
  out = gtk_symbolic_color_new_mix (in1, in2, factor);

  gtk_symbolic_color_resolve (out, NULL, mixed);

  gtk_symbolic_color_unref (out);
  gtk_symbolic_color_unref (in1);
  gtk_symbolic_color_unref (in2);
}

static gboolean
gimp_histogram_view_draw (GtkWidget *widget,
                          cairo_t   *cr)
{
  GimpHistogramView *view  = GIMP_HISTOGRAM_VIEW (widget);
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
  GdkRGBA            color;
  GdkRGBA            light;
  GdkRGBA            dark;
  GdkRGBA            color_in;
  GdkRGBA            color_out;
  GdkRGBA            bg_color_in;
  GdkRGBA            bg_color_out;
  GdkRGBA            rgb_color[3];

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, GTK_STYLE_CLASS_ENTRY);

  /*  Draw the background  */
  gtk_style_context_get_background_color (style, 0, &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_paint (cr);

  gtk_widget_get_allocation (widget, &allocation);

  border = view->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_translate (cr, 0.5, 0.5);

  /*  Draw the outer border  */
  gimp_get_style_color (widget, "grid-color", &grid_color);
  gdk_cairo_set_source_rgba (cr, &grid_color);
  cairo_rectangle (cr, border, border, width - 1, height - 1);
  cairo_stroke (cr);

  if (! view->histogram && ! view->bg_histogram)
    return FALSE;

  x1 = CLAMP (MIN (view->start, view->end), 0, view->n_bins - 1);
  x2 = CLAMP (MAX (view->start, view->end), 0, view->n_bins - 1);

  if (view->histogram)
    max = gimp_histogram_view_get_maximum (view, view->histogram,
                                           view->channel);

  if (view->bg_histogram)
    bg_max = gimp_histogram_view_get_maximum (view, view->bg_histogram,
                                              view->channel);

  gtk_style_context_get_color (style, GTK_STATE_FLAG_SELECTED,
                               &color_in);
  gtk_style_context_get_color (style, 0,
                               &color_out);

  gtk_style_context_get_background_color (style, GTK_STATE_FLAG_SELECTED,
                                          &bg_color_in);
  gtk_style_context_get_background_color (style, 0,
                                          &bg_color_out);

  gimp_gdk_rgba_shade (&bg_color_in, 1.3, &light);
  gimp_gdk_rgba_shade (&bg_color_in, 0.7, &dark);
  gimp_gdk_rgba_mix (&light, &dark, 0.5, &bg_color_in);

  gimp_gdk_rgba_shade (&bg_color_out, 1.3, &light);
  gimp_gdk_rgba_shade (&bg_color_out, 0.7, &dark);
  gimp_gdk_rgba_mix (&light, &dark, 0.5, &bg_color_out);

  if (view->channel == GIMP_HISTOGRAM_RGB)
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
          gtk_style_context_get_background_color (style, GTK_STATE_FLAG_SELECTED,
                                                  &color);

          gdk_cairo_set_source_rgba (cr, &color);

          cairo_move_to (cr, x + border, border);
          cairo_line_to (cr, x + border, border + height - 1);
          cairo_stroke (cr);
        }

      if (view->channel == GIMP_HISTOGRAM_RGB)
        {
          GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
          gint    c;

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c, cr,
                                            &black,
                                            CAIRO_OPERATOR_OVER,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c, cr,
                                            &rgb_color[c],
                                            CAIRO_OPERATOR_ADD,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          gimp_histogram_view_draw_spike (view, view->channel, cr,
                                          in_selection ? &color_in : &color_out,
                                          CAIRO_OPERATOR_OVER,
                                          NULL,
                                          x, i, j, max, bg_max, height, border);
        }
      else
        {
          gimp_histogram_view_draw_spike (view, view->channel, cr,
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
gimp_histogram_view_draw_spike (GimpHistogramView    *view,
                                GimpHistogramChannel  channel,
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
      do
        {
          gdouble v = gimp_histogram_get_value (view->histogram, channel, i++);

          if (v > value)
            value = v;
        }
      while (i < j);
    }

  if (bg_color && view->bg_histogram)
    {
      do
        {
          gdouble v = gimp_histogram_get_value (view->bg_histogram, channel, i++);

          if (v > bg_value)
            bg_value = v;
        }
      while (i < j);
    }

  if (value <= 0.0 && bg_value <= 0.0)
    return;

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      y    = (gint) (((height - 2) * value)    / max);
      bg_y = (gint) (((height - 2) * bg_value) / bg_max);
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
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
gimp_histogram_view_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      GtkAllocation allocation;
      gint          width;

      gtk_grab_add (widget);

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
gimp_histogram_view_button_release (GtkWidget      *widget,
                                    GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->button == 1)
    {
      gint start, end;

      gtk_grab_remove (widget);

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
  GtkAllocation      allocation;
  gint               width;

  gtk_widget_get_allocation (widget, &allocation);

  width = allocation.width - 2 * view->border_width;

  view->start = CLAMP (((mevent->x - view->border_width) * view->n_bins) / width,
                       0, view->n_bins - 1);

  gtk_widget_queue_draw (widget);

  return TRUE;
}


/*  public funcions  */

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
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->bg_histogram == NULL ||
                    gimp_histogram_n_channels (view->bg_histogram) ==
                    gimp_histogram_n_channels (histogram));
#endif

  if (view->histogram != histogram)
    {
      if (view->histogram)
        {
          g_signal_handlers_disconnect_by_func (view->histogram,
                                                gimp_histogram_view_notify,
                                                view);
          g_object_unref (view->histogram);
        }

      view->histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          g_signal_connect (histogram, "notify",
                            G_CALLBACK (gimp_histogram_view_notify),
                            view);

          if (view->n_bins != gimp_histogram_n_bins (histogram))
            gimp_histogram_view_update_bins (view);

          if (view->channel >= gimp_histogram_n_channels (histogram))
            gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
        }
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
gimp_histogram_view_set_background (GimpHistogramView *view,
                                    GimpHistogram     *histogram)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->histogram == NULL ||
                    gimp_histogram_n_channels (view->histogram) ==
                    gimp_histogram_n_channels (histogram));
#endif

  if (view->bg_histogram != histogram)
    {
      if (view->bg_histogram)
        g_object_unref (view->bg_histogram);

      view->bg_histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          if (view->n_bins != gimp_histogram_n_bins (histogram))
            gimp_histogram_view_update_bins (view);

          if (view->channel >= gimp_histogram_n_channels (histogram))
            gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
        }
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

GimpHistogram *
gimp_histogram_view_get_background (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->bg_histogram;
}

void
gimp_histogram_view_set_channel (GimpHistogramView    *view,
                                 GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (channel != view->channel)
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

  if (scale != view->scale)
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
gimp_histogram_view_get_range (GimpHistogramView *view,
                               gint              *start,
                               gint              *end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (start) *start = view->start;
  if (end)   *end   = view->end;
}


/*  private functions  */

static void
gimp_histogram_view_notify (GimpHistogram     *histogram,
                            const GParamSpec  *pspec,
                            GimpHistogramView *view)
{
  if (! strcmp (pspec->name, "n-bins"))
    {
      gimp_histogram_view_update_bins (view);
    }
  else
    {
      gtk_widget_queue_draw (GTK_WIDGET (view));
    }
}

static void
gimp_histogram_view_update_bins (GimpHistogramView *view)
{
  gint new_bins = 256;

  if (view->histogram)
    new_bins = gimp_histogram_n_bins (view->histogram);
  else if (view->bg_histogram)
    new_bins = gimp_histogram_n_bins (view->bg_histogram);

  view->start = ROUND (((gdouble) view->start *
                        (new_bins - 1) /
                        (view->n_bins - 1)));
  view->end   = ROUND (((gdouble) view->end   *
                        (new_bins - 1) /
                        (view->n_bins - 1)));

  view->n_bins = new_bins;

  g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                 view->start, view->end);
}
