/* parasitelist.h: Copyright 1998 Jay Cox <jaycox@earthlink.net>
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
#ifndef __PARASITE_LIST_H__
#define __PARASITE_LIST_H__

#include <glib.h>
#include <stdio.h>

#include "parasitelistF.h"
#include "gimpobject.h"

#include "libgimp/parasiteF.h"


#define GIMP_TYPE_PARASITE_LIST    (parasite_list_get_type ())
#define GIMP_PARASITE_LIST(obj)    (GIMP_CHECK_CAST ((obj), GIMP_TYPE_PARASITE_LIST, GimpParasiteList))
#define GIMP_IS_PARASITE_LIST(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_PARASITE_LIST))

/* Signals:
   add
   remove
*/

GtkType        parasite_list_get_type (void);

/* function declarations */

ParasiteList * parasite_list_new               (void);
ParasiteList * parasite_list_copy              (const ParasiteList *list);
void           parasite_list_add               (ParasiteList       *list,
						GimpParasite       *parasite);
void           parasite_list_remove            (ParasiteList       *list,
						const gchar        *name);
gint           parasite_list_length            (ParasiteList       *list);
gint           parasite_list_persistent_length (ParasiteList       *list);
void           parasite_list_foreach           (ParasiteList       *list,
						GHFunc              function,
						gpointer            user_data);
GimpParasite * parasite_list_find              (ParasiteList       *list,
						const gchar        *name);

void           parasite_shift_parent           (GimpParasite       *parasite);

#endif  /*  __PARASITE_LIST_H__  */
