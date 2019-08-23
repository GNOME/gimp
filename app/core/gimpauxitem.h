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

#ifndef __GIMP_AUX_ITEM_H__
#define __GIMP_AUX_ITEM_H__


#define GIMP_TYPE_AUX_ITEM            (gimp_aux_item_get_type ())
#define GIMP_AUX_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_AUX_ITEM, GimpAuxItem))
#define GIMP_AUX_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_AUX_ITEM, GimpAuxItemClass))
#define GIMP_IS_AUX_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_AUX_ITEM))
#define GIMP_IS_AUX_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_AUX_ITEM))
#define GIMP_AUX_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_AUX_ITEM, GimpAuxItemClass))


typedef struct _GimpAuxItemPrivate GimpAuxItemPrivate;
typedef struct _GimpAuxItemClass   GimpAuxItemClass;

struct _GimpAuxItem
{
  GObject             parent_instance;

  GimpAuxItemPrivate *priv;
};

struct _GimpAuxItemClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void (* removed) (GimpAuxItem *aux_item);
};


GType     gimp_aux_item_get_type (void) G_GNUC_CONST;

guint32   gimp_aux_item_get_id       (GimpAuxItem *aux_item);

void      gimp_aux_item_removed      (GimpAuxItem *aux_item);


#endif /* __GIMP_AUX_ITEM_H__ */
