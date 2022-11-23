/* LIBLIGMA - The LIGMA Library
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

#ifndef __LIGMA_WIDGETS_ENUMS_H__
#define __LIGMA_WIDGETS_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaAspectType:
 * @LIGMA_ASPECT_SQUARE:    it's a 1:1 square
 * @LIGMA_ASPECT_PORTRAIT:  it's higher than it's wide
 * @LIGMA_ASPECT_LANDSCAPE: it's wider than it's high
 *
 * Aspect ratios.
 **/
#define LIGMA_TYPE_ASPECT_TYPE (ligma_aspect_type_get_type ())

GType ligma_aspect_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ASPECT_SQUARE,    /*< desc="Square"    >*/
  LIGMA_ASPECT_PORTRAIT,  /*< desc="Portrait"  >*/
  LIGMA_ASPECT_LANDSCAPE  /*< desc="Landscape" >*/
} LigmaAspectType;


/**
 * LigmaChainPosition:
 * @LIGMA_CHAIN_TOP:    the chain is on top
 * @LIGMA_CHAIN_LEFT:   the chain is to the left
 * @LIGMA_CHAIN_BOTTOM: the chain is on bottom
 * @LIGMA_CHAIN_RIGHT:  the chain is to the right
 *
 * Possible chain positions for #LigmaChainButton.
 **/
#define LIGMA_TYPE_CHAIN_POSITION (ligma_chain_position_get_type ())

GType ligma_chain_position_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CHAIN_TOP,
  LIGMA_CHAIN_LEFT,
  LIGMA_CHAIN_BOTTOM,
  LIGMA_CHAIN_RIGHT
} LigmaChainPosition;


/**
 * LigmaColorAreaType:
 * @LIGMA_COLOR_AREA_FLAT:         don't display transparency
 * @LIGMA_COLOR_AREA_SMALL_CHECKS: display transparency using small checks
 * @LIGMA_COLOR_AREA_LARGE_CHECKS: display transparency using large checks
 *
 * The types of transparency display for #LigmaColorArea.
 **/
#define LIGMA_TYPE_COLOR_AREA_TYPE (ligma_color_area_type_get_type ())

GType ligma_color_area_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_AREA_FLAT = 0,
  LIGMA_COLOR_AREA_SMALL_CHECKS,
  LIGMA_COLOR_AREA_LARGE_CHECKS
} LigmaColorAreaType;


/**
 * LigmaColorSelectorChannel:
 * @LIGMA_COLOR_SELECTOR_HUE:            the hue channel
 * @LIGMA_COLOR_SELECTOR_SATURATION:     the saturation channel
 * @LIGMA_COLOR_SELECTOR_VALUE:          the value channel
 * @LIGMA_COLOR_SELECTOR_RED:            the red channel
 * @LIGMA_COLOR_SELECTOR_GREEN:          the green channel
 * @LIGMA_COLOR_SELECTOR_BLUE:           the blue channel
 * @LIGMA_COLOR_SELECTOR_ALPHA:          the alpha channel
 * @LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS:  the lightness channel
 * @LIGMA_COLOR_SELECTOR_LCH_CHROMA:     the chroma channel
 * @LIGMA_COLOR_SELECTOR_LCH_HUE:        the hue channel
 *
 * An enum to specify the types of color channels edited in
 * #LigmaColorSelector widgets.
 **/
#define LIGMA_TYPE_COLOR_SELECTOR_CHANNEL (ligma_color_selector_channel_get_type ())

GType ligma_color_selector_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_SELECTOR_HUE,           /*< desc="_H", help="HSV Hue"        >*/
  LIGMA_COLOR_SELECTOR_SATURATION,    /*< desc="_S", help="HSV Saturation" >*/
  LIGMA_COLOR_SELECTOR_VALUE,         /*< desc="_V", help="HSV Value"      >*/
  LIGMA_COLOR_SELECTOR_RED,           /*< desc="_R", help="Red"            >*/
  LIGMA_COLOR_SELECTOR_GREEN,         /*< desc="_G", help="Green"          >*/
  LIGMA_COLOR_SELECTOR_BLUE,          /*< desc="_B", help="Blue"           >*/
  LIGMA_COLOR_SELECTOR_ALPHA,         /*< desc="_A", help="Alpha"          >*/
  LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS, /*< desc="_L", help="LCh Lightness"  >*/
  LIGMA_COLOR_SELECTOR_LCH_CHROMA,    /*< desc="_C", help="LCh Chroma"     >*/
  LIGMA_COLOR_SELECTOR_LCH_HUE        /*< desc="_h", help="LCh Hue"        >*/
} LigmaColorSelectorChannel;


/**
 * LigmaColorSelectorModel:
 * @LIGMA_COLOR_SELECTOR_MODEL_RGB: RGB color model
 * @LIGMA_COLOR_SELECTOR_MODEL_LCH: CIE LCh color model
 * @LIGMA_COLOR_SELECTOR_MODEL_HSV: HSV color model
 *
 * An enum to specify the types of color spaces edited in
 * #LigmaColorSelector widgets.
 *
 * Since: 2.10
 **/
#define LIGMA_TYPE_COLOR_SELECTOR_MODEL (ligma_color_selector_model_get_type ())

GType ligma_color_selector_model_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_SELECTOR_MODEL_RGB, /*< desc="RGB", help="RGB color model"     >*/
  LIGMA_COLOR_SELECTOR_MODEL_LCH, /*< desc="LCH", help="CIE LCh color model" >*/
  LIGMA_COLOR_SELECTOR_MODEL_HSV  /*< desc="HSV", help="HSV color model"     >*/
} LigmaColorSelectorModel;


/**
 * LigmaIntComboBoxLayout:
 * @LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY:   show icons only
 * @LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED: show icons and abbreviated labels,
 *                                         when available
 * @LIGMA_INT_COMBO_BOX_LAYOUT_FULL:        show icons and full labels
 *
 * Possible layouts for #LigmaIntComboBox.
 *
 * Since: 2.10
 **/
#define LIGMA_TYPE_INT_COMBO_BOX_LAYOUT (ligma_int_combo_box_layout_get_type ())

GType ligma_int_combo_box_layout_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY,
  LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED,
  LIGMA_INT_COMBO_BOX_LAYOUT_FULL
} LigmaIntComboBoxLayout;


/**
 * LigmaPageSelectorTarget:
 * @LIGMA_PAGE_SELECTOR_TARGET_LAYERS: import as layers of one image
 * @LIGMA_PAGE_SELECTOR_TARGET_IMAGES: import as separate images
 *
 * Import targets for #LigmaPageSelector.
 **/
#define LIGMA_TYPE_PAGE_SELECTOR_TARGET (ligma_page_selector_target_get_type ())

GType ligma_page_selector_target_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PAGE_SELECTOR_TARGET_LAYERS, /*< desc="Layers" >*/
  LIGMA_PAGE_SELECTOR_TARGET_IMAGES  /*< desc="Images" >*/
} LigmaPageSelectorTarget;


/**
 * LigmaSizeEntryUpdatePolicy:
 * @LIGMA_SIZE_ENTRY_UPDATE_NONE:       the size entry's meaning is up to the user
 * @LIGMA_SIZE_ENTRY_UPDATE_SIZE:       the size entry displays values
 * @LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION: the size entry displays resolutions
 *
 * Update policies for #LigmaSizeEntry.
 **/
#define LIGMA_TYPE_SIZE_ENTRY_UPDATE_POLICY (ligma_size_entry_update_policy_get_type ())

GType ligma_size_entry_update_policy_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_SIZE_ENTRY_UPDATE_NONE       = 0,
  LIGMA_SIZE_ENTRY_UPDATE_SIZE       = 1,
  LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} LigmaSizeEntryUpdatePolicy;


/**
 * LigmaZoomType:
 * @LIGMA_ZOOM_IN:       zoom in
 * @LIGMA_ZOOM_OUT:      zoom out
 * @LIGMA_ZOOM_IN_MORE:  zoom in a lot
 * @LIGMA_ZOOM_OUT_MORE: zoom out a lot
 * @LIGMA_ZOOM_IN_MAX:   zoom in as far as possible
 * @LIGMA_ZOOM_OUT_MAX:  zoom out as far as possible
 * @LIGMA_ZOOM_TO:       zoom to a specific zoom factor
 * @LIGMA_ZOOM_SMOOTH:   zoom smoothly from a smooth scroll event
 * @LIGMA_ZOOM_PINCH:    zoom smoothly from a touchpad pinch gesture
 *
 * the zoom types for #LigmaZoomModel.
 **/
#define LIGMA_TYPE_ZOOM_TYPE (ligma_zoom_type_get_type ())

GType ligma_zoom_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ZOOM_IN,        /*< desc="Zoom in"  >*/
  LIGMA_ZOOM_OUT,       /*< desc="Zoom out" >*/
  LIGMA_ZOOM_IN_MORE,   /*< skip >*/
  LIGMA_ZOOM_OUT_MORE,  /*< skip >*/
  LIGMA_ZOOM_IN_MAX,    /*< skip >*/
  LIGMA_ZOOM_OUT_MAX,   /*< skip >*/
  LIGMA_ZOOM_TO,        /*< skip >*/
  LIGMA_ZOOM_SMOOTH,    /*< skip >*/
  LIGMA_ZOOM_PINCH,     /*< skip >*/
} LigmaZoomType;


G_END_DECLS

#endif  /* __LIGMA_WIDGETS_ENUMS_H__ */
