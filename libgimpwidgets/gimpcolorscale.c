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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"

#define SHADOW  1


static void     gimp_color_scale_class_init (GimpColorScaleClass *klass);
static void     gimp_color_scale_init       (GimpColorScale      *scale);

static void     gimp_color_scale_destroy       (GtkObject        *object);
static void     gimp_color_scale_size_allocate (GtkWidget        *widget,
                                                GtkAllocation    *allocation);
static gboolean gimp_color_scale_expose        (GtkWidget        *widget,
                                                GdkEventExpose   *event);

static void     gimp_color_scale_render        (GimpColorScale   *scale);


static GtkScaleClass * parent_class = NULL;


GType
gimp_color_scale_get_type (void)
{
  static GType scale_type = 0;

  if (! scale_type)
    {
      static const GTypeInfo scale_info =
      {
        sizeof (GimpColorScaleClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_color_scale_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpColorScale),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_color_scale_init,
      };

      scale_type = g_type_register_static (GTK_TYPE_SCALE,
                                           "GimpColorScale", &scale_info, 0);
    }

  return scale_type;
}

static void
gimp_color_scale_class_init (GimpColorScaleClass *klass)
{
  GtkObjectClass  *object_class;
  GtkWidgetClass  *widget_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class  = GTK_OBJECT_CLASS (klass);
  widget_class  = GTK_WIDGET_CLASS (klass);

  object_class->destroy = gimp_color_scale_destroy;

  widget_class->size_allocate = gimp_color_scale_size_allocate;
  widget_class->expose_event  = gimp_color_scale_expose;
}

static void
gimp_color_scale_init (GimpColorScale *scale)
{
  GtkRange *range = GTK_RANGE (scale);

  range->slider_size_fixed = TRUE;
  range->orientation       = GTK_ORIENTATION_HORIZONTAL;
  /* FIXME: range should be flippable */

  scale->channel = GIMP_COLOR_SELECTOR_HUE;

  gimp_rgba_set (&scale->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&scale->rgb, &scale->hsv);
}

static void
gimp_color_scale_destroy (GtkObject *object)
{
  GimpColorScale *scale;

  scale = GIMP_COLOR_SCALE (object);

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
  GimpColorScale *scale;
  GtkRange       *range;
  gint            focus = 0;
  gint            scale_width;
  gint            scale_height;

  scale = GIMP_COLOR_SCALE (widget);
  range = GTK_RANGE (widget);

  if (GTK_WIDGET_CAN_FOCUS (widget))
    {
      gint focus_padding = 0;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus,
                            "focus-padding",    &focus_padding,
                            NULL);
      focus += focus_padding;
    }

  range->min_slider_size =
    MIN (allocation->width, allocation->height) - 2 * focus;
  range->min_slider_size /= 2;  

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  scale_width  = range->range_rect.width  - 2 * (focus + SHADOW);
  scale_height = range->range_rect.height - 2 * (focus + SHADOW);

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

      gimp_color_scale_render (scale);
    }
}

static gboolean
gimp_color_scale_expose (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GimpColorScale *scale;
  GtkRange       *range;
  GdkRectangle    expose_area;	/* Relative to widget->allocation */
  GdkRectangle    area;
  gint            focus = 0;
  gint            slider_size;
  gint            x, y;
  gint            w, h;

  scale = GIMP_COLOR_SCALE (widget);
  
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
      gboolean sensitive;

      area.x += widget->allocation.x;
      area.y += widget->allocation.y;

      sensitive = GTK_WIDGET_STATE (widget) != GTK_STATE_INSENSITIVE;

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
                                        x + SHADOW + slider_size,
                                        y + SHADOW + 1,
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
                                        x + SHADOW + 1,
                                        y + SHADOW + slider_size,
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

  if (! range->adjustment)
    return FALSE;

  switch (range->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      area.x      = widget->allocation.x + range->slider_start;
      area.y      = y + SHADOW;
      area.width  = 2 * slider_size + 1;
      area.height = h - 2 * SHADOW;
      break;

    case GTK_ORIENTATION_VERTICAL:
      area.x      = x + SHADOW;
      area.y      = widget->allocation.y + range->slider_start;
      area.width  = w - 2 * SHADOW;
      area.height = 2 * slider_size + 1;
      break;
    }

  if (gdk_rectangle_intersect (&event->area, &area, &expose_area))
    {
      switch (range->orientation)
        {
        case GTK_ORIENTATION_HORIZONTAL:
          gdk_gc_set_clip_rectangle (widget->style->black_gc, &expose_area);

          for (w = area.width, x = area.x, y = area.y;
               w > 0; w -= 2, x++, y++)
            gdk_draw_line (widget->window, widget->style->black_gc,
                           x, y, x + w - 1, y);

          gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
          
          gdk_gc_set_clip_rectangle (widget->style->white_gc, &expose_area);

          for (w = area.width, x = area.x, y = area.y + area.height - 1;
               w > 0; w -= 2, x++, y--)
            gdk_draw_line (widget->window, widget->style->white_gc,
                           x, y, x + w - 1, y);

          gdk_gc_set_clip_rectangle (widget->style->white_gc, NULL);

          break;

        case GTK_ORIENTATION_VERTICAL:
          /* FIXME: unimplemented */
          break;
        }
    }

  return FALSE;
}

GtkWidget *
gimp_color_scale_new (GtkOrientation            orientation,
                      GimpColorSelectorChannel  channel,
                      const GimpRGB            *rgb,
                      const GimpHSV            *hsv)
{
  GimpColorScale *scale;

  scale = g_object_new (GIMP_TYPE_COLOR_SCALE, NULL);

  GTK_RANGE (scale)->orientation = orientation;

  scale->channel = channel;
  scale->rgb     = *rgb;
  scale->hsv     = *hsv;

  return GTK_WIDGET (scale);
}

void
gimp_color_scale_set_channel (GimpColorScale           *scale,
                              GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));

  if (channel != scale->channel)
    {
      scale->channel = channel;

      gimp_color_scale_render (scale);
      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

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

  gimp_color_scale_render (scale);
  gtk_widget_queue_draw (GTK_WIDGET (scale));
}

static void
gimp_color_scale_render (GimpColorScale *scale)
{
  GimpRGB   rgb;
  GimpHSV   hsv;
  guint     x, y;
  gdouble  *channel_value = NULL; /* shut up compiler */
  gboolean  to_rgb        = FALSE;
  guchar   *buf;
  guchar   *d;

  if ((buf = scale->buf) == NULL)
    return;

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

  if (GTK_RANGE (scale)->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      d = buf;

      for (x = 0; x < scale->width; x++)
        {
          *channel_value = (gdouble) x / (gdouble) (scale->width - 1);

          if (to_rgb)
            gimp_hsv_to_rgb (&hsv, &rgb);

          gimp_rgb_get_uchar (&rgb, d, d + 1, d + 2);
          d += 3;
        }

      d = buf + scale->rowstride;

      for (y = 1; y < scale->height; y++)
        {
          memcpy (d, buf, scale->rowstride);
          d += scale->rowstride;
        }
    }
  else
    {
      guchar r, g, b;

      for (y = 0; y < scale->height; y++)
        {
          d = buf;

          *channel_value = (gdouble) y / (gdouble) (scale->height - 1);

          if (to_rgb)
            gimp_hsv_to_rgb (&hsv, &rgb);

          gimp_rgb_get_uchar (&rgb, &r, &g, &b);

          for (x = 0; x < scale->width; x++)
            {
              *d++ = r;
              *d++ = g;
              *d++ = b;
            }

          buf += scale->rowstride;
        }
    }
}
