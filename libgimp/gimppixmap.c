/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimppixmap.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#include <stdio.h>

#include "gimppixmap.h"

struct _GimpPixmap
{
  GtkPixmap   pixmap;

  gchar     **xpm_data;
};

static void gimp_pixmap_destroy           (GtkObject  *object);
static void gimp_pixmap_realize           (GtkWidget  *widget);
static void gimp_pixmap_create_from_xpm_d (GimpPixmap *pixmap);

static GtkPixmapClass *parent_class = NULL;

static void
gimp_pixmap_destroy (GtkObject *object)
{
  GimpPixmap *pixmap;

  g_return_if_fail (pixmap = GIMP_PIXMAP (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_pixmap_class_init (GimpPixmapClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_pixmap_get_type ());

  object_class->destroy = gimp_pixmap_destroy;

  widget_class->realize = gimp_pixmap_realize;
}

static void
gimp_pixmap_init (GimpPixmap *pixmap)
{
  pixmap->xpm_data = NULL;
}

GtkType
gimp_pixmap_get_type (void)
{
  static guint pixmap_type = 0;

  if (!pixmap_type)
    {
      GtkTypeInfo pixmap_info =
      {
	"GimpPixmap",
	sizeof (GimpPixmap),
	sizeof (GimpPixmapClass),
	(GtkClassInitFunc) gimp_pixmap_class_init,
	(GtkObjectInitFunc) gimp_pixmap_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      pixmap_type = gtk_type_unique (gtk_pixmap_get_type (), &pixmap_info);
    }

  return pixmap_type;
}

/**
 * gimp_pixmap_new:
 * @xpm_data: A pointer to a XPM data structure as found in XPM files.
 *
 * Creates a new #GimpPixmap widget.
 *
 * Returns: A pointer to the new #GimpPixmap widget.
 *
 */
GtkWidget *
gimp_pixmap_new (gchar **xpm_data)
{
  GimpPixmap *pixmap;

  pixmap = gtk_type_new (gimp_pixmap_get_type ());

  GTK_PIXMAP (pixmap)->build_insensitive = TRUE;
  gimp_pixmap_set (pixmap, xpm_data);

  return GTK_WIDGET (pixmap);
}

/**
 * gimp_pixmap_set:
 * @pixmap: The pixmap widget you want to set the new xpm_data for.
 * @xpm_data: A pointer to a XPM data structure as found in XPM files.
 *
 * Sets a new image for an existing #GimpPixmap widget.
 *
 */
void
gimp_pixmap_set (GimpPixmap  *pixmap,
		 gchar      **xpm_data)
{
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GIMP_IS_PIXMAP (pixmap));

  pixmap->xpm_data = xpm_data;

  GTK_WIDGET (pixmap)->requisition.width  = 0;
  GTK_WIDGET (pixmap)->requisition.height = 0;

  if (! GTK_WIDGET_REALIZED (GTK_WIDGET (pixmap)))
    {
      if (xpm_data)
	{
	  gint width, height;

	  if (sscanf (xpm_data[0], "%d %d", &width, &height) != 2)
	    {
	      g_warning ("passed pointer is no XPM data");
	    }
	  else
	    {
	      GTK_WIDGET (pixmap)->requisition.width =
		width + GTK_MISC (pixmap)->xpad * 2;
	      GTK_WIDGET (pixmap)->requisition.height =
		height + GTK_MISC (pixmap)->ypad * 2;
	    }
	}
    }
  else
    {
      gimp_pixmap_create_from_xpm_d (pixmap);
    }
}

static void
gimp_pixmap_realize (GtkWidget *widget)
{
  if (GTK_WIDGET_CLASS (parent_class)->realize)
    (* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

  gimp_pixmap_create_from_xpm_d (GIMP_PIXMAP (widget));
}

static void
gimp_pixmap_create_from_xpm_d (GimpPixmap *pixmap)
{
  GtkStyle   *style;
  GdkPixmap  *gdk_pixmap = NULL;
  GdkBitmap  *mask       = NULL;

  if (pixmap->xpm_data)
    {
      GtkWidget  *widget;

      widget = GTK_WIDGET (pixmap);

      style = gtk_widget_get_style (widget);

      gdk_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						 &mask,
						 &style->bg[GTK_STATE_NORMAL],
						 pixmap->xpm_data);
    }

  gtk_pixmap_set (GTK_PIXMAP (pixmap), gdk_pixmap, mask);

  if (gdk_pixmap)
    gdk_pixmap_unref (gdk_pixmap);

  if (mask)
    gdk_bitmap_unref (mask);
}
