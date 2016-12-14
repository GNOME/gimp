/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplevelsconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <errno.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "core/gimpcurve.h"
#include "core/gimphistogram.h"

#include "gimpcurvesconfig.h"
#include "gimplevelsconfig.h"
#include "gimpoperationlevels.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_GAMMA,
  PROP_LOW_INPUT,
  PROP_HIGH_INPUT,
  PROP_LOW_OUTPUT,
  PROP_HIGH_OUTPUT
};


static void     gimp_levels_config_iface_init   (GimpConfigInterface *iface);

static void     gimp_levels_config_get_property (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     gimp_levels_config_set_property (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static gboolean gimp_levels_config_serialize    (GimpConfig       *config,
                                                 GimpConfigWriter *writer,
                                                 gpointer          data);
static gboolean gimp_levels_config_deserialize  (GimpConfig       *config,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);
static gboolean gimp_levels_config_equal        (GimpConfig       *a,
                                                 GimpConfig       *b);
static void     gimp_levels_config_reset        (GimpConfig       *config);
static gboolean gimp_levels_config_copy         (GimpConfig       *src,
                                                 GimpConfig       *dest,
                                                 GParamFlags       flags);


G_DEFINE_TYPE_WITH_CODE (GimpLevelsConfig, gimp_levels_config,
                         GIMP_TYPE_SETTINGS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_levels_config_iface_init))

#define parent_class gimp_levels_config_parent_class


static void
gimp_levels_config_class_init (GimpLevelsConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property        = gimp_levels_config_set_property;
  object_class->get_property        = gimp_levels_config_get_property;

  viewable_class->default_icon_name = "gimp-tool-levels";

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         _("The affected channel"),
                         GIMP_TYPE_HISTOGRAM_CHANNEL,
                         GIMP_HISTOGRAM_VALUE, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_GAMMA,
                           "gamma",
                           _("Gamma"),
                           _("Gamma"),
                           0.1, 10.0, 1.0, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LOW_INPUT,
                           "low-input",
                           _("Low Input"),
                           _("Low Input"),
                           0.0, 1.0, 0.0, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_HIGH_INPUT,
                           "high-input",
                           _("High Input"),
                           _("High Input"),
                           0.0, 1.0, 1.0, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LOW_OUTPUT,
                           "low-output",
                           _("Low Output"),
                           _("Low Output"),
                           0.0, 1.0, 0.0, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_HIGH_OUTPUT,
                           "high-output",
                           _("High Output"),
                           _("High Output"),
                           0.0, 1.0, 1.0, 0);
}

static void
gimp_levels_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_levels_config_serialize;
  iface->deserialize = gimp_levels_config_deserialize;
  iface->equal       = gimp_levels_config_equal;
  iface->reset       = gimp_levels_config_reset;
  iface->copy        = gimp_levels_config_copy;
}

static void
gimp_levels_config_init (GimpLevelsConfig *self)
{
  gimp_config_reset (GIMP_CONFIG (self));
}

static void
gimp_levels_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpLevelsConfig *self = GIMP_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_GAMMA:
      g_value_set_double (value, self->gamma[self->channel]);
      break;

    case PROP_LOW_INPUT:
      g_value_set_double (value, self->low_input[self->channel]);
      break;

    case PROP_HIGH_INPUT:
      g_value_set_double (value, self->high_input[self->channel]);
      break;

    case PROP_LOW_OUTPUT:
      g_value_set_double (value, self->low_output[self->channel]);
      break;

    case PROP_HIGH_OUTPUT:
      g_value_set_double (value, self->high_output[self->channel]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_levels_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpLevelsConfig *self = GIMP_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      g_object_notify (object, "gamma");
      g_object_notify (object, "low-input");
      g_object_notify (object, "high-input");
      g_object_notify (object, "low-output");
      g_object_notify (object, "high-output");
      break;

    case PROP_GAMMA:
      self->gamma[self->channel] = g_value_get_double (value);
      break;

    case PROP_LOW_INPUT:
      self->low_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_INPUT:
      self->high_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_LOW_OUTPUT:
      self->low_output[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_OUTPUT:
      self->high_output[self->channel] = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_levels_config_serialize (GimpConfig       *config,
                              GimpConfigWriter *writer,
                              gpointer          data)
{
  GimpLevelsConfig     *l_config = GIMP_LEVELS_CONFIG (config);
  GimpHistogramChannel  channel;
  GimpHistogramChannel  old_channel;
  gboolean              success = TRUE;

  if (! gimp_config_serialize_property_by_name (config, "time", writer))
    return FALSE;

  old_channel = l_config->channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      l_config->channel = channel;

      /*  serialize the channel properties manually (not using
       *  gimp_config_serialize_properties()), so the parent class'
       *  "time" property doesn't end up in the config file one per
       *  channel. See bug #700653.
       */
      success =
        (gimp_config_serialize_property_by_name (config, "channel",     writer) &&
         gimp_config_serialize_property_by_name (config, "gamma",       writer) &&
         gimp_config_serialize_property_by_name (config, "low-input",   writer) &&
         gimp_config_serialize_property_by_name (config, "high-input",  writer) &&
         gimp_config_serialize_property_by_name (config, "low-output",  writer) &&
         gimp_config_serialize_property_by_name (config, "high-output", writer));

      if (! success)
        break;
    }

  l_config->channel = old_channel;

  return success;
}

static gboolean
gimp_levels_config_deserialize (GimpConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  GimpLevelsConfig     *l_config = GIMP_LEVELS_CONFIG (config);
  GimpHistogramChannel  old_channel;
  gboolean              success = TRUE;

  old_channel = l_config->channel;

  success = gimp_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "channel", old_channel, NULL);

  return success;
}

static gboolean
gimp_levels_config_equal (GimpConfig *a,
                          GimpConfig *b)
{
  GimpLevelsConfig     *config_a = GIMP_LEVELS_CONFIG (a);
  GimpLevelsConfig     *config_b = GIMP_LEVELS_CONFIG (b);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      if (config_a->gamma[channel]       != config_b->gamma[channel]      ||
          config_a->low_input[channel]   != config_b->low_input[channel]  ||
          config_a->high_input[channel]  != config_b->high_input[channel] ||
          config_a->low_output[channel]  != config_b->low_output[channel] ||
          config_a->high_output[channel] != config_b->high_output[channel])
        return FALSE;
    }

  /* don't compare "channel" */

  return TRUE;
}

static void
gimp_levels_config_reset (GimpConfig *config)
{
  GimpLevelsConfig     *l_config = GIMP_LEVELS_CONFIG (config);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      l_config->channel = channel;
      gimp_levels_config_reset_channel (l_config);
    }

  gimp_config_reset_property (G_OBJECT (config), "channel");
}

static gboolean
gimp_levels_config_copy (GimpConfig  *src,
                         GimpConfig  *dest,
                         GParamFlags  flags)
{
  GimpLevelsConfig     *src_config  = GIMP_LEVELS_CONFIG (src);
  GimpLevelsConfig     *dest_config = GIMP_LEVELS_CONFIG (dest);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      dest_config->gamma[channel]       = src_config->gamma[channel];
      dest_config->low_input[channel]   = src_config->low_input[channel];
      dest_config->high_input[channel]  = src_config->high_input[channel];
      dest_config->low_output[channel]  = src_config->low_output[channel];
      dest_config->high_output[channel] = src_config->high_output[channel];
    }

  g_object_notify (G_OBJECT (dest), "gamma");
  g_object_notify (G_OBJECT (dest), "low-input");
  g_object_notify (G_OBJECT (dest), "high-input");
  g_object_notify (G_OBJECT (dest), "low-output");
  g_object_notify (G_OBJECT (dest), "high-output");

  dest_config->channel = src_config->channel;

  g_object_notify (G_OBJECT (dest), "channel");

  return TRUE;
}


/*  public functions  */

void
gimp_levels_config_reset_channel (GimpLevelsConfig *config)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  gimp_config_reset_property (G_OBJECT (config), "gamma");
  gimp_config_reset_property (G_OBJECT (config), "low-input");
  gimp_config_reset_property (G_OBJECT (config), "high-input");
  gimp_config_reset_property (G_OBJECT (config), "low-output");
  gimp_config_reset_property (G_OBJECT (config), "high-output");

  g_object_thaw_notify (G_OBJECT (config));
}

void
gimp_levels_config_stretch (GimpLevelsConfig *config,
                            GimpHistogram    *histogram,
                            gboolean          is_color)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  g_object_freeze_notify (G_OBJECT (config));

  if (is_color)
    {
      GimpHistogramChannel channel;

      /*  Set the overall value to defaults  */
      channel = config->channel;
      config->channel = GIMP_HISTOGRAM_VALUE;
      gimp_levels_config_reset_channel (config);
      config->channel = channel;

      for (channel = GIMP_HISTOGRAM_RED;
           channel <= GIMP_HISTOGRAM_BLUE;
           channel++)
        {
          gimp_levels_config_stretch_channel (config, histogram, channel);
        }
    }
  else
    {
      gimp_levels_config_stretch_channel (config, histogram,
                                          GIMP_HISTOGRAM_VALUE);
    }

  g_object_thaw_notify (G_OBJECT (config));
}

void
gimp_levels_config_stretch_channel (GimpLevelsConfig     *config,
                                    GimpHistogram        *histogram,
                                    GimpHistogramChannel  channel)
{
  gdouble count;
  gdouble bias = 0.006;
  gint    n_bins;
  gint    i;

  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  g_object_freeze_notify (G_OBJECT (config));

  config->gamma[channel]       = 1.0;
  config->low_output[channel]  = 0.0;
  config->high_output[channel] = 1.0;

  n_bins = gimp_histogram_n_bins (histogram);
  count  = gimp_histogram_get_count (histogram, channel, 0, n_bins - 1);

  if (count == 0.0)
    {
      config->low_input[channel]  = 0.0;
      config->high_input[channel] = 0.0;
    }
  else
    {
      gdouble new_count;
      gdouble percentage;
      gdouble next_percentage;

      /*  Set the low input  */
      new_count = 0.0;

      for (i = 0; i < (n_bins - 1); i++)
        {
          new_count += gimp_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             gimp_histogram_get_value (histogram,
                                                       channel,
                                                       i + 1)) / count;

          if (fabs (percentage - bias) < fabs (next_percentage - bias))
            {
              config->low_input[channel] = (gdouble) (i + 1) / (n_bins - 1);
              break;
            }
        }

      /*  Set the high input  */
      new_count = 0.0;

      for (i = (n_bins - 1); i > 0; i--)
        {
          new_count += gimp_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             gimp_histogram_get_value (histogram,
                                                       channel,
                                                       i - 1)) / count;

          if (fabs (percentage - bias) < fabs (next_percentage - bias))
            {
              config->high_input[channel] = (gdouble) (i - 1) / (n_bins - 1);
              break;
            }
        }
    }

  g_object_notify (G_OBJECT (config), "gamma");
  g_object_notify (G_OBJECT (config), "low-input");
  g_object_notify (G_OBJECT (config), "high-input");
  g_object_notify (G_OBJECT (config), "low-output");
  g_object_notify (G_OBJECT (config), "high-output");

  g_object_thaw_notify (G_OBJECT (config));
}

static gdouble
gimp_levels_config_input_from_color (GimpHistogramChannel  channel,
                                     const GimpRGB        *color)
{
  float rgba[4];
  gimp_rgba_get_pixel (color, babl_format ("RGBA float"), rgba);

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return MAX (MAX (rgba[0], rgba[1]), rgba[2]);

    case GIMP_HISTOGRAM_RED:
      return rgba[0];

    case GIMP_HISTOGRAM_GREEN:
      return rgba[1];

    case GIMP_HISTOGRAM_BLUE:
      return rgba[2];

    case GIMP_HISTOGRAM_ALPHA:
      return rgba[3];

    case GIMP_HISTOGRAM_RGB:
      return MIN (MIN (rgba[0], rgba[1]), rgba[2]);

    case GIMP_HISTOGRAM_LUMINANCE: // XXX: does this macro expect linear or
                                   //  gamma TRC?
      return GIMP_RGB_LUMINANCE (color->r, color->g, color->b);
    }

  return 0.0;
}

void
gimp_levels_config_adjust_by_colors (GimpLevelsConfig     *config,
                                     GimpHistogramChannel  channel,
                                     const GimpRGB        *black,
                                     const GimpRGB        *gray,
                                     const GimpRGB        *white)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  if (black)
    {
      config->low_input[channel] = gimp_levels_config_input_from_color (channel,
                                                                        black);
      g_object_notify (G_OBJECT (config), "low-input");
    }


  if (white)
    {
      config->high_input[channel] = gimp_levels_config_input_from_color (channel,
                                                                         white);
      g_object_notify (G_OBJECT (config), "high-input");
    }

  if (gray)
    {
      gdouble input;
      gdouble range;
      gdouble inten;
      gdouble out_light;
      gdouble lightness;

      /* Calculate lightness value */
      lightness = GIMP_RGB_LUMINANCE (gray->r, gray->g, gray->b);

      input = gimp_levels_config_input_from_color (channel, gray);

      range = config->high_input[channel] - config->low_input[channel];
      if (range <= 0)
        goto out;

      input -= config->low_input[channel];
      if (input < 0)
        goto out;

      /* Normalize input and lightness */
      inten = input / range;
      out_light = lightness / range;

      /* See bug 622054: picking pure black or white as gamma doesn't
       * work. But we cannot compare to 0.0 or 1.0 because cpus and
       * compilers are shit. If you try to check out_light using
       * printf() it will give exact 0.0 or 1.0 anyway, probably
       * because the generated code is different and out_light doesn't
       * live in a register. That must be why the cpu/compiler mafia
       * invented epsilon and defined this shit to be the programmer's
       * responsibility.
       */
      if (out_light <= 0.0001 || out_light >= 0.9999)
        goto out;

      /* Map selected color to corresponding lightness */
      config->gamma[channel] = log (inten) / log (out_light);
      config->gamma[channel] = CLAMP (config->gamma[channel], 0.1, 10.0);
      g_object_notify (G_OBJECT (config), "gamma");
    }

 out:
  g_object_thaw_notify (G_OBJECT (config));
}

GimpCurvesConfig *
gimp_levels_config_to_curves_config (GimpLevelsConfig *config)
{
  GimpCurvesConfig     *curves;
  GimpHistogramChannel  channel;

  g_return_val_if_fail (GIMP_IS_LEVELS_CONFIG (config), NULL);

  curves = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      GimpCurve  *curve    = curves->curve[channel];
      const gint  n_points = gimp_curve_get_n_points (curve);
      static const gint n  = 4;
      gint        point    = -1;
      gdouble     gamma    = config->gamma[channel];
      gdouble     delta_in;
      gdouble     delta_out;
      gdouble     x, y;

      /* clear the points set by default */
      gimp_curve_set_point (curve, 0, -1, -1);
      gimp_curve_set_point (curve, n_points - 1, -1, -1);

      delta_in  = config->high_input[channel] - config->low_input[channel];
      delta_out = config->high_output[channel] - config->low_output[channel];

      x = config->low_input[channel];
      y = config->low_output[channel];

      point = CLAMP (n_points * x, point + 1, n_points - 1 - n);
      gimp_curve_set_point (curve, point, x, y);

      if (delta_out != 0 && gamma != 1.0)
        {
          /* The Levels tool performs gamma adjustment, which is a
           * power law, while the Curves tool uses cubic Bézier
           * curves. Here we try to approximate this gamma adjustment
           * with a Bézier curve with 5 control points. Two of them
           * must be (low_input, low_output) and (high_input,
           * high_output), so we need to add 3 more control points in
           * the middle.
           */
          gint i;

          if (gamma > 1)
            {
              /* Case no. 1: γ > 1
               *
               * The curve should look like a horizontal
               * parabola. Since its curvature is greatest when x is
               * small, we add more control points there, so the
               * approximation is more accurate. I decided to set the
               * length of the consecutive segments to x₀, γ⋅x₀, γ²⋅x₀
               * and γ³⋅x₀ and I saw that the curves looked
               * good. Still, this is completely arbitrary.
               */
              gdouble dx = 0;
              gdouble x0;

              for (i = 0; i < n; ++i)
                dx = dx * gamma + 1;
              x0 = delta_in / dx;

              dx = 0;
              for (i = 1; i < n; ++i)
                {
                  dx = dx * gamma + x0;
                  x = config->low_input[channel] + dx;
                  y = config->low_output[channel] + delta_out *
                      gimp_operation_levels_map_input (config, channel, x);
                  point = CLAMP (n_points * x, point + 1, n_points - 1 - n + i);
                  gimp_curve_set_point (curve, point, x, y);
                }
            }
          else
            {
              /* Case no. 2: γ < 1
               *
               * The curve is the same as the one in case no. 1,
               * observed through a reflexion along the y = x axis. So
               * if we invert γ and swap the x and y axes we can use
               * the same method as in case no. 1.
               */
              GimpLevelsConfig *config_inv;
              gdouble           dy = 0;
              gdouble           y0;
              const gdouble     gamma_inv = 1 / gamma;

              config_inv = gimp_config_duplicate (GIMP_CONFIG (config));

              config_inv->gamma[channel]       = gamma_inv;
              config_inv->low_input[channel]   = config->low_output[channel];
              config_inv->low_output[channel]  = config->low_input[channel];
              config_inv->high_input[channel]  = config->high_output[channel];
              config_inv->high_output[channel] = config->high_input[channel];

              for (i = 0; i < n; ++i)
                dy = dy * gamma_inv + 1;
              y0 = delta_out / dy;

              dy = 0;
              for (i = 1; i < n; ++i)
                {
                  dy = dy * gamma_inv + y0;
                  y = config->low_output[channel] + dy;
                  x = config->low_input[channel] + delta_in *
                      gimp_operation_levels_map_input (config_inv, channel, y);
                  point = CLAMP (n_points * x, point + 1, n_points - 1 - n + i);
                  gimp_curve_set_point (curve, point, x, y);
                }

              g_object_unref (config_inv);
            }
        }

      x = config->high_input[channel];
      y = config->high_output[channel];

      point = CLAMP (n_points * x, point + 1, n_points - 1);
      gimp_curve_set_point (curve, point, x, y);
    }

  return curves;
}

gboolean
gimp_levels_config_load_cruft (GimpLevelsConfig  *config,
                               GInputStream      *input,
                               GError           **error)
{
  GDataInputStream *data_input;
  gint              low_input[5];
  gint              high_input[5];
  gint              low_output[5];
  gint              high_output[5];
  gdouble           gamma[5];
  gchar            *line;
  gsize             line_len;
  gint              i;

  g_return_val_if_fail (GIMP_IS_LEVELS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  line_len = 64;
  line = g_data_input_stream_read_line (data_input, &line_len,
                                        NULL, error);
  if (! line)
    return FALSE;

  if (strcmp (line, "# GIMP Levels File") != 0)
    {
      g_set_error_literal (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                           _("not a GIMP Levels file"));
      g_object_unref (data_input);
      g_free (line);
      return FALSE;
    }

  g_free (line);

  for (i = 0; i < 5; i++)
    {
      gchar  float_buf[32];
      gchar *endp;
      gint   fields;

      line_len = 64;
      line = g_data_input_stream_read_line (data_input, &line_len,
                                            NULL, error);
      if (! line)
        {
          g_object_unref (data_input);
          return FALSE;
        }

      fields = sscanf (line, "%d %d %d %d %31s",
                       &low_input[i],
                       &high_input[i],
                       &low_output[i],
                       &high_output[i],
                       float_buf);

      g_free (line);

      if (fields != 5)
        goto error;

      gamma[i] = g_ascii_strtod (float_buf, &endp);

      if (endp == float_buf || errno == ERANGE)
        goto error;
    }

  g_object_unref (data_input);

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      config->low_input[i]   = low_input[i]   / 255.0;
      config->high_input[i]  = high_input[i]  / 255.0;
      config->low_output[i]  = low_output[i]  / 255.0;
      config->high_output[i] = high_output[i] / 255.0;
      config->gamma[i]       = gamma[i];
    }

  g_object_notify (G_OBJECT (config), "gamma");
  g_object_notify (G_OBJECT (config), "low-input");
  g_object_notify (G_OBJECT (config), "high-input");
  g_object_notify (G_OBJECT (config), "low-output");
  g_object_notify (G_OBJECT (config), "high-output");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;

 error:
  g_object_unref (data_input);

  g_set_error_literal (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                       _("parse error"));
  return FALSE;
}

gboolean
gimp_levels_config_save_cruft (GimpLevelsConfig  *config,
                               GOutputStream     *output,
                               GError           **error)
{
  GString *string;
  gint     i;

  g_return_val_if_fail (GIMP_IS_LEVELS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  string = g_string_new ("# GIMP Levels File\n");

  for (i = 0; i < 5; i++)
    {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

      g_string_append_printf (string,
                              "%d %d %d %d %s\n",
                              (gint) (config->low_input[i]   * 255.999),
                              (gint) (config->high_input[i]  * 255.999),
                              (gint) (config->low_output[i]  * 255.999),
                              (gint) (config->high_output[i] * 255.999),
                              g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                                               config->gamma[i]));
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_prefix_error (error, _("Writing levels file failed: "));
      g_string_free (string, TRUE);
      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}
