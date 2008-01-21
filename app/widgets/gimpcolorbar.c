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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpcolorbar.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_COLOR,
  PROP_CHANNEL
};


/*  local function prototypes  */

static void      gimp_color_bar_set_property (GObject        *object,
                                              guint           property_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);

static gboolean  gimp_color_bar_expose       (GtkWidget      *widget,
                                              GdkEventExpose *event);


G_DEFINE_TYPE (GimpColorBar, gimp_color_bar, GTK_TYPE_MISC)

#define parent_class gimp_color_bar_parent_class


static void
gimp_color_bar_class_init (GimpColorBarClass *klass)
{
  GObjectClass   *object_class;
  GtkWidgetClass *widget_class;
  GimpRGB         white = { 1.0, 1.0, 1.0, 1.0 };

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = gimp_color_bar_set_property;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      NULL, NULL,
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color",
                                                        NULL, NULL,
                                                        FALSE, &white,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("histogram-channel",
                                                      NULL, NULL,
                                                      GIMP_TYPE_HISTOGRAM_CHANNEL,
                                                      GIMP_HISTOGRAM_VALUE,
                                                      GIMP_PARAM_WRITABLE));

  widget_class->expose_event = gimp_color_bar_expose;
}

static void
gimp_color_bar_init (GimpColorBar *bar)
{
  GTK_WIDGET_SET_FLAGS (bar, GTK_NO_WINDOW);

  bar->orientation  = GTK_ORIENTATION_HORIZONTAL;
}


static void
gimp_color_bar_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpColorBar *bar = GIMP_COLOR_BAR (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      bar->orientation = g_value_get_enum (value);
      break;
    case PROP_COLOR:
      gimp_color_bar_set_color (bar, g_value_get_boxed (value));
      break;
    case PROP_CHANNEL:
      gimp_color_bar_set_channel (bar, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_color_bar_expose (GtkWidget      *widget,
                       GdkEventExpose *event)
{
  GimpColorBar *bar = GIMP_COLOR_BAR (widget);
  guchar       *buf;
  guchar       *b;
  gint          x, y;
  gint          width, height;
  gint          i, j;

  x = GTK_MISC (bar)->xpad;
  y = GTK_MISC (bar)->ypad;

  width  = widget->allocation.width  - 2 * x;
  height = widget->allocation.height - 2 * y;

  if (width < 1 || height < 1)
    return TRUE;

  buf = g_alloca (width * height * 3);

  switch (bar->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      for (i = 0, b = buf; i < width; i++, b += 3)
        {
          guchar *src = bar->buf + 3 * ((i * 256) / width);

          b[0] = src[0];
          b[1] = src[1];
          b[2] = src[2];
        }

      for (i = 1; i < height; i++)
        memcpy (buf + i * width * 3, buf, width * 3);

      break;

    case GTK_ORIENTATION_VERTICAL:
      for (i = 0, b = buf; i < height; i++, b += 3 * width)
        {
          guchar *src  = bar->buf + 3 * (255 - ((i * 256) / height));
          guchar *dest = b;

          for (j = 0; j < width; j++, dest += 3)
            {
              dest[0] = src[0];
              dest[1] = src[1];
              dest[2] = src[2];
            }
        }
      break;
    }

  gdk_draw_rgb_image (widget->window, widget->style->black_gc,
                      widget->allocation.x + x, widget->allocation.y + y,
                      width, height,
                      GDK_RGB_DITHER_NORMAL,
                      buf, 3 * width);

  return TRUE;
}

/**
 * gimp_color_bar_new:
 * @orientation: whether the bar should be oriented horizontally or
 *               vertically
 *
 * Creates a new #GimpColorBar widget.
 *
 * Return value: The new #GimpColorBar widget.
 **/
GtkWidget *
gimp_color_bar_new (GtkOrientation  orientation)
{
  return g_object_new (GIMP_TYPE_COLOR_BAR,
                       "orientation", orientation,
                       NULL);
}

/**
 * gimp_color_bar_set_color:
 * @bar:   a #GimpColorBar widget
 * @color: a #GimpRGB color
 *
 * Makes the @bar display a gradient from black (on the left or the
 * bottom), to the given @color (on the right or at the top).
 **/
void
gimp_color_bar_set_color (GimpColorBar  *bar,
                          const GimpRGB *color)
{
  guchar *buf;
  gint    i;

  g_return_if_fail (GIMP_IS_COLOR_BAR (bar));
  g_return_if_fail (color != NULL);

  for (i = 0, buf = bar->buf; i < 256; i++, buf += 3)
    {
      buf[0] = ROUND (color->r * (gdouble) i);
      buf[1] = ROUND (color->g * (gdouble) i);
      buf[2] = ROUND (color->b * (gdouble) i);
    }

  gtk_widget_queue_draw (GTK_WIDGET (bar));
}

/**
 * gimp_color_bar_set_channel:
 * @bar:     a #GimpColorBar widget
 * @channel: a #GimpHistogramChannel
 *
 * Convenience function that calls gimp_color_bar_set_color() with the
 * color that matches the @channel.
 **/
void
gimp_color_bar_set_channel (GimpColorBar         *bar,
                            GimpHistogramChannel  channel)
{
  GimpRGB  color = { 1.0, 1.0, 1.0, 1.0 };

  g_return_if_fail (GIMP_IS_COLOR_BAR (bar));

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:
    case GIMP_HISTOGRAM_RGB:
      gimp_rgb_set (&color, 1.0, 1.0, 1.0);
      break;
    case GIMP_HISTOGRAM_RED:
      gimp_rgb_set (&color, 1.0, 0.0, 0.0);
      break;
    case GIMP_HISTOGRAM_GREEN:
      gimp_rgb_set (&color, 0.0, 1.0, 0.0);
      break;
    case GIMP_HISTOGRAM_BLUE:
      gimp_rgb_set (&color, 0.0, 0.0, 1.0);
      break;
    }

  gimp_color_bar_set_color (bar, &color);
}

/**
 * gimp_color_bar_set_buffers:
 * @bar:   a #GimpColorBar widget
 * @red:   an array of 256 values
 * @green: an array of 256 values
 * @blue:  an array of 256 values
 *
 * This function gives full control over the colors displayed by the
 * @bar widget. The 3 arrays can for example be taken from a #Levels
 * or a #Curves struct.
 **/
void
gimp_color_bar_set_buffers (GimpColorBar *bar,
                            const guchar *red,
                            const guchar *green,
                            const guchar *blue)
{
  guchar *buf;
  gint    i;

  g_return_if_fail (GIMP_IS_COLOR_BAR (bar));
  g_return_if_fail (red != NULL);
  g_return_if_fail (green != NULL);
  g_return_if_fail (blue != NULL);

  for (i = 0, buf = bar->buf; i < 256; i++, buf += 3)
    {
      buf[0] = red[i];
      buf[1] = green[i];
      buf[2] = blue[i];
    }

  gtk_widget_queue_draw (GTK_WIDGET (bar));
}
