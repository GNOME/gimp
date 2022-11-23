/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaitem.h
 * Copyright (C) Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_ITEM_H__
#define __LIGMA_ITEM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_ITEM (ligma_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaItem, ligma_item, LIGMA, ITEM, GObject)


struct _LigmaItemClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


gint32      ligma_item_get_id        (LigmaItem *item);
LigmaItem  * ligma_item_get_by_id     (gint32    item_id);

gboolean    ligma_item_is_valid      (LigmaItem *item);
gboolean    ligma_item_is_drawable   (LigmaItem *item);
gboolean    ligma_item_is_layer      (LigmaItem *item);
gboolean    ligma_item_is_text_layer (LigmaItem *item);
gboolean    ligma_item_is_channel    (LigmaItem *item);
gboolean    ligma_item_is_layer_mask (LigmaItem *item);
gboolean    ligma_item_is_selection  (LigmaItem *item);
gboolean    ligma_item_is_vectors    (LigmaItem *item);

GList     * ligma_item_list_children (LigmaItem *item);


G_END_DECLS

#endif /* __LIGMA_ITEM_H__ */
