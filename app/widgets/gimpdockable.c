/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.c
 * Copyright (C) 2001 Michael Natterer
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
#include "gimpdockbook.h"


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
  dockable->name         = NULL;
  dockable->short_name   = NULL;
  dockable->dockbook     = NULL;
  dockable->get_tab_func = NULL;
}

static void
gimp_dockable_destroy (GtkObject *object)
{
  GimpDockable *dockable;

  dockable = GIMP_DOCKABLE (object);

  g_free (dockable->name);
  g_free (dockable->short_name);

  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_dockable_new (const gchar            *name,
		   const gchar            *short_name,
		   GimpDockableGetTabFunc  get_tab_func)
{
  GimpDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (short_name != NULL, NULL);

  dockable = gtk_type_new (GIMP_TYPE_DOCKABLE);

  dockable->name        = g_strdup (name);
  dockable->short_name  = g_strdup (short_name);

  dockable->get_tab_func = get_tab_func;

  return GTK_WIDGET (dockable);
}

GtkWidget *
gimp_dockable_get_tab_widget (GimpDockable *dockable,
			      GimpDockbook *dockbook,
			      gint          size)
{
  g_return_val_if_fail (dockable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  g_return_val_if_fail (dockbook != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), NULL);

  g_return_val_if_fail (size >= -1 && size < 64, NULL);

  if (dockable->get_tab_func)
    {
      return dockable->get_tab_func (dockable, dockbook, size);
    }

  return gtk_label_new (dockable->short_name);
}
