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


static GtkDrawingAreaClass * parent_class = NULL;


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

      scale_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                           "GimpColorScale", 
                                           &scale_info, 0);
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
  scale->orientation = GTK_ORIENTATION_HORIZONTAL;
  scale->channel     = GIMP_COLOR_SELECTOR_HUE;

  gimp_rgba_set (&scale->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&scale->rgb, &scale->hsv);
}

static void
gimp_color_scale_destroy (GtkObject *object)
{
  GimpColorScale *scale;

  scale = GIMP_COLOR_SCALE (object);

  if (scale->adj)
    gimp_color_scale_set_adjustment (scale, NULL);

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

      gimp_color_scale_render (scale);
    }
}

static gboolean
gimp_color_scale_expose (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GimpColorScale *scale;
  guchar         *buf;

  scale = GIMP_COLOR_SCALE (widget);
  
  if (! scale->buf || ! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  buf = scale->buf + event->area.y * scale->rowstride + event->area.x * 3;

  gdk_draw_rgb_image_dithalign (widget->window,
                                widget->style->black_gc,
                                event->area.x,
                                event->area.y,
                                event->area.width,
                                event->area.height,
                                GDK_RGB_DITHER_MAX,
                                buf,
                                scale->rowstride,
                                event->area.x,
                                event->area.y);

  return FALSE;
}

/**
 * gimp_color_scale_new:
 * 
 * Creates a new #GimpColorScale widget.
 * 
 * Returns: Pointer to the new #GimpColorScale widget.
 **/
GtkWidget *
gimp_color_scale_new (GtkOrientation            orientation,
                      GimpColorSelectorChannel  channel,
                      const GimpRGB            *rgb,
                      const GimpHSV            *hsv)
{
  GimpColorScale *scale;

  scale = g_object_new (GIMP_TYPE_COLOR_SCALE, NULL);

  scale->orientation = orientation;
  scale->channel     = channel;
  scale->rgb         = *rgb;
  scale->hsv         = *hsv;

  return GTK_WIDGET (scale);
}

void
gimp_color_scale_set_orientation (GimpColorScale  *scale,
                                  GtkOrientation   orientation)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));

  if (orientation != scale->orientation)
    {
      scale->orientation = orientation;

      gimp_color_scale_render (scale);
    }
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

void
gimp_color_scale_set_adjustment (GimpColorScale *scale,
                                 GtkAdjustment  *adj)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALE (scale));
  g_return_if_fail (adj == NULL || GTK_IS_ADJUSTMENT (adj));

  if (adj == scale->adj)
    return;

  if (scale->adj)
    {
      g_object_unref (scale->adj);
      scale->adj = NULL;
    }

  scale->adj = adj;

  if (scale->adj)
    {
      g_object_ref (scale->adj);
      gtk_object_sink (GTK_OBJECT (scale->adj));
    }

  gimp_color_scale_render (scale);
}

static void
gimp_color_scale_render (GimpColorScale *scale)
{
}
