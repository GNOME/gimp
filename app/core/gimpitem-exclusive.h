/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaitem-exclusive.h
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ITEM_EXCLUSIVE_H__
#define __LIGMA_ITEM_EXCLUSIVE_H__

typedef gboolean        (* LigmaItemIsEnabledFunc)    (LigmaItem  *item);
typedef gboolean        (* LigmaItemCanSetFunc)       (LigmaItem  *item);
typedef void            (* LigmaItemSetFunc)          (LigmaItem  *item,
                                                      gboolean   enabled,
                                                      gboolean   push_undo);
typedef gboolean        (* LigmaItemIsPropLocked)     (LigmaItem  *item);
typedef LigmaUndo      * (*LigmaItemUndoPush)          (LigmaImage     *image,
                                                      const gchar   *undo_desc,
                                                      LigmaItem      *item);

void   ligma_item_toggle_exclusive_visible (LigmaItem             *item,
                                           gboolean              only_selected,
                                           LigmaContext          *context);
void   ligma_item_toggle_exclusive         (LigmaItem             *item,
                                           LigmaItemIsEnabledFunc is_enabled,
                                           LigmaItemSetFunc       set_prop,
                                           LigmaItemCanSetFunc    can_set,
                                           LigmaItemIsPropLocked  is_prop_locked,
                                           LigmaItemUndoPush      undo_push,
                                           const gchar          *undo_desc,
                                           LigmaUndoType          group_undo_type,
                                           gboolean              only_selected,
                                           LigmaContext          *context);


#endif /* __LIGMA_ITEM_EXCLUSIVE_H__ */
