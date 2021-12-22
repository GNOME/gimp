/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_ITEM_LIST_H__
#define __GIMP_ITEM_LIST_H__


#define GIMP_TYPE_ITEM_LIST            (gimp_item_list_get_type ())
#define GIMP_ITEM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_LIST, GimpItemList))
#define GIMP_ITEM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_LIST, GimpItemListClass))
#define GIMP_IS_ITEM_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_LIST))
#define GIMP_IS_ITEM_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_LIST))
#define GIMP_ITEM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_LIST, GimpItemListClass))


typedef struct _GimpItemListClass   GimpItemListClass;
typedef struct _GimpItemListPrivate GimpItemListPrivate;

struct _GimpItemList
{
  GimpObject          parent_instance;

  GimpItemListPrivate *p;
};

struct _GimpItemListClass
{
  GimpObjectClass  parent_class;

  /*  signals  */
  void  (* empty) (GimpItemList *set);
};


GType          gimp_item_list_get_type  (void) G_GNUC_CONST;

GimpItemList  * gimp_item_list_named_new    (GimpImage        *image,
                                             GType             item_type,
                                             const gchar      *name,
                                             GList            *items);

GimpItemList  * gimp_item_list_pattern_new  (GimpImage        *image,
                                             GType             item_type,
                                             GimpSelectMethod  pattern_syntax,
                                             const gchar      *pattern);

GType          gimp_item_list_get_item_type (GimpItemList     *set);
GList        * gimp_item_list_get_items     (GimpItemList     *set,
                                             GError          **error);
gboolean       gimp_item_list_is_pattern    (GimpItemList     *set,
                                             GimpSelectMethod *pattern_syntax);

void           gimp_item_list_add           (GimpItemList     *set,
                                             GimpItem         *item);

#endif /* __GIMP_ITEM_LIST_H__ */
