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
static void     gimp_color_scale_init       (GimpColorScale      *gcs);
static void     gimp_color_scale_destroy    (GtkObject           *object);

static void     gimp_color_scale_size_allocate (GtkWidget        *widget,
                                                GtkAllocation    *allocation);
static gboolean gimp_color_scale_expose        (GtkWidget        *widget,
                                                GdkEventExpose   *event);

static GtkDrawingAreaClass * parent_class = NULL;


GType
gimp_color_scale_get_type (void)
{
  static GType gcs_type = 0;

  if (! gcs_type)
    {
      static const GTypeInfo gcs_info =
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

      gcs_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                         "GimpColorScale", 
                                         &gcs_info, 0);
    }

  return gcs_type;
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
gimp_color_scale_init (GimpColorScale *gcs)
{
}

static void
gimp_color_scale_destroy (GtkObject *object)
{
  GimpColorScale *gcs;

  gcs = GIMP_COLOR_SCALE (object);

  if (gcs->buf)
    {
      g_free (gcs->buf);
      gcs->buf       = NULL;
      gcs->width     = 0;
      gcs->height    = 0;
      gcs->rowstride = 0;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_color_scale_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GimpColorScale *gcs;

  gcs = GIMP_COLOR_SCALE (widget);

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (widget->allocation.width  != gcs->width ||
      widget->allocation.height != gcs->height)
    {
      gcs->width  = widget->allocation.width;
      gcs->height = widget->allocation.height;

      gcs->rowstride = (gcs->width * 3 + 3) & ~0x3;

      g_free (gcs->buf);
      gcs->buf = g_new (guchar, gcs->rowstride * gcs->height);
    } 
}

static gboolean
gimp_color_scale_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpColorScale *gcs;
  guchar        *buf;

  gcs = GIMP_COLOR_SCALE (widget);
  
  if (! gcs->buf || ! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  buf = gcs->buf + event->area.y * gcs->rowstride + event->area.x * 3;

  gdk_draw_rgb_image_dithalign (widget->window,
                                widget->style->black_gc,
                                event->area.x,
                                event->area.y,
                                event->area.width,
                                event->area.height,
                                GDK_RGB_DITHER_MAX,
                                buf,
                                gcs->rowstride,
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
gimp_color_scale_new (void)
{
  GimpColorScale *gcs;

  gcs = g_object_new (GIMP_TYPE_COLOR_SCALE, NULL);

  return GTK_WIDGET (gcs);
}
