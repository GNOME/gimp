/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaicons.h
 * Copyright (C) 2001-2015 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_ICONS_H__
#define __LIGMA_ICONS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  random actions that don't fit in any category  */

#define LIGMA_ICON_ATTACH                    "ligma-attach"
#define LIGMA_ICON_DETACH                    "ligma-detach"
#define LIGMA_ICON_INVERT                    "ligma-invert"
#define LIGMA_ICON_RECORD                    "media-record"
#define LIGMA_ICON_RESET                     "ligma-reset"
#define LIGMA_ICON_SHRED                     "ligma-shred"


/*  random states/things that don't fit in any category  */

#define LIGMA_ICON_BUSINESS_CARD             "ligma-business-card"
#define LIGMA_ICON_CHAR_PICKER               "ligma-char-picker"
#define LIGMA_ICON_CURSOR                    "ligma-cursor"
#define LIGMA_ICON_DISPLAY                   "ligma-display"
#define LIGMA_ICON_GEGL                      "ligma-gegl"
#define LIGMA_ICON_LINKED                    "ligma-linked"
#define LIGMA_ICON_MARKER                    "ligma-marker"
#define LIGMA_ICON_SMARTPHONE                "ligma-smartphone"
#define LIGMA_ICON_TRANSPARENCY              "ligma-transparency"
#define LIGMA_ICON_VIDEO                     "ligma-video"
#define LIGMA_ICON_VISIBLE                   "ligma-visible"
#define LIGMA_ICON_WEB                       "ligma-web"


/*  random objects/entities that don't fit in any category  */

#define LIGMA_ICON_BRUSH                     LIGMA_ICON_TOOL_PAINTBRUSH
#define LIGMA_ICON_BUFFER                    LIGMA_ICON_EDIT_PASTE
#define LIGMA_ICON_COLORMAP                  "ligma-colormap"
#define LIGMA_ICON_DYNAMICS                  "ligma-dynamics"
#define LIGMA_ICON_FILE_MANAGER              "ligma-file-manager"
#define LIGMA_ICON_FONT                      "gtk-select-font"
#define LIGMA_ICON_GRADIENT                  LIGMA_ICON_TOOL_GRADIENT
#define LIGMA_ICON_GRID                      "ligma-grid"
#define LIGMA_ICON_INPUT_DEVICE              "ligma-input-device"
#define LIGMA_ICON_MYPAINT_BRUSH             LIGMA_ICON_TOOL_MYPAINT_BRUSH
#define LIGMA_ICON_PALETTE                   "gtk-select-color"
#define LIGMA_ICON_PATTERN                   "ligma-pattern"
#define LIGMA_ICON_PLUGIN                    "ligma-plugin"
#define LIGMA_ICON_SAMPLE_POINT              "ligma-sample-point"
#define LIGMA_ICON_SYMMETRY                  "ligma-symmetry"
#define LIGMA_ICON_TEMPLATE                  "ligma-template"
#define LIGMA_ICON_TOOL_PRESET               "ligma-tool-preset"


/*  not really icons  */

#define LIGMA_ICON_FRAME                     "ligma-frame"
#define LIGMA_ICON_TEXTURE                   "ligma-texture"


/*  icons that follow, or at least try to follow the FDO naming and
 *  category conventions; and groups of icons with a common prefix;
 *  all sorted alphabetically
 *
 *  see also:
 *  https://specifications.freedesktop.org/icon-naming-spec/latest/ar01s04.html
 *
 *  When icons are available as standard Freedesktop icons, we use these
 *  in priority. As a fallback, we use standard GTK+ icons. As last
 *  fallback, we create our own icons under the "ligma-" namespace.
 */

#define LIGMA_ICON_APPLICATION_EXIT          "application-exit"

#define LIGMA_ICON_ASPECT_PORTRAIT           "ligma-portrait"
#define LIGMA_ICON_ASPECT_LANDSCAPE          "ligma-landscape"

#define LIGMA_ICON_CAP_BUTT                  "ligma-cap-butt"
#define LIGMA_ICON_CAP_ROUND                 "ligma-cap-round"
#define LIGMA_ICON_CAP_SQUARE                "ligma-cap-square"

#define LIGMA_ICON_CENTER                    "ligma-center"
#define LIGMA_ICON_CENTER_HORIZONTAL         "ligma-hcenter"
#define LIGMA_ICON_CENTER_VERTICAL           "ligma-vcenter"

#define LIGMA_ICON_CHAIN_HORIZONTAL          "ligma-hchain"
#define LIGMA_ICON_CHAIN_HORIZONTAL_BROKEN   "ligma-hchain-broken"
#define LIGMA_ICON_CHAIN_VERTICAL            "ligma-vchain"
#define LIGMA_ICON_CHAIN_VERTICAL_BROKEN     "ligma-vchain-broken"

#define LIGMA_ICON_CHANNEL                   "ligma-channel"
#define LIGMA_ICON_CHANNEL_ALPHA             "ligma-channel-alpha"
#define LIGMA_ICON_CHANNEL_BLUE              "ligma-channel-blue"
#define LIGMA_ICON_CHANNEL_GRAY              "ligma-channel-gray"
#define LIGMA_ICON_CHANNEL_GREEN             "ligma-channel-green"
#define LIGMA_ICON_CHANNEL_INDEXED           "ligma-channel-indexed"
#define LIGMA_ICON_CHANNEL_RED               "ligma-channel-red"

#define LIGMA_ICON_CLOSE                     "ligma-close"
#define LIGMA_ICON_CLOSE_ALL                 "ligma-close-all"

#define LIGMA_ICON_COLOR_PICKER_BLACK        "ligma-color-picker-black"
#define LIGMA_ICON_COLOR_PICKER_GRAY         "ligma-color-picker-gray"
#define LIGMA_ICON_COLOR_PICKER_WHITE        "ligma-color-picker-white"
#define LIGMA_ICON_COLOR_PICK_FROM_SCREEN    "ligma-color-pick-from-screen"

#define LIGMA_ICON_COLOR_SELECTOR_CMYK       "ligma-color-cmyk"
#define LIGMA_ICON_COLOR_SELECTOR_TRIANGLE   "ligma-color-triangle"
#define LIGMA_ICON_COLOR_SELECTOR_WATER      "ligma-color-water"

#define LIGMA_ICON_COLOR_SPACE_LINEAR        "ligma-color-space-linear"
#define LIGMA_ICON_COLOR_SPACE_NON_LINEAR    "ligma-color-space-non-linear"
#define LIGMA_ICON_COLOR_SPACE_PERCEPTUAL    "ligma-color-space-perceptual"

#define LIGMA_ICON_COLORS_DEFAULT            "ligma-default-colors"
#define LIGMA_ICON_COLORS_SWAP               "ligma-swap-colors"

#define LIGMA_ICON_CONTROLLER                "ligma-controller"
#define LIGMA_ICON_CONTROLLER_KEYBOARD       "ligma-controller-keyboard"
#define LIGMA_ICON_CONTROLLER_LINUX_INPUT    "ligma-controller-linux-input"
#define LIGMA_ICON_CONTROLLER_MIDI           "ligma-controller-midi"
#define LIGMA_ICON_CONTROLLER_MOUSE          LIGMA_ICON_CURSOR
#define LIGMA_ICON_CONTROLLER_WHEEL          "ligma-controller-wheel"

#define LIGMA_ICON_CONVERT_RGB               "ligma-convert-rgb"
#define LIGMA_ICON_CONVERT_GRAYSCALE         "ligma-convert-grayscale"
#define LIGMA_ICON_CONVERT_INDEXED           "ligma-convert-indexed"
#define LIGMA_ICON_CONVERT_PRECISION         LIGMA_ICON_CONVERT_RGB

#define LIGMA_ICON_CURVE_FREE                "ligma-curve-free"
#define LIGMA_ICON_CURVE_SMOOTH              "ligma-curve-smooth"

#define LIGMA_ICON_DIALOG_CHANNELS           "ligma-channels"
#define LIGMA_ICON_DIALOG_DASHBOARD          "ligma-dashboard"
#define LIGMA_ICON_DIALOG_DEVICE_STATUS      "ligma-device-status"
#define LIGMA_ICON_DIALOG_ERROR              "dialog-error"
#define LIGMA_ICON_DIALOG_IMAGES             "ligma-images"
#define LIGMA_ICON_DIALOG_INFORMATION        "dialog-information"
#define LIGMA_ICON_DIALOG_LAYERS             "ligma-layers"
#define LIGMA_ICON_DIALOG_NAVIGATION         "ligma-navigation"
#define LIGMA_ICON_DIALOG_PATHS              "ligma-paths"
#define LIGMA_ICON_DIALOG_QUESTION           "dialog-question"
#define LIGMA_ICON_DIALOG_RESHOW_FILTER      "ligma-reshow-filter"
#define LIGMA_ICON_DIALOG_TOOLS              "ligma-tools"
#define LIGMA_ICON_DIALOG_TOOL_OPTIONS       "ligma-tool-options"
#define LIGMA_ICON_DIALOG_UNDO_HISTORY       "ligma-undo-history"
#define LIGMA_ICON_DIALOG_WARNING            "dialog-warning"

#define LIGMA_ICON_DISPLAY_FILTER              "ligma-display-filter"
#define LIGMA_ICON_DISPLAY_FILTER_CLIP_WARNING "ligma-display-filter-clip-warning"
#define LIGMA_ICON_DISPLAY_FILTER_COLORBLIND   "ligma-display-filter-colorblind"
#define LIGMA_ICON_DISPLAY_FILTER_CONTRAST     "ligma-display-filter-contrast"
#define LIGMA_ICON_DISPLAY_FILTER_GAMMA        "ligma-display-filter-gamma"
#define LIGMA_ICON_DISPLAY_FILTER_LCMS         "ligma-display-filter-lcms"
#define LIGMA_ICON_DISPLAY_FILTER_PROOF        "ligma-display-filter-proof"

#define LIGMA_ICON_LOCK                      "ligma-lock"
#define LIGMA_ICON_LOCK_ALPHA                "ligma-lock-alpha"
#define LIGMA_ICON_LOCK_CONTENT              "ligma-lock-content"
#define LIGMA_ICON_LOCK_POSITION             "ligma-lock-position"
#define LIGMA_ICON_LOCK_VISIBILITY           "ligma-lock-visibility"
#define LIGMA_ICON_LOCK_MULTI                "ligma-lock-multi"

#define LIGMA_ICON_DOCUMENT_NEW              "document-new"
#define LIGMA_ICON_DOCUMENT_OPEN             "document-open"
#define LIGMA_ICON_DOCUMENT_OPEN_RECENT      "document-open-recent"
#define LIGMA_ICON_DOCUMENT_PAGE_SETUP       "document-page-setup"
#define LIGMA_ICON_DOCUMENT_PRINT            "document-print"
#define LIGMA_ICON_DOCUMENT_PRINT_RESOLUTION "document-print"
#define LIGMA_ICON_DOCUMENT_PROPERTIES       "document-properties"
#define LIGMA_ICON_DOCUMENT_REVERT           "document-revert"
#define LIGMA_ICON_DOCUMENT_SAVE             "document-save"
#define LIGMA_ICON_DOCUMENT_SAVE_AS          "document-save-as"

#define LIGMA_ICON_EDIT                      "gtk-edit"
#define LIGMA_ICON_EDIT_CLEAR                "edit-clear"
#define LIGMA_ICON_EDIT_COPY                 "edit-copy"
#define LIGMA_ICON_EDIT_CUT                  "edit-cut"
#define LIGMA_ICON_EDIT_DELETE               "edit-delete"
#define LIGMA_ICON_EDIT_FIND                 "edit-find"
#define LIGMA_ICON_EDIT_PASTE                "edit-paste"
#define LIGMA_ICON_EDIT_PASTE_AS_NEW         "ligma-paste-as-new"
#define LIGMA_ICON_EDIT_PASTE_INTO           "ligma-paste-into"
#define LIGMA_ICON_EDIT_REDO                 "edit-redo"
#define LIGMA_ICON_EDIT_UNDO                 "edit-undo"

#define LIGMA_ICON_EVEN_HORIZONTAL_GAP       "ligma-even-horizontal-gap"
#define LIGMA_ICON_EVEN_VERTICAL_GAP         "ligma-even-vertical-gap"

#define LIGMA_ICON_FILL_HORIZONTAL           "ligma-hfill"
#define LIGMA_ICON_FILL_VERTICAL             "ligma-vfill"

#define LIGMA_ICON_FOLDER_NEW                "folder-new"

#define LIGMA_ICON_FORMAT_INDENT_MORE         "format-indent-more"
#define LIGMA_ICON_FORMAT_INDENT_LESS         "format-indent-less"
#define LIGMA_ICON_FORMAT_JUSTIFY_CENTER      "format-justify-center"
#define LIGMA_ICON_FORMAT_JUSTIFY_FILL        "format-justify-fill"
#define LIGMA_ICON_FORMAT_JUSTIFY_LEFT        "format-justify-left"
#define LIGMA_ICON_FORMAT_JUSTIFY_RIGHT       "format-justify-right"
#define LIGMA_ICON_FORMAT_TEXT_BOLD           "format-text-bold"
#define LIGMA_ICON_FORMAT_TEXT_ITALIC         "format-text-italic"
#define LIGMA_ICON_FORMAT_TEXT_STRIKETHROUGH  "format-text-strikethrough"
#define LIGMA_ICON_FORMAT_TEXT_UNDERLINE      "format-text-underline"
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_LTR  "format-text-direction-ltr"
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_RTL  "format-text-direction-rtl"
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL           "ligma-text-dir-ttb-rtl" /* use FDO */
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT   "ligma-text-dir-ttb-rtl-upright" /* use FDO */
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR           "ligma-text-dir-ttb-ltr" /* use FDO */
#define LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT   "ligma-text-dir-ttb-ltr-upright" /* use FDO */
#define LIGMA_ICON_FORMAT_TEXT_SPACING_LETTER "ligma-letter-spacing"
#define LIGMA_ICON_FORMAT_TEXT_SPACING_LINE   "ligma-line-spacing"

#define LIGMA_ICON_GRADIENT_LINEAR               "ligma-gradient-linear"
#define LIGMA_ICON_GRADIENT_BILINEAR             "ligma-gradient-bilinear"
#define LIGMA_ICON_GRADIENT_RADIAL               "ligma-gradient-radial"
#define LIGMA_ICON_GRADIENT_SQUARE               "ligma-gradient-square"
#define LIGMA_ICON_GRADIENT_CONICAL_SYMMETRIC    "ligma-gradient-conical-symmetric"
#define LIGMA_ICON_GRADIENT_CONICAL_ASYMMETRIC   "ligma-gradient-conical-asymmetric"
#define LIGMA_ICON_GRADIENT_SHAPEBURST_ANGULAR   "ligma-gradient-shapeburst-angular"
#define LIGMA_ICON_GRADIENT_SHAPEBURST_SPHERICAL "ligma-gradient-shapeburst-spherical"
#define LIGMA_ICON_GRADIENT_SHAPEBURST_DIMPLED   "ligma-gradient-shapeburst-dimpled"
#define LIGMA_ICON_GRADIENT_SPIRAL_CLOCKWISE     "ligma-gradient-spiral-clockwise"
#define LIGMA_ICON_GRADIENT_SPIRAL_ANTICLOCKWISE "ligma-gradient-spiral-anticlockwise"

#define LIGMA_ICON_GRAVITY_EAST              "ligma-gravity-east"
#define LIGMA_ICON_GRAVITY_NORTH             "ligma-gravity-north"
#define LIGMA_ICON_GRAVITY_NORTH_EAST        "ligma-gravity-north-east"
#define LIGMA_ICON_GRAVITY_NORTH_WEST        "ligma-gravity-north-west"
#define LIGMA_ICON_GRAVITY_SOUTH             "ligma-gravity-south"
#define LIGMA_ICON_GRAVITY_SOUTH_EAST        "ligma-gravity-south-east"
#define LIGMA_ICON_GRAVITY_SOUTH_WEST        "ligma-gravity-south-west"
#define LIGMA_ICON_GRAVITY_WEST              "ligma-gravity-west"

#define LIGMA_ICON_GO_BOTTOM                 "go-bottom"
#define LIGMA_ICON_GO_DOWN                   "go-down"
#define LIGMA_ICON_GO_FIRST                  "go-first"
#define LIGMA_ICON_GO_HOME                   "go-home"
#define LIGMA_ICON_GO_LAST                   "go-last"
#define LIGMA_ICON_GO_TOP                    "go-top"
#define LIGMA_ICON_GO_UP                     "go-up"
#define LIGMA_ICON_GO_PREVIOUS               "go-previous"
#define LIGMA_ICON_GO_NEXT                   "go-next"

#define LIGMA_ICON_HELP                      "system-help"
#define LIGMA_ICON_HELP_ABOUT                "help-about"
#define LIGMA_ICON_HELP_USER_MANUAL          "ligma-user-manual"

#define LIGMA_ICON_HISTOGRAM                 "ligma-histogram"
#define LIGMA_ICON_HISTOGRAM_LINEAR          "ligma-histogram-linear"
#define LIGMA_ICON_HISTOGRAM_LOGARITHMIC     "ligma-histogram-logarithmic"

#define LIGMA_ICON_IMAGE                     "ligma-image"
#define LIGMA_ICON_IMAGE_OPEN                "ligma-image-open"
#define LIGMA_ICON_IMAGE_RELOAD              "ligma-image-reload"

#define LIGMA_ICON_JOIN_MITER                "ligma-join-miter"
#define LIGMA_ICON_JOIN_ROUND                "ligma-join-round"
#define LIGMA_ICON_JOIN_BEVEL                "ligma-join-bevel"

#define LIGMA_ICON_LAYER                     "ligma-layer"
#define LIGMA_ICON_LAYER_ANCHOR              "ligma-anchor"
#define LIGMA_ICON_LAYER_FLOATING_SELECTION  "ligma-floating-selection"
#define LIGMA_ICON_LAYER_MASK                "ligma-layer-mask"
#define LIGMA_ICON_LAYER_MERGE_DOWN          "ligma-merge-down"
#define LIGMA_ICON_LAYER_TEXT_LAYER          "ligma-text-layer"
#define LIGMA_ICON_LAYER_TO_IMAGESIZE        "ligma-layer-to-imagesize"

#define LIGMA_ICON_LIST                      "ligma-list"
#define LIGMA_ICON_LIST_ADD                  "list-add"
#define LIGMA_ICON_LIST_REMOVE               "list-remove"

#define LIGMA_ICON_MENU_LEFT                 "ligma-menu-left"
#define LIGMA_ICON_MENU_RIGHT                "ligma-menu-right"

#define LIGMA_ICON_OBJECT_DUPLICATE          "ligma-duplicate"
#define LIGMA_ICON_OBJECT_FLIP_HORIZONTAL    "object-flip-horizontal"
#define LIGMA_ICON_OBJECT_FLIP_VERTICAL      "object-flip-vertical"
#define LIGMA_ICON_OBJECT_RESIZE             "ligma-resize"
#define LIGMA_ICON_OBJECT_ROTATE_180         "ligma-rotate-180"
#define LIGMA_ICON_OBJECT_ROTATE_270         "object-rotate-left"
#define LIGMA_ICON_OBJECT_ROTATE_90          "object-rotate-right"
#define LIGMA_ICON_OBJECT_SCALE              "ligma-scale"

#define LIGMA_ICON_PATH                      "ligma-path"
#define LIGMA_ICON_PATH_STROKE               "ligma-path-stroke"

#define LIGMA_ICON_PIVOT_CENTER              "ligma-pivot-center"
#define LIGMA_ICON_PIVOT_EAST                "ligma-pivot-east"
#define LIGMA_ICON_PIVOT_NORTH               "ligma-pivot-north"
#define LIGMA_ICON_PIVOT_NORTH_EAST          "ligma-pivot-north-east"
#define LIGMA_ICON_PIVOT_NORTH_WEST          "ligma-pivot-north-west"
#define LIGMA_ICON_PIVOT_SOUTH               "ligma-pivot-south"
#define LIGMA_ICON_PIVOT_SOUTH_EAST          "ligma-pivot-south-east"
#define LIGMA_ICON_PIVOT_SOUTH_WEST          "ligma-pivot-south-west"
#define LIGMA_ICON_PIVOT_WEST                "ligma-pivot-west"

#define LIGMA_ICON_PREFERENCES_SYSTEM        "preferences-system"

#define LIGMA_ICON_PROCESS_STOP              "process-stop"

#define LIGMA_ICON_QUICK_MASK_OFF            "ligma-quick-mask-off"
#define LIGMA_ICON_QUICK_MASK_ON             "ligma-quick-mask-on"

#define LIGMA_ICON_SELECTION                 "ligma-selection"
#define LIGMA_ICON_SELECTION_ADD             "ligma-selection-add"
#define LIGMA_ICON_SELECTION_ALL             "ligma-selection-all"
#define LIGMA_ICON_SELECTION_BORDER          "ligma-selection-border"
#define LIGMA_ICON_SELECTION_GROW            "ligma-selection-grow"
#define LIGMA_ICON_SELECTION_INTERSECT       "ligma-selection-intersect"
#define LIGMA_ICON_SELECTION_NONE            "ligma-selection-none"
#define LIGMA_ICON_SELECTION_REPLACE         "ligma-selection-replace"
#define LIGMA_ICON_SELECTION_SHRINK          "ligma-selection-shrink"
#define LIGMA_ICON_SELECTION_STROKE          "ligma-selection-stroke"
#define LIGMA_ICON_SELECTION_SUBTRACT        "ligma-selection-subtract"
#define LIGMA_ICON_SELECTION_TO_CHANNEL      "ligma-selection-to-channel"
#define LIGMA_ICON_SELECTION_TO_PATH         "ligma-selection-to-path"

#define LIGMA_ICON_SHAPE_CIRCLE              "ligma-shape-circle"
#define LIGMA_ICON_SHAPE_DIAMOND             "ligma-shape-diamond"
#define LIGMA_ICON_SHAPE_SQUARE              "ligma-shape-square"

#define LIGMA_ICON_SYSTEM_RUN                "system-run"

#define LIGMA_ICON_TOOL_AIRBRUSH             "ligma-tool-airbrush"
#define LIGMA_ICON_TOOL_ALIGN                "ligma-tool-align"
#define LIGMA_ICON_TOOL_BLUR                 "ligma-tool-blur"
#define LIGMA_ICON_TOOL_BRIGHTNESS_CONTRAST  "ligma-tool-brightness-contrast"
#define LIGMA_ICON_TOOL_BUCKET_FILL          "ligma-tool-bucket-fill"
#define LIGMA_ICON_TOOL_BY_COLOR_SELECT      "ligma-tool-by-color-select"
#define LIGMA_ICON_TOOL_CAGE                 "ligma-tool-cage"
#define LIGMA_ICON_TOOL_CLONE                "ligma-tool-clone"
#define LIGMA_ICON_TOOL_COLORIZE             "ligma-tool-colorize"
#define LIGMA_ICON_TOOL_COLOR_BALANCE        "ligma-tool-color-balance"
#define LIGMA_ICON_TOOL_COLOR_PICKER         "ligma-tool-color-picker"
#define LIGMA_ICON_TOOL_COLOR_TEMPERATURE    "ligma-tool-color-temperature"
#define LIGMA_ICON_TOOL_CROP                 "ligma-tool-crop"
#define LIGMA_ICON_TOOL_CURVES               "ligma-tool-curves"
#define LIGMA_ICON_TOOL_DESATURATE           "ligma-tool-desaturate"
#define LIGMA_ICON_TOOL_DODGE                "ligma-tool-dodge"
#define LIGMA_ICON_TOOL_ELLIPSE_SELECT       "ligma-tool-ellipse-select"
#define LIGMA_ICON_TOOL_ERASER               "ligma-tool-eraser"
#define LIGMA_ICON_TOOL_EXPOSURE             "ligma-tool-exposure"
#define LIGMA_ICON_TOOL_FLIP                 "ligma-tool-flip"
#define LIGMA_ICON_TOOL_FOREGROUND_SELECT    "ligma-tool-foreground-select"
#define LIGMA_ICON_TOOL_FREE_SELECT          "ligma-tool-free-select"
#define LIGMA_ICON_TOOL_FUZZY_SELECT         "ligma-tool-fuzzy-select"
#define LIGMA_ICON_TOOL_GRADIENT             "ligma-tool-gradient"
#define LIGMA_ICON_TOOL_HANDLE_TRANSFORM     "ligma-tool-handle-transform"
#define LIGMA_ICON_TOOL_HEAL                 "ligma-tool-heal"
#define LIGMA_ICON_TOOL_HUE_SATURATION       "ligma-tool-hue-saturation"
#define LIGMA_ICON_TOOL_INK                  "ligma-tool-ink"
#define LIGMA_ICON_TOOL_ISCISSORS            "ligma-tool-iscissors"
#define LIGMA_ICON_TOOL_LEVELS               "ligma-tool-levels"
#define LIGMA_ICON_TOOL_MEASURE              "ligma-tool-measure"
#define LIGMA_ICON_TOOL_MOVE                 "ligma-tool-move"
#define LIGMA_ICON_TOOL_MYPAINT_BRUSH        "ligma-tool-mypaint-brush"
#define LIGMA_ICON_TOOL_N_POINT_DEFORMATION  "ligma-tool-n-point-deformation"
#define LIGMA_ICON_TOOL_OFFSET               "ligma-tool-offset"
#define LIGMA_ICON_TOOL_PAINTBRUSH           "ligma-tool-paintbrush"
#define LIGMA_ICON_TOOL_PAINT_SELECT         "ligma-tool-paint-select"
#define LIGMA_ICON_TOOL_PATH                 "ligma-tool-path"
#define LIGMA_ICON_TOOL_PENCIL               "ligma-tool-pencil"
#define LIGMA_ICON_TOOL_PERSPECTIVE          "ligma-tool-perspective"
#define LIGMA_ICON_TOOL_PERSPECTIVE_CLONE    "ligma-tool-perspective-clone"
#define LIGMA_ICON_TOOL_POSTERIZE            "ligma-tool-posterize"
#define LIGMA_ICON_TOOL_RECT_SELECT          "ligma-tool-rect-select"
#define LIGMA_ICON_TOOL_ROTATE               "ligma-tool-rotate"
#define LIGMA_ICON_TOOL_SCALE                "ligma-tool-scale"
#define LIGMA_ICON_TOOL_SEAMLESS_CLONE       "ligma-tool-seamless-clone"
#define LIGMA_ICON_TOOL_SHADOWS_HIGHLIGHTS   "ligma-tool-shadows-highlights"
#define LIGMA_ICON_TOOL_SHEAR                "ligma-tool-shear"
#define LIGMA_ICON_TOOL_SMUDGE               "ligma-tool-smudge"
#define LIGMA_ICON_TOOL_TEXT                 "ligma-tool-text"
#define LIGMA_ICON_TOOL_THRESHOLD            "ligma-tool-threshold"
#define LIGMA_ICON_TOOL_TRANSFORM_3D         "ligma-tool-transform-3d"
#define LIGMA_ICON_TOOL_UNIFIED_TRANSFORM    "ligma-tool-unified-transform"
#define LIGMA_ICON_TOOL_WARP                 "ligma-tool-warp"
#define LIGMA_ICON_TOOL_ZOOM                 "ligma-tool-zoom"

#define LIGMA_ICON_TRANSFORM_3D_CAMERA       "ligma-transform-3d-camera"
#define LIGMA_ICON_TRANSFORM_3D_MOVE         "ligma-transform-3d-move"
#define LIGMA_ICON_TRANSFORM_3D_ROTATE       "ligma-transform-3d-rotate"

#define LIGMA_ICON_VIEW_REFRESH              "view-refresh"
#define LIGMA_ICON_VIEW_FULLSCREEN           "view-fullscreen"

#define LIGMA_ICON_WILBER                    "ligma-wilber"
#define LIGMA_ICON_WILBER_EEK                "ligma-wilber-eek"

#define LIGMA_ICON_WINDOW_CLOSE              "window-close"
#define LIGMA_ICON_WINDOW_MOVE_TO_SCREEN     "ligma-move-to-screen"
#define LIGMA_ICON_WINDOW_NEW                "window-new"

#define LIGMA_ICON_ZOOM_IN                   "zoom-in"
#define LIGMA_ICON_ZOOM_ORIGINAL             "zoom-original"
#define LIGMA_ICON_ZOOM_OUT                  "zoom-out"
#define LIGMA_ICON_ZOOM_FIT_BEST             "zoom-fit-best"
#define LIGMA_ICON_ZOOM_FOLLOW_WINDOW        "ligma-zoom-follow-window"


void   ligma_icons_init           (void);

void   ligma_icons_set_icon_theme (GFile *path);


G_END_DECLS

#endif /* __LIGMA_ICONS_H__ */
