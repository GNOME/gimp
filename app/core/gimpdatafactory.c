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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "gimpdata.h"
#include "gimpdatalist.h"
#include "gimpdatafactory.h"
#include "gimpcontext.h"
#include "gimpmarshal.h"


static void   gimp_data_factory_class_init (GimpDataFactoryClass *klass);
static void   gimp_data_factory_init       (GimpDataFactory      *factory);
static void   gimp_data_factory_destroy    (GtkObject            *object);


static GimpObjectClass *parent_class = NULL;


GtkType
gimp_data_factory_get_type (void)
{
  static guint factory_type = 0;

  if (! factory_type)
    {
      GtkTypeInfo factory_info =
      {
	"GimpDataFactory",
	sizeof (GimpDataFactory),
	sizeof (GimpDataFactoryClass),
	(GtkClassInitFunc) gimp_data_factory_class_init,
	(GtkObjectInitFunc) gimp_data_factory_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      factory_type = gtk_type_unique (GIMP_TYPE_OBJECT, &factory_info);
    }

  return factory_type;
}

static void
gimp_data_factory_class_init (GimpDataFactoryClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = gimp_data_factory_destroy;
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->container              = NULL;
  factory->data_path              = NULL;
  factory->new_default_data_func  = NULL;
  factory->new_standard_data_func = NULL;
}

static void
gimp_data_factory_destroy (GtkObject *object)
{
  GimpDataFactory *factory;

  factory = GIMP_DATA_FACTORY (object);

  if (factory->container)
    gtk_object_unref (GTK_OBJECT (factory->container));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpDataFactory *
gimp_data_factory_new (GtkType                   data_type,
		       const gchar             **data_path,
		       GimpDataNewDefaultFunc    default_func,
		       GimpDataNewStandardFunc   standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (gtk_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (data_path != NULL, NULL);
  g_return_val_if_fail (default_func != NULL, NULL);
  g_return_val_if_fail (standard_func != NULL, NULL);

  factory = gtk_type_new (GIMP_TYPE_DATA_FACTORY);

  factory->container = GIMP_CONTAINER (gimp_data_list_new (data_type));

  gtk_object_ref (GTK_OBJECT (factory->container));
  gtk_object_sink (GTK_OBJECT (factory->container));

  factory->data_path                = data_path;
  factory->new_default_data_func    = default_func;
  factory->new_standard_data_func   = standard_func;

  return factory;
}

void
gimp_data_factory_data_init (GimpDataFactory *factory)
{
  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  if (! (factory->data_path && *factory->data_path))
    return;

  gimp_data_list_save_and_clear (GIMP_DATA_LIST (factory->container),
				 *factory->data_path);
}
