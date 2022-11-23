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

#ifndef __LIGMA_IMAGE_GUIDES_H__
#define __LIGMA_IMAGE_GUIDES_H__


/*  public guide adding API
 */
LigmaGuide * ligma_image_add_hguide     (LigmaImage *image,
                                       gint       position,
                                       gboolean   push_undo);
LigmaGuide * ligma_image_add_vguide     (LigmaImage *image,
                                       gint       position,
                                       gboolean   push_undo);

/*  internal guide adding API, does not check the guide's position and
 *  is publicly declared only to be used from undo
 */
void        ligma_image_add_guide      (LigmaImage *image,
                                       LigmaGuide *guide,
                                       gint       position);

void        ligma_image_remove_guide   (LigmaImage *image,
                                       LigmaGuide *guide,
                                       gboolean   push_undo);
void        ligma_image_move_guide     (LigmaImage *image,
                                       LigmaGuide *guide,
                                       gint       position,
                                       gboolean   push_undo);

GList     * ligma_image_get_guides     (LigmaImage *image);
LigmaGuide * ligma_image_get_guide      (LigmaImage *image,
                                       guint32    id);
LigmaGuide * ligma_image_get_next_guide (LigmaImage *image,
                                       guint32    id,
                                       gboolean  *guide_found);


#endif /* __LIGMA_IMAGE_GUIDES_H__ */
