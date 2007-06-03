/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscale.c
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"


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

  GTK_SCALE (scale)->draw_value = FALSE;

  range->slider_size_fixed = TRUE;
  range->orientation       = GTK_ORIENTATION_HORIZONTAL;
  range->flippable         = TRUE;
  /* range->update_policy     = GTK_UPDATE_DELAYED; */

  scale->channel      = GIMP_COLOR_SELECTOR_VALUE;
  scale->needs_render = TRUE;

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
  gint            focus = 0;
  gint            trough_border;
  gint            scale_width;
  gint            scale_height;

  gtk_widget_style_get (widget,
                        "trough-border", &trough_border,
                        NULL);

  if (GTK_WIDGET_CAN_FOCUS (widget))
    {
      gint focus_padding = 0;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus,
                            "focus-padding",    &focus_padding,
                            NULL);
      focus += focus_padding;
    }

  range->min_slider_size = (MIN (widget->requisition.width,
                                 widget->requisition.height) - 2 * focus) / 2;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  scale_width  = range->range_rect.width  - 2 * (focus + trough_border);
  scale_height = range->range_rect.height - 2 * (focus + trough_border);

  switch (range->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      scale_width  -= range->min_slider_size - 1;
      scale_height -= 2;
      break;

    case GTK_ORIENTATION_VERTICAL:
      scale_width  -= 2;
      scale_height -= range->min_slider_size - 1;
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
  if (widget->state == GTK_STATE_INSENSITIVE ||
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
  GimpColorScale *scale = GIMP_COLOR_SCALE (widget);
  GtkRange       *range;
  GdkRectangle    expose_area;        /* Relative to widget->allocation */
  GdkRectangle    area;
  gint            focus = 0;
  gint            trough_border;
  gint            slider_size;
  gint            x, y;
  gint            w, h;

  if (! scale->buf || ! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  range = GTK_RANGE (scale);

  /* This is ugly as it relies heavily on GTK+ internals, but I see no
   * other way to force the range to recalculate its layout. Might
   * break if GtkRange internals change.
   */
  if (range->need_recalc)
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);
      GTK_WIDGET_CLASS (parent_class)->size_allocate (widget,
                                                      &widget->allocation);
      GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    }

  gtk_widget_style_get (widget,
                        "trough-border", &trough_border,
                        NULL);

  if (GTK_WIDGET_CAN_FOCUS (widget))
    {
      gint focus_padding = 0;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus,
                            "focus-padding",    &focus_padding,
                            NULL);
      focus += focus_padding;
    }

  x = widget->allocation.x + range->range_rect.x + focus;
  y = widget->allocation.y + range->range_rect.y + focus;
  w = range->range_rect.width  - 2 * focus;
  h = range->range_rect.height - 2 * focus;

  slider_size = range->min_slider_size / 2;

  expose_area = event->area;
  expose_area.x -= widget->allocation.x;
  expose_area.y -= widget->allocation.y;

  if (gdk_rectangle_intersect (&expose_area, &range->range_rect, &area))
    {
      gboolean sensitive = (GTK_WIDGET_STATE (widget) != GTK_STATE_INSENSITIVE);

      if (scale->needs_render)
        {
          gimp_color_scale_render (scale);

          if (! sensitive)
            gimp_color_scale_render_stipple (scale);

          scale->needs_render = FALSE;
        }

      area.x += widget->allocation.x;
      area.y += widget->allocation.y;

      gtk_paint_box (widget->style, widget->window,
                     sensitive ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE,
                     GTK_SHADOW_IN,
                     &area, widget, "trough",
                     x, y, w, h);

      gdk_gc_set_clip_rectangle (widget->style->black_gc, &area);

      switch (range->orientation)
        {
        case GTK_ORIENTATION_HORIZONTAL:
          gdk_draw_rgb_image_dithalign (widget->window,
                                        widget->style->black_gc,
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
          gdk_draw_rgb_image_dithalign (widget->window,
                                        widget->style->black_gc,
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

      gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
    }

  if (GTK_WIDGET_IS_SENSITIVE (widget) && GTK_WIDGET_HAS_FOCUS (range))
    gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
                     &area, widget, "trough",
                     widget->allocation.x + range->range_rect.x,
                     widget->allocation.y + range->range_rect.y,
                     range->range_rect.width,
                     range->range_rect.height);

  switch (range->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      area.x      = widget->allocation.x + range->slider_start;
      area.y      = y + trough_border;
      area.width  = 2 * slider_size + 1;
      area.height = h - 2 * trough_border;
      break;

    case GTK_ORIENTATION_VERTICAL:
      area.x      = x + trough_border;
      area.y      = widget->allocation.y + range->slider_start;
      area.width  = w - 2 * trough_border;
      area.height = 2 * slider_size + 1;
      break;
    }

  if (gdk_rectangle_intersect (&event->area, &area, &expose_area))
    {
      GdkGC *gc;

      gc = (GTK_WIDGET_IS_SENSITIVE (widget) ?
            widget->style->black_gc :
            widget->style->dark_gc[GTK_STATE_INSENSITIVE]);

      gdk_gc_set_clip_rectangle (gc, &expose_area);
      switch (range->orientation)
        {
        case GTK_ORIENTATION_HORIZONTAL:
          for (w = area.width, x = area.x, y = area.y;
               w > 0; w -= 2, x++, y++)
            gdk_draw_line (widget->window, gc, x, y, x + w - 1, y);
          break;
        case GTK_ORIENTATION_VERTICAL:
          for (h = area.height, x = area.x, y = area.y;
               h > 0; h -= 2, x++, y++)
            gdk_draw_line (widget->window, gc, x, y, x, y + h - 1);
          break;
        }
      gdk_gc_set_clip_rectangle (gc, NULL);

      gc = (GTK_WIDGET_IS_SENSITIVE (widget) ?
            widget->style->white_gc :
            widget->style->light_gc[GTK_STATE_INSENSITIVE]);

      gdk_gc_set_clip_rectangle (gc, &expose_area);
      switch (range->orientation)
        {
        case GTK_ORIENTATION_HORIZONTAL:
          for (w = area.width, x = area.x, y = area.y + area.height - 1;
               w > 0; w -= 2, x++, y--)
            gdk_draw_line (widget->window, gc, x, y, x + w - 1, y);
          break;
        case GTK_ORIENTATION_VERTICAL:
          for (h = area.height, x = area.x + area.width - 1, y = area.y;
               h > 0; h -= 2, x--, y++)
            gdk_draw_line (widget->window, gc, x, y, x, y + h - 1);
          break;
        }
      gdk_gc_set_clip_rectangle (gc, NULL);
    }

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
  GimpColorScale *scale;
  GtkRange       *range;

  scale = g_object_new (GIMP_TYPE_COLOR_SCALE, NULL);

  scale->channel = channel;

  range = GTK_RANGE (scale);
  range->orientation = orientation;
  range->flippable   = (orientation == GTK_ORIENTATION_HORIZONTAL);

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
  if (range->orientation == GTK_ORIENTATION_HORIZONTAL)
    return
      (range->inverted && !range->flippable) ||
      (range->inverted && range->flippable &&
       gtk_widget_get_direction (GTK_WIDGET (range)) == GTK_TEXT_DIR_LTR) ||
      (!range->inverted && range->flippable &&
       gtk_widget_get_direction (GTK_WIDGET (range)) == GTK_TEXT_DIR_RTL);
  else
    return range->inverted;
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

  switch (range->orientation)
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

  switch (range->orientation)
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
  guchar    *buf;
  guchar     insensitive[3];
  guint      x, y;

  if ((buf = scale->buf) == NULL)
    return;

  insensitive[0] = widget->style->bg[GTK_STATE_INSENSITIVE].red   >> 8;
  insensitive[1] = widget->style->bg[GTK_STATE_INSENSITIVE].green >> 8;
  insensitive[2] = widget->style->bg[GTK_STATE_INSENSITIVE].blue  >> 8;

  for (y = 0; y < scale->height; y++, buf += scale->rowstride)
    {
      guchar *d = buf + 3 * (y % 2);

      for (x = 0; x < scale->width; x += 2, d += 6)
        {
          d[0] = insensitive[0];
          d[1] = insensitive[1];
          d[2] = insensitive[2];
        }
    }
}
