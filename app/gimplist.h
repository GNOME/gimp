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

#ifndef __GIMP_LIST_H__
#define __GIMP_LIST_H__


#include <glib.h>
#include "gimplistF.h"


/* GimpList - a typed list of objects with signals for adding and
 * removing of stuff. If it is weak, destroyed objects get removed
 * automatically. If it is not, it refs them so they won't be freed
 * till they are removed. (Though they can be destroyed, of course)
 */


#define GIMP_TYPE_LIST    gimp_list_get_type ()
#define GIMP_LIST(obj)    GTK_CHECK_CAST (obj, GIMP_TYPE_LIST, GimpList)
#define GIMP_IS_LIST(obj) GTK_CHECK_TYPE (obj, gimp_list_get_type())

     
/* Signals:
   add
   remove
*/


GtkType    gimp_list_get_type (void);

GimpList * gimp_list_new      (GtkType   type,
			       gboolean  weak);

GtkType    gimp_list_type     (GimpList *list);
gboolean   gimp_list_add      (GimpList *list,
			       gpointer  object);
gboolean   gimp_list_remove   (GimpList *list,
			       gpointer  object);
gboolean   gimp_list_have     (GimpList *list,
			       gpointer  object);
void       gimp_list_foreach  (GimpList *list,
			       GFunc     func,
			       gpointer  user_data);
gint       gimp_list_size     (GimpList *list);


#endif  /* __GIMP_LIST_H__ */
