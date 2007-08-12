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

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  in button size:  */

#define GIMP_STOCK_ANCHOR                   "gimp-anchor"
#define GIMP_STOCK_CENTER                   "gimp-center"
#define GIMP_STOCK_DUPLICATE                "gimp-duplicate"
#define GIMP_STOCK_EDIT                     "gimp-edit"
#define GIMP_STOCK_LINKED                   "gimp-linked"
#define GIMP_STOCK_PASTE_AS_NEW             "gimp-paste-as-new"
#define GIMP_STOCK_PASTE_INTO               "gimp-paste-into"
#define GIMP_STOCK_RESET                    "gimp-reset"
#define GIMP_STOCK_VISIBLE                  "gimp-visible"

#define GIMP_STOCK_GRADIENT_LINEAR               "gimp-gradient-linear"
#define GIMP_STOCK_GRADIENT_BILINEAR             "gimp-gradient-bilinear"
#define GIMP_STOCK_GRADIENT_RADIAL               "gimp-gradient-radial"
#define GIMP_STOCK_GRADIENT_SQUARE               "gimp-gradient-square"
#define GIMP_STOCK_GRADIENT_CONICAL_SYMMETRIC    "gimp-gradient-conical-symmetric"
#define GIMP_STOCK_GRADIENT_CONICAL_ASYMMETRIC   "gimp-gradient-conical-asymmetric"
#define GIMP_STOCK_GRADIENT_SHAPEBURST_ANGULAR   "gimp-gradient-shapeburst-angular"
#define GIMP_STOCK_GRADIENT_SHAPEBURST_SPHERICAL "gimp-gradient-shapeburst-spherical"
#define GIMP_STOCK_GRADIENT_SHAPEBURST_DIMPLED   "gimp-gradient-shapeburst-dimpled"
#define GIMP_STOCK_GRADIENT_SPIRAL_CLOCKWISE     "gimp-gradient-spiral-clockwise"
#define GIMP_STOCK_GRADIENT_SPIRAL_ANTICLOCKWISE "gimp-gradient-spiral-anticlockwise"


#define GIMP_STOCK_GRAVITY_EAST             "gimp-gravity-east"
#define GIMP_STOCK_GRAVITY_NORTH            "gimp-gravity-north"
#define GIMP_STOCK_GRAVITY_NORTH_EAST       "gimp-gravity-north-east"
#define GIMP_STOCK_GRAVITY_NORTH_WEST       "gimp-gravity-north-west"
#define GIMP_STOCK_GRAVITY_SOUTH            "gimp-gravity-south"
#define GIMP_STOCK_GRAVITY_SOUTH_EAST       "gimp-gravity-south-east"
#define GIMP_STOCK_GRAVITY_SOUTH_WEST       "gimp-gravity-south-west"
#define GIMP_STOCK_GRAVITY_WEST             "gimp-gravity-west"

#define GIMP_STOCK_HCENTER                  "gimp-hcenter"
#define GIMP_STOCK_VCENTER                  "gimp-vcenter"

#define GIMP_STOCK_HCHAIN                   "gimp-hchain"
#define GIMP_STOCK_HCHAIN_BROKEN            "gimp-hchain-broken"
#define GIMP_STOCK_VCHAIN                   "gimp-vchain"
#define GIMP_STOCK_VCHAIN_BROKEN            "gimp-vchain-broken"

#define GIMP_STOCK_SELECTION                "gimp-selection"
#define GIMP_STOCK_SELECTION_REPLACE        "gimp-selection-replace"
#define GIMP_STOCK_SELECTION_ADD            "gimp-selection-add"
#define GIMP_STOCK_SELECTION_SUBTRACT       "gimp-selection-subtract"
#define GIMP_STOCK_SELECTION_INTERSECT      "gimp-selection-intersect"
#define GIMP_STOCK_SELECTION_STROKE         "gimp-selection-stroke"
#define GIMP_STOCK_SELECTION_TO_CHANNEL     "gimp-selection-to-channel"
#define GIMP_STOCK_SELECTION_TO_PATH        "gimp-selection-to-path"

#define GIMP_STOCK_PATH_STROKE              "gimp-path-stroke"

#define GIMP_STOCK_CURVE_FREE               "gimp-curve-free"
#define GIMP_STOCK_CURVE_SMOOTH             "gimp-curve-smooth"

#define GIMP_STOCK_COLOR_PICKER_BLACK       "gimp-color-picker-black"
#define GIMP_STOCK_COLOR_PICKER_GRAY        "gimp-color-picker-gray"
#define GIMP_STOCK_COLOR_PICKER_WHITE       "gimp-color-picker-white"
#define GIMP_STOCK_COLOR_TRIANGLE           "gimp-color-triangle"
#define GIMP_STOCK_COLOR_PICK_FROM_SCREEN   "gimp-color-pick-from-screen"

#define GIMP_STOCK_CHAR_PICKER              "gimp-char-picker"
#define GIMP_STOCK_LETTER_SPACING           "gimp-letter-spacing"
#define GIMP_STOCK_LINE_SPACING             "gimp-line-spacing"
#define GIMP_STOCK_PRINT_RESOLUTION         "gimp-print-resolution"

#define GIMP_STOCK_TEXT_DIR_LTR             "gimp-text-dir-ltr"
#define GIMP_STOCK_TEXT_DIR_RTL             "gimp-text-dir-rtl"

#define GIMP_STOCK_TOOL_AIRBRUSH            "gimp-tool-airbrush"
#define GIMP_STOCK_TOOL_ALIGN               "gimp-tool-align"
#define GIMP_STOCK_TOOL_BLEND               "gimp-tool-blend"
#define GIMP_STOCK_TOOL_BLUR                "gimp-tool-blur"
#define GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST "gimp-tool-brightness-contrast"
#define GIMP_STOCK_TOOL_BUCKET_FILL         "gimp-tool-bucket-fill"
#define GIMP_STOCK_TOOL_BY_COLOR_SELECT     "gimp-tool-by-color-select"
#define GIMP_STOCK_TOOL_CLONE               "gimp-tool-clone"
#define GIMP_STOCK_TOOL_COLOR_BALANCE       "gimp-tool-color-balance"
#define GIMP_STOCK_TOOL_COLOR_PICKER        "gimp-tool-color-picker"
#define GIMP_STOCK_TOOL_COLORIZE            "gimp-tool-colorize"
#define GIMP_STOCK_TOOL_CROP                "gimp-tool-crop"
#define GIMP_STOCK_TOOL_CURVES              "gimp-tool-curves"
#define GIMP_STOCK_TOOL_DODGE               "gimp-tool-dodge"
#define GIMP_STOCK_TOOL_ELLIPSE_SELECT      "gimp-tool-ellipse-select"
#define GIMP_STOCK_TOOL_ERASER              "gimp-tool-eraser"
#define GIMP_STOCK_TOOL_FLIP                "gimp-tool-flip"
#define GIMP_STOCK_TOOL_FREE_SELECT         "gimp-tool-free-select"
#define GIMP_STOCK_TOOL_FOREGROUND_SELECT   "gimp-tool-foreground-select"
#define GIMP_STOCK_TOOL_FUZZY_SELECT        "gimp-tool-fuzzy-select"
#define GIMP_STOCK_TOOL_HEAL                "gimp-tool-heal"
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
#define GIMP_STOCK_TOOL_PERSPECTIVE_CLONE   "gimp-tool-perspective-clone"
#define GIMP_STOCK_TOOL_POSTERIZE           "gimp-tool-posterize"
#define GIMP_STOCK_TOOL_RECT_SELECT         "gimp-tool-rect-select"
#define GIMP_STOCK_TOOL_ROTATE              "gimp-tool-rotate"
#define GIMP_STOCK_TOOL_SCALE               "gimp-tool-scale"
#define GIMP_STOCK_TOOL_SHEAR               "gimp-tool-shear"
#define GIMP_STOCK_TOOL_SMUDGE              "gimp-tool-smudge"
#define GIMP_STOCK_TOOL_TEXT                "gimp-tool-text"
#define GIMP_STOCK_TOOL_THRESHOLD           "gimp-tool-threshold"
#define GIMP_STOCK_TOOL_ZOOM                "gimp-tool-zoom"

/*  in menu size:  */

#define GIMP_STOCK_AUTO_MODE                "gimp-auto-mode"

#define GIMP_STOCK_CONVERT_RGB              "gimp-convert-rgb"
#define GIMP_STOCK_CONVERT_GRAYSCALE        "gimp-convert-grayscale"
#define GIMP_STOCK_CONVERT_INDEXED          "gimp-convert-indexed"
#define GIMP_STOCK_INVERT                   "gimp-invert"
#define GIMP_STOCK_MERGE_DOWN               "gimp-merge-down"
#define GIMP_STOCK_LAYER_TO_IMAGESIZE       "gimp-layer-to-imagesize"
#define GIMP_STOCK_PLUGIN                   "gimp-plugin"
#define GIMP_STOCK_UNDO_HISTORY             "gimp-undo-history"
#define GIMP_STOCK_RESHOW_FILTER            "gimp-reshow-filter"
#define GIMP_STOCK_ROTATE_90                "gimp-rotate-90"
#define GIMP_STOCK_ROTATE_180               "gimp-rotate-180"
#define GIMP_STOCK_ROTATE_270               "gimp-rotate-270"
#define GIMP_STOCK_RESIZE                   "gimp-resize"
#define GIMP_STOCK_SCALE                    "gimp-scale"
#define GIMP_STOCK_FLIP_HORIZONTAL          "gimp-flip-horizontal"
#define GIMP_STOCK_FLIP_VERTICAL            "gimp-flip-vertical"

#define GIMP_STOCK_IMAGE                    "gimp-image"
#define GIMP_STOCK_LAYER                    "gimp-layer"
#define GIMP_STOCK_TEXT_LAYER               "gimp-text-layer"
#define GIMP_STOCK_FLOATING_SELECTION       "gimp-floating-selection"
#define GIMP_STOCK_CHANNEL                  "gimp-channel"
#define GIMP_STOCK_CHANNEL_RED              "gimp-channel-red"
#define GIMP_STOCK_CHANNEL_GREEN            "gimp-channel-green"
#define GIMP_STOCK_CHANNEL_BLUE             "gimp-channel-blue"
#define GIMP_STOCK_CHANNEL_GRAY             "gimp-channel-gray"
#define GIMP_STOCK_CHANNEL_INDEXED          "gimp-channel-indexed"
#define GIMP_STOCK_CHANNEL_ALPHA            "gimp-channel-alpha"
#define GIMP_STOCK_LAYER_MASK               "gimp-layer-mask"
#define GIMP_STOCK_PATH                     "gimp-path"
#define GIMP_STOCK_TEMPLATE                 "gimp-template"
#define GIMP_STOCK_TRANSPARENCY             "gimp-transparency"
#define GIMP_STOCK_COLORMAP                 "gimp-colormap"

#ifndef GIMP_DISABLE_DEPRECATED
#define GIMP_STOCK_INDEXED_PALETTE          "gimp-colormap"
#endif /* GIMP_DISABLE_DEPRECATED */

#define GIMP_STOCK_IMAGES                   "gimp-images"
#define GIMP_STOCK_LAYERS                   "gimp-layers"
#define GIMP_STOCK_CHANNELS                 "gimp-channels"
#define GIMP_STOCK_PATHS                    "gimp-paths"

#define GIMP_STOCK_SELECTION_ALL            "gimp-selection-all"
#define GIMP_STOCK_SELECTION_NONE           "gimp-selection-none"
#define GIMP_STOCK_SELECTION_GROW           "gimp-selection-grow"
#define GIMP_STOCK_SELECTION_SHRINK         "gimp-selection-shrink"
#define GIMP_STOCK_SELECTION_BORDER         "gimp-selection-border"

#define GIMP_STOCK_NAVIGATION               "gimp-navigation"
#define GIMP_STOCK_QUICK_MASK_OFF           "gimp-quick-mask-off"
#define GIMP_STOCK_QUICK_MASK_ON            "gimp-quick-mask-on"

#ifndef GIMP_DISABLE_DEPRECATED
#define GIMP_STOCK_QMASK_OFF                "gimp-quick-mask-off"
#define GIMP_STOCK_QMASK_ON                 "gimp-quick-mask-on"
#endif /* GIMP_DISABLE_DEPRECATED */

#define GIMP_STOCK_HISTOGRAM                "gimp-histogram"
#define GIMP_STOCK_HISTOGRAM_LINEAR         "gimp-histogram-linear"
#define GIMP_STOCK_HISTOGRAM_LOGARITHMIC    "gimp-histogram-logarithmic"

#define GIMP_STOCK_CLOSE                    "gimp-close"
#define GIMP_STOCK_MENU_LEFT                "gimp-menu-left"
#define GIMP_STOCK_MENU_RIGHT               "gimp-menu-right"
#define GIMP_STOCK_MOVE_TO_SCREEN           "gimp_move-to-screen"
#define GIMP_STOCK_DEFAULT_COLORS           "gimp-default-colors"
#define GIMP_STOCK_SWAP_COLORS              "gimp-swap-colors"
#define GIMP_STOCK_ZOOM_FOLLOW_WINDOW       "gimp-zoom-follow-window"

#define GIMP_STOCK_TOOLS                    "gimp-tools"
#define GIMP_STOCK_TOOL_OPTIONS             "gimp-tool-options"
#define GIMP_STOCK_DEVICE_STATUS            "gimp-device-status"
#define GIMP_STOCK_CURSOR                   "gimp-cursor"
#define GIMP_STOCK_SAMPLE_POINT             "gimp-sample-point"

#define GIMP_STOCK_CONTROLLER               "gimp-controller"
#define GIMP_STOCK_CONTROLLER_KEYBOARD      "gimp-controller-keyboard"
#define GIMP_STOCK_CONTROLLER_LINUX_INPUT   "gimp-controller-linux-input"
#define GIMP_STOCK_CONTROLLER_MIDI          "gimp-controller-midi"
#define GIMP_STOCK_CONTROLLER_WHEEL         "gimp-controller-wheel"

#define GIMP_STOCK_DISPLAY_FILTER           "gimp-display-filter"
#define GIMP_STOCK_DISPLAY_FILTER_COLORBLIND "gimp-display-filter-colorblind"
#define GIMP_STOCK_DISPLAY_FILTER_CONTRAST  "gimp-display-filter-contrast"
#define GIMP_STOCK_DISPLAY_FILTER_GAMMA     "gimp-display-filter-gamma"
#define GIMP_STOCK_DISPLAY_FILTER_LCMS      "gimp-display-filter-lcms"
#define GIMP_STOCK_DISPLAY_FILTER_PROOF     "gimp-display-filter-proof"

#define GIMP_STOCK_LIST                     "gimp-list"
#define GIMP_STOCK_GRID                     "gimp-grid"

#define GIMP_STOCK_PORTRAIT                 "gimp-portrait"
#define GIMP_STOCK_LANDSCAPE                "gimp-landscape"

#define GIMP_STOCK_WEB                      "gimp-web"
#define GIMP_STOCK_VIDEO                    "gimp-video"

#define GIMP_STOCK_SHAPE_CIRCLE             "gimp-shape-circle"
#define GIMP_STOCK_SHAPE_DIAMOND            "gimp-shape-diamond"
#define GIMP_STOCK_SHAPE_SQUARE             "gimp-shape-square"

#define GIMP_STOCK_CAP_BUTT                 "gimp-cap-butt"
#define GIMP_STOCK_CAP_ROUND                "gimp-cap-round"
#define GIMP_STOCK_CAP_SQUARE               "gimp-cap-square"

#define GIMP_STOCK_JOIN_MITER               "gimp-join-miter"
#define GIMP_STOCK_JOIN_ROUND               "gimp-join-round"
#define GIMP_STOCK_JOIN_BEVEL               "gimp-join-bevel"

/*  in dialog size:  */

#define GIMP_STOCK_ERROR                    "gimp-error"
#define GIMP_STOCK_INFO                     "gimp-info"
#define GIMP_STOCK_QUESTION                 "gimp-question"
#define GIMP_STOCK_WARNING                  "gimp-warning"
#define GIMP_STOCK_WILBER                   "gimp-wilber"
#define GIMP_STOCK_WILBER_EEK               "gimp-wilber-eek"
#define GIMP_STOCK_FRAME                    "gimp-frame"
#define GIMP_STOCK_TEXTURE                  "gimp-texture"


/*  missing icons:  */

#define GIMP_STOCK_BRUSH                    GIMP_STOCK_TOOL_PAINTBRUSH
#define GIMP_STOCK_BUFFER                   GTK_STOCK_PASTE
#define GIMP_STOCK_DETACH                   GTK_STOCK_CONVERT
#define GIMP_STOCK_FONT                     GTK_STOCK_SELECT_FONT
#define GIMP_STOCK_GRADIENT                 GIMP_STOCK_TOOL_BLEND
#define GIMP_STOCK_PALETTE                  GTK_STOCK_SELECT_COLOR
#define GIMP_STOCK_PATTERN                  GIMP_STOCK_TOOL_BUCKET_FILL


void   gimp_stock_init (void);


G_END_DECLS

#endif /* __GIMP_STOCK_H__ */
