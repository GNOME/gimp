/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.c
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

#include "gimpdockable.h"


static void   gimp_dockable_class_init (GimpDockableClass *klass);
static void   gimp_dockable_init       (GimpDockable      *dockable);

static void   gimp_dockable_destroy    (GtkObject         *object);


static GtkVBoxClass *parent_class = NULL;


GtkType
gimp_dockable_get_type (void)
{
  static GtkType dockable_type = 0;

  if (! dockable_type)
    {
      static const GtkTypeInfo dockable_info =
      {
	"GimpDockable",
	sizeof (GimpDockable),
	sizeof (GimpDockableClass),
	(GtkClassInitFunc) gimp_dockable_class_init,
	(GtkObjectInitFunc) gimp_dockable_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dockable_type = gtk_type_unique (GTK_TYPE_VBOX, &dockable_info);
    }

  return dockable_type;
}

static void
gimp_dockable_class_init (GimpDockableClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = gimp_dockable_destroy;
}

static void
gimp_dockable_init (GimpDockable *dockable)
{
  dockable->name = NULL;
  dockable->dock = NULL;
}

static void
gimp_dockable_destroy (GtkObject *object)
{
  GimpDockable *dockable;

  dockable = GIMP_DOCKABLE (object);

  g_free (dockable->name);

  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_dockable_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return GTK_WIDGET (gtk_type_new (GIMP_TYPE_DOCKABLE));
}
