/* parasitelistP.h: Copyright 1998 Jay Cox <jaycox@earthlink.net>
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
#ifndef __PARASITE_LIST_P_H__
#define __PARASITE_LIST_P_H__

#include "gimpobjectP.h"
#include "parasitelist.h"

struct _ParasiteList
{
  GimpObject  object;
  GHashTable *table;
};

typedef struct _ParasiteListClass
{
  GimpObjectClass parent_class;

  void (* add)    (ParasiteList *list,
		   GimpParasite *parasite);
  void (* remove) (ParasiteList *list,
		   GimpParasite *parasite);
} ParasiteListClass;

#define PARASITE_LIST_CLASS(class) GIMP_CHECK_CLASS_CAST (class, parasite_list_get_type(), ParasiteListClass)

#endif  /*  __PARASITE_LIST_P_H__  */
