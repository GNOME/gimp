/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_WIDGETS_ENUMS_H__
#define __GIMP_WIDGETS_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_ASPECT_TYPE (gimp_aspect_type_get_type ())

GType gimp_aspect_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ASPECT_SQUARE,    /*< desc="Square"    >*/
  GIMP_ASPECT_PORTRAIT,  /*< desc="Portrait"  >*/
  GIMP_ASPECT_LANDSCAPE  /*< desc="Landscape" >*/
} GimpAspectType;


#define GIMP_TYPE_CHAIN_POSITION (gimp_chain_position_get_type ())

GType gimp_chain_position_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CHAIN_TOP,
  GIMP_CHAIN_LEFT,
  GIMP_CHAIN_BOTTOM,
  GIMP_CHAIN_RIGHT
} GimpChainPosition;


#define GIMP_TYPE_COLOR_AREA_TYPE (gimp_color_area_type_get_type ())

GType gimp_color_area_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_AREA_FLAT = 0,
  GIMP_COLOR_AREA_SMALL_CHECKS,
  GIMP_COLOR_AREA_LARGE_CHECKS
} GimpColorAreaType;


#define GIMP_TYPE_COLOR_SELECTOR_CHANNEL (gimp_color_selector_channel_get_type ())

GType gimp_color_selector_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_SELECTOR_HUE,        /*< desc="_H", help="Hue"        >*/
  GIMP_COLOR_SELECTOR_SATURATION, /*< desc="_S", help="Saturation" >*/
  GIMP_COLOR_SELECTOR_VALUE,      /*< desc="_V", help="Value"      >*/
  GIMP_COLOR_SELECTOR_RED,        /*< desc="_R", help="Red"        >*/
  GIMP_COLOR_SELECTOR_GREEN,      /*< desc="_G", help="Green"      >*/
  GIMP_COLOR_SELECTOR_BLUE,       /*< desc="_B", help="Blue"       >*/
  GIMP_COLOR_SELECTOR_ALPHA       /*< desc="_A", help="Alpha"      >*/
} GimpColorSelectorChannel;


#define GIMP_TYPE_PAGE_SELECTOR_TARGET (gimp_page_selector_target_get_type ())

GType gimp_page_selector_target_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAGE_SELECTOR_TARGET_LAYERS, /*< desc="Layers" >*/
  GIMP_PAGE_SELECTOR_TARGET_IMAGES  /*< desc="Images" >*/
} GimpPageSelectorTarget;


#define GIMP_TYPE_SIZE_ENTRY_UPDATE_POLICY (gimp_size_entry_update_policy_get_type ())

GType gimp_size_entry_update_policy_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SIZE_ENTRY_UPDATE_NONE       = 0,
  GIMP_SIZE_ENTRY_UPDATE_SIZE       = 1,
  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} GimpSizeEntryUpdatePolicy;


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
