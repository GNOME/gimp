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

#include "apptypes.h"
#include "procedural_db.h"
#include "gimpimageF.h"
#include "palette_entries.h"

/* adam's extra palette stuff */
typedef enum
{
  MAKE_PALETTE   = 0,
  REUSE_PALETTE  = 1,
  WEB_PALETTE    = 2,
  MONO_PALETTE   = 3,
  CUSTOM_PALETTE = 4
} ConvertPaletteType;

/* adam's extra dither stuff */
typedef enum
{
  NO_DITHER         = 0,
  FS_DITHER         = 1,
  FSLOWBLEED_DITHER = 2,
  FIXED_DITHER      = 3,

  NODESTRUCT_DITHER = 4 /* NEVER USE NODESTRUCT_DITHER EXPLICITLY */
} ConvertDitherType;

#define MAXNUMCOLORS 256

/*  convert functions  */
void convert_to_rgb        (GimpImage *);
void convert_to_grayscale  (GimpImage *);
void convert_to_indexed    (GimpImage *);

void convert_image        (GimpImage *,
			   GimpImageBaseType,
			   gint num_cols,
			   ConvertDitherType,
			   gint alpha_dither,
			   gint remdups,
			   ConvertPaletteType);

extern PaletteEntries *theCustomPalette;

#endif  /*  __CONVERT_H__  */
