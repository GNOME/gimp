/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include "gimpobject.h"


static void
gimp_object_init (GimpObject *object)
{
}

static void
gimp_object_class_init (GimpObjectClass *klass)
{
}

GtkType 
gimp_object_get_type (void)
{
  static GtkType object_type = 0;

  if (! object_type)
    {
      GtkTypeInfo object_info =
      {
        "GimpObject",
        sizeof (GimpObject),
        sizeof (GimpObjectClass),
        (GtkClassInitFunc) gimp_object_class_init,
        (GtkObjectInitFunc) gimp_object_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      object_type = gtk_type_unique (GTK_TYPE_OBJECT, &object_info);
    }

  return object_type;
}
