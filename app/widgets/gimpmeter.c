/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmameter.c
 * Copyright (C) 2017 Ell
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
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "ligmameter.h"


#define BORDER_WIDTH 1.0
#define REV          (2.0 * G_PI)

#define SAMPLE(i)    (meter->priv->samples + (i) * meter->priv->n_values)
#define SAMPLE_SIZE  (meter->priv->n_values * sizeof (gdouble))
#define VALUE(i, j)  ((SAFE_CLAMP (SAMPLE (i)[j],                              \
                                   meter->priv->range_min,                     \
                                   meter->priv->range_max) -                   \
                        meter->priv->range_min) /                              \
                       (meter->priv->range_max - meter->priv->range_min))


enum
{
  PROP_0,
  PROP_SIZE,
  PROP_REFRESH_RATE,
  PROP_RANGE_MIN,
  PROP_RANGE_MAX,
  PROP_N_VALUES,
  PROP_HISTORY_VISIBLE,
  PROP_HISTORY_DURATION,
  PROP_HISTORY_RESOLUTION,
  PROP_LED_ACTIVE,
  PROP_LED_COLOR
};


typedef struct
{
  gboolean              active;
  gboolean              show_in_gauge;
  gboolean              show_in_history;
  LigmaRGB               color;
  LigmaInterpolationType interpolation;
} Value;

struct _LigmaMeterPrivate
{
  GMutex   mutex;

  gint      size;
  gdouble   refresh_rate;
  gdouble   range_min;
  gdouble   range_max;
  gint      n_values;
  Value    *values;
  gboolean  history_visible;
  gdouble   history_duration;
  gdouble   history_resolution;
  gboolean  led_active;
  LigmaRGB   led_color;

  gdouble  *samples;
  gint      n_samples;
  gint      sample_duration;
  gint64    last_sample_time;
  gint64    current_time;
  gdouble  *uniform_sample;
  gint      timeout_id;
};


/*  local function prototypes  */

static void       ligma_meter_dispose                (GObject        *object);
static void       ligma_meter_finalize               (GObject        *object);
static void       ligma_meter_set_property           (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void       ligma_meter_get_property           (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static void       ligma_meter_map                    (GtkWidget      *widget);
static void       ligma_meter_unmap                  (GtkWidget      *widget);
static void       ligma_meter_get_preferred_width    (GtkWidget      *widget,
                                                     gint           *minimum_width,
                                                     gint           *natural_width);
static void       ligma_meter_get_preferred_height   (GtkWidget      *widget,
                                                     gint           *minimum_height,
                                                     gint           *natural_height);
static gboolean   ligma_meter_draw                   (GtkWidget      *widget,
                                                     cairo_t        *cr);

static gboolean   ligma_meter_timeout                (LigmaMeter      *meter);

static void       ligma_meter_clear_history_unlocked (LigmaMeter      *meter);
static void       ligma_meter_update_samples         (LigmaMeter      *meter);
static void       ligma_meter_shift_samples          (LigmaMeter      *meter);

static void       ligma_meter_mask_sample            (LigmaMeter      *meter,
                                                     const gdouble  *sample,
                                                     gdouble        *result);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaMeter, ligma_meter, GTK_TYPE_WIDGET)

#define parent_class ligma_meter_parent_class


/*  private functions  */


static void
ligma_meter_class_init (LigmaMeterClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = ligma_meter_dispose;
  object_class->finalize             = ligma_meter_finalize;
  object_class->get_property         = ligma_meter_get_property;
  object_class->set_property         = ligma_meter_set_property;

  widget_class->map                  = ligma_meter_map;
  widget_class->unmap                = ligma_meter_unmap;
  widget_class->get_preferred_width  = ligma_meter_get_preferred_width;
  widget_class->get_preferred_height = ligma_meter_get_preferred_height;
  widget_class->draw                 = ligma_meter_draw;

  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     NULL, NULL,
                                                     32, 1024, 48,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_REFRESH_RATE,
                                   g_param_spec_double ("refresh-rate",
                                                        NULL, NULL,
                                                        0.001, 1000.0, 8.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RANGE_MIN,
                                   g_param_spec_double ("range-min",
                                                        NULL, NULL,
                                                        0.0, G_MAXDOUBLE, 0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RANGE_MAX,
                                   g_param_spec_double ("range-max",
                                                        NULL, NULL,
                                                        0.0, G_MAXDOUBLE, 1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_N_VALUES,
                                   g_param_spec_int ("n-values",
                                                     NULL, NULL,
                                                     0, 32, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HISTORY_VISIBLE,
                                   g_param_spec_boolean ("history-visible",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HISTORY_DURATION,
                                   g_param_spec_double ("history-duration",
                                                        NULL, NULL,
                                                        0.0, 3600.0, 60.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HISTORY_RESOLUTION,
                                   g_param_spec_double ("history-resolution",
                                                        NULL, NULL,
                                                        0.1, 3600.0, 1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));


  g_object_class_install_property (object_class, PROP_LED_ACTIVE,
                                   g_param_spec_boolean ("led-active",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LED_COLOR,
                                   ligma_param_spec_rgb ("led-color",
                                                        NULL, NULL,
                                                        TRUE, &(LigmaRGB) {},
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_meter_init (LigmaMeter *meter)
{
  meter->priv = ligma_meter_get_instance_private (meter);

  g_mutex_init (&meter->priv->mutex);

  gtk_widget_set_has_window (GTK_WIDGET (meter), FALSE);

  meter->priv->range_min          = 0.0;
  meter->priv->range_max          = 1.0;
  meter->priv->n_values           = 0;
  meter->priv->history_duration   = 60.0;
  meter->priv->history_resolution = 1.0;

  ligma_meter_update_samples (meter);
}

static void
ligma_meter_dispose (GObject *object)
{
  LigmaMeter *meter = LIGMA_METER (object);

  g_clear_pointer (&meter->priv->values, g_free);
  g_clear_pointer (&meter->priv->samples, g_free);
  g_clear_pointer (&meter->priv->uniform_sample, g_free);

  if (meter->priv->timeout_id)
    {
      g_source_remove (meter->priv->timeout_id);
      meter->priv->timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_meter_finalize (GObject *object)
{
  LigmaMeter *meter = LIGMA_METER (object);

  g_mutex_clear (&meter->priv->mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_meter_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaMeter *meter = LIGMA_METER (object);

  switch (property_id)
    {
    case PROP_SIZE:
      ligma_meter_set_size (meter, g_value_get_int (value));
      break;

    case PROP_REFRESH_RATE:
      ligma_meter_set_refresh_rate (meter, g_value_get_double (value));
      break;

    case PROP_RANGE_MIN:
      ligma_meter_set_range (meter,
                            g_value_get_double (value),
                            ligma_meter_get_range_max (meter));
      break;

    case PROP_RANGE_MAX:
      ligma_meter_set_range (meter,
                            ligma_meter_get_range_min (meter),
                            g_value_get_double (value));
      break;

    case PROP_N_VALUES:
      ligma_meter_set_n_values (meter, g_value_get_int (value));
      break;

    case PROP_HISTORY_VISIBLE:
      ligma_meter_set_history_visible (meter, g_value_get_boolean (value));
      break;

    case PROP_HISTORY_DURATION:
      ligma_meter_set_history_duration (meter, g_value_get_double (value));
      break;

    case PROP_HISTORY_RESOLUTION:
      ligma_meter_set_history_resolution (meter, g_value_get_double (value));
      break;

    case PROP_LED_ACTIVE:
      ligma_meter_set_led_active (meter, g_value_get_boolean (value));
      break;

    case PROP_LED_COLOR:
      ligma_meter_set_led_color (meter, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_meter_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaMeter *meter = LIGMA_METER (object);

  switch (property_id)
    {
    case PROP_SIZE:
      g_value_set_int (value, ligma_meter_get_size (meter));
      break;

    case PROP_REFRESH_RATE:
      g_value_set_double (value, ligma_meter_get_refresh_rate (meter));
      break;

    case PROP_RANGE_MIN:
      g_value_set_double (value, ligma_meter_get_range_min (meter));
      break;

    case PROP_RANGE_MAX:
      g_value_set_double (value, ligma_meter_get_range_max (meter));
      break;

    case PROP_N_VALUES:
      g_value_set_int (value, ligma_meter_get_n_values (meter));
      break;

    case PROP_HISTORY_VISIBLE:
      g_value_set_boolean (value, ligma_meter_get_history_visible (meter));
      break;

    case PROP_HISTORY_DURATION:
      g_value_set_int (value, ligma_meter_get_history_duration (meter));
      break;

    case PROP_HISTORY_RESOLUTION:
      g_value_set_int (value, ligma_meter_get_history_resolution (meter));
      break;

    case PROP_LED_ACTIVE:
      g_value_set_boolean (value, ligma_meter_get_led_active (meter));
      break;

    case PROP_LED_COLOR:
      g_value_set_boxed (value, ligma_meter_get_led_color (meter));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_meter_map (GtkWidget *widget)
{
  LigmaMeter *meter = LIGMA_METER (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (! meter->priv->timeout_id)
    {
      gint interval = ROUND (1000.0 / meter->priv->refresh_rate);

      meter->priv->timeout_id = g_timeout_add (interval,
                                               (GSourceFunc) ligma_meter_timeout,
                                               meter);
    }
}

static void
ligma_meter_unmap (GtkWidget *widget)
{
  LigmaMeter *meter = LIGMA_METER (widget);

  if (meter->priv->timeout_id)
    {
      g_source_remove (meter->priv->timeout_id);
      meter->priv->timeout_id = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_meter_get_preferred_width (GtkWidget *widget,
                                gint      *minimum_width,
                                gint      *natural_width)
{
  LigmaMeter *meter = LIGMA_METER (widget);
  gint       hsize = meter->priv->size;

  if (meter->priv->history_visible)
    hsize *= 3;

  *minimum_width = *natural_width = ceil (hsize + 2.0 * BORDER_WIDTH);
}

static void
ligma_meter_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum_height,
                                 gint      *natural_height)
{
  LigmaMeter *meter = LIGMA_METER (widget);
  gint       vsize = meter->priv->size;

  *minimum_height = *natural_height = ceil (3.0 / 4.0 * vsize + 4.0 * BORDER_WIDTH);
}

static gboolean
ligma_meter_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  LigmaMeter       *meter = LIGMA_METER (widget);
  GtkAllocation    allocation;
  gint             size  = meter->priv->size;
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkStateFlags    state = gtk_style_context_get_state (style);
  GdkRGBA          fg;
  gint             i;
  gint             j;
  gint             k;

  g_mutex_lock (&meter->priv->mutex);

  gtk_widget_get_allocation (widget, &allocation);

  gtk_style_context_get_color (style, state, &fg);

  /* translate to gauge center */
  cairo_translate (cr,
                   0.5 * BORDER_WIDTH +       0.5 * size,
                   1.5 * BORDER_WIDTH + 2.0 / 3.0 * (allocation.height - 4.0 * BORDER_WIDTH));

  cairo_save (cr);

  /* paint led */
  if (meter->priv->led_active)
    {
      cairo_arc (cr,
                 0.0, 0.0,
                 0.06 * size,
                 0.0 * REV, 1.0 * REV);

      ligma_cairo_set_source_rgba (cr, &meter->priv->led_color);
      cairo_fill (cr);
    }

  /* clip to gauge interior */
  cairo_arc          (cr,
                      0.0, 0.0,
                      0.5 * size,
                      5.0 / 12.0 * REV, 1.0 / 12.0 * REV);
  cairo_arc_negative (cr,
                      0.0, 0.0,
                      0.1 * size,
                      1.0 / 12.0 * REV, 5.0 / 12.0 * REV);
  cairo_close_path (cr);
  cairo_clip (cr);

  /* paint gauge background */
  gdk_cairo_set_source_rgba (cr, &fg);
  cairo_paint_with_alpha (cr, 0.2);

  /* paint values of last sample */
  if (meter->priv->range_min < meter->priv->range_max)
    {
      for (i = 0; i < meter->priv->n_values; i++)
        {
          gdouble v = VALUE (0, i);

          if (! meter->priv->values[i].active ||
              ! meter->priv->values[i].show_in_gauge)
            {
              continue;
            }

          ligma_cairo_set_source_rgba (cr, &meter->priv->values[i].color);
          cairo_move_to (cr, 0.0, 0.0);
          cairo_arc (cr,
                     0.0, 0.0,

                     0.5 * size,
                     5.0 / 12.0 * REV, (5.0 / 12.0 + 2.0 / 3.0 * v) * REV);
          cairo_line_to (cr, 0.0, 0.0);
          cairo_close_path (cr);
          cairo_fill (cr);
        }
    }

  cairo_restore (cr);

  /* paint gauge border */
  gdk_cairo_set_source_rgba (cr, &fg);
  cairo_set_line_width (cr, BORDER_WIDTH);
  cairo_arc          (cr,
                      0.0, 0.0,
                      0.5 * size,
                      5.0 / 12.0 * REV, 1.0 / 12.0 * REV);
  cairo_arc_negative (cr,
                      0.0, 0.0,
                      0.1 * size,
                      1.0 / 12.0 * REV, 5.0 / 12.0 * REV);
  cairo_close_path (cr);
  cairo_stroke (cr);

  /* history */
  if (meter->priv->history_visible)
    {
      gdouble a1, a2;
      gdouble history_x1, history_y1;
      gdouble history_x2, history_y2;

      cairo_save (cr);

      a1 = +asin (0.25 / 0.6);
      a2 = -asin (0.50 / 0.6);

      /* clip to history interior */
      cairo_arc_negative (cr,
                          0.0, 0.0,
                          0.6 * size,
                          a1, a2);
      cairo_line_to (cr,
                     allocation.width - BORDER_WIDTH - 0.5 * size,
                     -0.50 * size);
      cairo_line_to (cr,
                     allocation.width - BORDER_WIDTH - 0.5 * size,
                     0.25 * size);
      cairo_close_path (cr);

      cairo_path_extents (cr,
                          &history_x1, &history_y1,
                          &history_x2, &history_y2);

      history_x1 = floor (history_x1);
      history_y1 = floor (history_y1);
      history_x2 = ceil  (history_x2);
      history_y2 = ceil  (history_y2);

      cairo_clip (cr);

      /* paint history background */
      gdk_cairo_set_source_rgba (cr, &fg);
      cairo_paint_with_alpha (cr, 0.2);

      /* history graph */
      if (meter->priv->range_min < meter->priv->range_max)
        {
          gdouble sample_width = (history_x2 - history_x1) /
                                 (meter->priv->n_samples - 4);
          gdouble dx           = 1.0 / sample_width;

          cairo_save (cr);

          /* translate to history bottom-right, and scale so that the
           * x-axis points left, and has a length of one sample, and
           * the y-axis points up, and has a length of the history
           * window.
           */
          cairo_translate (cr, history_x2, history_y2);
          cairo_scale (cr, -sample_width, -(history_y2 - history_y1));
          cairo_translate (cr,
                           (gdouble) (meter->priv->current_time     -
                                      meter->priv->last_sample_time *
                                      meter->priv->sample_duration) /
                           meter->priv->sample_duration             -
                           2.0,
                           0.0);

          /* paint history graph for each value */
          for (i = 0; i < meter->priv->n_values; i++)
            {
              gdouble y;

              if (! meter->priv->values[i].active ||
                  ! meter->priv->values[i].show_in_history)
                {
                  continue;
                }

              ligma_cairo_set_source_rgba (cr, &meter->priv->values[i].color);
              cairo_move_to (cr, 0.0, 0.0);

              switch (meter->priv->values[i].interpolation)
                {
                case LIGMA_INTERPOLATION_NONE:
                  {
                    for (j = 1; j < meter->priv->n_samples - 2; j++)
                      {
                        gdouble y0 = VALUE (j - 1, i);
                        gdouble y1 = VALUE (j,     i);

                        cairo_line_to (cr, j, y0);
                        cairo_line_to (cr, j, y1);
                      }
                  }
                  break;

                case LIGMA_INTERPOLATION_LINEAR:
                  {
                    for (j = 1; j < meter->priv->n_samples - 2; j++)
                      {
                        gdouble y = VALUE (j, i);

                        cairo_line_to (cr, j, y);
                      }
                  }
                  break;

                case LIGMA_INTERPOLATION_CUBIC:
                default:
                  {
                    for (j = 1; j < meter->priv->n_samples - 2; j++)
                      {
                        gdouble y[4];
                        gdouble t[2];
                        gdouble c[4];
                        gdouble x;

                        for (k = 0; k < 4; k++)
                          y[k] = VALUE (j + k - 1, i);

                        for (k = 0; k < 2; k++)
                          {
                            t[k] = (y[k + 2] - y[k]) / 2.0;
                            t[k] = CLAMP (t[k], y[k + 1] - 1.0, y[k + 1]);
                            t[k] = CLAMP (t[k], -y[k + 1], 1.0 - y[k + 1]);
                          }

                        c[0] = y[1];
                        c[1] = t[0];
                        c[2] = 3 * (y[2] - y[1]) - 2 * t[0] - t[1];
                        c[3] = t[0] + t[1] - 2 * (y[2] - y[1]);

                        for (x = 0.0; x < 1.0; x += dx)
                          {
                            gdouble y = ((c[3] * x + c[2]) * x + c[1]) * x + c[0];

                            cairo_line_to (cr, j + x, y);
                          }
                      }
                  }
                  break;
                }

              y = VALUE (j, i);

              cairo_line_to (cr, meter->priv->n_samples - 2, y);
              cairo_line_to (cr, meter->priv->n_samples - 2, 0.0);
              cairo_close_path (cr);
              cairo_fill (cr);
            }

          cairo_restore (cr);
        }

      /* paint history grid */
      cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
      cairo_set_source_rgba (cr, fg.red, fg.green, fg.blue, 0.3);

      for (i = 1; i < 4; i++)
        {
          cairo_move_to (cr,
                         history_x1,
                         history_y1 + i / 4.0 * (history_y2 - history_y1));
          cairo_rel_line_to (cr, history_x2 - history_x1, 0.0);
          cairo_stroke (cr);
        }

      for (i = 1; i < 6; i++)
        {
          cairo_move_to (cr,
                         history_x1 + i / 6.0 * (history_x2 - history_x1),
                         history_y1);
          cairo_rel_line_to (cr, 0.0, history_y2 - history_y1);
          cairo_stroke (cr);
        }

      cairo_restore (cr);

      /* paint history border */
      cairo_arc_negative (cr,
                          0.0, 0.0,
                          0.6 * size,
                          a1, a2);
      cairo_line_to (cr,
                     allocation.width - BORDER_WIDTH - 0.5 * size,
                     -0.50 * size);
      cairo_line_to (cr,
                     allocation.width - BORDER_WIDTH - 0.5 * size,
                     0.25 * size);
      cairo_close_path (cr);
      cairo_stroke (cr);
    }

  g_mutex_unlock (&meter->priv->mutex);

  return FALSE;
}

static gboolean
ligma_meter_timeout (LigmaMeter *meter)
{
  gboolean uniform = TRUE;
  gboolean redraw  = TRUE;
  gdouble  sample0[meter->priv->n_values];
  gint     i;

  g_mutex_lock (&meter->priv->mutex);

  ligma_meter_shift_samples (meter);

  ligma_meter_mask_sample (meter, SAMPLE (0), sample0);

  if (meter->priv->history_visible)
    {
      for (i = 1; uniform && i < meter->priv->n_samples; i++)
        {
          gdouble sample[meter->priv->n_values];

          ligma_meter_mask_sample (meter, SAMPLE (i), sample);

          uniform = ! memcmp (sample0, sample, SAMPLE_SIZE);
        }
    }

  if (uniform && meter->priv->uniform_sample)
    redraw = memcmp (sample0, meter->priv->uniform_sample, SAMPLE_SIZE);

  if (uniform)
    {
      if (! meter->priv->uniform_sample)
        meter->priv->uniform_sample = g_malloc (SAMPLE_SIZE);

      memcpy (meter->priv->uniform_sample, sample0, SAMPLE_SIZE);
    }
  else
    {
      g_clear_pointer (&meter->priv->uniform_sample, g_free);
    }

  g_mutex_unlock (&meter->priv->mutex);

  if (redraw)
    gtk_widget_queue_draw (GTK_WIDGET (meter));

  return G_SOURCE_CONTINUE;
}

static void
ligma_meter_clear_history_unlocked (LigmaMeter *meter)
{
  meter->priv->current_time     = g_get_monotonic_time ();
  meter->priv->last_sample_time = meter->priv->current_time /
                                  meter->priv->sample_duration;

  memset (meter->priv->samples, 0, meter->priv->n_samples * SAMPLE_SIZE);

  g_clear_pointer (&meter->priv->uniform_sample, g_free);
}

static void
ligma_meter_update_samples (LigmaMeter *meter)
{
  meter->priv->n_samples = ceil (meter->priv->history_duration /
                                 meter->priv->history_resolution) + 4;

  meter->priv->samples = g_renew (gdouble, meter->priv->samples,
                                  meter->priv->n_samples *
                                  meter->priv->n_values);

  meter->priv->sample_duration = ROUND (meter->priv->history_resolution *
                                        G_TIME_SPAN_SECOND);

  ligma_meter_clear_history_unlocked (meter);
}

static void
ligma_meter_shift_samples (LigmaMeter *meter)
{
  gint64 time;
  gint   n_new_samples;

  meter->priv->current_time = g_get_monotonic_time ();

  time = meter->priv->current_time / meter->priv->sample_duration;

  n_new_samples = MIN (time - meter->priv->last_sample_time,
                       meter->priv->n_samples - 1);

  memmove (SAMPLE (n_new_samples), SAMPLE (0),
           (meter->priv->n_samples - n_new_samples) * SAMPLE_SIZE);
  gegl_memset_pattern (SAMPLE (0), SAMPLE (n_new_samples), SAMPLE_SIZE,
                       n_new_samples);

  meter->priv->last_sample_time = time;
}

static void
ligma_meter_mask_sample (LigmaMeter     *meter,
                        const gdouble *sample,
                        gdouble       *result)
{
  gint i;

  for (i = 0; i < meter->priv->n_values; i++)
    {
      if (meter->priv->values[i].active         &&
          (meter->priv->values[i].show_in_gauge ||
           meter->priv->values[i].show_in_history))
        {
          result[i] = sample[i];
        }
      else
        {
          result[i] = 0.0;
        }
    }
}


/*  public functions  */


GtkWidget *
ligma_meter_new (gint n_values)
{
  return g_object_new (LIGMA_TYPE_METER,
                       "n-values", n_values,
                       NULL);
}

void
ligma_meter_set_size (LigmaMeter *meter,
                     gint       size)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (size > 0);

  if (size != meter->priv->size)
    {
      meter->priv->size = size;

      gtk_widget_queue_resize (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "size");
    }
}

gint
ligma_meter_get_size (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0);

  return meter->priv->size;
}

void
ligma_meter_set_refresh_rate (LigmaMeter *meter,
                             gdouble    rate)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (rate > 0.0);

  if (rate != meter->priv->refresh_rate)
    {
      meter->priv->refresh_rate = rate;

      if (meter->priv->timeout_id)
        {
          gint interval = ROUND (1000.0 / meter->priv->refresh_rate);

          g_source_remove (meter->priv->timeout_id);

          meter->priv->timeout_id = g_timeout_add (interval,
                                                   (GSourceFunc) ligma_meter_timeout,
                                                   meter);
        }

      g_object_notify (G_OBJECT (meter), "refresh-rate");
    }
}

gdouble
ligma_meter_get_refresh_rate (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0);

  return meter->priv->refresh_rate;
}

void
ligma_meter_set_range (LigmaMeter *meter,
                      gdouble    min,
                      gdouble    max)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (min <= max);

  if (min != meter->priv->range_min)
    {
      g_mutex_lock (&meter->priv->mutex);

      meter->priv->range_min = min;

      g_mutex_unlock (&meter->priv->mutex);

      gtk_widget_queue_draw (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "range-min");
    }

  if (max != meter->priv->range_max)
    {
      g_mutex_lock (&meter->priv->mutex);

      meter->priv->range_max = max;

      g_mutex_unlock (&meter->priv->mutex);

      gtk_widget_queue_draw (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "range-max");
    }
}

gdouble
ligma_meter_get_range_min (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0.0);

  return meter->priv->range_min;
}

gdouble
ligma_meter_get_range_max (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0.0);

  return meter->priv->range_max;
}

void
ligma_meter_set_n_values (LigmaMeter *meter,
                         gint       n_values)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (n_values >= 0);

  if (n_values != meter->priv->n_values)
    {
      g_mutex_lock (&meter->priv->mutex);

      meter->priv->values = g_renew (Value, meter->priv->values, n_values);

      if (n_values > meter->priv->n_values)
        {
          gegl_memset_pattern (meter->priv->values,
                               &(Value) { .active          = TRUE,
                                          .show_in_gauge   = TRUE,
                                          .show_in_history = TRUE,
                                          .interpolation   = LIGMA_INTERPOLATION_CUBIC},
                               sizeof (Value),
                               n_values - meter->priv->n_values);
        }

      meter->priv->n_values = n_values;

      ligma_meter_update_samples (meter);

      g_mutex_unlock (&meter->priv->mutex);

      gtk_widget_queue_draw (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "n-values");
    }
}

gint
ligma_meter_get_n_values (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0);

  return meter->priv->n_values;
}

void
ligma_meter_set_value_active (LigmaMeter *meter,
                             gint       value,
                             gboolean   active)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (value >= 0 && value < meter->priv->n_values);

  if (active != meter->priv->values[value].active)
    {
      meter->priv->values[value].active = active;

      gtk_widget_queue_draw (GTK_WIDGET (meter));
    }
}

gboolean
ligma_meter_get_value_active (LigmaMeter *meter,
                             gint       value)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), FALSE);
  g_return_val_if_fail (value >= 0 && value < meter->priv->n_values, FALSE);

  return meter->priv->values[value].active;
}


void
ligma_meter_set_value_color (LigmaMeter     *meter,
                            gint           value,
                            const LigmaRGB *color)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (value >= 0 && value < meter->priv->n_values);
  g_return_if_fail (color != NULL);

  if (memcmp (color, &meter->priv->values[value].color, sizeof (LigmaRGB)))
    {
      meter->priv->values[value].color = *color;

      gtk_widget_queue_draw (GTK_WIDGET (meter));
    }
}

const LigmaRGB *
ligma_meter_get_value_color (LigmaMeter *meter,
                            gint       value)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), NULL);
  g_return_val_if_fail (value >= 0 && value < meter->priv->n_values, NULL);

  return &meter->priv->values[value].color;
}

void
ligma_meter_set_value_interpolation (LigmaMeter             *meter,
                                    gint                   value,
                                    LigmaInterpolationType  interpolation)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (value >= 0 && value < meter->priv->n_values);

  if (meter->priv->values[value].interpolation != interpolation)
    {
      meter->priv->values[value].interpolation = interpolation;

      gtk_widget_queue_draw (GTK_WIDGET (meter));
    }
}

LigmaInterpolationType
ligma_meter_get_value_interpolation (LigmaMeter *meter,
                                    gint       value)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), LIGMA_INTERPOLATION_NONE);
  g_return_val_if_fail (value >= 0 && value < meter->priv->n_values,
                        LIGMA_INTERPOLATION_NONE);

  return meter->priv->values[value].interpolation;
}

void
ligma_meter_set_value_show_in_gauge (LigmaMeter *meter,
                                    gint       value,
                                    gboolean   show)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (value >= 0 && value < meter->priv->n_values);

  if (meter->priv->values[value].show_in_gauge != show)
    {
      meter->priv->values[value].show_in_gauge = show;

      gtk_widget_queue_draw (GTK_WIDGET (meter));
    }
}

gboolean
ligma_meter_get_value_show_in_gauge (LigmaMeter *meter,
                                    gint       value)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), FALSE);
  g_return_val_if_fail (value >= 0 && value < meter->priv->n_values, FALSE);

  return meter->priv->values[value].show_in_gauge;
}

void
ligma_meter_set_value_show_in_history (LigmaMeter *meter,
                                      gint       value,
                                      gboolean   show)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (value >= 0 && value < meter->priv->n_values);

  if (meter->priv->values[value].show_in_history != show)
    {
      meter->priv->values[value].show_in_history = show;

      gtk_widget_queue_draw (GTK_WIDGET (meter));
    }
}

gboolean
ligma_meter_get_value_show_in_history (LigmaMeter *meter,
                                      gint       value)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), FALSE);
  g_return_val_if_fail (value >= 0 && value < meter->priv->n_values, FALSE);

  return meter->priv->values[value].show_in_history;
}

void
ligma_meter_set_history_visible (LigmaMeter *meter,
                                gboolean   visible)
{
  g_return_if_fail (LIGMA_IS_METER (meter));

  if (visible != meter->priv->history_visible)
    {
      meter->priv->history_visible = visible;

      gtk_widget_queue_resize (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "history-visible");
    }
}

gboolean
ligma_meter_get_history_visible (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), FALSE);

  return meter->priv->history_visible;
}

void
ligma_meter_set_history_duration (LigmaMeter *meter,
                                 gdouble    duration)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (duration >= 0.0);

  if (duration != meter->priv->history_duration)
    {
      g_mutex_lock (&meter->priv->mutex);

      meter->priv->history_duration = duration;

      ligma_meter_update_samples (meter);

      g_mutex_unlock (&meter->priv->mutex);

      g_object_notify (G_OBJECT (meter), "history-duration");
    }
}

gdouble
ligma_meter_get_history_duration (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0.0);

  return meter->priv->history_duration;
}

void
ligma_meter_set_history_resolution (LigmaMeter *meter,
                                   gdouble    resolution)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (resolution > 0.0);

  if (resolution != meter->priv->history_resolution)
    {
      g_mutex_lock (&meter->priv->mutex);

      meter->priv->history_resolution = resolution;

      ligma_meter_update_samples (meter);

      g_mutex_unlock (&meter->priv->mutex);

      g_object_notify (G_OBJECT (meter), "history-resolution");
    }
}

gdouble
ligma_meter_get_history_resolution (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), 0.0);

  return meter->priv->history_resolution;
}

void
ligma_meter_clear_history (LigmaMeter *meter)
{
  g_return_if_fail (LIGMA_IS_METER (meter));

  g_mutex_lock (&meter->priv->mutex);

  ligma_meter_clear_history_unlocked (meter);

  g_mutex_unlock (&meter->priv->mutex);

  gtk_widget_queue_draw (GTK_WIDGET (meter));
}

void
ligma_meter_add_sample (LigmaMeter     *meter,
                       const gdouble *sample)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (sample != NULL || meter->priv->n_values == 0);

  g_mutex_lock (&meter->priv->mutex);

  ligma_meter_shift_samples (meter);

  memcpy (SAMPLE (0), sample, SAMPLE_SIZE);

  g_mutex_unlock (&meter->priv->mutex);
}

void
ligma_meter_set_led_active (LigmaMeter *meter,
                           gboolean   active)
{
  g_return_if_fail (LIGMA_IS_METER (meter));

  if (active != meter->priv->led_active)
    {
      meter->priv->led_active = active;

      gtk_widget_queue_draw (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "led-active");
    }
}

gboolean
ligma_meter_get_led_active (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), FALSE);

  return meter->priv->led_active;
}

void
ligma_meter_set_led_color (LigmaMeter     *meter,
                          const LigmaRGB *color)
{
  g_return_if_fail (LIGMA_IS_METER (meter));
  g_return_if_fail (color != NULL);

  if (memcmp (color, &meter->priv->led_color, sizeof (LigmaRGB)))
    {
      meter->priv->led_color = *color;

      if (meter->priv->led_active)
        gtk_widget_queue_draw (GTK_WIDGET (meter));

      g_object_notify (G_OBJECT (meter), "led-color");
    }
}

const LigmaRGB *
ligma_meter_get_led_color (LigmaMeter *meter)
{
  g_return_val_if_fail (LIGMA_IS_METER (meter), NULL);

  return &meter->priv->led_color;
}
