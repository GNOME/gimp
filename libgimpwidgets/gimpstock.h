/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_STOCK_H__
#define __GIMP_STOCK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


#define GIMP_STOCK_ANCHOR       "gimp-anchor"
#define GIMP_STOCK_DELETE       "gimp-delete"
#define GIMP_STOCK_DUPLICATE    "gimp-duplicate"
#define GIMP_STOCK_EDIT         "gimp-edit"
#define GIMP_STOCK_LINKED       "gimp-linked"
#define GIMP_STOCK_LOWER        "gimp-lower"
#define GIMP_STOCK_NEW          "gimp-new"
#define GIMP_STOCK_PASTE        "gimp-paste"
#define GIMP_STOCK_PASTE_AS_NEW "gimp-paste-as-new"
#define GIMP_STOCK_PASTE_INTO   "gimp-paste-into"
#define GIMP_STOCK_RAISE        "gimp-raise"
#define GIMP_STOCK_REFRESH      "gimp-refresh"
#define GIMP_STOCK_RESET        "gimp-reset"
#define GIMP_STOCK_STROKE       "gimp-stroke"
#define GIMP_STOCK_TO_PATH      "gimp-to-path"
#define GIMP_STOCK_TO_SELECTION "gimp-to-selection"
#define GIMP_STOCK_VISIBLE      "gimp-visible"

#define GIMP_STOCK_NAVIGATION   "gimp-navigation"
#define GIMP_STOCK_QMASK_OFF    "gimp-qmask-off"
#define GIMP_STOCK_QMASK_ON     "gimp-qmask-on"

#define GIMP_STOCK_SELECTION_REPLACE   "gimp-selection-replace"
#define GIMP_STOCK_SELECTION_ADD       "gimp-selection-add"
#define GIMP_STOCK_SELECTION_SUBTRACT  "gimp-selection-subtract"
#define GIMP_STOCK_SELECTION_INTERSECT "gimp-selection-intersect"

#define GIMP_STOCK_TOOL_AIRBRUSH            "gimp-tool-airbrush"
#define GIMP_STOCK_TOOL_BEZIER_SELECT       "gimp-tool-bezier-select"
#define GIMP_STOCK_TOOL_BLEND               "gimp-tool-blend"
#define GIMP_STOCK_TOOL_BLUR                "gimp-tool-blur"
#define GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST "gimp-tool-brightness-contrast"
#define GIMP_STOCK_TOOL_BUCKET_FILL         "gimp-tool-bucket-fill"
#define GIMP_STOCK_TOOL_BY_COLOR_SELECT     "gimp-tool-by-color-select"
#define GIMP_STOCK_TOOL_CLONE               "gimp-tool-clone"
#define GIMP_STOCK_TOOL_COLOR_BALANCE       "gimp-tool-color-balance"
#define GIMP_STOCK_TOOL_COLOR_PICKER        "gimp-tool-color-picker"
#define GIMP_STOCK_TOOL_CROP                "gimp-tool-crop"
#define GIMP_STOCK_TOOL_CURVES              "gimp-tool-curves"
#define GIMP_STOCK_TOOL_DODGE               "gimp-tool-dodge"
#define GIMP_STOCK_TOOL_ELLIPSE_SELECT      "gimp-tool-ellipse-select"
#define GIMP_STOCK_TOOL_ERASER              "gimp-tool-eraser"
#define GIMP_STOCK_TOOL_FLIP                "gimp-tool-flip"
#define GIMP_STOCK_TOOL_FREE_SELECT         "gimp-tool-free-select"
#define GIMP_STOCK_TOOL_FUZZY_SELECT        "gimp-tool-fuzzy-select"
#define GIMP_STOCK_TOOL_HISTOGRAM           "gimp-tool-histogram"
#define GIMP_STOCK_TOOL_HUE_SATURATION      "gimp-tool-hue-saturation"
#define GIMP_STOCK_TOOL_INK                 "gimp-tool-ink"
#define GIMP_STOCK_TOOL_ISCISSORS           "gimp-tool-iscissors"
#define GIMP_STOCK_TOOL_LEVELS              "gimp-tool-levels"
#define GIMP_STOCK_TOOL_MEASURE             "gimp-tool-measure"
#define GIMP_STOCK_TOOL_MOVE                "gimp-tool-move"
#define GIMP_STOCK_TOOL_PAINTBRUSH          "gimp-tool-paintbrush"
#define GIMP_STOCK_TOOL_PATH                "gimp-tool-path"
#define GIMP_STOCK_TOOL_PENCIL              "gimp-tool-pencil"
#define GIMP_STOCK_TOOL_PERSPECTIVE         "gimp-tool-perspective"
#define GIMP_STOCK_TOOL_POSTERIZE           "gimp-tool-posterize"
#define GIMP_STOCK_TOOL_RECT_SELECT         "gimp-tool-rect-select"
#define GIMP_STOCK_TOOL_ROTATE              "gimp-tool-rotate"
#define GIMP_STOCK_TOOL_SCALE               "gimp-tool-scale"
#define GIMP_STOCK_TOOL_SHEAR               "gimp-tool-shear"
#define GIMP_STOCK_TOOL_SMUDGE              "gimp-tool-smudge"
#define GIMP_STOCK_TOOL_TEXT                "gimp-tool-text"
#define GIMP_STOCK_TOOL_THRESHOLD           "gimp-tool-threshold"
#define GIMP_STOCK_TOOL_ZOOM                "gimp-tool-zoom"


void   gimp_stock_init (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_STOCK_H__ */
