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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"


/**
 * SECTION: gimpcolorscale
 * @title: GimpColorScale
 * @short_description: Fancy colored sliders.
 *
 * Fancy colored sliders.
 **/


static void     gimp_color_scale_destroy        (GtkObject       *object);
static void     gimp_color_scale_size_allocate  (GtkWidget       *widget,
                                                 GtkAllocation   *allocation);
static void     gimp_color_scale_state_changed  (GtkWidget       *widget,
                                                 GtkStateType     previous_state);
static gboolean gimp_color_scale_button_press   (GtkWidget       *widget,
                                                 GdkEventButton  *event);
static gboolean gimp_color_scale_button_release (GtkWidget       *widget,
                                                 GdkEventButton  *event);
static gboolean gimp_color_scale_expose         (GtkWidget       *widget,
                                                 GdkEventExpose  *event);

static void     gimp_color_scale_render         (GimpColorScale  *scale);
static void     gimp_color_scale_render_alpha   (GimpColorScale  *scale);
static void     gimp_color_scale_render_stipple (GimpColorScale  *scale);


G_DEFINE_TYPE (GimpColorScale, gimp_color_scale, GTK_TYPE_SCALE)

#define parent_class gimp_color_scale_parent_class


static void
gimp_color_scale_class_init (GimpColorScaleClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->destroy              = gimp_color_scale_destroy;

  widget_class->size_allocate        = gimp_color_scale_size_allocate;
  widget_class->state_changed        = gimp_color_scale_state_changed;
  widget_class->button_press_event   = gimp_color_scale_button_press;
  widget_class->button_release_event = gimp_color_scale_button_release;
  widget_class->expose_event         = gimp_color_scale_expose;
}

static void
gimp_color_scale_init (GimpColorScale *scale)
{
  GtkRange *range = GTK_RANGE (scale);

  gtk_range_set_slider_size_fixed (range, TRUE);
  /* range->update_policy     = GTK_UPDATE_DELAYED; */

  gtk_range_set_flippable (GTK_RANGE (scale), TRUE);

  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  scale->channel      = GIMP_COLOR_SELECTOR_VALUE;
  scale->needs_render = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (range),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_rgba_set (&scale->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&scale->rgb, &scale->hsv);
}

static void
gimp_color_scale_destroy (GtkObject *object)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (object);

  if (scale->buf)
    {
      g_free (scale->buf);
      scale->buf       = NULL;
      scale->width     = 0;
      scale->height    = 0;
      scale->rowstride = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_color_scale_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GimpColorScale *scale = GIMP_COLOR_SCALE (widget);
  GtkRange       *range = GTK_RANGE (widget);
  GdkRectangle    range_rect;
  gint            focus = 0;
  gint            trough_border;
  gint            scale_width;
  gint            scale_height;

  gtk_widget_style_get (widget,
                        "trough-border", &trough_border,
                        NULL);

  if (gtk_widget_get_can_focus (widget))
    {
      gint focus_padding = 0;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus,
                            "focus-padding",    &focus_padding,
                            NULL);
      focus += focus_padding;
    }

  gtk_range_set_min_slider_size (range,
                                 (MIN (allocation->width,
                                       allocation->height) - 2 * focus) / 2);

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gtk_range_get_range_rect (range, &range_rect);

  scale_width  = range_rect.width  - 2 * (focus + trough_border);
  scale_height = range_rect.height - 2 * (focus + trough_border);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      scale_width  -= gtk_range_get_min_slider_size (range) - 1;
      scale_height -= 2;
      break;

    case GTK_ORIENTATION_VERTICAL:
      scale_width  -= 2;
      scale_height -= gtk_range_get_min_slider_size (range) - 1;
      break;
    }

  if (scale_width != scale->width || scale_height != scale->height)
    {
      scale->width  = scale_width;
      scale->height = scale_height;

      scale->rowstride = (scale->width * 3 + 3) & ~0x3;

      g_free (scale->buf);
      scale->buf = g_new (guchar, scale->rowstride * scale->height);

      scale->needs_render = TRUE;
    }
}

static void
gimp_color_scale_state_changed (GtkWidget    *widget,
                                GtkStateType  previous_state)
{
  if (gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE ||
      previous_state == GTK_STATE_INSENSITIVE)
    {
      GIMP_COLOR_SCALE (widget)->needs_render = TRUE;
    }

  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, previous_state);
}

static gboolean
gimp_color_scale_button_press (GtkWidget      *widget,
                               GdkEventButton *event)
{
  if (event->button == 1)
    {
      GdkEventButton *my_event;
      gboolean        retval;

      my_event = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
      my_event->button = 2;

      retval = GTK_WIDGET_CLASS (parent_class)->button_press_event (widget,
                                                                    my_event);

      gdk_event_free ((GdkEvent *) my_event);

      return retval;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
gimp_color_scale_button_release (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  if (event->button == 1)
    {
      GdkEventButton *my_event;
      gboolean        retval;

      my_event = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
      my_event->button = 2;

      retval = GTK_WIDGET_CLASS (parent_class)->button_release_event (widget,
                                                                      my_event);

      gdk_event_free ((GdkEvent *) my_event);

      return retval;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_color_scale_expose (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GimpColorScale *scale  = GIMP_COLOR_SCALE (widget);
  GtkRange       *range  = GTK_RANGE (widget);
  GtkStyle       *style  = gtk_widget_get_style (widget);
  GdkWindow      *window = gtk_widget_get_window (widget);
  cairo_t        *cr;
  GtkAllocation   allocation;
  GdkRectangle    range_rect;
  GdkRectangle    expose_area;        /* Relative to widget->allocation */
  GdkRectangle    area;
  gint            focus = 0;
  gint            trough_border;
  gint            slider_start;
  gint            slider_size;
  gint            x, y;
  gint            w, h;

  if (! scale->buf || ! gtk_widget_is_drawable (widget))
    return FALSE;

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  gtk_widget_style_get (widget,
                        "trough-border", &trough_border,
                        NULL);

  if (gtk_widget_get_can_focus (widget))
    {
      gint focus_padding = 0;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus,
                            "focus-padding",    &focus_padding,
                            NULL);
      focus += focus_padding;
    }

  gtk_widget_get_allocation (widget, &allocation);

  gtk_range_get_range_rect (range, &range_rect);
  gtk_range_get_slider_range (range, &slider_start, NULL);

  x = allocation.x + range_rect.x + focus;
  y = allocation.y + range_rect.y + focus;
  w = range_rect.width  - 2 * focus;
  h = range_rect.height - 2 * focus;

  slider_size = gtk_range_get_min_slider_size (range) / 2;

  expose_area = event->area;
  expose_area.x -= allocation.x;
  expose_area.y -= allocation.y;

  if (gdk_rectangle_intersect (&expose_area, &range_rect, &area))
    {
      gboolean sensitive = gtk_widget_is_sensitive (widget);

      if (scale->needs_render)
        {
          gimp_color_scale_render (scale);

          if (! sensitive)
            gimp_color_scale_render_stipple (scale);

          scale->needs_render = FALSE;
        }

      area.x += allocation.x;
      area.y += allocation.y;

      gtk_paint_box (style, window,
                     sensitive ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE,
                     GTK_SHADOW_IN,
                     &area, widget, "trough",
                     x, y, w, h);

      gdk_gc_set_clip_rectangle (style->black_gc, &area);

      switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
        {
        case GTK_ORIENTATION_HORIZONTAL:
          gdk_draw_rgb_image_dithalign (window,
                                        style->black_gc,
                                        x + trough_border + slider_size,
                                        y + trough_border + 1,
                                        scale->width,
                                        scale->height,
                                        GDK_RGB_DITHER_MAX,
                                        scale->buf,
                                        scale->rowstride,
                                        0, 0);
          break;

        case GTK_ORIENTATION_VERTICAL:
          gdk_draw_rgb_image_dithalign (window,
                                        style->black_gc,
                                        x + trough_border + 1,
                                        y + trough_border + slider_size,
                                        scale->width,
                                        scale->height,
                                        GDK_RGB_DITHER_MAX,
                                        scale->buf,
                                        scale->rowstride,
                                        0, 0);
          break;
        }

      gdk_gc_set_clip_rectangle (style->black_gc, NULL);
    }

  if (gtk_widget_has_focus (widget))
    gtk_paint_focus (style, window, gtk_widget_get_state (widget),
                     &area, widget, "trough",
                     allocation.x + range_rect.x,
                     allocation.y + range_rect.y,
                     range_rect.width,
                     range_rect.height);

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      area.x      = allocation.x + slider_start;
      area.y      = y + trough_border;
      area.width  = 2 * slider_size + 1;
      area.height = h - 2 * trough_border;
      break;

    case GTK_ORIENTATION_VERTICAL:
      area.x      = x + trough_border;
      area.y      = allocation.y + slider_start;
      area.width  = w - 2 * trough_border;
      area.height = 2 * slider_size + 1;
      break;
    }

  if (gdk_rectangle_intersect (&event->area, &area, &expose_area))
    {
      if (gtk_widget_is_sensitive (widget))
        gdk_cairo_set_source_color (cr, &style->black);
      else
        gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_INSENSITIVE]);

      switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
        {
        case GTK_ORIENTATION_HORIZONTAL:
          cairo_move_to (cr, area.x, area.y);
          cairo_line_to (cr, area.x + area.width, area.y);
          cairo_line_to (cr, area.x + area.width / 2 + 0.5, area.y + area.height / 2 - 1);
          break;

        case GTK_ORIENTATION_VERTICAL:
          cairo_move_to (cr, area.x, area.y);
          cairo_line_to (cr, area.x, area.y + area.height);
          cairo_line_to (cr, area.x + area.width / 2 - 1, area.y + area.height / 2 + 0.5);
          break;
        }

      cairo_close_path (cr);
      cairo_fill (cr);

      if (gtk_widget_is_sensitive (widget))
        gdk_cairo_set_source_color (cr, &style->white);
      else
        gdk_cairo_set_source_color (cr, &style->light[GTK_STATE_INSENSITIVE]);

      switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
        {
        case GTK_ORIENTATION_HORIZONTAL:
          cairo_move_to (cr, area.x, area.y + area.height);
          cairo_line_to (cr, area.x + area.width, area.y + area.height);
          cairo_line_to (cr, area.x + area.width / 2 + 0.5, area.y + area.height / 2 + 1);
          break;

        case GTK_ORIENTATION_VERTICAL:
          cairo_move_to (cr, area.x + area.width, area.y);
          cairo_line_to (cr, area.x + area.width, area.y + area.height);
          cairo_line_to (cr, area.x + area.width / 2 + 1, area.y + area.height / 2 + 1);
          break;
        }

      cairo_close_path (cr);
      cairo_fill (cr);
    }

  cairo_destroy (cr);

  return FALSE;
}

/**
 * gimp_color_scale_new:
 * @orientation: the scale's orientation (horizontal or vertical)
 * @channel: the scale's color channel
 *
 * Creates a new #GimpColorScale widget.
 *
 * Return value: a new #GimpColorScale widget
 **/
GtkWidget *
gimp_color_scale_new (GtkOrientation            orientation,
                      GimpColorSelectorChannel  channel)
{
  GimpColorScale *scale = g_object_new (GIMP_TYPE_COLOR_SCALE,
                                        "orientation", orientation,
                                        NULL);

  scale->channel = channel;

  gtk_range_set_flippable (GTK_RANGE (scale),
                           orientation == GTK_ORIENTATION_HORIZONTAL);

  return GTK_WIDGET (scale);
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
    }
}

/**
 * gimp_color_scale_set_color:
 * @scale: a #GimpColorScale widget
 * @rgb: the new color as #GimpRGB
 * @hsv: the new color as #GimpHSV
 *
 * Changes the color value of the @scale.
 **/
void
gimp_color_scale_set_color (GimpColorScale *scale,
                            const GimpRGB  *rgb,
                            const GimpHSV  *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  scale->rgb = *rgb;
  scale->hsv = *hsv;

  scale->needs_render = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (scale));
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
  GimpRGB   rgb;
  GimpHSV   hsv;
  guint     x, y;
  gdouble  *channel_value = NULL; /* shut up compiler */
  gboolean  to_rgb        = FALSE;
  gboolean  invert;
  guchar   *buf;
  guchar   *d;
  guchar    r, g, b;

  if ((buf = scale->buf) == NULL)
    return;

  if (scale->channel == GIMP_COLOR_SELECTOR_ALPHA)
    {
      gimp_color_scale_render_alpha (scale);
      return;
    }

  rgb = scale->rgb;
  hsv = scale->hsv;

  switch (scale->channel)
    {
    case GIMP_COLOR_SELECTOR_HUE:        channel_value = &hsv.h; break;
    case GIMP_COLOR_SELECTOR_SATURATION: channel_value = &hsv.s; break;
    case GIMP_COLOR_SELECTOR_VALUE:      channel_value = &hsv.v; break;
    case GIMP_COLOR_SELECTOR_RED:        channel_value = &rgb.r; break;
    case GIMP_COLOR_SELECTOR_GREEN:      channel_value = &rgb.g; break;
    case GIMP_COLOR_SELECTOR_BLUE:       channel_value = &rgb.b; break;
    case GIMP_COLOR_SELECTOR_ALPHA:      channel_value = &rgb.a; break;
    }

  switch (scale->channel)
    {
    case GIMP_COLOR_SELECTOR_HUE:
    case GIMP_COLOR_SELECTOR_SATURATION:
    case GIMP_COLOR_SELECTOR_VALUE:
      to_rgb = TRUE;
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
          gdouble value = (gdouble) x / (gdouble) (scale->width - 1);

          if (invert)
            value = 1.0 - value;

          *channel_value = value;

          if (to_rgb)
            gimp_hsv_to_rgb (&hsv, &rgb);

          gimp_rgb_get_uchar (&rgb, d, d + 1, d + 2);
        }

      d = buf + scale->rowstride;
      for (y = 1; y < scale->height; y++)
        {
          memcpy (d, buf, scale->rowstride);
          d += scale->rowstride;
        }
      break;

    case GTK_ORIENTATION_VERTICAL:
      for (y = 0; y < scale->height; y++)
        {
          gdouble value = (gdouble) y / (gdouble) (scale->height - 1);

          if (invert)
            value = 1.0 - value;

          *channel_value = value;

          if (to_rgb)
            gimp_hsv_to_rgb (&hsv, &rgb);

          gimp_rgb_get_uchar (&rgb, &r, &g, &b);

          for (x = 0, d = buf; x < scale->width; x++, d += 3)
            {
              d[0] = r;
              d[1] = g;
              d[2] = b;
            }

          buf += scale->rowstride;
        }
      break;
    }
}

static void
gimp_color_scale_render_alpha (GimpColorScale *scale)
{
  GtkRange *range = GTK_RANGE (scale);
  GimpRGB   rgb;
  gboolean  invert;
  gdouble   a;
  guint     x, y;
  guchar   *buf;
  guchar   *d, *l;

  invert = should_invert (range);

  buf = scale->buf;
  rgb = scale->rgb;

  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (range)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      {
        guchar  *light;
        guchar  *dark;

        light = buf;
        /* this won't work correctly for very thin scales */
        dark  = (scale->height > GIMP_CHECK_SIZE_SM ?
                 buf + GIMP_CHECK_SIZE_SM * scale->rowstride : light);

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

            l[0] = (GIMP_CHECK_LIGHT +
                    (rgb.r - GIMP_CHECK_LIGHT) * a) * 255.999;
            l[1] = (GIMP_CHECK_LIGHT +
                    (rgb.g - GIMP_CHECK_LIGHT) * a) * 255.999;
            l[2] = (GIMP_CHECK_LIGHT +
                    (rgb.b - GIMP_CHECK_LIGHT) * a) * 255.999;
            l += 3;

            d[0] = (GIMP_CHECK_DARK +
                    (rgb.r - GIMP_CHECK_DARK) * a) * 255.999;
            d[1] = (GIMP_CHECK_DARK +
                    (rgb.g - GIMP_CHECK_DARK) * a) * 255.999;
            d[2] = (GIMP_CHECK_DARK +
                    (rgb.b - GIMP_CHECK_DARK) * a) * 255.999;
            d += 3;
          }

        for (y = 0, d = buf; y < scale->height; y++, d += scale->rowstride)
          {
            if (y == 0 || y == GIMP_CHECK_SIZE_SM)
              continue;

            if ((y / GIMP_CHECK_SIZE_SM) & 1)
              memcpy (d, dark, scale->rowstride);
            else
              memcpy (d, light, scale->rowstride);
          }
      }
      break;

    case GTK_ORIENTATION_VERTICAL:
      {
        guchar  light[3];
        guchar  dark[3];

        for (y = 0, d = buf; y < scale->height; y++, d += scale->rowstride)
          {
            a = (gdouble) y / (gdouble) (scale->height - 1);

            if (invert)
              a = 1.0 - a;

            light[0] = (GIMP_CHECK_LIGHT +
                        (rgb.r - GIMP_CHECK_LIGHT) * a) * 255.999;
            light[1] = (GIMP_CHECK_LIGHT +
                        (rgb.g - GIMP_CHECK_LIGHT) * a) * 255.999;
            light[2] = (GIMP_CHECK_LIGHT +
                        (rgb.b - GIMP_CHECK_LIGHT) * a) * 255.999;

            dark[0] = (GIMP_CHECK_DARK +
                       (rgb.r - GIMP_CHECK_DARK) * a) * 255.999;
            dark[1] = (GIMP_CHECK_DARK +
                       (rgb.g - GIMP_CHECK_DARK) * a) * 255.999;
            dark[2] = (GIMP_CHECK_DARK +
                       (rgb.b - GIMP_CHECK_DARK) * a) * 255.999;

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

/*
 * This could be integrated into the render functions which might be
 * slightly faster. But we trade speed for keeping the code simple.
 */
static void
gimp_color_scale_render_stipple (GimpColorScale *scale)
{
  GtkWidget *widget = GTK_WIDGET (scale);
  GtkStyle  *style  = gtk_widget_get_style (widget);
  guchar    *buf;
  guchar     insensitive[3];
  guint      x, y;

  if ((buf = scale->buf) == NULL)
    return;

  insensitive[0] = style->bg[GTK_STATE_INSENSITIVE].red   >> 8;
  insensitive[1] = style->bg[GTK_STATE_INSENSITIVE].green >> 8;
  insensitive[2] = style->bg[GTK_STATE_INSENSITIVE].blue  >> 8;

  for (y = 0; y < scale->height; y++, buf += scale->rowstride)
    {
      guchar *d = buf + 3 * (y % 2);

      for (x = 0; x < scale->width - (y % 2); x += 2, d += 6)
        {
          d[0] = insensitive[0];
          d[1] = insensitive[1];
          d[2] = insensitive[2];
        }
    }
}
