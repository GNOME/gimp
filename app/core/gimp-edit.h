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

#ifndef __LIGMA_EDIT_H__
#define __LIGMA_EDIT_H__


LigmaObject  * ligma_edit_cut                (LigmaImage       *image,
                                            GList           *drawables,
                                            LigmaContext     *context,
                                            GError         **error);
LigmaObject  * ligma_edit_copy               (LigmaImage       *image,
                                            GList           *drawables,
                                            LigmaContext     *context,
                                            GError         **error);
LigmaBuffer  * ligma_edit_copy_visible       (LigmaImage       *image,
                                            LigmaContext     *context,
                                            GError         **error);

GList       * ligma_edit_paste              (LigmaImage       *image,
                                            GList           *drawables,
                                            LigmaObject      *paste,
                                            LigmaPasteType    paste_type,
                                            LigmaContext     *context,
                                            gboolean         merged,
                                            gint             viewport_x,
                                            gint             viewport_y,
                                            gint             viewport_width,
                                            gint             viewport_height);
LigmaImage   * ligma_edit_paste_as_new_image (Ligma            *ligma,
                                            LigmaObject      *paste);

const gchar * ligma_edit_named_cut          (LigmaImage       *image,
                                            const gchar     *name,
                                            GList           *drawables,
                                            LigmaContext     *context,
                                            GError         **error);
const gchar * ligma_edit_named_copy         (LigmaImage       *image,
                                            const gchar     *name,
                                            GList           *drawables,
                                            LigmaContext     *context,
                                            GError         **error);
const gchar * ligma_edit_named_copy_visible (LigmaImage       *image,
                                            const gchar     *name,
                                            LigmaContext     *context,
                                            GError         **error);


#endif  /*  __LIGMA_EDIT_H__  */
