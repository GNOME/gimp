/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawable-operation.h
 * Copyright (C) 2007 Øyvind Kolås <pippin@ligma.org>
 *                    Sven Neumann <sven@ligma.org>
 *                    Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DRAWABLE_OPERATION_H__
#define __LIGMA_DRAWABLE_OPERATION_H__


void   ligma_drawable_apply_operation             (LigmaDrawable *drawable,
                                                  LigmaProgress *progress,
                                                  const gchar  *undo_desc,
                                                  GeglNode     *operation);
void   ligma_drawable_apply_operation_with_config (LigmaDrawable *drawable,
                                                  LigmaProgress *progress,
                                                  const gchar  *undo_desc,
                                                  GeglNode     *operation,
                                                  GObject      *config);
void   ligma_drawable_apply_operation_by_name     (LigmaDrawable *drawable,
                                                  LigmaProgress *progress,
                                                  const gchar  *undo_desc,
                                                  const gchar  *operation_type,
                                                  GObject      *config);


#endif /* __LIGMA_DRAWABLE_OPERATION_H__ */
