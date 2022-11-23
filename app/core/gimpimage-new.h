/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_IMAGE_NEW_H__
#define __LIGMA_IMAGE_NEW_H__


LigmaTemplate * ligma_image_new_get_last_template (Ligma            *ligma,
                                                 LigmaImage       *image);
void           ligma_image_new_set_last_template (Ligma            *ligma,
                                                 LigmaTemplate    *template);

LigmaImage    * ligma_image_new_from_template     (Ligma            *ligma,
                                                 LigmaTemplate    *template,
                                                 LigmaContext     *context);
LigmaImage    * ligma_image_new_from_drawable     (Ligma            *ligma,
                                                 LigmaDrawable    *drawable);
LigmaImage    * ligma_image_new_from_drawables    (Ligma            *ligma,
                                                 GList           *drawables,
                                                 gboolean         copy_selection,
                                                 gboolean         tag_copies);
LigmaImage    * ligma_image_new_from_component    (Ligma            *ligma,
                                                 LigmaImage       *image,
                                                 LigmaChannelType  component);
LigmaImage    * ligma_image_new_from_buffer       (Ligma            *ligma,
                                                 LigmaBuffer      *buffer);
LigmaImage    * ligma_image_new_from_pixbuf       (Ligma            *ligma,
                                                 GdkPixbuf       *pixbuf,
                                                 const gchar     *layer_name);


#endif /* __LIGMA_IMAGE_NEW__ */
