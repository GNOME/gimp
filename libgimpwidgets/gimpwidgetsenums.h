/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_WIDGETS_ENUMS_H__
#define __GIMP_WIDGETS_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpAspectType:
 * @GIMP_ASPECT_SQUARE:    it's a 1:1 square
 * @GIMP_ASPECT_PORTRAIT:  it's higher than it's wide
 * @GIMP_ASPECT_LANDSCAPE: it's wider than it's high
 *
 * Aspect ratios.
 **/
#define GIMP_TYPE_ASPECT_TYPE (gimp_aspect_type_get_type ())

GType gimp_aspect_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ASPECT_SQUARE,    /*< desc="Square"    >*/
  GIMP_ASPECT_PORTRAIT,  /*< desc="Portrait"  >*/
  GIMP_ASPECT_LANDSCAPE  /*< desc="Landscape" >*/
} GimpAspectType;


/**
 * GimpChainPosition:
 * @GIMP_CHAIN_TOP:    the chain is on top
 * @GIMP_CHAIN_LEFT:   the chain is to the left
 * @GIMP_CHAIN_BOTTOM: the chain is on bottom
 * @GIMP_CHAIN_RIGHT:  the chain is to the right
 *
 * Possible chain positions for #GimpChainButton.
 **/
#define GIMP_TYPE_CHAIN_POSITION (gimp_chain_position_get_type ())

GType gimp_chain_position_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CHAIN_TOP,
  GIMP_CHAIN_LEFT,
  GIMP_CHAIN_BOTTOM,
  GIMP_CHAIN_RIGHT
} GimpChainPosition;


/**
 * GimpColorAreaType:
 * @GIMP_COLOR_AREA_FLAT:         don't display transparency
 * @GIMP_COLOR_AREA_SMALL_CHECKS: display transparency using small checks
 * @GIMP_COLOR_AREA_LARGE_CHECKS: display transparency using large checks
 *
 * The types of transparency display for #GimpColorArea.
 **/
#define GIMP_TYPE_COLOR_AREA_TYPE (gimp_color_area_type_get_type ())

GType gimp_color_area_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_AREA_FLAT = 0,
  GIMP_COLOR_AREA_SMALL_CHECKS,
  GIMP_COLOR_AREA_LARGE_CHECKS
} GimpColorAreaType;


/**
 * GimpColorSelectorChannel:
 * @GIMP_COLOR_SELECTOR_HUE:            the hue channel
 * @GIMP_COLOR_SELECTOR_SATURATION:     the saturation channel
 * @GIMP_COLOR_SELECTOR_VALUE:          the value channel
 * @GIMP_COLOR_SELECTOR_RED:            the red channel
 * @GIMP_COLOR_SELECTOR_GREEN:          the green channel
 * @GIMP_COLOR_SELECTOR_BLUE:           the blue channel
 * @GIMP_COLOR_SELECTOR_ALPHA:          the alpha channel
 * @GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:  the lightness channel
 * @GIMP_COLOR_SELECTOR_LCH_CHROMA:     the chroma channel
 * @GIMP_COLOR_SELECTOR_LCH_HUE:        the hue channel
 *
 * An enum to specify the types of color channels edited in
 * #GimpColorSelector widgets.
 **/
#define GIMP_TYPE_COLOR_SELECTOR_CHANNEL (gimp_color_selector_channel_get_type ())

GType gimp_color_selector_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_SELECTOR_HUE,           /*< desc="_H", help="HSV Hue"        >*/
  GIMP_COLOR_SELECTOR_SATURATION,    /*< desc="_S", help="HSV Saturation" >*/
  GIMP_COLOR_SELECTOR_VALUE,         /*< desc="_V", help="HSV Value"      >*/
  GIMP_COLOR_SELECTOR_RED,           /*< desc="_R", help="Red"            >*/
  GIMP_COLOR_SELECTOR_GREEN,         /*< desc="_G", help="Green"          >*/
  GIMP_COLOR_SELECTOR_BLUE,          /*< desc="_B", help="Blue"           >*/
  GIMP_COLOR_SELECTOR_ALPHA,         /*< desc="_A", help="Alpha"          >*/
  GIMP_COLOR_SELECTOR_LCH_LIGHTNESS, /*< desc="_L", help="LCh Lightness"  >*/
  GIMP_COLOR_SELECTOR_LCH_CHROMA,    /*< desc="_C", help="LCh Chroma"     >*/
  GIMP_COLOR_SELECTOR_LCH_HUE        /*< desc="_h", help="LCh Hue"        >*/
} GimpColorSelectorChannel;


/**
 * GimpColorSelectorModel:
 * @GIMP_COLOR_SELECTOR_MODEL_RGB: RGB color model
 * @GIMP_COLOR_SELECTOR_MODEL_LCH: CIE LCh color model
 * @GIMP_COLOR_SELECTOR_MODEL_HSV: HSV color model
 *
 * An enum to specify the types of color spaces edited in
 * #GimpColorSelector widgets.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_COLOR_SELECTOR_MODEL (gimp_color_selector_model_get_type ())

GType gimp_color_selector_model_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_SELECTOR_MODEL_RGB, /*< desc="RGB", help="RGB color model"     >*/
  GIMP_COLOR_SELECTOR_MODEL_LCH, /*< desc="LCH", help="CIE LCh color model" >*/
  GIMP_COLOR_SELECTOR_MODEL_HSV  /*< desc="HSV", help="HSV color model"     >*/
} GimpColorSelectorModel;


/**
 * GimpIntComboBoxLayout:
 * @GIMP_INT_COMBO_BOX_LAYOUT_ICON_ONLY:   show icons only
 * @GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED: show icons and abbreviated labels,
 *                                         when available
 * @GIMP_INT_COMBO_BOX_LAYOUT_FULL:        show icons and full labels
 *
 * Possible layouts for #GimpIntComboBox.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_INT_COMBO_BOX_LAYOUT (gimp_int_combo_box_layout_get_type ())

GType gimp_int_combo_box_layout_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INT_COMBO_BOX_LAYOUT_ICON_ONLY,
  GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED,
  GIMP_INT_COMBO_BOX_LAYOUT_FULL
} GimpIntComboBoxLayout;


/**
 * GimpPageSelectorTarget:
 * @GIMP_PAGE_SELECTOR_TARGET_LAYERS: import as layers of one image
 * @GIMP_PAGE_SELECTOR_TARGET_IMAGES: import as separate images
 *
 * Import targets for #GimpPageSelector.
 **/
#define GIMP_TYPE_PAGE_SELECTOR_TARGET (gimp_page_selector_target_get_type ())

GType gimp_page_selector_target_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAGE_SELECTOR_TARGET_LAYERS, /*< desc="Layers" >*/
  GIMP_PAGE_SELECTOR_TARGET_IMAGES  /*< desc="Images" >*/
} GimpPageSelectorTarget;


/**
 * GimpSizeEntryUpdatePolicy:
 * @GIMP_SIZE_ENTRY_UPDATE_NONE:       the size entry's meaning is up to the user
 * @GIMP_SIZE_ENTRY_UPDATE_SIZE:       the size entry displays values
 * @GIMP_SIZE_ENTRY_UPDATE_RESOLUTION: the size entry displays resolutions
 *
 * Update policies for #GimpSizeEntry.
 **/
#define GIMP_TYPE_SIZE_ENTRY_UPDATE_POLICY (gimp_size_entry_update_policy_get_type ())

GType gimp_size_entry_update_policy_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SIZE_ENTRY_UPDATE_NONE       = 0,
  GIMP_SIZE_ENTRY_UPDATE_SIZE       = 1,
  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} GimpSizeEntryUpdatePolicy;


/**
 * GimpZoomType:
 * @GIMP_ZOOM_IN:       zoom in
 * @GIMP_ZOOM_OUT:      zoom out
 * @GIMP_ZOOM_IN_MORE:  zoom in a lot
 * @GIMP_ZOOM_OUT_MORE: zoom out a lot
 * @GIMP_ZOOM_IN_MAX:   zoom in as far as possible
 * @GIMP_ZOOM_OUT_MAX:  zoom out as far as possible
 * @GIMP_ZOOM_TO:       zoom to a specific zoom factor
 *
 * the zoom types for #GimpZoomModel.
 **/
#define GIMP_TYPE_ZOOM_TYPE (gimp_zoom_type_get_type ())

GType gimp_zoom_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ZOOM_IN,        /*< desc="Zoom in"  >*/
  GIMP_ZOOM_OUT,       /*< desc="Zoom out" >*/
  GIMP_ZOOM_IN_MORE,   /*< skip >*/
  GIMP_ZOOM_OUT_MORE,  /*< skip >*/
  GIMP_ZOOM_IN_MAX,    /*< skip >*/
  GIMP_ZOOM_OUT_MAX,   /*< skip >*/
  GIMP_ZOOM_TO         /*< skip >*/
} GimpZoomType;


G_END_DECLS

#endif  /* __GIMP_WIDGETS_ENUMS_H__ */
