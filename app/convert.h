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
#ifndef __CONVERT_H__
#define __CONVERT_H__

#include "procedural_db.h"
#include "gimpimageF.h"
#include "palette_entries.h"

/* adam's extra palette stuff */
typedef enum {
  MAKE_PALETTE,
  REUSE_PALETTE,
  WEB_PALETTE,
  MONO_PALETTE,
  CUSTOM_PALETTE
} ConvertPaletteType;

#define MAXNUMCOLORS 256

/*  convert functions  */
void convert_to_rgb        (GimpImage *);
void convert_to_grayscale  (GimpImage *);
void convert_to_indexed    (GimpImage *);

void convert_image         (GimpImage *, int, int, int, int);

extern PaletteEntriesP theCustomPalette;

#endif  /*  __CONVERT_H__  */
