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
#ifndef __GIMPSET_H__
#define __GIMPSET_H__

#include <glib.h>
#include "gimpsetF.h"

/* GimpSet - a (usually) typed set of objects with signals for adding
   and removing of stuff. If it is weak, destroyed objects get removed
   automatically. If it is not, it refs them so they won't be freed
   till they are removed. (Though they can be destroyed, of course).

   If GTK_TYPE_NONE is specified at gimpset creation time, no type
   checking is performed by gimp_set_add() and the
   gimp_set_{add,remove}_handler() functions should not be used.  It
   is also illegal to ask for a weak untyped gimpset.
*/

#define GIMP_TYPE_SET    gimp_set_get_type()
#define GIMP_SET(obj)    GTK_CHECK_CAST (obj, GIMP_TYPE_SET, GimpSet)
#define GIMP_IS_SET(obj) GTK_CHECK_TYPE (obj, gimp_set_get_type())

/* Signals:
   add
   remove
   active_changed
*/

typedef guint GimpSetHandlerId;

GtkType         gimp_set_get_type (void);
GimpSet       * gimp_set_new      (GtkType   type,
				   gboolean  weak);

GtkType		gimp_set_type	  (GimpSet  *set);

gboolean       	gimp_set_add	  (GimpSet  *gimpset,
				   gpointer  ob);
gboolean       	gimp_set_remove	  (GimpSet  *gimpset,
				   gpointer  ob);

gboolean	gimp_set_have	  (GimpSet  *gimpset,
				   gpointer  ob);
void		gimp_set_foreach  (GimpSet  *gimpset,
				   GFunc     func,
				   gpointer  user_data);
gint		gimp_set_size	  (GimpSet* gimpset);

void		gimp_set_set_active (GimpSet  *gimpset,
				     gpointer  ob);
gpointer	gimp_set_get_active (GimpSet  *gimpset);

GimpSetHandlerId gimp_set_add_handler   (GimpSet          *set,
					 const gchar      *signame,
					 GtkSignalFunc     handler,
					 gpointer          user_data);
void		gimp_set_remove_handler (GimpSet          *set,
					 GimpSetHandlerId  id);

#endif /* __GIMP_SET_H__ */
