/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkHWrapBox: Horizontal wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>

#include "gimpconstrainedhwrapbox.h"


static void    gimp_constrained_hwrap_box_class_init   (GimpConstrainedHWrapBoxClass *klass);
static void    gimp_constrained_hwrap_box_init         (GimpConstrainedHWrapBox      *hwbox);
static void    gimp_constrained_hwrap_box_size_request (GtkWidget                    *widget,
							GtkRequisition               *requisition);


static GtkHWrapBoxClass *parent_class = NULL;


GtkType
gimp_constrained_hwrap_box_get_type (void)
{
  static GtkType constrained_hwrap_box_type = 0;

  if (! constrained_hwrap_box_type)
    {
      static const GtkTypeInfo constrained_hwrap_box_info =
      {
	"GimpConstrainedHWrapBox",
	sizeof (GimpConstrainedHWrapBox),
	sizeof (GimpConstrainedHWrapBoxClass),
	(GtkClassInitFunc) gimp_constrained_hwrap_box_class_init,
	(GtkObjectInitFunc) gimp_constrained_hwrap_box_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      constrained_hwrap_box_type = gtk_type_unique (GTK_TYPE_HWRAP_BOX,
						    &constrained_hwrap_box_info);
    }

  return constrained_hwrap_box_type;
}

static void
gimp_constrained_hwrap_box_class_init (GimpConstrainedHWrapBoxClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = gtk_type_class (GTK_TYPE_HWRAP_BOX);

  widget_class->size_request = gimp_constrained_hwrap_box_size_request;
}

static void
gimp_constrained_hwrap_box_init (GimpConstrainedHWrapBox *hwbox)
{
}

GtkWidget*
gimp_constrained_hwrap_box_new (gboolean homogeneous)
{
  GimpConstrainedHWrapBox *hwbox;

  hwbox = GIMP_CONSTRAINED_HWRAP_BOX (gtk_widget_new (GIMP_TYPE_CONSTRAINED_HWRAP_BOX, NULL));

  GTK_WRAP_BOX (hwbox)->homogeneous = homogeneous ? TRUE : FALSE;

  return GTK_WIDGET (hwbox);
}

static void
gimp_constrained_hwrap_box_size_request (GtkWidget      *widget,
					 GtkRequisition *requisition)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  
  g_return_if_fail (requisition != NULL);
  
  if (widget->parent                                  &&
      GTK_IS_VIEWPORT (widget->parent)                &&
      widget->parent->parent                          &&
      GTK_IS_SCROLLED_WINDOW (widget->parent->parent) &&
      wbox->children                                  &&
      wbox->children->widget)
    {
      GtkWidget *scrolled_win;
      gint       child_width;
      gint       child_height;
      gint       viewport_width;
      gint       columns;
      gint       rows;

      scrolled_win = widget->parent->parent;

      child_width  = wbox->children->widget->requisition.width;
      child_height = wbox->children->widget->requisition.height;

      viewport_width =
	(scrolled_win->allocation.width -
	 GTK_SCROLLED_WINDOW (scrolled_win)->vscrollbar->allocation.width -
	 GTK_SCROLLED_WINDOW_CLASS (GTK_OBJECT (scrolled_win)->klass)->scrollbar_spacing -
	 scrolled_win->style->klass->xthickness * 2);

      columns = 
	(viewport_width + wbox->hspacing) / (child_width + wbox->hspacing);

      columns = MAX (1, columns);

      requisition->width = (child_width + wbox->hspacing) * columns;

      rows = wbox->n_children / columns;

      if (rows * columns < wbox->n_children)
	rows++;

      requisition->height = (child_height + wbox->vspacing) * rows;
    }
  else if (GTK_WIDGET_CLASS (parent_class)->size_request)
    GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}
