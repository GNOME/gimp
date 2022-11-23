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

#ifndef __LIGMA_DISPLAY_FOREACH_H__
#define __LIGMA_DISPLAY_FOREACH_H__


gboolean        ligma_displays_dirty            (Ligma      *ligma);
LigmaContainer * ligma_displays_get_dirty_images (Ligma      *ligma);
void            ligma_displays_delete           (Ligma      *ligma);
void            ligma_displays_close            (Ligma      *ligma);
void            ligma_displays_reconnect        (Ligma      *ligma,
                                                LigmaImage *old,
                                                LigmaImage *new);

gint            ligma_displays_get_num_visible  (Ligma      *ligma);

void            ligma_displays_set_busy         (Ligma      *ligma);
void            ligma_displays_unset_busy       (Ligma      *ligma);


#endif /*  __LIGMA_DISPLAY_FOREACH_H__  */
