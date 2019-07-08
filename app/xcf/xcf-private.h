/* GIMP - The GNU Image Manipulation Program
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

#ifndef __XCF_PRIVATE_H__
#define __XCF_PRIVATE_H__


#define XCF_TILE_WIDTH                  64
#define XCF_TILE_HEIGHT                 64
#define XCF_TILE_MAX_DATA_LENGTH_FACTOR 1.5

typedef enum
{
  PROP_END                =  0,
  PROP_COLORMAP           =  1,
  PROP_ACTIVE_LAYER       =  2,
  PROP_ACTIVE_CHANNEL     =  3,
  PROP_SELECTION          =  4,
  PROP_FLOATING_SELECTION =  5,
  PROP_OPACITY            =  6,
  PROP_MODE               =  7,
  PROP_VISIBLE            =  8,
  PROP_LINKED             =  9,
  PROP_LOCK_ALPHA         = 10,
  PROP_APPLY_MASK         = 11,
  PROP_EDIT_MASK          = 12,
  PROP_SHOW_MASK          = 13,
  PROP_SHOW_MASKED        = 14,
  PROP_OFFSETS            = 15,
  PROP_COLOR              = 16,
  PROP_COMPRESSION        = 17,
  PROP_GUIDES             = 18,
  PROP_RESOLUTION         = 19,
  PROP_TATTOO             = 20,
  PROP_PARASITES          = 21,
  PROP_UNIT               = 22,
  PROP_PATHS              = 23,
  PROP_USER_UNIT          = 24,
  PROP_VECTORS            = 25,
  PROP_TEXT_LAYER_FLAGS   = 26,
  PROP_OLD_SAMPLE_POINTS  = 27,
  PROP_LOCK_CONTENT       = 28,
  PROP_GROUP_ITEM         = 29,
  PROP_ITEM_PATH          = 30,
  PROP_GROUP_ITEM_FLAGS   = 31,
  PROP_LOCK_POSITION      = 32,
  PROP_FLOAT_OPACITY      = 33,
  PROP_COLOR_TAG          = 34,
  PROP_COMPOSITE_MODE     = 35,
  PROP_COMPOSITE_SPACE    = 36,
  PROP_BLEND_SPACE        = 37,
  PROP_FLOAT_COLOR        = 38,
  PROP_SAMPLE_POINTS      = 39,
} PropType;

typedef enum
{
  COMPRESS_NONE              =  0,
  COMPRESS_RLE               =  1,
  COMPRESS_ZLIB              =  2,  /* unused */
  COMPRESS_FRACTAL           =  3   /* unused */
} XcfCompressionType;

typedef enum
{
  XCF_ORIENTATION_HORIZONTAL = 1,
  XCF_ORIENTATION_VERTICAL   = 2
} XcfOrientationType;

typedef enum
{
  XCF_STROKETYPE_STROKE        = 0,
  XCF_STROKETYPE_BEZIER_STROKE = 1
} XcfStrokeType;

typedef enum
{
  XCF_GROUP_ITEM_EXPANDED      = 1
} XcfGroupItemFlagsType;

typedef struct _XcfInfo  XcfInfo;

struct _XcfInfo
{
  Gimp               *gimp;
  GimpProgress       *progress;
  GInputStream       *input;
  GOutputStream      *output;
  GSeekable          *seekable;
  goffset             cp;
  gint                bytes_per_offset;
  GFile              *file;
  GimpTattoo          tattoo_state;
  GimpLayer          *active_layer;
  GimpChannel        *active_channel;
  GimpDrawable       *floating_sel_drawable;
  GimpLayer          *floating_sel;
  goffset             floating_sel_offset;
  XcfCompressionType  compression;
  gint                file_version;
};


#endif /* __XCF_PRIVATE_H__ */
