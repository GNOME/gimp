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

#ifndef __GIMP_OBJECT_H__
#define __GIMP_OBJECT_H__


#include <gtk/gtkobject.h>


#define GIMP_TYPE_OBJECT            (gimp_object_get_type ())
#define GIMP_OBJECT(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_OBJECT, GimpObject))
#define GIMP_OBJECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OBJECT, GimpObjectClass))
#define GIMP_IS_OBJECT(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_OBJECT))
#define GIMP_IS_OBJECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OBJECT))

typedef struct _GimpObject GimpObject;
typedef struct _GimpObjectClass GimpObjectClass;

struct _GimpObject
{
  GtkObject  parent_instance;

  gchar     *name;
};

struct _GimpObjectClass
{
  GtkObjectClass  parent_class;

  void (* name_changed) (GimpObject *object);
};


GtkType       gimp_object_get_type     (void);

void          gimp_object_set_name     (GimpObject       *object,
					const gchar      *name);
const gchar * gimp_object_get_name     (const GimpObject *object);
void          gimp_object_name_changed (GimpObject       *object);


#endif  /* __GIMP_OBJECT_H__ */
