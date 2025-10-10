/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpicons.h
 * Copyright (C) 2001-2015 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_ICONS_H__
#define __GIMP_ICONS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  random actions that don't fit in any category  */

#define GIMP_ICON_ATTACH                    "gimp-attach"
#define GIMP_ICON_DETACH                    "gimp-detach"
#define GIMP_ICON_INVERT                    "gimp-invert"
#define GIMP_ICON_RECORD                    "media-record"
#define GIMP_ICON_RESET                     "gimp-reset"
#define GIMP_ICON_SHRED                     "gimp-shred"


/*  random states/things that don't fit in any category  */

#define GIMP_ICON_BUSINESS_CARD             "gimp-business-card"
#define GIMP_ICON_CHAR_PICKER               "gimp-char-picker"
#define GIMP_ICON_CURSOR                    "gimp-cursor"
#define GIMP_ICON_DISPLAY                   "gimp-display"
#define GIMP_ICON_GEGL                      "gimp-gegl"
#define GIMP_ICON_LINKED                    "gimp-linked"
#define GIMP_ICON_MARKER                    "gimp-marker"
#define GIMP_ICON_SMARTPHONE                "gimp-smartphone"
#define GIMP_ICON_TRANSPARENCY              "gimp-transparency"
#define GIMP_ICON_VIDEO                     "gimp-video"
#define GIMP_ICON_VISIBLE                   "gimp-visible"
#define GIMP_ICON_WEB                       "gimp-web"


/*  random objects/entities that don't fit in any category  */

#define GIMP_ICON_BRUSH                     GIMP_ICON_TOOL_PAINTBRUSH
#define GIMP_ICON_BUFFER                    GIMP_ICON_EDIT_PASTE
#define GIMP_ICON_COLORMAP                  "gimp-colormap"
#define GIMP_ICON_DYNAMICS                  "gimp-dynamics"
#define GIMP_ICON_FILE_MANAGER              "gimp-file-manager"
#define GIMP_ICON_FONT                      "gtk-select-font"
#define GIMP_ICON_GRADIENT                  GIMP_ICON_TOOL_GRADIENT
#define GIMP_ICON_GRID                      "gimp-grid"
#define GIMP_ICON_INPUT_DEVICE              "gimp-input-device"
#define GIMP_ICON_MYPAINT_BRUSH             GIMP_ICON_TOOL_MYPAINT_BRUSH
#define GIMP_ICON_PALETTE                   "gtk-select-color"
#define GIMP_ICON_PATTERN                   "gimp-pattern"
#define GIMP_ICON_PLUGIN                    "gimp-plugin"
#define GIMP_ICON_SAMPLE_POINT              "gimp-sample-point"
#define GIMP_ICON_SYMMETRY                  "gimp-symmetry"
#define GIMP_ICON_TEMPLATE                  "gimp-template"
#define GIMP_ICON_TOOL_PRESET               "gimp-tool-preset"


/*  not really icons  */

#define GIMP_ICON_FRAME                     "gimp-frame"
#define GIMP_ICON_TEXTURE                   "gimp-texture"


/*  icons that follow, or at least try to follow the FDO naming and
 *  category conventions; and groups of icons with a common prefix;
 *  all sorted alphabetically
 *
 *  see also:
 *  https://specifications.freedesktop.org/icon-naming-spec/latest/ar01s04.html
 *
 *  When icons are available as standard Freedesktop icons, we use these
 *  in priority. As a fallback, we use standard GTK+ icons. As last
 *  fallback, we create our own icons under the "gimp-" namespace.
 */

#define GIMP_ICON_APPLICATION_EXIT          "application-exit"

#define GIMP_ICON_ASPECT_PORTRAIT           "gimp-portrait"
#define GIMP_ICON_ASPECT_LANDSCAPE          "gimp-landscape"

#define GIMP_ICON_CAP_BUTT                  "gimp-cap-butt"
#define GIMP_ICON_CAP_ROUND                 "gimp-cap-round"
#define GIMP_ICON_CAP_SQUARE                "gimp-cap-square"

#define GIMP_ICON_CENTER                    "gimp-center"
#define GIMP_ICON_CENTER_HORIZONTAL         "gimp-hcenter"
#define GIMP_ICON_CENTER_VERTICAL           "gimp-vcenter"

#define GIMP_ICON_CHAIN_HORIZONTAL          "gimp-hchain"
#define GIMP_ICON_CHAIN_HORIZONTAL_BROKEN   "gimp-hchain-broken"
#define GIMP_ICON_CHAIN_VERTICAL            "gimp-vchain"
#define GIMP_ICON_CHAIN_VERTICAL_BROKEN     "gimp-vchain-broken"

#define GIMP_ICON_CHANNEL                   "gimp-channel"
#define GIMP_ICON_CHANNEL_ALPHA             "gimp-channel-alpha"
#define GIMP_ICON_CHANNEL_BLUE              "gimp-channel-blue"
#define GIMP_ICON_CHANNEL_GRAY              "gimp-channel-gray"
#define GIMP_ICON_CHANNEL_GREEN             "gimp-channel-green"
#define GIMP_ICON_CHANNEL_INDEXED           "gimp-channel-indexed"
#define GIMP_ICON_CHANNEL_RED               "gimp-channel-red"

#define GIMP_ICON_CLOSE                     "gimp-close"
#define GIMP_ICON_CLOSE_ALL                 "gimp-close-all"

#define GIMP_ICON_COLOR_PICKER_BLACK        "gimp-color-picker-black"
#define GIMP_ICON_COLOR_PICKER_GRAY         "gimp-color-picker-gray"
#define GIMP_ICON_COLOR_PICKER_WHITE        "gimp-color-picker-white"
#define GIMP_ICON_COLOR_PICK_FROM_SCREEN    "gimp-color-pick-from-screen"

#define GIMP_ICON_COLOR_SELECTOR_CMYK       "gimp-color-cmyk"
#define GIMP_ICON_COLOR_SELECTOR_TRIANGLE   "gimp-color-triangle"
#define GIMP_ICON_COLOR_SELECTOR_WATER      "gimp-color-water"

#define GIMP_ICON_COLOR_SPACE_LINEAR        "gimp-color-space-linear"
#define GIMP_ICON_COLOR_SPACE_NON_LINEAR    "gimp-color-space-non-linear"
#define GIMP_ICON_COLOR_SPACE_PERCEPTUAL    "gimp-color-space-perceptual"

#define GIMP_ICON_COLORS_DEFAULT            "gimp-default-colors"
#define GIMP_ICON_COLORS_SWAP               "gimp-swap-colors"

#define GIMP_ICON_CONTROLLER                "gimp-controller"
#define GIMP_ICON_CONTROLLER_KEYBOARD       "gimp-controller-keyboard"
#define GIMP_ICON_CONTROLLER_LINUX_INPUT    "gimp-controller-linux-input"
#define GIMP_ICON_CONTROLLER_MIDI           "gimp-controller-midi"
#define GIMP_ICON_CONTROLLER_MOUSE          GIMP_ICON_CURSOR
#define GIMP_ICON_CONTROLLER_WHEEL          "gimp-controller-wheel"

#define GIMP_ICON_CONVERT_RGB               "gimp-convert-rgb"
#define GIMP_ICON_CONVERT_GRAYSCALE         "gimp-convert-grayscale"
#define GIMP_ICON_CONVERT_INDEXED           "gimp-convert-indexed"
#define GIMP_ICON_CONVERT_PRECISION         GIMP_ICON_CONVERT_RGB

#define GIMP_ICON_CURVE_FREE                "gimp-curve-free"
#define GIMP_ICON_CURVE_SMOOTH              "gimp-curve-smooth"

#define GIMP_ICON_DIALOG_CHANNELS           "gimp-channels"
#define GIMP_ICON_DIALOG_DASHBOARD          "gimp-dashboard"
#define GIMP_ICON_DIALOG_DEVICE_STATUS      "gimp-device-status"
#define GIMP_ICON_DIALOG_ERROR              "dialog-error"
#define GIMP_ICON_DIALOG_IMAGES             "gimp-images"
#define GIMP_ICON_DIALOG_INFORMATION        "dialog-information"
#define GIMP_ICON_DIALOG_LAYERS             "gimp-layers"
#define GIMP_ICON_DIALOG_NAVIGATION         "gimp-navigation"
#define GIMP_ICON_DIALOG_PATHS              "gimp-paths"
#define GIMP_ICON_DIALOG_QUESTION           "dialog-question"
#define GIMP_ICON_DIALOG_RESHOW_FILTER      "gimp-reshow-filter"
#define GIMP_ICON_DIALOG_TOOLS              "gimp-tools"
#define GIMP_ICON_DIALOG_TOOL_OPTIONS       "gimp-tool-options"
#define GIMP_ICON_DIALOG_UNDO_HISTORY       "gimp-undo-history"
#define GIMP_ICON_DIALOG_WARNING            "dialog-warning"

#define GIMP_ICON_DISPLAY_FILTER              "gimp-display-filter"
#define GIMP_ICON_DISPLAY_FILTER_CLIP_WARNING "gimp-display-filter-clip-warning"
#define GIMP_ICON_DISPLAY_FILTER_COLORBLIND   "gimp-display-filter-colorblind"
#define GIMP_ICON_DISPLAY_FILTER_CONTRAST     "gimp-display-filter-contrast"
#define GIMP_ICON_DISPLAY_FILTER_GAMMA        "gimp-display-filter-gamma"
#define GIMP_ICON_DISPLAY_FILTER_LCMS         "gimp-display-filter-lcms"
#define GIMP_ICON_DISPLAY_FILTER_PROOF        "gimp-display-filter-proof"

#define GIMP_ICON_LOCK                      "gimp-lock"
#define GIMP_ICON_LOCK_ALPHA                "gimp-lock-alpha"
#define GIMP_ICON_LOCK_CONTENT              "gimp-lock-content"
#define GIMP_ICON_LOCK_PATH                 "gimp-lock-path"
#define GIMP_ICON_LOCK_POSITION             "gimp-lock-position"
#define GIMP_ICON_LOCK_VISIBILITY           "gimp-lock-visibility"
#define GIMP_ICON_LOCK_MULTI                "gimp-lock-multi"

#define GIMP_ICON_DOCUMENT_NEW              "document-new"
#define GIMP_ICON_DOCUMENT_OPEN             "document-open"
#define GIMP_ICON_DOCUMENT_OPEN_RECENT      "document-open-recent"
#define GIMP_ICON_DOCUMENT_PAGE_SETUP       "document-page-setup"
#define GIMP_ICON_DOCUMENT_PRINT            "document-print"
#define GIMP_ICON_DOCUMENT_PRINT_RESOLUTION "document-print"
#define GIMP_ICON_DOCUMENT_PROPERTIES       "document-properties"
#define GIMP_ICON_DOCUMENT_REVERT           "document-revert"
#define GIMP_ICON_DOCUMENT_SAVE             "document-save"
#define GIMP_ICON_DOCUMENT_SAVE_AS          "document-save-as"

#define GIMP_ICON_EDIT                      "gtk-edit"
#define GIMP_ICON_EDIT_CLEAR                "edit-clear"
#define GIMP_ICON_EDIT_COPY                 "edit-copy"
#define GIMP_ICON_EDIT_CUT                  "edit-cut"
#define GIMP_ICON_EDIT_DELETE               "edit-delete"
#define GIMP_ICON_EDIT_FIND                 "edit-find"
#define GIMP_ICON_EDIT_PASTE                "edit-paste"
#define GIMP_ICON_EDIT_PASTE_AS_NEW         "gimp-paste-as-new"
#define GIMP_ICON_EDIT_PASTE_INTO           "gimp-paste-into"
#define GIMP_ICON_EDIT_REDO                 "edit-redo"
#define GIMP_ICON_EDIT_UNDO                 "edit-undo"

#define GIMP_ICON_EFFECT                    "gimp-effects"

#define GIMP_ICON_EVEN_HORIZONTAL_GAP       "gimp-even-horizontal-gap"
#define GIMP_ICON_EVEN_VERTICAL_GAP         "gimp-even-vertical-gap"

#define GIMP_ICON_FILL_HORIZONTAL           "gimp-hfill"
#define GIMP_ICON_FILL_VERTICAL             "gimp-vfill"

#define GIMP_ICON_FOLDER_NEW                "folder-new"

#define GIMP_ICON_FORMAT_INDENT_MORE         "format-indent-more"
#define GIMP_ICON_FORMAT_INDENT_LESS         "format-indent-less"
#define GIMP_ICON_FORMAT_JUSTIFY_CENTER      "format-justify-center"
#define GIMP_ICON_FORMAT_JUSTIFY_FILL        "format-justify-fill"
#define GIMP_ICON_FORMAT_JUSTIFY_LEFT        "format-justify-left"
#define GIMP_ICON_FORMAT_JUSTIFY_RIGHT       "format-justify-right"
#define GIMP_ICON_FORMAT_TEXT_BOLD           "format-text-bold"
#define GIMP_ICON_FORMAT_TEXT_ITALIC         "format-text-italic"
#define GIMP_ICON_FORMAT_TEXT_STRIKETHROUGH  "format-text-strikethrough"
#define GIMP_ICON_FORMAT_TEXT_UNDERLINE      "format-text-underline"
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_LTR  "format-text-direction-ltr"
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_RTL  "format-text-direction-rtl"
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL           "gimp-text-dir-ttb-rtl" /* use FDO */
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT   "gimp-text-dir-ttb-rtl-upright" /* use FDO */
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR           "gimp-text-dir-ttb-ltr" /* use FDO */
#define GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT   "gimp-text-dir-ttb-ltr-upright" /* use FDO */
#define GIMP_ICON_FORMAT_TEXT_SPACING_LETTER "gimp-letter-spacing"
#define GIMP_ICON_FORMAT_TEXT_SPACING_LINE   "gimp-line-spacing"

#define GIMP_ICON_GRADIENT_LINEAR               "gimp-gradient-linear"
#define GIMP_ICON_GRADIENT_BILINEAR             "gimp-gradient-bilinear"
#define GIMP_ICON_GRADIENT_RADIAL               "gimp-gradient-radial"
#define GIMP_ICON_GRADIENT_SQUARE               "gimp-gradient-square"
#define GIMP_ICON_GRADIENT_CONICAL_SYMMETRIC    "gimp-gradient-conical-symmetric"
#define GIMP_ICON_GRADIENT_CONICAL_ASYMMETRIC   "gimp-gradient-conical-asymmetric"
#define GIMP_ICON_GRADIENT_SHAPEBURST_ANGULAR   "gimp-gradient-shapeburst-angular"
#define GIMP_ICON_GRADIENT_SHAPEBURST_SPHERICAL "gimp-gradient-shapeburst-spherical"
#define GIMP_ICON_GRADIENT_SHAPEBURST_DIMPLED   "gimp-gradient-shapeburst-dimpled"
#define GIMP_ICON_GRADIENT_SPIRAL_CLOCKWISE     "gimp-gradient-spiral-clockwise"
#define GIMP_ICON_GRADIENT_SPIRAL_ANTICLOCKWISE "gimp-gradient-spiral-anticlockwise"

#define GIMP_ICON_GRAVITY_EAST              "gimp-gravity-east"
#define GIMP_ICON_GRAVITY_NORTH             "gimp-gravity-north"
#define GIMP_ICON_GRAVITY_NORTH_EAST        "gimp-gravity-north-east"
#define GIMP_ICON_GRAVITY_NORTH_WEST        "gimp-gravity-north-west"
#define GIMP_ICON_GRAVITY_SOUTH             "gimp-gravity-south"
#define GIMP_ICON_GRAVITY_SOUTH_EAST        "gimp-gravity-south-east"
#define GIMP_ICON_GRAVITY_SOUTH_WEST        "gimp-gravity-south-west"
#define GIMP_ICON_GRAVITY_WEST              "gimp-gravity-west"

#define GIMP_ICON_GO_BOTTOM                 "go-bottom"
#define GIMP_ICON_GO_DOWN                   "go-down"
#define GIMP_ICON_GO_FIRST                  "go-first"
#define GIMP_ICON_GO_HOME                   "go-home"
#define GIMP_ICON_GO_LAST                   "go-last"
#define GIMP_ICON_GO_TOP                    "go-top"
#define GIMP_ICON_GO_UP                     "go-up"
#define GIMP_ICON_GO_PREVIOUS               "go-previous"
#define GIMP_ICON_GO_NEXT                   "go-next"

#define GIMP_ICON_HELP                      "system-help"
#define GIMP_ICON_HELP_ABOUT                "help-about"
#define GIMP_ICON_HELP_USER_MANUAL          "gimp-user-manual"

#define GIMP_ICON_HISTOGRAM                 "gimp-histogram"
#define GIMP_ICON_HISTOGRAM_LINEAR          "gimp-histogram-linear"
#define GIMP_ICON_HISTOGRAM_LOGARITHMIC     "gimp-histogram-logarithmic"

#define GIMP_ICON_IMAGE                     "gimp-image"
#define GIMP_ICON_IMAGE_OPEN                "gimp-image-open"
#define GIMP_ICON_IMAGE_RELOAD              "gimp-image-reload"

#define GIMP_ICON_JOIN_MITER                "gimp-join-miter"
#define GIMP_ICON_JOIN_ROUND                "gimp-join-round"
#define GIMP_ICON_JOIN_BEVEL                "gimp-join-bevel"

#define GIMP_ICON_LAYER                     "gimp-layer"
#define GIMP_ICON_LAYER_ANCHOR              "gimp-anchor"
#define GIMP_ICON_LAYER_FLOATING_SELECTION  "gimp-floating-selection"
/* TODO: create "gimp-link-layer" */
#define GIMP_ICON_LAYER_LINK_LAYER          "emblem-symbolic-link"
#define GIMP_ICON_LAYER_MASK                "gimp-layer-mask"
#define GIMP_ICON_LAYER_MERGE_DOWN          "gimp-merge-down"
#define GIMP_ICON_LAYER_TEXT_LAYER          "gimp-text-layer"
#define GIMP_ICON_LAYER_TO_IMAGESIZE        "gimp-layer-to-imagesize"
/* TODO: create "gimp-vector-layer" */
#define GIMP_ICON_LAYER_VECTOR_LAYER        "gimp-tool-path"

#define GIMP_ICON_LIST                      "gimp-list"
#define GIMP_ICON_LIST_ADD                  "list-add"
#define GIMP_ICON_LIST_REMOVE               "list-remove"

#define GIMP_ICON_MENU_LEFT                 "gimp-menu-left"
#define GIMP_ICON_MENU_RIGHT                "gimp-menu-right"

#define GIMP_ICON_OBJECT_DUPLICATE          "gimp-duplicate"
#define GIMP_ICON_OBJECT_FLIP_HORIZONTAL    "object-flip-horizontal"
#define GIMP_ICON_OBJECT_FLIP_VERTICAL      "object-flip-vertical"
#define GIMP_ICON_OBJECT_RESIZE             "gimp-resize"
#define GIMP_ICON_OBJECT_ROTATE_180         "gimp-rotate-180"
#define GIMP_ICON_OBJECT_ROTATE_270         "object-rotate-left"
#define GIMP_ICON_OBJECT_ROTATE_90          "object-rotate-right"
#define GIMP_ICON_OBJECT_SCALE              "gimp-scale"

#define GIMP_ICON_PATH                      "gimp-path"
#define GIMP_ICON_PATH_STROKE               "gimp-path-stroke"

#define GIMP_ICON_PIVOT_CENTER              "gimp-pivot-center"
#define GIMP_ICON_PIVOT_EAST                "gimp-pivot-east"
#define GIMP_ICON_PIVOT_NORTH               "gimp-pivot-north"
#define GIMP_ICON_PIVOT_NORTH_EAST          "gimp-pivot-north-east"
#define GIMP_ICON_PIVOT_NORTH_WEST          "gimp-pivot-north-west"
#define GIMP_ICON_PIVOT_SOUTH               "gimp-pivot-south"
#define GIMP_ICON_PIVOT_SOUTH_EAST          "gimp-pivot-south-east"
#define GIMP_ICON_PIVOT_SOUTH_WEST          "gimp-pivot-south-west"
#define GIMP_ICON_PIVOT_WEST                "gimp-pivot-west"

#define GIMP_ICON_PREFERENCES_SYSTEM        "preferences-system"

#define GIMP_ICON_PROCESS_STOP              "process-stop"

#define GIMP_ICON_QUICK_MASK_OFF            "gimp-quick-mask-off"
#define GIMP_ICON_QUICK_MASK_ON             "gimp-quick-mask-on"

#define GIMP_ICON_SELECTION                 "gimp-selection"
#define GIMP_ICON_SELECTION_ADD             "gimp-selection-add"
#define GIMP_ICON_SELECTION_ALL             "gimp-selection-all"
#define GIMP_ICON_SELECTION_BORDER          "gimp-selection-border"
#define GIMP_ICON_SELECTION_GROW            "gimp-selection-grow"
#define GIMP_ICON_SELECTION_INTERSECT       "gimp-selection-intersect"
#define GIMP_ICON_SELECTION_NONE            "gimp-selection-none"
#define GIMP_ICON_SELECTION_REPLACE         "gimp-selection-replace"
#define GIMP_ICON_SELECTION_SHRINK          "gimp-selection-shrink"
#define GIMP_ICON_SELECTION_STROKE          "gimp-selection-stroke"
#define GIMP_ICON_SELECTION_SUBTRACT        "gimp-selection-subtract"
#define GIMP_ICON_SELECTION_TO_CHANNEL      "gimp-selection-to-channel"
#define GIMP_ICON_SELECTION_TO_PATH         "gimp-selection-to-path"

#define GIMP_ICON_SHAPE_CIRCLE              "gimp-shape-circle"
#define GIMP_ICON_SHAPE_DIAMOND             "gimp-shape-diamond"
#define GIMP_ICON_SHAPE_SQUARE              "gimp-shape-square"

#define GIMP_ICON_SYSTEM_RUN                "system-run"

#define GIMP_ICON_TOOL_AIRBRUSH             "gimp-tool-airbrush"
#define GIMP_ICON_TOOL_ALIGN                "gimp-tool-align"
#define GIMP_ICON_TOOL_BLUR                 "gimp-tool-blur"
#define GIMP_ICON_TOOL_BRIGHTNESS_CONTRAST  "gimp-tool-brightness-contrast"
#define GIMP_ICON_TOOL_BUCKET_FILL          "gimp-tool-bucket-fill"
#define GIMP_ICON_TOOL_BY_COLOR_SELECT      "gimp-tool-by-color-select"
#define GIMP_ICON_TOOL_CAGE                 "gimp-tool-cage"
#define GIMP_ICON_TOOL_CLONE                "gimp-tool-clone"
#define GIMP_ICON_TOOL_COLORIZE             "gimp-tool-colorize"
#define GIMP_ICON_TOOL_COLOR_BALANCE        "gimp-tool-color-balance"
#define GIMP_ICON_TOOL_COLOR_PICKER         "gimp-tool-color-picker"
#define GIMP_ICON_TOOL_COLOR_TEMPERATURE    "gimp-tool-color-temperature"
#define GIMP_ICON_TOOL_CROP                 "gimp-tool-crop"
#define GIMP_ICON_TOOL_CURVES               "gimp-tool-curves"
#define GIMP_ICON_TOOL_DESATURATE           "gimp-tool-desaturate"
#define GIMP_ICON_TOOL_DODGE                "gimp-tool-dodge"
#define GIMP_ICON_TOOL_ELLIPSE_SELECT       "gimp-tool-ellipse-select"
#define GIMP_ICON_TOOL_ERASER               "gimp-tool-eraser"
#define GIMP_ICON_TOOL_EXPOSURE             "gimp-tool-exposure"
#define GIMP_ICON_TOOL_FLIP                 "gimp-tool-flip"
#define GIMP_ICON_TOOL_FOREGROUND_SELECT    "gimp-tool-foreground-select"
#define GIMP_ICON_TOOL_FREE_SELECT          "gimp-tool-free-select"
#define GIMP_ICON_TOOL_FUZZY_SELECT         "gimp-tool-fuzzy-select"
#define GIMP_ICON_TOOL_GRADIENT             "gimp-tool-gradient"
#define GIMP_ICON_TOOL_HANDLE_TRANSFORM     "gimp-tool-handle-transform"
#define GIMP_ICON_TOOL_HEAL                 "gimp-tool-heal"
#define GIMP_ICON_TOOL_HUE_SATURATION       "gimp-tool-hue-saturation"
#define GIMP_ICON_TOOL_INK                  "gimp-tool-ink"
#define GIMP_ICON_TOOL_ISCISSORS            "gimp-tool-iscissors"
#define GIMP_ICON_TOOL_LEVELS               "gimp-tool-levels"
#define GIMP_ICON_TOOL_MEASURE              "gimp-tool-measure"
#define GIMP_ICON_TOOL_MOVE                 "gimp-tool-move"
#define GIMP_ICON_TOOL_MYPAINT_BRUSH        "gimp-tool-mypaint-brush"
#define GIMP_ICON_TOOL_N_POINT_DEFORMATION  "gimp-tool-n-point-deformation"
#define GIMP_ICON_TOOL_OFFSET               "gimp-tool-offset"
#define GIMP_ICON_TOOL_PAINTBRUSH           "gimp-tool-paintbrush"
#define GIMP_ICON_TOOL_PAINT_SELECT         "gimp-tool-paint-select"
#define GIMP_ICON_TOOL_PATH                 "gimp-tool-path"
#define GIMP_ICON_TOOL_PENCIL               "gimp-tool-pencil"
#define GIMP_ICON_TOOL_PERSPECTIVE          "gimp-tool-perspective"
#define GIMP_ICON_TOOL_PERSPECTIVE_CLONE    "gimp-tool-perspective-clone"
#define GIMP_ICON_TOOL_POSTERIZE            "gimp-tool-posterize"
#define GIMP_ICON_TOOL_RECT_SELECT          "gimp-tool-rect-select"
#define GIMP_ICON_TOOL_ROTATE               "gimp-tool-rotate"
#define GIMP_ICON_TOOL_SCALE                "gimp-tool-scale"
#define GIMP_ICON_TOOL_SEAMLESS_CLONE       "gimp-tool-seamless-clone"
#define GIMP_ICON_TOOL_SHADOWS_HIGHLIGHTS   "gimp-tool-shadows-highlights"
#define GIMP_ICON_TOOL_SHEAR                "gimp-tool-shear"
#define GIMP_ICON_TOOL_SMUDGE               "gimp-tool-smudge"
#define GIMP_ICON_TOOL_TEXT                 "gimp-tool-text"
#define GIMP_ICON_TOOL_THRESHOLD            "gimp-tool-threshold"
#define GIMP_ICON_TOOL_TRANSFORM_3D         "gimp-tool-transform-3d"
#define GIMP_ICON_TOOL_UNIFIED_TRANSFORM    "gimp-tool-unified-transform"
#define GIMP_ICON_TOOL_WARP                 "gimp-tool-warp"
#define GIMP_ICON_TOOL_ZOOM                 "gimp-tool-zoom"

#define GIMP_ICON_TRANSFORM_3D_CAMERA       "gimp-transform-3d-camera"
#define GIMP_ICON_TRANSFORM_3D_MOVE         "gimp-transform-3d-move"
#define GIMP_ICON_TRANSFORM_3D_ROTATE       "gimp-transform-3d-rotate"

#define GIMP_ICON_VIEW_REFRESH              "view-refresh"
#define GIMP_ICON_VIEW_FULLSCREEN           "view-fullscreen"
#define GIMP_ICON_VIEW_SHRINK_WRAP          "view-shrink-wrap"
#define GIMP_ICON_VIEW_ZOOM_FILL            "view-zoom-fill"

#define GIMP_ICON_WILBER                    "gimp-wilber"
#define GIMP_ICON_WILBER_EEK                "gimp-wilber-eek"

#define GIMP_ICON_WINDOW_CLOSE              "window-close"
#define GIMP_ICON_WINDOW_MOVE_TO_SCREEN     "gimp-move-to-screen"
#define GIMP_ICON_WINDOW_NEW                "window-new"

#define GIMP_ICON_ZOOM_IN                   "zoom-in"
#define GIMP_ICON_ZOOM_ORIGINAL             "zoom-original"
#define GIMP_ICON_ZOOM_OUT                  "zoom-out"
#define GIMP_ICON_ZOOM_FIT_BEST             "zoom-fit-best"
#define GIMP_ICON_ZOOM_FOLLOW_WINDOW        "gimp-zoom-follow-window"


void     gimp_icons_init           (void);

gboolean gimp_icons_set_icon_theme (GFile *path);


G_END_DECLS

#endif /* __GIMP_ICONS_H__ */
