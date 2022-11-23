/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawable-filters.h
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

#ifndef __LIGMA_DRAWABLE_FILTERS_H__
#define __LIGMA_DRAWABLE_FILTERS_H__


LigmaContainer * ligma_drawable_get_filters   (LigmaDrawable *drawable);

gboolean        ligma_drawable_has_filters   (LigmaDrawable *drawable);

void            ligma_drawable_add_filter    (LigmaDrawable *drawable,
                                             LigmaFilter   *filter);
void            ligma_drawable_remove_filter (LigmaDrawable *drawable,
                                             LigmaFilter   *filter);

gboolean        ligma_drawable_has_filter    (LigmaDrawable *drawable,
                                             LigmaFilter   *filter);

gboolean        ligma_drawable_merge_filter  (LigmaDrawable *drawable,
                                             LigmaFilter   *filter,
                                             LigmaProgress *progress,
                                             const gchar  *undo_desc,
                                             const Babl   *format,
                                             gboolean      clip,
                                             gboolean      cancellable,
                                             gboolean      update);


#endif /* __LIGMA_DRAWABLE_FILTERS_H__ */
