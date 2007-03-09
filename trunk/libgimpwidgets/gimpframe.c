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

#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpframe.h"


#define DEFAULT_LABEL_SPACING       6
#define DEFAULT_LABEL_BOLD          TRUE

#define GIMP_FRAME_INDENT_KEY       "gimp-frame-indent"
#define GIMP_FRAME_IN_EXPANDER_KEY  "gimp-frame-in-expander"


static void      gimp_frame_size_request        (GtkWidget      *widget,
                                                 GtkRequisition *requisition);
static void      gimp_frame_size_allocate       (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);
static void      gimp_frame_child_allocate      (GtkFrame       *frame,
                                                 GtkAllocation  *allocation);
static void      gimp_frame_style_set           (GtkWidget      *widget,
                                                 GtkStyle       *previous);
static gboolean  gimp_frame_expose_event        (GtkWidget      *widget,
                                                 GdkEventExpose *event);
static void      gimp_frame_label_widget_notify (GtkFrame       *frame);
static gint      gimp_frame_get_indent          (GtkWidget      *widget);
static gint      gimp_frame_get_label_spacing   (GtkFrame       *frame);


G_DEFINE_TYPE (GimpFrame, gimp_frame, GTK_TYPE_FRAME)

#define parent_class gimp_frame_parent_class


static void
gimp_frame_class_init (GimpFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkFrameClass  *frame_class  = GTK_FRAME_CLASS (klass);

  widget_class->size_request  = gimp_frame_size_request;
  widget_class->size_allocate = gimp_frame_size_allocate;
  widget_class->style_set     = gimp_frame_style_set;
  widget_class->expose_event  = gimp_frame_expose_event;

  frame_class->compute_child_allocation = gimp_frame_child_allocate;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("label-bold",
                                                                 NULL, NULL,
                                                                 DEFAULT_LABEL_BOLD,
                                                                 G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("label-spacing",
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
      gtk_widget_size_request (frame->label_widget, requisition);
    }
  else
    {
      requisition->width  = 0;
      requisition->height = 0;
    }

  requisition->height += gimp_frame_get_label_spacing (frame);

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      gint indent = gimp_frame_get_indent (widget);

      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width = MAX (requisition->width,
                                child_requisition.width + indent);
      requisition->height += child_requisition.height;
    }

  requisition->width  += 2 * GTK_CONTAINER (widget)->border_width;
  requisition->height += 2 * GTK_CONTAINER (widget)->border_width;
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
      gint            border = GTK_CONTAINER (widget)->border_width;

      gtk_widget_get_child_requisition (frame->label_widget,
                                        &label_requisition);

      label_allocation.x      = allocation->x + border;
      label_allocation.y      = allocation->y + border;
      label_allocation.width  = MAX (label_requisition.width,
                                     allocation->width - 2 * border);
      label_allocation.height = label_requisition.height;

      gtk_widget_size_allocate (frame->label_widget, &label_allocation);
    }
}

static void
gimp_frame_child_allocate (GtkFrame      *frame,
                           GtkAllocation *child_allocation)
{
  GtkWidget     *widget     = GTK_WIDGET (frame);
  GtkAllocation *allocation = &widget->allocation;
  gint           border     = GTK_CONTAINER (frame)->border_width;
  gint           spacing    = 0;
  gint           indent     = gimp_frame_get_indent (widget);

  if (frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget))
    {
      GtkRequisition  child_requisition;

      gtk_widget_get_child_requisition (frame->label_widget,
                                        &child_requisition);
      spacing += child_requisition.height;
    }

  spacing += gimp_frame_get_label_spacing (frame);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
    child_allocation->x    = border + indent;
  else
    child_allocation->x    = border;

  child_allocation->y      = border + spacing;
  child_allocation->width  = MAX (1,
                                  allocation->width - 2 * border - indent);
  child_allocation->height = MAX (1,
                                  allocation->height -
                                  child_allocation->y - border);

  child_allocation->x += allocation->x;
  child_allocation->y += allocation->y;
}

static void
gimp_frame_style_set (GtkWidget *widget,
                      GtkStyle  *previous)
{
  /*  font changes invalidate the indentation  */
  g_object_set_data (G_OBJECT (widget), GIMP_FRAME_INDENT_KEY, NULL);

  /*  for "label_bold"  */
  gimp_frame_label_widget_notify (GTK_FRAME (widget));
}

static gboolean
gimp_frame_expose_event (GtkWidget      *widget,
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
  if (frame->label_widget)
    {
      GtkLabel *label = NULL;

      if (GTK_IS_LABEL (frame->label_widget))
        {
          label = GTK_LABEL (frame->label_widget);

          gtk_misc_set_alignment (GTK_MISC (label),
                                  frame->label_xalign, frame->label_yalign);
        }
      else if (GTK_IS_BIN (frame->label_widget))
        {
          GtkWidget *child = gtk_bin_get_child (GTK_BIN (frame->label_widget));

          if (GTK_IS_LABEL (child))
            label = GTK_LABEL (child);
        }

      if (label)
        {
          PangoAttrList  *attrs = pango_attr_list_new ();
          PangoAttribute *attr;
          gboolean        bold;

          gtk_widget_style_get (GTK_WIDGET (frame), "label_bold", &bold, NULL);

          attr = pango_attr_weight_new (bold ?
                                        PANGO_WEIGHT_BOLD :
                                        PANGO_WEIGHT_NORMAL);
          attr->start_index = 0;
          attr->end_index   = -1;
          pango_attr_list_insert (attrs, attr);

          gtk_label_set_attributes (label, attrs);

          pango_attr_list_unref (attrs);
        }
    }
}

static gint
gimp_frame_get_indent (GtkWidget *widget)
{
  gint width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                   GIMP_FRAME_INDENT_KEY));

  if (! width)
    {
      PangoLayout *layout;

      /*  the HIG suggests to use four spaces so do just that  */
      layout = gtk_widget_create_pango_layout (widget, "    ");
      pango_layout_get_pixel_size (layout, &width, NULL);
      g_object_unref (layout);

      g_object_set_data (G_OBJECT (widget),
                         GIMP_FRAME_INDENT_KEY, GINT_TO_POINTER (width));
    }

  return width;
}

static gint
gimp_frame_get_label_spacing (GtkFrame *frame)
{
  gint spacing = 0;

  if ((frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget)) ||
      (g_object_get_data (G_OBJECT (frame), GIMP_FRAME_IN_EXPANDER_KEY)))
    {
      gtk_widget_style_get (GTK_WIDGET (frame),
                            "label_spacing", &spacing,
                            NULL);
    }

  return spacing;
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
  GtkWidget *frame;
  gboolean   expander = FALSE;

  /*  somewhat hackish, should perhaps be an object property of GimpFrame  */
  if (label && strcmp (label, "<expander>") == 0)
    {
      expander = TRUE;
      label    = NULL;
    }

  frame = g_object_new (GIMP_TYPE_FRAME,
                        "label", label,
                        NULL);

  if (expander)
    g_object_set_data (G_OBJECT (frame),
                       GIMP_FRAME_IN_EXPANDER_KEY, (gpointer) TRUE);

  return frame;
}
