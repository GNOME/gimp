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
#include "libgimp/parasiteF.h"
#include "parasitelistF.h"
#include "gimpobject.h"

#define GIMP_TYPE_PARASITE_LIST  (parasite_list_get_type ())
#define GIMP_PARASITE_LIST(obj)  (GIMP_CHECK_CAST ((obj), GIMP_TYPE_PARASITE_LIST, GimpParasiteList))
#define GIMP_IS_PARASITE_LIST(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_PARASITE_LIST))

/* Signals:
   add
   remove
*/

GtkType       parasite_list_get_type (void);

/* function declarations */

ParasiteList *parasite_list_new      (void);
ParasiteList *parasite_list_copy     (const ParasiteList *list);
void          parasite_list_add      (ParasiteList *list, Parasite *p);
void          parasite_list_remove   (ParasiteList *list, const char *name);
gint          parasite_list_length   (ParasiteList *list);
gint          parasite_list_persistent_length (ParasiteList *list);
void          parasite_list_foreach  (ParasiteList *list, GHFunc function,
				      gpointer user_data);
Parasite     *parasite_list_find     (ParasiteList *list, const char *name);

int           parasite_list_save     (ParasiteList *list, FILE *fp);
void          parasite_list_load     (ParasiteList *list, FILE *fp);
void          parasite_shift_parent  (Parasite *p);

#endif  /*  __GIMP_PARASITE_H__  */
