/* parasitelist.h: Copyright 1998 Jay Cox <jaycox@ligma.org>
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

#ifndef __LIGMA_PARASITE_LIST_H__
#define __LIGMA_PARASITE_LIST_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_PARASITE_LIST            (ligma_parasite_list_get_type ())
#define LIGMA_PARASITE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PARASITE_LIST, LigmaParasiteList))
#define LIGMA_PARASITE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PARASITE_LIST, LigmaParasiteListClass))
#define LIGMA_IS_PARASITE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PARASITE_LIST))
#define LIGMA_IS_PARASITE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PARASITE_LIST))
#define LIGMA_PARASITE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PARASITE_LIST, LigmaParasiteListClass))


typedef struct _LigmaParasiteListClass LigmaParasiteListClass;

struct _LigmaParasiteList
{
  LigmaObject  object;

  GHashTable *table;
};

struct _LigmaParasiteListClass
{
  LigmaObjectClass parent_class;

  void (* add)    (LigmaParasiteList *list,
                   LigmaParasite     *parasite);
  void (* remove) (LigmaParasiteList *list,
                   LigmaParasite     *parasite);
};


GType                ligma_parasite_list_get_type (void) G_GNUC_CONST;

LigmaParasiteList   * ligma_parasite_list_new      (void);
LigmaParasiteList   * ligma_parasite_list_copy     (LigmaParasiteList       *list);
void                 ligma_parasite_list_add      (LigmaParasiteList       *list,
                                                  const LigmaParasite     *parasite);
void                 ligma_parasite_list_remove   (LigmaParasiteList       *list,
                                                  const gchar            *name);
gint                 ligma_parasite_list_length   (LigmaParasiteList       *list);
gint                 ligma_parasite_list_persistent_length (LigmaParasiteList *list);
void                 ligma_parasite_list_foreach  (LigmaParasiteList       *list,
                                                  GHFunc                  function,
                                                  gpointer                user_data);
const LigmaParasite * ligma_parasite_list_find     (LigmaParasiteList       *list,
                                                  const gchar            *name);


#endif  /*  __LIGMA_PARASITE_LIST_H__  */
