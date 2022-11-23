/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_ITEM_LIST_H__
#define __LIGMA_ITEM_LIST_H__


#define LIGMA_TYPE_ITEM_LIST            (ligma_item_list_get_type ())
#define LIGMA_ITEM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM_LIST, LigmaItemList))
#define LIGMA_ITEM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM_LIST, LigmaItemListClass))
#define LIGMA_IS_ITEM_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM_LIST))
#define LIGMA_IS_ITEM_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM_LIST))
#define LIGMA_ITEM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ITEM_LIST, LigmaItemListClass))


typedef struct _LigmaItemListClass   LigmaItemListClass;
typedef struct _LigmaItemListPrivate LigmaItemListPrivate;

struct _LigmaItemList
{
  LigmaObject          parent_instance;

  LigmaItemListPrivate *p;
};

struct _LigmaItemListClass
{
  LigmaObjectClass  parent_class;

  /*  signals  */
  void  (* empty) (LigmaItemList *set);
};


GType          ligma_item_list_get_type  (void) G_GNUC_CONST;

LigmaItemList  * ligma_item_list_named_new    (LigmaImage        *image,
                                             GType             item_type,
                                             const gchar      *name,
                                             GList            *items);

LigmaItemList  * ligma_item_list_pattern_new  (LigmaImage        *image,
                                             GType             item_type,
                                             LigmaSelectMethod  pattern_syntax,
                                             const gchar      *pattern);

GType          ligma_item_list_get_item_type (LigmaItemList     *set);
GList        * ligma_item_list_get_items     (LigmaItemList     *set,
                                             GError          **error);
gboolean       ligma_item_list_is_pattern    (LigmaItemList     *set,
                                             LigmaSelectMethod *pattern_syntax);

void           ligma_item_list_add           (LigmaItemList     *set,
                                             LigmaItem         *item);

#endif /* __LIGMA_ITEM_LIST_H__ */
