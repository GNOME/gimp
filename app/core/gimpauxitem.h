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

#ifndef __LIGMA_AUX_ITEM_H__
#define __LIGMA_AUX_ITEM_H__


#define LIGMA_TYPE_AUX_ITEM            (ligma_aux_item_get_type ())
#define LIGMA_AUX_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_AUX_ITEM, LigmaAuxItem))
#define LIGMA_AUX_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_AUX_ITEM, LigmaAuxItemClass))
#define LIGMA_IS_AUX_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_AUX_ITEM))
#define LIGMA_IS_AUX_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_AUX_ITEM))
#define LIGMA_AUX_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_AUX_ITEM, LigmaAuxItemClass))


typedef struct _LigmaAuxItemPrivate LigmaAuxItemPrivate;
typedef struct _LigmaAuxItemClass   LigmaAuxItemClass;

struct _LigmaAuxItem
{
  GObject             parent_instance;

  LigmaAuxItemPrivate *priv;
};

struct _LigmaAuxItemClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void (* removed) (LigmaAuxItem *aux_item);
};


GType     ligma_aux_item_get_type (void) G_GNUC_CONST;

guint32   ligma_aux_item_get_id       (LigmaAuxItem *aux_item);

void      ligma_aux_item_removed      (LigmaAuxItem *aux_item);


#endif /* __LIGMA_AUX_ITEM_H__ */
