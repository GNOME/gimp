/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoverlayframe.c
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimp-cairo.h"

#include "gimpoverlayframe.h"
#include "gimpwidgets-utils.h"


static void       gimp_overlay_frame_size_request  (GtkWidget      *widget,
                                                    GtkRequisition *requisition);
static void       gimp_overlay_frame_size_allocate (GtkWidget      *widget,
                                                    GtkAllocation  *allocation);
static gboolean   gimp_overlay_frame_expose        (GtkWidget      *widget,
                                                    GdkEventExpose *eevent);


G_DEFINE_TYPE (GimpOverlayFrame, gimp_overlay_frame, GTK_TYPE_BIN)

#define parent_class gimp_overlay_frame_parent_class


static void
gimp_overlay_frame_class_init (GimpOverlayFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->size_request  = gimp_overlay_frame_size_request;
  widget_class->size_allocate = gimp_overlay_frame_size_allocate;
  widget_class->expose_event  = gimp_overlay_frame_expose;
}

static void
gimp_overlay_frame_init (GimpOverlayFrame *frame)
{
  gtk_widget_set_app_paintable (GTK_WIDGET (frame), TRUE);
}

static void
gimp_overlay_frame_size_request (GtkWidget      *widget,
                                 GtkRequisition *requisition)
{
  GtkWidget      *child = gtk_bin_get_child (GTK_BIN (widget));
  GtkRequisition  child_requisition;
  gint            border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition->width  = border_width * 2;
  requisition->height = border_width * 2;

  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_size_request (child, &child_requisition);
    }
  else
    {
      child_requisition.width  = 0;
      child_requisition.height = 0;
    }

  requisition->width  += child_requisition.width;
  requisition->height += child_requisition.height;
}

static void
gimp_overlay_frame_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
  GtkWidget     *child = gtk_bin_get_child (GTK_BIN (widget));
  GtkAllocation  child_allocation;
  gint           border_width;

  gtk_widget_set_allocation (widget, allocation);

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (child && gtk_widget_get_visible (child))
    {
      child_allocation.x      = allocation->x + border_width;
      child_allocation.y      = allocation->y + border_width;
      child_allocation.width  = MAX (allocation->width  - 2 * border_width, 0);
      child_allocation.height = MAX (allocation->height - 2 * border_width, 0);

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static gboolean
gimp_overlay_frame_expose (GtkWidget      *widget,
                           GdkEventExpose *eevent)
{
  cairo_t       *cr    = gdk_cairo_create (gtk_widget_get_window (widget));
  GtkStyle      *style = gtk_widget_get_style (widget);
  GtkAllocation  allocation;
  gboolean       rgba;
  gint           border_width;

  rgba = gdk_screen_get_rgba_colormap (gtk_widget_get_screen (widget)) != NULL;

  gdk_cairo_region (cr, eevent->region);
  cairo_clip (cr);

  if (rgba)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    }

  gtk_widget_get_allocation (widget, &allocation);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (rgba)
    {
      gimp_cairo_rounded_rectangle (cr,
                                    0.0,              0.0,
                                    allocation.width, allocation.height,
                                    border_width);
    }
  else
    {
      cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    }

  cairo_clip_preserve (cr);

  gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);
  cairo_paint (cr);

  if (border_width > 0)
    {
      cairo_set_line_width (cr, 2.0);
      gdk_cairo_set_source_color (cr, &style->fg[GTK_STATE_NORMAL]);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);
}

GtkWidget *
gimp_overlay_frame_new (void)
{
  return g_object_new (GIMP_TYPE_OVERLAY_FRAME, NULL);
}
