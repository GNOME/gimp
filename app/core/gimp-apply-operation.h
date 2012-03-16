/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-apply-operation.h
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_APPLY_OPERATION_H__
#define __GIMP_APPLY_OPERATION_H__


void   gimp_apply_operation (GeglBuffer          *src_buffer,
                             GimpProgress        *progress,
                             const gchar         *undo_desc,
                             GeglNode            *operation,
                             gboolean             linear,
                             GeglBuffer          *dest_buffer,
                             const GeglRectangle *dest_rect);


#endif /* __GIMP_APPLY_OPERATION_H__ */
