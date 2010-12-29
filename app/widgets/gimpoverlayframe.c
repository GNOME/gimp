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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "gimpoverlayframe.h"
#include "gimpwidgets-utils.h"


static void       gimp_overlay_frame_get_preferred_width  (GtkWidget     *widget,
                                                           gint          *minimum_width,
                                                           gint          *natural_width);
static void       gimp_overlay_frame_get_preferred_height (GtkWidget     *widget,
                                                           gint          *minimum_height,
                                                           gint          *natural_height);
static void       gimp_overlay_frame_size_allocate        (GtkWidget     *widget,
                                                           GtkAllocation *allocation);
static gboolean   gimp_overlay_frame_draw                 (GtkWidget     *widget,
                                                           cairo_t       *cr);


G_DEFINE_TYPE (GimpOverlayFrame, gimp_overlay_frame, GTK_TYPE_BIN)

#define parent_class gimp_overlay_frame_parent_class


static void
gimp_overlay_frame_class_init (GimpOverlayFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->get_preferred_width  = gimp_overlay_frame_get_preferred_width;
  widget_class->get_preferred_height = gimp_overlay_frame_get_preferred_height;
  widget_class->size_allocate        = gimp_overlay_frame_size_allocate;
  widget_class->draw                 = gimp_overlay_frame_draw;
}

static void
gimp_overlay_frame_init (GimpOverlayFrame *frame)
{
  gtk_widget_set_app_paintable (GTK_WIDGET (frame), TRUE);
}

static void
gimp_overlay_frame_get_preferred_width (GtkWidget *widget,
                                        gint      *minimum_width,
                                        gint      *natural_width)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  gint       border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (child && gtk_widget_get_visible (child))
    gtk_widget_get_preferred_width (child, minimum_width, natural_width);
  else
    *minimum_width = *natural_width = 0;

  *minimum_width += 2 * border_width;
  *natural_width += 2 * border_width;
}

static void
gimp_overlay_frame_get_preferred_height (GtkWidget *widget,
                                         gint      *minimum_height,
                                         gint      *natural_height)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  gint       border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (child && gtk_widget_get_visible (child))
    gtk_widget_get_preferred_height (child, minimum_height, natural_height);
  else
    *minimum_height = *natural_height = 0;

  *minimum_height += 2 * border_width;
  *natural_height += 2 * border_width;
}

static void
gimp_overlay_frame_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));

  gtk_widget_set_allocation (widget, allocation);

  if (child && gtk_widget_get_visible (child))
    {
      GtkAllocation child_allocation;
      gint          border_width;

      border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

      child_allocation.x      = allocation->x + border_width;
      child_allocation.y      = allocation->y + border_width;
      child_allocation.width  = MAX (allocation->width  - 2 * border_width, 0);
      child_allocation.height = MAX (allocation->height - 2 * border_width, 0);

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static gboolean
gimp_overlay_frame_draw (GtkWidget *widget,
                         cairo_t   *cr)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GdkRGBA          color;
  gboolean         rgba;
  gint             border_width;

  rgba = gdk_screen_get_rgba_visual (gtk_widget_get_screen (widget)) != NULL;

  if (rgba)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    }

  gtk_widget_get_allocation (widget, &allocation);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (rgba && border_width > 0)
    {
#define DEG_TO_RAD(deg) ((deg) * (G_PI / 180.0))

      cairo_arc (cr,
                 border_width,
                 border_width,
                 border_width,
                 DEG_TO_RAD (180),
                 DEG_TO_RAD (270));
      cairo_line_to (cr,
                     allocation.width - border_width,
                     0);

      cairo_arc (cr,
                 allocation.width - border_width,
                 border_width,
                 border_width,
                 DEG_TO_RAD (270),
                 DEG_TO_RAD (0));
      cairo_line_to (cr,
                     allocation.width,
                     allocation.height - border_width);

      cairo_arc (cr,
                 allocation.width  - border_width,
                 allocation.height - border_width,
                 border_width,
                 DEG_TO_RAD (0),
                 DEG_TO_RAD (90));
      cairo_line_to (cr,
                     border_width,
                     allocation.height);

      cairo_arc (cr,
                 border_width,
                 allocation.height - border_width,
                 border_width,
                 DEG_TO_RAD (90),
                 DEG_TO_RAD (180));
      cairo_close_path (cr);
    }
  else
    {
      cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    }

  cairo_clip_preserve (cr);

  gtk_style_context_get_background_color (style,
                                          gtk_widget_get_state_flags (widget),
                                          &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_paint (cr);

  if (border_width > 0)
    {
      cairo_set_line_width (cr, 2.0);
      gtk_style_context_get_color (style,
                                   gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_stroke (cr);
    }

  return GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
}

GtkWidget *
gimp_overlay_frame_new (void)
{
  return g_object_new (GIMP_TYPE_OVERLAY_FRAME, NULL);
}
