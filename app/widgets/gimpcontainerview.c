/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "gimpcontainerview.h"


static void   gimp_container_view_class_init (GimpContainerViewClass *klass);
static void   gimp_container_view_init       (GimpContainerView      *panel);
static void   gimp_container_view_destroy    (GtkObject                  *object);


static GtkVBoxClass *parent_class = NULL;


GtkType
gimp_container_view_get_type (void)
{
  static guint view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpContainerView",
	sizeof (GimpContainerView),
	sizeof (GimpContainerViewClass),
	(GtkClassInitFunc) gimp_container_view_class_init,
	(GtkObjectInitFunc) gimp_container_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GTK_TYPE_VBOX, &view_info);
    }

  return view_type;
}

static void
gimp_container_view_class_init (GimpContainerViewClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;
  
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = gimp_container_view_destroy;
}

static void
gimp_container_view_init (GimpContainerView *view)
{
  view->container = NULL;
}

static void
gimp_container_view_destroy (GtkObject *object)
{
  GimpContainerView *view;

  view = GIMP_CONTAINER_VIEW (object);
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}
