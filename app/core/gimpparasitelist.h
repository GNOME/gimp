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


#define GIMP_TYPE_PARASITE_LIST            (gimp_parasite_list_get_type ())
#define GIMP_PARASITE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PARASITE_LIST, GimpParasiteList))
#define GIMP_PARASITE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PARASITE_LIST, GimpParasiteListClass))
#define GIMP_IS_PARASITE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PARASITE_LIST))
#define GIMP_IS_PARASITE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PARASITE_LIST))
#define GIMP_PARASITE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PARASITE_LIST, GimpParasiteListClass))


typedef struct _GimpParasiteListClass GimpParasiteListClass;

struct _GimpParasiteList
{
  GimpObject  object;

  GHashTable *table;
};

struct _GimpParasiteListClass
{
  GimpObjectClass parent_class;

  void (* add)    (GimpParasiteList *list,
                   GimpParasite     *parasite);
  void (* remove) (GimpParasiteList *list,
                   GimpParasite     *parasite);
};


GType                gimp_parasite_list_get_type (void) G_GNUC_CONST;

GimpParasiteList   * gimp_parasite_list_new      (void);
GimpParasiteList   * gimp_parasite_list_copy     (GimpParasiteList       *list);
void                 gimp_parasite_list_add      (GimpParasiteList       *list,
                                                  const GimpParasite     *parasite);
void                 gimp_parasite_list_remove   (GimpParasiteList       *list,
                                                  const gchar            *name);
gint                 gimp_parasite_list_length   (GimpParasiteList       *list);
gint                 gimp_parasite_list_persistent_length (GimpParasiteList *list);
void                 gimp_parasite_list_foreach  (GimpParasiteList       *list,
                                                  GHFunc                  function,
                                                  gpointer                user_data);
const GimpParasite * gimp_parasite_list_find     (GimpParasiteList       *list,
                                                  const gchar            *name);


#endif  /*  __GIMP_PARASITE_LIST_H__  */
