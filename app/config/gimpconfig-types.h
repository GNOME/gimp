/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ValueTypes for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONFIG_TYPES_H__
#define __GIMP_CONFIG_TYPES_H__


#define GIMP_TYPE_MATRIX2               (gimp_matrix2_get_type ())
#define GIMP_VALUE_HOLDS_MATRIX2(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_MATRIX2))

GType   gimp_matrix2_get_type           (void) G_GNUC_CONST;


#define GIMP_TYPE_PATH                  (gimp_path_get_type ())
#define GIMP_VALUE_HOLDS_PATH(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_PATH))

GType   gimp_path_get_type              (void) G_GNUC_CONST;


#endif /* __GIMP_CONFIG_TYPES_H__ */
