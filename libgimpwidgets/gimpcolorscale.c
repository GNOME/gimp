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

  scale = GIMP_COLOR_SCALE (widget);

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (widget->allocation.width  != scale->width ||
      widget->allocation.height != scale->height)
    {
      scale->width  = widget->allocation.width;
      scale->height = widget->allocation.height;

      scale->rowstride = (scale->width * 3 + 3) & ~0x3;

      g_free (scale->buf);
      scale->buf = g_new (guchar, scale->rowstride * scale->height);

      GTK_RANGE (scale)->min_slider_size = MIN (scale->width, scale->height);

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
  gint            focus         = 0;
  gint            focus_padding = 0;
  guchar         *buf;

  scale = GIMP_COLOR_SCALE (widget);
  
  if (! scale->buf || ! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  if (GTK_WIDGET_CAN_FOCUS (widget))
    gtk_widget_style_get (widget,
                          "focus-line-width", &focus,
                          "focus-padding",    &focus_padding,
                          NULL);
  focus += focus_padding;

  range = GTK_RANGE (scale);
  
  expose_area = event->area;
  expose_area.x -= widget->allocation.x;
  expose_area.y -= widget->allocation.y;

  /* This is ugly as it relies heavily on GTK+ internals, but I see no
     other way to force the range to recalculate its layout. Might
     break if GtkRange internals change.
   */
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, &widget->allocation);
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (gdk_rectangle_intersect (&expose_area, &range->range_rect, &area))
    {
      buf = scale->buf + area.y * scale->rowstride + area.x * 3;

      area.x += widget->allocation.x;
      area.y += widget->allocation.y;

      gdk_draw_rgb_image_dithalign (widget->window,
                                    widget->style->black_gc,
                                    area.x + focus,
                                    area.y + focus,
                                    area.width  - 2 * focus,
                                    area.height - 2 * focus,
                                    GDK_RGB_DITHER_MAX,
                                    buf,
                                    scale->rowstride,
                                    event->area.x,
                                    event->area.y);
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
      area.x      = range->slider_start;
      area.y      = range->range_rect.y + focus;
      area.width  = range->slider_end - range->slider_start;
      area.height = range->range_rect.height - 2 * focus;
      break;

    case GTK_ORIENTATION_VERTICAL:
      area.x      = range->range_rect.x + focus;
      area.y      = range->slider_start;
      area.width  = range->range_rect.width - 2 * focus;
      area.height = range->slider_end - range->slider_start;
      break;
    }

  if (gdk_rectangle_intersect (&expose_area, &area, &area))
    {
      area.x += widget->allocation.x;
      area.y += widget->allocation.y;
  
      gtk_paint_box (widget->style, widget->window, GTK_WIDGET_STATE (widget),
                     GTK_SHADOW_ETCHED_OUT, &event->area, widget, "slider",
                     area.x, area.y, area.width, area.height);
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
}

static void
gimp_color_scale_render (GimpColorScale *scale)
{
  guint   y;
  guchar *buf;

  if ((buf = scale->buf) == NULL)
    return;

  for (y = 0; y < scale->height; y++)
    {
      guchar *d = buf;
      guint   w = scale->width;

      while (w--)
        {  
          d[0] = w << 1;
          d[1] = w << 1;
          d[2] = w << 1;
          d += 3;
        }

      buf += scale->rowstride;
    }
}
