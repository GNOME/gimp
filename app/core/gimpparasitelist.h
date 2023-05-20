/* parasitelist.h: Copyright 1998 Jay Cox <jaycox@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PARASITE_LIST_H__
#define __GIMP_PARASITE_LIST_H__


#include "gimpobject.h"


#define GIMP_TYPE_PARASITE_LIST (gimp_parasite_list_get_type ())
G_DECLARE_FINAL_TYPE (GimpParasiteList, gimp_parasite_list,
                      GIMP, PARASITE_LIST, GimpObject)

GimpParasiteList   * gimp_parasite_list_new      (void);
GimpParasiteList   * gimp_parasite_list_copy     (GimpParasiteList       *list);
void                 gimp_parasite_list_add      (GimpParasiteList       *list,
                                                  const GimpParasite     *parasite);
void                 gimp_parasite_list_remove   (GimpParasiteList       *list,
                                                  const gchar            *name);
guint                gimp_parasite_list_persistent_length (GimpParasiteList *list);
const GimpParasite * gimp_parasite_list_find     (GimpParasiteList       *list,
                                                  const gchar            *name);
const GimpParasite * gimp_parasite_list_find_full  (GimpParasiteList     *list,
                                                    const gchar          *name,
                                                    guint                *index);
gchar **             gimp_parasite_list_list_names (GimpParasiteList     *list);

#endif  /*  __GIMP_PARASITE_LIST_H__  */
