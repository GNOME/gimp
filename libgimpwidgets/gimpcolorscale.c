/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscale.c
 * Copyright (C) 2002-2010  Sven Neumann <sven@ligma.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacairo-utils.h"
#include "ligmacolorscale.h"
#include "ligmawidgetsutils.h"


/**
 * SECTION: ligmacolorscale
 * @title: LigmaColorScale
 * @short_description: Fancy colored sliders.
 *
 * Fancy colored sliders.
 **/


enum
{
  PROP_0,
  PROP_CHANNEL
};


typedef struct _LigmaLCH  LigmaLCH;

struct _LigmaLCH
{
  gdouble l, c, h, a;
};


struct _LigmaColorScalePrivate
{
  LigmaColorConfig          *config;
  LigmaColorTransform       *transform;
  guchar                    oog_color[3];

  LigmaColorSelectorChannel  channel;
  LigmaRGB                   rgb;
  LigmaHSV                   hsv;

  guchar                   *buf;
  guint                     width;
  guint                     height;
  guint                     rowstride;

  gboolean                  needs_render;
};

#define GET_PRIVATE(obj) (((LigmaColorScale *) (obj))->priv)


static void     ligma_color_scale_dispose             (GObject          *object);
static void     ligma_color_scale_finalize            (GObject          *object);
static void     ligma_color_scale_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);
static void     ligma_color_scale_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);

static void     ligma_color_scale_size_allocate       (GtkWidget        *widget,
                                                      GtkAllocation    *allocation);
static gboolean ligma_color_scale_draw                (GtkWidget        *widget,
                                                      cairo_t          *cr);

static void     ligma_color_scale_render              (LigmaColorScale   *scale);
static void     ligma_color_scale_render_alpha        (LigmaColorScale   *scale);

static void     ligma_color_scale_create_transform    (LigmaColorScale   *scale);
static void     ligma_color_scale_destroy_transform   (LigmaColorScale   *scale);
static void     ligma_color_scale_notify_config       (LigmaColorConfig  *config,
                                                      const GParamSpec *pspec,
                                                      LigmaColorScale   *scale);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorScale, ligma_color_scale, GTK_TYPE_SCALE)

#define parent_class ligma_color_scale_parent_class

static const Babl *fish_rgb_to_lch = NULL;
static const Babl *fish_lch_to_rgb = NULL;


static void
ligma_color_scale_class_init (LigmaColorScaleClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose       = ligma_color_scale_dispose;
  object_class->finalize      = ligma_color_scale_finalize;
  object_class->get_property  = ligma_color_scale_get_property;
  object_class->set_property  = ligma_color_scale_set_property;

  widget_class->size_allocate = ligma_color_scale_size_allocate;
  widget_class->draw          = ligma_color_scale_draw;

  /**
   * LigmaColorScale:channel:
   *
   * The channel which is edited by the color scale.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("channel",
                                                      "Channel",
                                                      "The channel which is edited by the color scale",
                                                      LIGMA_TYPE_COLOR_SELECTOR_CHANNEL,
                                                      LIGMA_COLOR_SELECTOR_VALUE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "LigmaColorScale");

  fish_rgb_to_lch = babl_fish (babl_format ("R'G'B'A double"),
                               babl_format ("CIE LCH(ab) double"));
  fish_lch_to_rgb = babl_fish (babl_format ("CIE LCH(ab) double"),
                               babl_format ("R'G'B' double"));
}

static void
ligma_color_scale_init (LigmaColorScale *scale)
{
  LigmaColorScalePrivate *priv;
  GtkRange              *range = GTK_RANGE (scale);
  GtkCssProvider        *css;

  scale->priv = ligma_color_scale_get_instance_private (scale);

  priv = scale->priv;

  gtk_widget_set_can_focus (GTK_WIDGET (scale), TRUE);

  gtk_range_set_slider_size_fixed (range, TRUE);
  gtk_range_set_flippable (GTK_RANGE (scale), TRUE);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  priv->channel      = LIGMA_COLOR_SELECTOR_VALUE;
  priv->needs_render = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (range),
                                  GTK_ORIENTATION_HORIZONTAL);

  ligma_rgba_set (&priv->rgb, 0.0, 0.0, 0.0, 1.0);
  ligma_rgb_to_hsv (&priv->rgb, &priv->hsv);

  ligma_widget_track_monitor (GTK_WIDGET (scale),
                             G_CALLBACK (ligma_color_scale_destroy_transform),
                             NULL, NULL);

  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css,
                                   "LigmaColorScale {"
                                   "  padding:    2px 12px 2px 12px;"
                                   "  min-width:  24px;"
                                   "  min-height: 24px;"
                                   "}\n"
                                   "LigmaColorScale contents trough {"
                                   "  min-width:  20px;"
                                   "  min-height: 20px;"
                                   "}\n"
                                   "LigmaColorScale contents trough slider {"
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
ligma_color_scale_dispose (GObject *object)
{
  LigmaColorScale *scale = LIGMA_COLOR_SCALE (object);

  ligma_color_scale_set_color_config (scale, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_scale_finalize (GObject *object)
{
  LigmaColorScalePrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->buf, g_free);
  priv->width     = 0;
  priv->height    = 0;
  priv->rowstride = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_scale_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaColorScalePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, priv->channel);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_scale_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaColorScale *scale = LIGMA_COLOR_SCALE (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      ligma_color_scale_set_channel (scale, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_scale_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  LigmaColorScalePrivate *priv  = GET_PRIVATE (widget);
  GtkRange              *range = GTK_RANGE (widget);
  GdkRectangle           range_rect;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gtk_range_get_range_rect (range, &range_rect);

  if (range_rect.width  != priv->width ||
      range_rect.height != priv->height)
    {
      priv->width  = range_rect.width;
      priv->height = range_rect.height;

      priv->rowstride = priv->width * 4;

      g_free (priv->buf);
      priv->buf = g_new (guchar, priv->rowstride * priv->height);

      priv->needs_render = TRUE;
    }
}

static gboolean
ligma_color_scale_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaColorScale        *scale     = LIGMA_COLOR_SCALE (widget);
  LigmaColorScalePrivate *priv      = GET_PRIVATE (widget);
  GtkRange              *range     = GTK_RANGE (widget);
  GtkStyleContext       *context   = gtk_widget_get_style_context (widget);
  GdkRectangle           range_rect;
  GdkRectangle           area      = { 0, };
  cairo_surface_t       *buffer;
  gint                   slider_start;
  gint                   slider_end;
  gint                   slider_mid;
  gint                   slider_size;

  if (! priv->buf)
    return FALSE;

  gtk_range_get_range_rect (range, &range_rect);
  gtk_range_get_slider_range (range, &slider_start, &slider_end);

  slider_mid = slider_start + (slider_end - slider_start) / 2;
  slider_size = 6;

  if (priv->needs_render)
    {
      ligma_color_scale_render (scale);

      priv->needs_render = FALSE;
    }

  if (! priv->transform)
    ligma_color_scale_create_transform (scale);

  if (priv->transform)
    {
      const Babl *format = babl_format ("cairo-RGB24");
      guchar     *buf    = g_new (guchar, priv->rowstride * priv->height);
      guchar     *src    = priv->buf;
      guchar     *dest   = buf;
      guint       i;

      for (i = 0; i < priv->height; i++)
        {
          ligma_color_transform_process_pixels (priv->transform,
                                               format, src,
                                               format, dest,
                                               priv->width);

          src  += priv->rowstride;
          dest += priv->rowstride;
        }

      buffer = cairo_image_surface_create_for_data (buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    priv->width,
                                                    priv->height,
                                                    priv->rowstride);
      cairo_surface_set_user_data (buffer, NULL,
                                   buf, (cairo_destroy_func_t) g_free);
    }
  else
    {
      buffer = cairo_image_surface_create_for_data (priv->buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    priv->width,
                                                    priv->height,
                                                    priv->rowstride);
    }

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
 * ligma_color_scale_new:
 * @orientation: the scale's orientation (horizontal or vertical)
 * @channel: the scale's color channel
 *
 * Creates a new #LigmaColorScale widget.
 *
 * Returns: a new #LigmaColorScale widget
 **/
GtkWidget *
ligma_color_scale_new (GtkOrientation            orientation,
                      LigmaColorSelectorChannel  channel)
{
  LigmaColorScale *scale = g_object_new (LIGMA_TYPE_COLOR_SCALE,
                                        "orientation", orientation,
                                        "channel",     channel,
                                        NULL);

  gtk_range_set_flippable (GTK_RANGE (scale),
                           orientation == GTK_ORIENTATION_HORIZONTAL);

  return GTK_WIDGET (scale);
}

/**
 * ligma_color_scale_set_channel:
 * @scale: a #LigmaColorScale widget
 * @channel: the new color channel
 *
 * Changes the color channel displayed by the @scale.
 **/
void
ligma_color_scale_set_channel (LigmaColorScale           *scale,
                              LigmaColorSelectorChannel  channel)
{
  LigmaColorScalePrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SCALE (scale));

  priv = GET_PRIVATE (scale);

  if (channel != priv->channel)
    {
      priv->channel = channel;

      priv->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (scale));

      g_object_notify (G_OBJECT (scale), "channel");
    }
}

/**
 * ligma_color_scale_set_color:
 * @scale: a #LigmaColorScale widget
 * @rgb: the new color as #LigmaRGB
 * @hsv: the new color as #LigmaHSV
 *
 * Changes the color value of the @scale.
 **/
void
ligma_color_scale_set_color (LigmaColorScale *scale,
                            const LigmaRGB  *rgb,
                            const LigmaHSV  *hsv)
{
  LigmaColorScalePrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SCALE (scale));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  priv = GET_PRIVATE (scale);

  priv->rgb = *rgb;
  priv->hsv = *hsv;

  priv->needs_render = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (scale));
}

/**
 * ligma_color_scale_set_color_config:
 * @scale:  a #LigmaColorScale widget.
 * @config: a #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this color scale.
 *
 * Since: 2.10
 */
void
ligma_color_scale_set_color_config (LigmaColorScale  *scale,
                                   LigmaColorConfig *config)
{
  LigmaColorScalePrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SCALE (scale));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (scale);

  if (config != priv->config)
    {
      if (priv->config)
        {
          g_signal_handlers_disconnect_by_func (priv->config,
                                                ligma_color_scale_notify_config,
                                                scale);

          ligma_color_scale_destroy_transform (scale);
        }

      g_set_object (&priv->config, config);

      if (priv->config)
        {
          g_signal_connect (priv->config, "notify",
                            G_CALLBACK (ligma_color_scale_notify_config),
                            scale);

          ligma_color_scale_notify_config (priv->config, NULL, scale);
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
ligma_color_scale_render (LigmaColorScale *scale)
{
  LigmaColorScalePrivate *priv  = GET_PRIVATE (scale);
  GtkRange              *range = GTK_RANGE (scale);
  LigmaRGB                rgb;
  LigmaHSV                hsv;
  LigmaLCH                lch;
  gint                   multiplier = 1;
  guint                  x, y;
  gdouble               *channel_value = NULL; /* shut up compiler */
  gboolean               from_hsv      = FALSE;
  gboolean               from_lch      = FALSE;
  gboolean               invert;
  guchar                *buf;
  guchar                *d;

  if ((buf = priv->buf) == NULL)
    return;

  if (priv->channel == LIGMA_COLOR_SELECTOR_ALPHA)
    {
      ligma_color_scale_render_alpha (scale);
      return;
    }

  rgb = priv->rgb;
  hsv = priv->hsv;
  babl_process (fish_rgb_to_lch, &rgb, &lch, 1);

  switch (priv->channel)
    {
    case LIGMA_COLOR_SELECTOR_HUE:        channel_value = &hsv.h; break;
    case LIGMA_COLOR_SELECTOR_SATURATION: channel_value = &hsv.s; break;
    case LIGMA_COLOR_SELECTOR_VALUE:      channel_value = &hsv.v; break;

    case LIGMA_COLOR_SELECTOR_RED:        channel_value = &rgb.r; break;
    case LIGMA_COLOR_SELECTOR_GREEN:      channel_value = &rgb.g; break;
    case LIGMA_COLOR_SELECTOR_BLUE:       channel_value = &rgb.b; break;
    case LIGMA_COLOR_SELECTOR_ALPHA:      channel_value = &rgb.a; break;

    case LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS: channel_value = &lch.l; break;
    case LIGMA_COLOR_SELECTOR_LCH_CHROMA:    channel_value = &lch.c; break;
    case LIGMA_COLOR_SELECTOR_LCH_HUE:       channel_value = &lch.h; break;
    }

  switch (priv->channel)
    {
    case LIGMA_COLOR_SELECTOR_HUE:
    case LIGMA_COLOR_SELECTOR_SATURATION:
    case LIGMA_COLOR_SELECTOR_VALUE:
      from_hsv = TRUE;
      break;

    case LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS:
      multiplier = 100;
      from_lch = TRUE;
      break;
    case LIGMA_COLOR_SELECTOR_LCH_CHROMA:
      multiplier = 200;
      from_lch = TRUE;
      break;
    case LIGMA_COLOR_SELECTOR_LCH_HUE:
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
      for (x = 0, d = buf; x < priv->width; x++, d += 4)
        {
          gdouble value = (gdouble) x * multiplier / (gdouble) (priv->width - 1);
          guchar  r, g, b;

          if (invert)
            value = multiplier - value;

          *channel_value = value;

          if (from_hsv)
            ligma_hsv_to_rgb (&hsv, &rgb);
          else if (from_lch)
            babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

          if (rgb.r < 0.0 || rgb.r > 1.0 ||
              rgb.g < 0.0 || rgb.g > 1.0 ||
              rgb.b < 0.0 || rgb.b > 1.0)
            {
              r = priv->oog_color[0];
              g = priv->oog_color[1];
              b = priv->oog_color[2];
            }
          else
            {
              ligma_rgb_get_uchar (&rgb, &r, &g, &b);
            }

          LIGMA_CAIRO_RGB24_SET_PIXEL (d, r, g, b);
        }

      d = buf + priv->rowstride;
      for (y = 1; y < priv->height; y++)
        {
          memcpy (d, buf, priv->rowstride);
          d += priv->rowstride;
        }
      break;

    case GTK_ORIENTATION_VERTICAL:
      for (y = 0; y < priv->height; y++)
        {
          gdouble value = (gdouble) y * multiplier / (gdouble) (priv->height - 1);
          guchar  r, g, b;

          if (invert)
            value = multiplier - value;

          *channel_value = value;

          if (from_hsv)
            ligma_hsv_to_rgb (&hsv, &rgb);
          else if (from_lch)
            babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

          if (rgb.r < 0.0 || rgb.r > 1.0 ||
              rgb.g < 0.0 || rgb.g > 1.0 ||
              rgb.b < 0.0 || rgb.b > 1.0)
            {
              r = priv->oog_color[0];
              g = priv->oog_color[1];
              b = priv->oog_color[2];
            }
          else
            {
              ligma_rgb_get_uchar (&rgb, &r, &g, &b);
            }

          for (x = 0, d = buf; x < priv->width; x++, d += 4)
            {
              LIGMA_CAIRO_RGB24_SET_PIXEL (d, r, g, b);
            }

          buf += priv->rowstride;
        }
      break;
    }
}

static void
ligma_color_scale_render_alpha (LigmaColorScale *scale)
{
  LigmaColorScalePrivate *priv  = GET_PRIVATE (scale);
  GtkRange              *range = GTK_RANGE (scale);
  LigmaRGB                rgb;
  gboolean               invert;
  gdouble                a;
  guint                  x, y;
  guchar                *buf;
  guchar                *d, *l;

  invert = should_invert (range);

  buf = priv->buf;
  rgb = priv->rgb;

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      {
        guchar  *light;
        guchar  *dark;

        light = buf;
        /* this won't work correctly for very thin scales */
        dark  = (priv->height > LIGMA_CHECK_SIZE_SM ?
                 buf + LIGMA_CHECK_SIZE_SM * priv->rowstride : light);

        for (x = 0, d = light, l = dark; x < priv->width; x++)
          {
            if ((x % LIGMA_CHECK_SIZE_SM) == 0)
              {
                guchar *t;

                t = d;
                d = l;
                l = t;
              }

            a = (gdouble) x / (gdouble) (priv->width - 1);

            if (invert)
              a = 1.0 - a;

            LIGMA_CAIRO_RGB24_SET_PIXEL (l,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.r - LIGMA_CHECK_LIGHT) * a) * 255.999,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.g - LIGMA_CHECK_LIGHT) * a) * 255.999,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.b - LIGMA_CHECK_LIGHT) * a) * 255.999);
            l += 4;

            LIGMA_CAIRO_RGB24_SET_PIXEL (d,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.r - LIGMA_CHECK_DARK) * a) * 255.999,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.g - LIGMA_CHECK_DARK) * a) * 255.999,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.b - LIGMA_CHECK_DARK) * a) * 255.999);
            d += 4;
          }

        for (y = 0, d = buf; y < priv->height; y++, d += priv->rowstride)
          {
            if (y == 0 || y == LIGMA_CHECK_SIZE_SM)
              continue;

            if ((y / LIGMA_CHECK_SIZE_SM) & 1)
              memcpy (d, dark, priv->rowstride);
            else
              memcpy (d, light, priv->rowstride);
          }
      }
      break;

    case GTK_ORIENTATION_VERTICAL:
      {
        guchar  light[4] = {0xff, 0xff, 0xff, 0xff};
        guchar  dark[4] = {0xff, 0xff, 0xff, 0xff};

        for (y = 0, d = buf; y < priv->height; y++, d += priv->rowstride)
          {
            a = (gdouble) y / (gdouble) (priv->height - 1);

            if (invert)
              a = 1.0 - a;

            LIGMA_CAIRO_RGB24_SET_PIXEL (light,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.r - LIGMA_CHECK_LIGHT) * a) * 255.999,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.g - LIGMA_CHECK_LIGHT) * a) * 255.999,
                                        (LIGMA_CHECK_LIGHT +
                                         (rgb.b - LIGMA_CHECK_LIGHT) * a) * 255.999);

            LIGMA_CAIRO_RGB24_SET_PIXEL (dark,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.r - LIGMA_CHECK_DARK) * a) * 255.999,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.g - LIGMA_CHECK_DARK) * a) * 255.999,
                                        (LIGMA_CHECK_DARK +
                                         (rgb.b - LIGMA_CHECK_DARK) * a) * 255.999);

            for (x = 0, l = d; x < priv->width; x++, l += 4)
              {
                if (((x / LIGMA_CHECK_SIZE_SM) ^ (y / LIGMA_CHECK_SIZE_SM)) & 1)
                  {
                    l[0] = light[0];
                    l[1] = light[1];
                    l[2] = light[2];
                    l[3] = light[3];
                  }
                else
                  {
                    l[0] = dark[0];
                    l[1] = dark[1];
                    l[2] = dark[2];
                    l[3] = dark[3];
                  }
              }
          }
      }
      break;
    }
}

static void
ligma_color_scale_create_transform (LigmaColorScale *scale)
{
  LigmaColorScalePrivate *priv = GET_PRIVATE (scale);

  if (priv->config)
    {
      static LigmaColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = ligma_color_profile_new_rgb_srgb ();

      priv->transform = ligma_widget_get_color_transform (GTK_WIDGET (scale),
                                                         priv->config,
                                                         profile,
                                                         format,
                                                         format,
                                                         NULL,
                                                         LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                         FALSE);
    }
}

static void
ligma_color_scale_destroy_transform (LigmaColorScale *scale)
{
  LigmaColorScalePrivate *priv = GET_PRIVATE (scale);

  if (priv->transform)
    {
      g_object_unref (priv->transform);
      priv->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (scale));
}

static void
ligma_color_scale_notify_config (LigmaColorConfig  *config,
                                const GParamSpec *pspec,
                                LigmaColorScale   *scale)
{
  LigmaColorScalePrivate *priv = GET_PRIVATE (scale);
  LigmaRGB                color;

  ligma_color_scale_destroy_transform (scale);

  ligma_color_config_get_out_of_gamut_color (config, &color);
  ligma_rgb_get_uchar (&color,
                      priv->oog_color,
                      priv->oog_color + 1,
                      priv->oog_color + 2);
  priv->needs_render = TRUE;
}
