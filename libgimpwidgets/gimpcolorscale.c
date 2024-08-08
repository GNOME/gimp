/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscale.c
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcairo-utils.h"
#include "gimpcolorscale.h"
#include "gimpwidgetsutils.h"


/**
 * SECTION: gimpcolorscale
 * @title: GimpColorScale
 * @short_description: Fancy colored sliders.
 *
 * Fancy colored sliders.
 **/


enum
{
  PROP_0,
  PROP_CHANNEL
};


struct _GimpColorScale
{
  GtkScale                  parent_instance;

  GimpColorConfig          *config;
  guchar                    oog_color[3];
  const Babl               *format;

  GimpColorSelectorChannel  channel;
  GeglColor                *color;

  guchar                   *buf;
  guint                     width;
  guint                     height;
  guint                     rowstride;

  gboolean                  needs_render;
};


static void     gimp_color_scale_dispose             (GObject          *object);
static void     gimp_color_scale_finalize            (GObject          *object);
static void     gimp_color_scale_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);
static void     gimp_color_scale_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);

static void     gimp_color_scale_size_allocate       (GtkWidget        *widget,
                                                      GtkAllocation    *allocation);
static gboolean gimp_color_scale_draw                (GtkWidget        *widget,
                                                      cairo_t          *cr);

static void     gimp_color_scale_render              (GimpColorScale   *scale);
static void     gimp_color_scale_render_alpha        (GimpColorScale   *scale);

static void     gimp_color_scale_notify_config       (GimpColorConfig  *config,
                                                      const GParamSpec *pspec,
                                                      GimpColorScale   *scale);


G_DEFINE_TYPE (GimpColorScale, gimp_color_scale, GTK_TYPE_SCALE)

#define parent_class gimp_color_scale_parent_class

static const Babl *fish_lch_to_rgb   = NULL;
static const Babl *fish_hsv_to_rgb   = NULL;
static const Babl *fish_rgb_to_cairo = NULL;


static void
gimp_color_scale_class_init (GimpColorScaleClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose       = gimp_color_scale_dispose;
  object_class->finalize      = gimp_color_scale_finalize;
  object_class->get_property  = gimp_color_scale_get_property;
  object_class->set_property  = gimp_color_scale_set_property;

  widget_class->size_allocate = gimp_color_scale_size_allocate;
  widget_class->draw          = gimp_color_scale_draw;

  /**
   * GimpColorScale:channel:
   *
   * The channel which is edited by the color scale.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("channel",
                                                      "Channel",
                                                      "The channel which is edited by the color scale",
                                                      GIMP_TYPE_COLOR_SELECTOR_CHANNEL,
                                                      GIMP_COLOR_SELECTOR_VALUE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "GimpColorScale");

  fish_lch_to_rgb   = babl_fish (babl_format ("CIE LCH(ab) float"),
                                 babl_format ("R'G'B' double"));
  fish_hsv_to_rgb   = babl_fish (babl_format ("HSV float"),
                                 babl_format ("R'G'B' double"));
  fish_rgb_to_cairo = babl_fish (babl_format ("R'G'B' u8"),
                                 babl_format ("cairo-RGB24"));
}

static void
gimp_color_scale_init (GimpColorScale *scale)
{
  GtkRange       *range = GTK_RANGE (scale);
  GtkCssProvider *css;

  gtk_widget_set_can_focus (GTK_WIDGET (scale), TRUE);

  gtk_range_set_slider_size_fixed (range, TRUE);
  gtk_range_set_flippable (GTK_RANGE (scale), TRUE);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  scale->channel      = GIMP_COLOR_SELECTOR_VALUE;
  scale->needs_render = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (range),
                                  GTK_ORIENTATION_HORIZONTAL);

  scale->color = gegl_color_new ("black");

  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css,
                                   "GimpColorScale {"
                                   "  padding:    2px 12px 2px 12px;"
                                   "  min-width:  24px;"
                                   "  min-height: 24px;"
                                   "}\n"
                                   "GimpColorScale contents trough {"
                                   "  min-width:  20px;"
                                   "  min-height: 20px;"
                                   "}\n"
                                   "GimpColorScale contents trough slider {"
                                   "  min-width:  12px;"
                                   "  min-height: 12px;"
                                   "  margin:     -6px -6px -6px -6px;"
                                   "}",
                                   -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (scale)),
                                  GTK_STYLE_PROVIDER (css),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
}

static void
gimp_color_scale_dispose (GObject *object)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (object);

  gimp_color_scale_set_color_config (scale, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_scale_finalize (GObject *object)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (object);

  g_clear_pointer (&scale->buf, g_free);
  scale->width     = 0;
  scale->height    = 0;
  scale->rowstride = 0;
  g_object_unref (scale->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_scale_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, scale->channel);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_scale_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      gimp_color_scale_set_channel (scale, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_scale_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (widget);
  GtkRange       *range = GTK_RANGE (widget);
  GdkRectangle    range_rect;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gtk_range_get_range_rect (range, &range_rect);

  if (range_rect.width  != scale->width ||
      range_rect.height != scale->height)
    {
      scale->width  = range_rect.width;
      scale->height = range_rect.height;

      /* TODO: we should move to CAIRO_FORMAT_RGBA128F. */
      scale->rowstride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, scale->width);

      g_free (scale->buf);
      scale->buf = g_new (guchar, 3 * scale->width * scale->height);

      scale->needs_render = TRUE;
    }
}

static gboolean
gimp_color_scale_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpColorScale  *scale   = GIMP_COLOR_SCALE (widget);
  GtkRange        *range   = GTK_RANGE (widget);
  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GdkRectangle     range_rect;
  GdkRectangle     area    = { 0, };
  cairo_surface_t *buffer;
  guchar          *buf     = NULL;
  guchar          *src;
  guchar          *dest;
  gint             slider_start;
  gint             slider_end;
  gint             slider_mid;
  gint             slider_size;
  const Babl      *render_space;

  if (! scale->buf)
    return FALSE;

  gtk_range_get_range_rect (range, &range_rect);
  gtk_range_get_slider_range (range, &slider_start, &slider_end);

  slider_mid = slider_start + (slider_end - slider_start) / 2;
  slider_size = 6;

  if (scale->needs_render)
    {
      gimp_color_scale_render (scale);

      scale->needs_render = FALSE;
    }

  render_space = gimp_widget_get_render_space (widget, scale->config);
  fish_rgb_to_cairo = babl_fish (babl_format_with_space ("R'G'B' u8", scale->format),
                                 babl_format_with_space ("cairo-RGB24", render_space)),

  src  = scale->buf;
  buf  = g_new (guchar, scale->rowstride * scale->height);
  dest = buf;
  /* We convert per line because the cairo rowstride may be bigger than the
   * real contents.
   */
  for (gint i = 0; i < scale->height; i++)
    {
      babl_process (fish_rgb_to_cairo, src, dest, scale->width);
      src  += 3 * scale->width;
      dest += scale->rowstride;
    }

  buffer = cairo_image_surface_create_for_data (buf,
                                                CAIRO_FORMAT_RGB24,
                                                scale->width,
                                                scale->height,
                                                scale->rowstride);
  cairo_surface_set_user_data (buffer, NULL, buf, (cairo_destroy_func_t) g_free);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      cairo_set_source_surface (cr, buffer,
                                range_rect.x, range_rect.y);
      break;

    case GTK_ORIENTATION_VERTICAL:
      cairo_set_source_surface (cr, buffer,
                                range_rect.x, range_rect.y);
      break;
    }

  cairo_surface_destroy (buffer);

  if (! gtk_widget_is_sensitive (widget))
    {
      static cairo_pattern_t *pattern = NULL;

      if (! pattern)
        {
          static const guchar  stipple[] = { 0,   255, 0, 0,
                                             255, 0,   0, 0 };
          cairo_surface_t     *surface;
          gint                 stride;

          stride = cairo_format_stride_for_width (CAIRO_FORMAT_A8, 2);

          surface = cairo_image_surface_create_for_data ((guchar *) stipple,
                                                         CAIRO_FORMAT_A8,
                                                         2, 2, stride);
          pattern = cairo_pattern_create_for_surface (surface);
          cairo_surface_destroy (surface);

          cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
        }

      cairo_mask (cr, pattern);
    }
  else
    {
      cairo_paint (cr);
    }

  if (gtk_widget_has_focus (widget))
    gtk_render_focus (context, cr,
                      0, 0,
                      gtk_widget_get_allocated_width  (widget),
                      gtk_widget_get_allocated_height (widget));

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      area.x      = slider_mid - slider_size;
      area.y      = range_rect.y;
      area.width  = 2 * slider_size;
      area.height = range_rect.height;
      break;

    case GTK_ORIENTATION_VERTICAL:
      area.x      = range_rect.x;
      area.y      = slider_mid - slider_size;
      area.width  = range_rect.width;
      area.height = 2 * slider_size;
      break;
    }

  if (gtk_widget_is_sensitive (widget))
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  else
    cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      cairo_move_to (cr, area.x, area.y);
      cairo_line_to (cr, area.x + area.width, area.y);
      cairo_line_to (cr,
                     area.x + area.width / 2 + 0.5,
                     area.y + slider_size);
      break;

    case GTK_ORIENTATION_VERTICAL:
      cairo_move_to (cr, area.x, area.y);
      cairo_line_to (cr, area.x, area.y + area.height);
      cairo_line_to (cr,
                     area.x + slider_size,
                     area.y + area.height / 2 + 0.5);
      break;
    }

  cairo_close_path (cr);
  cairo_fill (cr);

  if (gtk_widget_is_sensitive (widget))
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  else
    cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      cairo_move_to (cr, area.x, area.y + area.height);
      cairo_line_to (cr, area.x + area.width, area.y + area.height);
      cairo_line_to (cr,
                     area.x + area.width / 2 + 0.5,
                     area.y + area.height - slider_size);
      break;

    case GTK_ORIENTATION_VERTICAL:
      cairo_move_to (cr, area.x + area.width, area.y);
      cairo_line_to (cr, area.x + area.width, area.y + area.height);
      cairo_line_to (cr,
                     area.x + area.width - slider_size,
                     area.y + area.height / 2 + 0.5);
      break;
    }

  cairo_close_path (cr);
  cairo_fill (cr);

  return FALSE;
}

/**
 * gimp_color_scale_new:
 * @orientation: the scale's orientation (horizontal or vertical)
 * @channel: the scale's color channel
 *
 * Creates a new #GimpColorScale widget.
 *
 * Returns: a new #GimpColorScale widget
 **/
GtkWidget *
gimp_color_scale_new (GtkOrientation            orientation,
                      GimpColorSelectorChannel  channel)
{
  GimpColorScale *scale = g_object_new (GIMP_TYPE_COLOR_SCALE,
                                        "orientation", orientation,
                                        "channel",     channel,
                                        NULL);

  gtk_range_set_flippable (GTK_RANGE (scale),
                           orientation == GTK_ORIENTATION_HORIZONTAL);

  return GTK_WIDGET (scale);
}

/**
 * gimp_color_scale_set_format:
 * @scale:  a #GimpColorScale widget
 * @format: the Babl format represented by @scale.
 *
 * Changes the color format displayed by the @scale.
 **/
void
gimp_color_scale_set_format (GimpColorScale *scale,
                             const Babl     *format)
{
  if (scale->format != format)
    {
      scale->format = format;
      fish_lch_to_rgb = babl_fish (babl_format ("CIE LCH(ab) float"),
                                   babl_format_with_space ("R'G'B' double", format));
      fish_hsv_to_rgb = babl_fish (babl_format_with_space ("HSV float", format),
                                   babl_format_with_space ("R'G'B' double", format));
      scale->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

/**
 * gimp_color_scale_set_channel:
 * @scale: a #GimpColorScale widget
 * @channel: the new color channel
 *
 * Changes the color channel displayed by the @scale.
 **/
void
gimp_color_scale_set_channel (GimpColorScale           *scale,
                              GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));

  if (channel != scale->channel)
    {
      scale->channel = channel;

      scale->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (scale));

      g_object_notify (G_OBJECT (scale), "channel");
    }
}

/**
 * gimp_color_scale_set_color:
 * @scale: a #GimpColorScale widget
 * @color: the new color.
 *
 * Changes the color value of the @scale.
 **/
void
gimp_color_scale_set_color (GimpColorScale *scale,
                            GeglColor      *color)
{
  GeglColor *old_color;

  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));
  g_return_if_fail (GEGL_IS_COLOR (color));

  old_color = scale->color;
  scale->color = gegl_color_duplicate (color);

  if (! gimp_color_is_perceptually_identical (old_color, scale->color))
    {
      scale->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }

  g_object_unref (old_color);
}

/**
 * gimp_color_scale_set_color_config:
 * @scale:  a #GimpColorScale widget.
 * @config: a #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color scale.
 *
 * Since: 2.10
 */
void
gimp_color_scale_set_color_config (GimpColorScale  *scale,
                                   GimpColorConfig *config)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  if (config != scale->config)
    {
      if (scale->config)
        {
          g_signal_handlers_disconnect_by_func (scale->config,
                                                gimp_color_scale_notify_config,
                                                scale);
        }

      g_set_object (&scale->config, config);

      if (scale->config)
        {
          g_signal_connect (scale->config, "notify",
                            G_CALLBACK (gimp_color_scale_notify_config),
                            scale);

          gimp_color_scale_notify_config (scale->config, NULL, scale);
        }
    }
}


/* as in gtkrange.c */
static gboolean
should_invert (GtkRange *range)
{
  gboolean inverted  = gtk_range_get_inverted (range);
  gboolean flippable = gtk_range_get_flippable (range);

  if (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)) ==
      GTK_ORIENTATION_HORIZONTAL)
    {
      return
        (inverted && !flippable) ||
        (inverted && flippable &&
         gtk_widget_get_direction (GTK_WIDGET (range)) == GTK_TEXT_DIR_LTR) ||
        (!inverted && flippable &&
         gtk_widget_get_direction (GTK_WIDGET (range)) == GTK_TEXT_DIR_RTL);
    }
  else
    {
      return inverted;
    }
}

static void
gimp_color_scale_render (GimpColorScale *scale)
{
  GtkRange *range = GTK_RANGE (scale);
  gdouble   rgb[4];
  gfloat    hsv[3];
  gfloat    lch[3];
  gint      multiplier = 1;
  guint     x, y;
  gdouble  *channel_value   = NULL;
  gfloat   *channel_value_f = NULL;
  gboolean  from_hsv        = FALSE;
  gboolean  from_lch        = FALSE;
  gboolean  invert;
  guchar   *buf;
  guchar   *d;

  if ((buf = scale->buf) == NULL)
    return;

  if (scale->channel == GIMP_COLOR_SELECTOR_ALPHA)
    {
      gimp_color_scale_render_alpha (scale);
      return;
    }

  gegl_color_get_pixel (scale->color, babl_format_with_space ("R'G'B'A double", scale->format), rgb);
  gegl_color_get_pixel (scale->color, babl_format_with_space ("HSV float", scale->format), hsv);
  gegl_color_get_pixel (scale->color, babl_format ("CIE LCH(ab) float"), lch);

  switch (scale->channel)
    {
    case GIMP_COLOR_SELECTOR_HUE:        channel_value_f = &hsv[0]; break;
    case GIMP_COLOR_SELECTOR_SATURATION: channel_value_f = &hsv[1]; break;
    case GIMP_COLOR_SELECTOR_VALUE:      channel_value_f = &hsv[2]; break;

    case GIMP_COLOR_SELECTOR_RED:        channel_value = &rgb[0]; break;
    case GIMP_COLOR_SELECTOR_GREEN:      channel_value = &rgb[1]; break;
    case GIMP_COLOR_SELECTOR_BLUE:       channel_value = &rgb[2]; break;
    case GIMP_COLOR_SELECTOR_ALPHA:      channel_value = &rgb[3]; break;

    case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS: channel_value_f = &lch[0]; break;
    case GIMP_COLOR_SELECTOR_LCH_CHROMA:    channel_value_f = &lch[1]; break;
    case GIMP_COLOR_SELECTOR_LCH_HUE:       channel_value_f = &lch[2]; break;
    }

  switch (scale->channel)
    {
    case GIMP_COLOR_SELECTOR_HUE:
    case GIMP_COLOR_SELECTOR_SATURATION:
    case GIMP_COLOR_SELECTOR_VALUE:
      from_hsv = TRUE;
      break;

    case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:
      multiplier = 100;
      from_lch = TRUE;
      break;
    case GIMP_COLOR_SELECTOR_LCH_CHROMA:
      multiplier = 200;
      from_lch = TRUE;
      break;
    case GIMP_COLOR_SELECTOR_LCH_HUE:
      multiplier = 360;
      from_lch = TRUE;
      break;

    default:
      break;
    }

  invert = should_invert (range);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      for (x = 0, d = buf; x < scale->width; x++, d += 3)
        {
          gdouble value = (gdouble) x * multiplier / (gdouble) (scale->width - 1);

          if (invert)
            value = multiplier - value;

          if (channel_value)
            *channel_value = value;
          else if (channel_value_f)
            *channel_value_f = (gfloat) value;

          if (from_hsv)
            babl_process (fish_hsv_to_rgb, &hsv, &rgb, 1);
          else if (from_lch)
            babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

          /* This is only checking if a color is within the sRGB gamut. I want
           * to check compared to the image's space (anySpace) or softproof
           * space. TODO.
           */
          if (rgb[0] < 0.0 || rgb[0] > 1.0 ||
              rgb[1] < 0.0 || rgb[1] > 1.0 ||
              rgb[2] < 0.0 || rgb[2] > 1.0)
            {
              d[0] = scale->oog_color[0];
              d[1] = scale->oog_color[1];
              d[2] = scale->oog_color[2];
            }
          else
            {
              d[0] = rgb[0] * 255;
              d[1] = rgb[1] * 255;
              d[2] = rgb[2] * 255;
            }
        }

      d = buf + scale->width * 3;
      for (y = 1; y < scale->height; y++)
        {
          memcpy (d, buf, scale->width * 3);
          d += scale->width * 3;
        }
      break;

    case GTK_ORIENTATION_VERTICAL:
      for (y = 0; y < scale->height; y++)
        {
          gdouble value = (gdouble) y * multiplier / (gdouble) (scale->height - 1);
          guchar  u8rgb[3];

          if (invert)
            value = multiplier - value;

          if (channel_value)
            *channel_value = value;
          else if (channel_value_f)
            *channel_value_f = (gfloat) value;

          if (from_hsv)
            babl_process (fish_hsv_to_rgb, &hsv, &rgb, 1);
          else if (from_lch)
            babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

          if (rgb[0] < 0.0 || rgb[0] > 1.0 ||
              rgb[1] < 0.0 || rgb[1] > 1.0 ||
              rgb[2] < 0.0 || rgb[2] > 1.0)
            {
              u8rgb[0] = scale->oog_color[0];
              u8rgb[1] = scale->oog_color[1];
              u8rgb[2] = scale->oog_color[2];
            }
          else
            {
              u8rgb[0] = rgb[0] * 255;
              u8rgb[1] = rgb[1] * 255;
              u8rgb[2] = rgb[2] * 255;
            }

          for (x = 0, d = buf; x < scale->width; x++, d += 3)
            {
              d[0] = u8rgb[0];
              d[1] = u8rgb[1];
              d[2] = u8rgb[2];
            }

          buf += scale->width * 3;
        }
      break;
    }
}

static void
gimp_color_scale_render_alpha (GimpColorScale *scale)
{
  GtkRange *range = GTK_RANGE (scale);
  gdouble   rgb[3];
  gboolean  invert;
  gdouble   a;
  guint     x, y;
  guchar   *buf;
  guchar   *d, *l;

  invert = should_invert (range);

  buf = scale->buf;
  gegl_color_get_pixel (scale->color,
                        babl_format_with_space ("R'G'B' double", scale->format),
                        rgb);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      {
        guchar  *light;
        guchar  *dark;

        light = buf;
        /* this won't work correctly for very thin scales */
        dark  = (scale->height > GIMP_CHECK_SIZE_SM ?
                 buf + GIMP_CHECK_SIZE_SM * 3 * scale->width : light);

        for (x = 0, d = light, l = dark; x < scale->width; x++)
          {
            if ((x % GIMP_CHECK_SIZE_SM) == 0)
              {
                guchar *t;

                t = d;
                d = l;
                l = t;
              }

            a = (gdouble) x / (gdouble) (scale->width - 1);

            if (invert)
              a = 1.0 - a;

            l[0] = (GIMP_CHECK_LIGHT + (rgb[0] - GIMP_CHECK_LIGHT) * a) * 255.999;
            l[1] = (GIMP_CHECK_LIGHT + (rgb[1] - GIMP_CHECK_LIGHT) * a) * 255.999;
            l[2] = (GIMP_CHECK_LIGHT + (rgb[2] - GIMP_CHECK_LIGHT) * a) * 255.999;
            l += 3;

            d[0] = (GIMP_CHECK_DARK + (rgb[0] - GIMP_CHECK_DARK) * a) * 255.999;
            d[1] = (GIMP_CHECK_DARK + (rgb[1] - GIMP_CHECK_DARK) * a) * 255.999;
            d[2] = (GIMP_CHECK_DARK + (rgb[2] - GIMP_CHECK_DARK) * a) * 255.999;
            d += 3;
          }

        for (y = 0, d = buf; y < scale->height; y++, d += 3 * scale->width)
          {
            if (y == 0 || y == GIMP_CHECK_SIZE_SM)
              continue;

            if ((y / GIMP_CHECK_SIZE_SM) & 1)
              memcpy (d, dark, 3 * scale->width);
            else
              memcpy (d, light, 3 * scale->width);
          }
      }
      break;

    case GTK_ORIENTATION_VERTICAL:
      {
        guchar  light[3] = {0xff, 0xff, 0xff};
        guchar  dark[3]  = {0xff, 0xff, 0xff};

        for (y = 0, d = buf; y < scale->height; y++, d += scale->width * 3)
          {
            a = (gdouble) y / (gdouble) (scale->height - 1);

            if (invert)
              a = 1.0 - a;

            light[0] = (GIMP_CHECK_LIGHT + (rgb[0] - GIMP_CHECK_LIGHT) * a) * 255.999;
            light[1] = (GIMP_CHECK_LIGHT + (rgb[1] - GIMP_CHECK_LIGHT) * a) * 255.999;
            light[2] = (GIMP_CHECK_LIGHT + (rgb[2] - GIMP_CHECK_LIGHT) * a) * 255.999;

            dark[0] = (GIMP_CHECK_DARK + (rgb[0] - GIMP_CHECK_DARK) * a) * 255.999;
            dark[1] = (GIMP_CHECK_DARK + (rgb[1] - GIMP_CHECK_DARK) * a) * 255.999;
            dark[2] = (GIMP_CHECK_DARK + (rgb[2] - GIMP_CHECK_DARK) * a) * 255.999;

            for (x = 0, l = d; x < scale->width; x++, l += 3)
              {
                if (((x / GIMP_CHECK_SIZE_SM) ^ (y / GIMP_CHECK_SIZE_SM)) & 1)
                  {
                    l[0] = light[0];
                    l[1] = light[1];
                    l[2] = light[2];
                  }
                else
                  {
                    l[0] = dark[0];
                    l[1] = dark[1];
                    l[2] = dark[2];
                  }
              }
          }
      }
      break;
    }
}

static void
gimp_color_scale_notify_config (GimpColorConfig  *config,
                                const GParamSpec *pspec,
                                GimpColorScale   *scale)
{
  GeglColor *color;

  color = gimp_color_config_get_out_of_gamut_color (config);
  /* TODO: shouldn't this be color-managed too, using the target space into
   * consideration?
   */
  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), scale->oog_color);
  scale->needs_render = TRUE;

  g_object_unref (color);
}
