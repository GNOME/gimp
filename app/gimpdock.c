/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpdock.h"
#include "gimpdockable.h"


static void   gimp_dock_class_init (GimpDockClass *klass);
static void   gimp_dock_init       (GimpDock      *dock);

static void   gimp_dock_destroy    (GtkObject     *object);


static GtkWindowClass *parent_class = NULL;


GtkType
gimp_dock_get_type (void)
{
  static GtkType dock_type = 0;

  if (! dock_type)
    {
      static const GtkTypeInfo dock_info =
      {
	"GimpDock",
	sizeof (GimpDock),
	sizeof (GimpDockClass),
	(GtkClassInitFunc) gimp_dock_class_init,
	(GtkObjectInitFunc) gimp_dock_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dock_type = gtk_type_unique (GTK_TYPE_WINDOW, &dock_info);
    }

  return dock_type;
}

static void
gimp_dock_class_init (GimpDockClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_WINDOW);

  object_class->destroy = gimp_dock_destroy;
}

static void
gimp_dock_init (GimpDock *dock)
{
  dock->vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock), dock->vbox);
  gtk_widget_show (dock->vbox);

  dock->notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (dock->vbox), dock->notebook);
  gtk_widget_show (dock->notebook);
}

static void
gimp_dock_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_dock_new (void)
{
  return GTK_WIDGET (gtk_type_new (GIMP_TYPE_DOCK));
}

void
gimp_dock_add (GimpDock     *dock,
	       GimpDockable *dockable)
{
  g_return_if_fail (dock != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_return_if_fail (dockable != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  gtk_notebook_append_page_menu (GTK_NOTEBOOK (dock->notebook),
				 GTK_WIDGET (dockable),
				 gtk_label_new (dockable->name),
				 gtk_label_new (dockable->name));
}
