/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpframe.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimp3migration.h"
#include "gimpframe.h"


/**
 * SECTION: gimpframe
 * @title: GimpFrame
 * @short_description: A widget providing a HIG-compliant subclass
 *                     of #GtkFrame.
 *
 * A widget providing a HIG-compliant subclass of #GtkFrame.
 **/


#define DEFAULT_LABEL_SPACING       6
#define DEFAULT_LABEL_BOLD          TRUE

#define GIMP_FRAME_INDENT_KEY       "gimp-frame-indent"
#define GIMP_FRAME_IN_EXPANDER_KEY  "gimp-frame-in-expander"


static void      gimp_frame_get_preferred_width  (GtkWidget      *widget,
                                                  gint           *minimum_width,
                                                  gint           *natural_width);
static void      gimp_frame_get_preferred_height (GtkWidget      *widget,
                                                  gint           *minimum_height,
                                                  gint           *natural_height);
static void      gimp_frame_size_allocate        (GtkWidget      *widget,
                                                  GtkAllocation  *allocation);
static void      gimp_frame_style_set            (GtkWidget      *widget,
                                                  GtkStyle       *previous);
static gboolean  gimp_frame_draw                 (GtkWidget      *widget,
                                                  cairo_t        *cr);
static void      gimp_frame_child_allocate       (GtkFrame       *frame,
                                                  GtkAllocation  *allocation);
static void      gimp_frame_label_widget_notify  (GtkFrame       *frame);
static gint      gimp_frame_get_indent           (GtkWidget      *widget);
static gint      gimp_frame_get_label_spacing    (GtkFrame       *frame);


G_DEFINE_TYPE (GimpFrame, gimp_frame, GTK_TYPE_FRAME)

#define parent_class gimp_frame_parent_class


static void
gimp_frame_class_init (GimpFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->get_preferred_width  = gimp_frame_get_preferred_width;
  widget_class->get_preferred_height = gimp_frame_get_preferred_height;
  widget_class->size_allocate        = gimp_frame_size_allocate;
  widget_class->style_set            = gimp_frame_style_set;
  widget_class->draw                 = gimp_frame_draw;

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
  GtkFrame       *frame        = GTK_FRAME (widget);
  GtkWidget      *label_widget = gtk_frame_get_label_widget (frame);
  GtkWidget      *child        = gtk_bin_get_child (GTK_BIN (widget));
  GtkRequisition  child_requisition;
  gint            border_width;

  if (label_widget && gtk_widget_get_visible (label_widget))
    {
      gtk_widget_get_preferred_size (label_widget, requisition, NULL);
    }
  else
    {
      requisition->width  = 0;
      requisition->height = 0;
    }

  requisition->height += gimp_frame_get_label_spacing (frame);

  if (child && gtk_widget_get_visible (child))
    {
      gint indent = gimp_frame_get_indent (widget);

      gtk_widget_get_preferred_size (child, &child_requisition, NULL);

      requisition->width = MAX (requisition->width,
                                child_requisition.width + indent);
      requisition->height += child_requisition.height;
    }

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition->width  += 2 * border_width;
  requisition->height += 2 * border_width;
}

static void
gimp_frame_get_preferred_width (GtkWidget *widget,
                                gint      *minimum_width,
                                gint      *natural_width)
{
  GtkRequisition requisition;

  gimp_frame_size_request (widget, &requisition);

  *minimum_width = *natural_width = requisition.width;
}

static void
gimp_frame_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum_height,
                                 gint      *natural_height)
{
  GtkRequisition requisition;

  gimp_frame_size_request (widget, &requisition);

  *minimum_height = *natural_height = requisition.height;
}

static void
gimp_frame_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  GtkFrame      *frame        = GTK_FRAME (widget);
  GtkWidget     *label_widget = gtk_frame_get_label_widget (frame);
  GtkWidget     *child        = gtk_bin_get_child (GTK_BIN (widget));
  GtkAllocation  child_allocation;

  /* must not chain up here */
  gtk_widget_set_allocation (widget, allocation);

  gimp_frame_child_allocate (frame, &child_allocation);

  if (child && gtk_widget_get_visible (child))
    gtk_widget_size_allocate (child, &child_allocation);

  if (label_widget && gtk_widget_get_visible (label_widget))
    {
      GtkAllocation   label_allocation;
      GtkRequisition  label_requisition;
      gint            border_width;

      border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

      gtk_widget_get_preferred_size (label_widget, &label_requisition, NULL);

      label_allocation.x      = allocation->x + border_width;
      label_allocation.y      = allocation->y + border_width;
      label_allocation.width  = MAX (label_requisition.width,
                                     allocation->width - 2 * border_width);
      label_allocation.height = label_requisition.height;

      gtk_widget_size_allocate (label_widget, &label_allocation);
    }
}

static void
gimp_frame_child_allocate (GtkFrame      *frame,
                           GtkAllocation *child_allocation)
{
  GtkWidget     *widget       = GTK_WIDGET (frame);
  GtkWidget     *label_widget = gtk_frame_get_label_widget (frame);
  GtkAllocation  allocation;
  gint           border_width;
  gint           spacing      = 0;
  gint           indent       = gimp_frame_get_indent (widget);

  gtk_widget_get_allocation (widget, &allocation);

  border_width = gtk_container_get_border_width (GTK_CONTAINER (frame));

  if (label_widget && gtk_widget_get_visible (label_widget))
    {
      GtkRequisition  child_requisition;

      gtk_widget_get_preferred_size (label_widget, &child_requisition, NULL);
      spacing += child_requisition.height;
    }

  spacing += gimp_frame_get_label_spacing (frame);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
    child_allocation->x    = border_width + indent;
  else
    child_allocation->x    = border_width;

  child_allocation->y      = border_width + spacing;
  child_allocation->width  = MAX (1,
                                  allocation.width - 2 * border_width - indent);
  child_allocation->height = MAX (1,
                                  allocation.height -
                                  child_allocation->y - border_width);

  child_allocation->x += allocation.x;
  child_allocation->y += allocation.y;
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
gimp_frame_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  GtkWidgetClass *widget_class = g_type_class_peek_parent (parent_class);

  return widget_class->draw (widget, cr);
}

static void
gimp_frame_label_widget_notify (GtkFrame *frame)
{
  GtkWidget *label_widget = gtk_frame_get_label_widget (frame);

  if (label_widget)
    {
      GtkLabel *label = NULL;

      if (GTK_IS_LABEL (label_widget))
        {
          gfloat xalign, yalign;

          label = GTK_LABEL (label_widget);

          gtk_frame_get_label_align (frame, &xalign, &yalign);
          gtk_label_set_xalign (GTK_LABEL (label), xalign);
          gtk_label_set_yalign (GTK_LABEL (label), yalign);
        }
      else if (GTK_IS_BIN (label_widget))
        {
          GtkWidget *child = gtk_bin_get_child (GTK_BIN (label_widget));

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
  GtkWidget *label_widget = gtk_frame_get_label_widget (frame);
  gint       spacing      = 0;

  if ((label_widget && gtk_widget_get_visible (label_widget)) ||
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
 * Since: 2.2
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
