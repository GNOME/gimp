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


#define GIMP_TYPE_OBJECT         (gimp_object_get_type ())
#define GIMP_OBJECT(obj)         (GTK_CHECK_CAST (obj, GIMP_TYPE_OBJECT, GimpObject))
#define GIMP_IS_OBJECT(obj)      (GTK_CHECK_TYPE (obj, GIMP_TYPE_OBJECT))
#define GIMP_OBJECT_CLASS(klass) (GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_OBJECT, GimpObjectClass))


typedef struct _GimpObjectClass GimpObjectClass;

struct _GimpObject
{
  GtkObject object;
};

struct _GimpObjectClass
{
  GtkObjectClass parent_class;
};


#define GIMP_TYPE_INIT(typevar, obtype, classtype, obinit, classinit, parent) \
if(!typevar){ \
	GtkTypeInfo _info={#obtype, \
			   sizeof(obtype), \
			   sizeof(classtype), \
			   (GtkClassInitFunc)classinit, \
			   (GtkObjectInitFunc)obinit, \
			   NULL, NULL, NULL}; \
	typevar=gtk_type_unique(parent, &_info); \
}


GtkType  gimp_object_get_type (void);


#endif  /* __GIMP_OBJECT_H__ */
