/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __CORE_ENUMS_H__
#define __CORE_ENUMS_H__

/* These enums that are registered with the type system. */


#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,
  GIMP_GRAY,
  GIMP_INDEXED
} GimpImageBaseType;


#define GIMP_TYPE_PREVIEW_SIZE (gimp_preview_size_get_type ())

GType gimp_preview_size_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_PREVIEW_SIZE_NONE        = 0,
  GIMP_PREVIEW_SIZE_TINY        = 16,
  GIMP_PREVIEW_SIZE_EXTRA_SMALL = 24,
  GIMP_PREVIEW_SIZE_SMALL       = 32,
  GIMP_PREVIEW_SIZE_MEDIUM      = 48,
  GIMP_PREVIEW_SIZE_LARGE       = 64,
  GIMP_PREVIEW_SIZE_EXTRA_LARGE = 96,
  GIMP_PREVIEW_SIZE_HUGE        = 128,
  GIMP_PREVIEW_SIZE_ENORMOUS    = 192,
  GIMP_PREVIEW_SIZE_GIGANTIC    = 256
} GimpPreviewSize;

#endif /* __CORE_TYPES_H__ */
