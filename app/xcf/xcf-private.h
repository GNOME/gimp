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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XCF_PRIVATE_H__
#define __XCF_PRIVATE_H__

/**
 * SECTION:xcf-private
 * @Short_description:Common definitions for the XCF functions
 *
 * Common enum and struct definitions for the XCF functions
 */


/**
* PropType:
* @PROP_END:                 marks the end of any property list
* @PROP_COLORMAP:            stores the color map in indexed images
* @PROP_ACTIVE_LAYER:        marks the active layer in an image
* @PROP_ACTIVE_CHANNEL:      marks the active channel in an image
* @PROP_SELECTION:           marks the selection
* @PROP_FLOATING_SELECTION:  marks the layer that is the floating selection
* @PROP_OPACITY:             specifies the layer's or channel opacity
* @PROP_MODE:                layer mode of the layer
* @PROP_VISIBLE:             specifies the visibility of a layer or channel
* @PROP_LINKED:              controls the behavior of Transform tools with
*                            linked elements (layers, channels, paths)
* @PROP_LOCK_ALPHA:          prevents drawing tools from increasing alpha of any
*                            pixel in the layer
* @PROP_APPLY_MASK:          specifies whether the layer mask shall be applied
*                            to the layer
* @PROP_EDIT_MASK:           specifies whether the layer mask is currently being
*                            edited
* @PROP_SHOW_MASK:           specifies whether the layer mask is visible
* @PROP_SHOW_MASKED:         specifies whether a channel is shown as a mask
* @PROP_OFFSETS:             gives the coordinates of the upper left corner of
*                            the layer relative to the upper left corner of the
*                            canvas
* @PROP_COLOR:               specifies the color of the screen that is used to
*                            represent the channel when it is visible in the UI
* @PROP_COMPRESSION:         defines the encoding of pixels in tile data blocks
*                            in the entire XCF file
* @PROP_GUIDES:              stores the horizontal or vertical positions of
*                            guides
* @PROP_RESOLUTION:          specifies the intended physical size of the image's
*                            pixels
* @PROP_TATTOO:              unique identifier for the denoted image, channel or
*                            layer
* @PROP_PARASITES:           stores parasites
* @PROP_UNIT:                specifies the units used to specify resolution in
*                            the Scale Image and Print Size dialogs
* @PROP_PATHS:               stores the paths (old method up to GIMP 1.2)
* @PROP_USER_UNIT:           defines non-standard units
* @PROP_VECTORS:             stores the paths (since GIMP 1.3.21)
* @PROP_TEXT_LAYER_FLAGS:    specifies the text layer behavior
* @PROP_SAMPLE_POINTS:       specifies the sample points
* @PROP_LOCK_CONTENT:        specifies whether the layer, channel or path cannot
*                            be edited
* @PROP_GROUP_ITEM:          indicates that the layer is a layer group
* @PROP_ITEM_PATH:           TODO describe
* @PROP_GROUP_ITEM_FLAGS:    specifies flags for the layer group
*
* Enum for property types.
*/
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
  PROP_SAMPLE_POINTS      = 27,
  PROP_LOCK_CONTENT       = 28,
  PROP_GROUP_ITEM         = 29,
  PROP_ITEM_PATH          = 30,
  PROP_GROUP_ITEM_FLAGS   = 31
} PropType;

/**
 * XcfCompressionType:
 * @COMPRESS_NONE:    no compression
 * @COMPRESS_RLE:     Run-Length-Encoding
 * @COMPRESS_ZLIB:    reserved for zlib-based compression
 * @COMPRESS_FRACTAL: reserved for fractal compression
 *
 * Enum for image compression types. Currently we only use @COMPRESS_RLE
 */
typedef enum
{
  COMPRESS_NONE              =  0,
  COMPRESS_RLE               =  1,
  COMPRESS_ZLIB              =  2,  /* unused */
  COMPRESS_FRACTAL           =  3   /* unused */
} XcfCompressionType;

/**
 * XcfOrientationType:
 * @XCF_ORIENTATION_HORIZONTAL: Horizontal guide
 * @XCF_ORIENTATION_VERTICAL:   Vertical guide
 *
 * Enum for guide orientations
 */
typedef enum
{
  XCF_ORIENTATION_HORIZONTAL = 1,
  XCF_ORIENTATION_VERTICAL   = 2
} XcfOrientationType;


/**
 * XcfStrokeType:
 * @XCF_STROKETYPE_STROKE:        Vector stroke is a plain stroke (??, unused)
 * @XCF_STROKETYPE_BEZIER_STROKE: Vector stroke is a Bezier stroke.
 *
 * Enum for path strokes
 */

 /* TODO: verify XCF_STROKETYPE_STROKE
 * TODO: unused - use or remove
 */
typedef enum
{
  XCF_STROKETYPE_STROKE        = 0,
  XCF_STROKETYPE_BEZIER_STROKE = 1
} XcfStrokeType;

/**
 * XcfGroupItemFlagsType:
 * @XCF_GROUP_ITEM_EXPANDED: specfies whether the layer group is expanded.
 *
 * Enum for group item flags
 */
typedef enum
{
  XCF_GROUP_ITEM_EXPANDED      = 1
} XcfGroupItemFlagsType;

/**
* XcfInfo:
* @gimp:                  #Gimp instance
* @progress:              progress indicator
* @fp:                    file stream of the XCF file
* @cp:                    position in the XCF file.
* @filename:              image filename
* @tattoo_state:          tattoo state associated with the image
* @active_layer:          active layer of the image
* @active_channel:        active channel of the image
* @floating_sel_drawable: the drawable the floating layer is attached to
* @floating_sel:          floating selection of the image
* @floating_sel_offset:   the floating selection's position in the XCF file
* @swap_num:              unused (TODO: use or remove)
* @ref_count:             unused (TODO: use or remove)
* @compression:           file compression (see @XcfCompressionType)
* @file_version:          file format version (see xcf_save_choose_format())
*
* XCF file information structure.
*/
typedef struct _XcfInfo  XcfInfo;

struct _XcfInfo
{
  Gimp               *gimp;
  GimpProgress       *progress;
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
