/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpframe.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpframe.h"


#define DEFAULT_LABEL_SPACING  6


static void      gimp_frame_class_init          (GimpFrameClass *klass);
static void      gimp_frame_init                (GimpFrame      *frame);
static void      gimp_frame_size_request        (GtkWidget      *widget,
                                                 GtkRequisition *requisition);
static void      gimp_frame_size_allocate       (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);
static void      gimp_frame_child_allocate      (GtkFrame       *frame,
                                                 GtkAllocation  *allocation);
static gboolean  gimp_frame_expose              (GtkWidget      *widget,
                                                 GdkEventExpose *event);
static void      gimp_frame_label_widget_notify (GtkFrame       *frame);
static gint      gimp_frame_left_margin         (GtkWidget      *widget);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_frame_get_type (void)
{
  static GType frame_type = 0;

  if (! frame_type)
    {
      static const GTypeInfo frame_info =
      {
        sizeof (GimpFrameClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)    gimp_frame_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpFrame),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_frame_init,
      };

      frame_type = g_type_register_static (GTK_TYPE_FRAME,
                                           "GimpFrame",
                                           &frame_info, 0);
    }

  return frame_type;
}

static void
gimp_frame_class_init (GimpFrameClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkFrameClass  *frame_class;

  parent_class = g_type_class_peek_parent (klass);

  widget_class = GTK_WIDGET_CLASS (klass);
  frame_class  = GTK_FRAME_CLASS (klass);

  widget_class->size_request  = gimp_frame_size_request;
  widget_class->size_allocate = gimp_frame_size_allocate;
  widget_class->expose_event  = gimp_frame_expose;

  frame_class->compute_child_allocation = gimp_frame_child_allocate;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("label_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_LABEL_SPACING,
                                                             G_PARAM_READABLE));
}


static void
gimp_frame_init (GimpFrame *frame)
{
  g_signal_connect (frame, "notify::label-widget",
                    G_CALLBACK (gimp_frame_label_widget_notify),
                    NULL);
}

static void
gimp_frame_size_request (GtkWidget      *widget,
                         GtkRequisition *requisition)
{
  GtkFrame       *frame = GTK_FRAME (widget);
  GtkBin         *bin   = GTK_BIN (widget);
  GtkRequisition  child_requisition;

  if (frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget))
    {
      gint  spacing;

      gtk_widget_size_request (frame->label_widget, requisition);

      gtk_widget_style_get (widget,
                            "label_spacing", &spacing,
                            NULL);

      requisition->height += spacing;
    }
  else
    {
      requisition->width  = 0;
      requisition->height = 0;
    }

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      gint left_margin = gimp_frame_left_margin (widget);

      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width = MAX (requisition->width,
                                child_requisition.width + left_margin);
      requisition->height += child_requisition.height;
    }

  requisition->width  += GTK_CONTAINER (widget)->border_width;
  requisition->height += GTK_CONTAINER (widget)->border_width;
}

static void
gimp_frame_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  GtkFrame *frame = GTK_FRAME (widget);
  GtkBin   *bin   = GTK_BIN (widget);

  widget->allocation = *allocation;

  gimp_frame_child_allocate (frame, &frame->child_allocation);

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    gtk_widget_size_allocate (bin->child, &frame->child_allocation);

  if (frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget))
    {
      GtkAllocation   label_allocation;
      GtkRequisition  label_requisition;
      gfloat          xalign;

      gtk_widget_get_child_requisition (frame->label_widget,
                                        &label_requisition);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
	xalign = frame->label_xalign;
      else
	xalign = 1.0 - frame->label_xalign;

      label_allocation.x      = (allocation->x +
                                 xalign * (gfloat) (allocation->width -
                                                    label_requisition.width));
      label_allocation.y      = allocation->y;
      label_allocation.width  = label_requisition.width;
      label_allocation.height = label_requisition.height;

      gtk_widget_size_allocate (frame->label_widget, &label_allocation);
    }
}

static void
gimp_frame_child_allocate (GtkFrame      *frame,
                           GtkAllocation *child_allocation)
{
  GtkWidget     *widget       = GTK_WIDGET (frame);
  GtkAllocation *allocation   = &widget->allocation;
  gint           border_width = GTK_CONTAINER (frame)->border_width;
  gint           top_margin   = 0;
  gint           left_margin  = gimp_frame_left_margin (widget);

  if (frame->label_widget)
    {
      GtkRequisition  child_requisition;

      gtk_widget_get_child_requisition (frame->label_widget,
                                        &child_requisition);

      gtk_widget_style_get (widget,
                            "label_spacing", &top_margin,
                            NULL);

      top_margin += child_requisition.height;
    }

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
    child_allocation->x    = border_width + left_margin;
  else
    child_allocation->x    = border_width;

  child_allocation->y      = border_width + top_margin;
  child_allocation->width  = MAX (1,
                                  allocation->width -
                                  2 * border_width - left_margin);
  child_allocation->height = MAX (1,
                                  allocation->height -
                                  child_allocation->y - border_width);

  child_allocation->x += allocation->x;
  child_allocation->y += allocation->y;
}

static gboolean
gimp_frame_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GtkWidgetClass *widget_class = g_type_class_peek_parent (parent_class);

      return widget_class->expose_event (widget, event);
    }

  return FALSE;
}

static void
gimp_frame_label_widget_notify (GtkFrame *frame)
{
  if (frame->label_widget && GTK_IS_LABEL (frame->label_widget))
    {
      PangoAttrList  *attrs;
      PangoAttribute *attr;

      attrs = pango_attr_list_new ();

      attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
      attr->start_index = 0;
      attr->end_index   = -1;
      pango_attr_list_insert (attrs, attr);

      gtk_label_set_attributes (GTK_LABEL (frame->label_widget), attrs);

      pango_attr_list_unref (attrs);
    }
}

static gint
gimp_frame_left_margin (GtkWidget *widget)
{
  PangoLayout   *layout;
  gint           width;

  /*  the HIG suggests to use four spaces so do just that  */
  layout = gtk_widget_create_pango_layout (widget, "    ");
  pango_layout_get_pixel_size (layout, &width, NULL);
  g_object_unref (layout);

  return width;
}

/**
 * gimp_frame_new:
 * @label: text to set as the frame's title label (or %NULL for no title)
 *
 * Creates a #GimpFrame widget. A #GimpFrame is a HIG-compliant
 * variant of #GtkFrame. It doesn't render a frame at all but
 * otherwise behaves like a frame. The frame's title is rendered in
 * bold and the frame content is indented four spaces as suggested by
 * the GNOME HIG (see http://developer.gnome.org/projects/gup/hig/).
 *
 * Return value: a new #GimpFrame widget
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_frame_new (const gchar *label)
{
  return g_object_new (GIMP_TYPE_FRAME,
                       "label", label,
                       NULL);
}
