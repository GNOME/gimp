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

#ifndef __XCF_PRIVATE_H__
#define __XCF_PRIVATE_H__


typedef enum
{
  PROP_END                   =  0,
  PROP_COLORMAP              =  1,
  PROP_ACTIVE_LAYER          =  2,
  PROP_ACTIVE_CHANNEL        =  3,
  PROP_SELECTION             =  4,
  PROP_FLOATING_SELECTION    =  5,
  PROP_OPACITY               =  6,
  PROP_MODE                  =  7,
  PROP_VISIBLE               =  8,
  PROP_LINKED                =  9,
  PROP_PRESERVE_TRANSPARENCY = 10,
  PROP_APPLY_MASK            = 11,
  PROP_EDIT_MASK             = 12,
  PROP_SHOW_MASK             = 13,
  PROP_SHOW_MASKED           = 14,
  PROP_OFFSETS               = 15,
  PROP_COLOR                 = 16,
  PROP_COMPRESSION           = 17,
  PROP_GUIDES                = 18,
  PROP_RESOLUTION            = 19,
  PROP_TATTOO                = 20,
  PROP_PARASITES             = 21,
  PROP_UNIT                  = 22,
  PROP_PATHS                 = 23,
  PROP_USER_UNIT             = 24,
  PROP_VECTORS               = 25,
  PROP_TEXT_LAYER_FLAGS      = 26
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

typedef struct _XcfInfo  XcfInfo;

struct _XcfInfo
{
  FILE               *fp;
  guint               cp;
  const gchar        *filename;
  GimpTattoo          tattoo_state;
  GimpLayer          *active_layer;
  GimpChannel        *active_channel;
  GimpDrawable       *floating_sel_drawable;
  GimpLayer          *floating_sel;
  guint               floating_sel_offset;
  gint                swap_num;
  gint               *ref_count;
  XcfCompressionType  compression;
  gint                file_version;
};


#endif /* __XCF_PRIVATE_H__ */
