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

#ifndef __LIGMA_IMAGE__UNDO_H__
#define __LIGMA_IMAGE__UNDO_H__


gboolean        ligma_image_undo_is_enabled      (LigmaImage     *image);
gboolean        ligma_image_undo_enable          (LigmaImage     *image);
gboolean        ligma_image_undo_disable         (LigmaImage     *image);
gboolean        ligma_image_undo_freeze          (LigmaImage     *image);
gboolean        ligma_image_undo_thaw            (LigmaImage     *image);

gboolean        ligma_image_undo                 (LigmaImage     *image);
gboolean        ligma_image_redo                 (LigmaImage     *image);

gboolean        ligma_image_strong_undo          (LigmaImage     *image);
gboolean        ligma_image_strong_redo          (LigmaImage     *image);

LigmaUndoStack * ligma_image_get_undo_stack       (LigmaImage     *image);
LigmaUndoStack * ligma_image_get_redo_stack       (LigmaImage     *image);

void            ligma_image_undo_free            (LigmaImage     *image);

gint            ligma_image_get_undo_group_count (LigmaImage     *image);
gboolean        ligma_image_undo_group_start     (LigmaImage     *image,
                                                 LigmaUndoType   undo_type,
                                                 const gchar   *name);
gboolean        ligma_image_undo_group_end       (LigmaImage     *image);

LigmaUndo      * ligma_image_undo_push            (LigmaImage     *image,
                                                 GType          object_type,
                                                 LigmaUndoType   undo_type,
                                                 const gchar   *name,
                                                 LigmaDirtyMask  dirty_mask,
                                                 ...) G_GNUC_NULL_TERMINATED;

LigmaUndo      * ligma_image_undo_can_compress    (LigmaImage     *image,
                                                 GType          object_type,
                                                 LigmaUndoType   undo_type);


#endif /* __LIGMA_IMAGE__UNDO_H__ */
