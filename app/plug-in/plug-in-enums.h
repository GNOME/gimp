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

#ifndef __PLUG_IN_ENUMS_H__
#define __PLUG_IN_ENUMS_H__


#define GIMP_TYPE_PLUG_IN_IMAGE_TYPE (gimp_plug_in_image_type_get_type ())

GType gimp_plug_in_image_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_PLUG_IN_RGB_IMAGE      = 1 << 0,
  GIMP_PLUG_IN_GRAY_IMAGE     = 1 << 1,
  GIMP_PLUG_IN_INDEXED_IMAGE  = 1 << 2,
  GIMP_PLUG_IN_RGBA_IMAGE     = 1 << 3,
  GIMP_PLUG_IN_GRAYA_IMAGE    = 1 << 4,
  GIMP_PLUG_IN_INDEXEDA_IMAGE = 1 << 5
} GimpPlugInImageType;


#define GIMP_TYPE_PLUG_CALL_MODE (gimp_plug_in_call_mode_get_type ())

GType gimp_plug_in_call_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_PLUG_IN_CALL_NONE,
  GIMP_PLUG_IN_CALL_RUN,
  GIMP_PLUG_IN_CALL_QUERY,
  GIMP_PLUG_IN_CALL_INIT
} GimpPlugInCallMode;


#endif /* __PLUG_IN_ENUMS_H__ */
