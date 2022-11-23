/* LIGMA - The GNU Image Manipulation Program
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "tools-actions.h"
#include "tools-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry tools_actions[] =
{
  { "tools-menu",           NULL, NC_("tools-action", "_Tools")           },
  { "tools-select-menu",    NULL, NC_("tools-action", "_Selection Tools") },
  { "tools-paint-menu",     NULL, NC_("tools-action", "_Paint Tools")     },
  { "tools-transform-menu", NULL, NC_("tools-action", "_Transform Tools") },
  { "tools-color-menu",     NULL, NC_("tools-action", "_Color Tools")     },
};

static const LigmaStringActionEntry tools_alternative_actions[] =
{
  { "tools-by-color-select-short", LIGMA_ICON_TOOL_BY_COLOR_SELECT,
    NC_("tools-action", "_By Color"), NULL,
    NC_("tools-action", "Select regions with similar colors"),
    "ligma-by-color-select-tool",
    LIGMA_HELP_TOOL_BY_COLOR_SELECT },

  { "tools-rotate-arbitrary", LIGMA_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), "",
    NC_("tools-action", "Rotate drawable by an arbitrary angle"),
    "ligma-rotate-layer",
    LIGMA_HELP_TOOL_ROTATE },

  { "tools-rotate-image-arbitrary", LIGMA_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), "",
    NC_("tools-action", "Rotate image by an arbitrary angle"),
    "ligma-rotate-image",
    LIGMA_HELP_TOOL_ROTATE }
};

static const LigmaEnumActionEntry tools_color_average_radius_actions[] =
{
  { "tools-color-average-radius-set", LIGMA_ICON_TOOL_COLOR_PICKER,
    "Set Color Picker Radius", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaDoubleActionEntry tools_paintbrush_pixel_size_actions[] =
{
  { "tools-paintbrush-pixel-size-set", LIGMA_ICON_TOOL_MYPAINT_BRUSH,
    "Set Brush Size in Pixel", NULL, NULL,
    1.0, NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_size_actions[] =
{
  { "tools-paintbrush-size-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Size", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_aspect_ratio_actions[] =
{
  { "tools-paintbrush-aspect-ratio-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Aspect Ratio", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_angle_actions[] =
{
  { "tools-paintbrush-angle-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Angle", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_spacing_actions[] =
{
  { "tools-paintbrush-spacing-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Spacing", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_hardness_actions[] =
{
  { "tools-paintbrush-hardness-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Hardness", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_paintbrush_force_actions[] =
{
  { "tools-paintbrush-force-set", LIGMA_ICON_TOOL_PAINTBRUSH,
    "Set Brush Force", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaDoubleActionEntry tools_ink_blob_pixel_size_actions[] =
{
  { "tools-ink-blob-pixel-size-set", LIGMA_ICON_TOOL_INK,
    "Set Ink Blob Size in Pixel", NULL, NULL,
    1.0, NULL }
};

static const LigmaEnumActionEntry tools_ink_blob_size_actions[] =
{
  { "tools-ink-blob-size-set", LIGMA_ICON_TOOL_INK,
    "Set Ink Blob Size", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_ink_blob_aspect_actions[] =
{
  { "tools-ink-blob-aspect-set", LIGMA_ICON_TOOL_INK,
    "Set Ink Blob Aspect", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_ink_blob_angle_actions[] =
{
  { "tools-ink-blob-angle-set", LIGMA_ICON_TOOL_INK,
    "Set Ink Blob Angle", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_airbrush_rate_actions[] =
{
  { "tools-airbrush-rate-set", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-rate-minimum", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-rate-maximum", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease-skip", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase-skip", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry tools_airbrush_flow_actions[] =
{
  { "tools-airbrush-flow-set", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-flow-minimum", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-flow-maximum", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease-skip", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase-skip", LIGMA_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry tools_mybrush_radius_actions[] =
{
  { "tools-mypaint-brush-radius-set", LIGMA_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Radius", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaDoubleActionEntry tools_mybrush_pixel_size_actions[] =
{
  { "tools-mypaint-brush-pixel-size-set", LIGMA_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Diameter in Pixel", NULL, NULL,
    1.0, NULL }
};

static const LigmaEnumActionEntry tools_mybrush_hardness_actions[] =
{
  { "tools-mypaint-brush-hardness-set", LIGMA_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Hardness", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_foreground_select_brush_size_actions[] =
{
  { "tools-foreground-select-brush-size-set",
    LIGMA_ICON_TOOL_FOREGROUND_SELECT,
    "Set Foreground Select Brush Size", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_transform_preview_opacity_actions[] =
{
  { "tools-transform-preview-opacity-set", LIGMA_ICON_TOOL_PERSPECTIVE,
    "Set Transform Tool Preview Opacity", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaDoubleActionEntry tools_warp_effect_pixel_size_actions[] =
{
  { "tools-warp-effect-pixel-size-set", LIGMA_ICON_TOOL_WARP,
    "Set Warp Effect Size in Pixel", NULL, NULL,
    1.0, NULL }
};

static const LigmaEnumActionEntry tools_warp_effect_size_actions[] =
{
  { "tools-warp-effect-size-set", LIGMA_ICON_TOOL_WARP,
    "Set Warp Effect Size", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_warp_effect_hardness_actions[] =
{
  { "tools-warp-effect-hardness-set", LIGMA_ICON_TOOL_WARP,
    "Set Warp Effect Hardness", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const LigmaEnumActionEntry tools_opacity_actions[] =
{
  { "tools-opacity-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-opacity-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set to Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-opacity-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-opacity-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-opacity-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 1"), "less", NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 1"), "greater", NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 10"), "<primary>less", NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 10"), "<primary>greater", NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_size_actions[] =
{
  { "tools-size-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-size-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set to Default Value"), "backslash", NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-size-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-size-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-size-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 1"), "bracketleft", NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 1"), "bracketright", NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 10"), "braceleft", NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 10"), "braceright", NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_aspect_actions[] =
{
  { "tools-aspect-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-aspect-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set To Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-aspect-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-aspect-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-aspect-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_angle_actions[] =
{
  { "tools-angle-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-angle-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set Angle To Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-angle-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-angle-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-angle-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 1°"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 1°"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 15°"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 15°"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_spacing_actions[] =
{
  { "tools-spacing-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-spacing-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set To Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-spacing-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-spacing-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-spacing-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_hardness_actions[] =
{
  { "tools-hardness-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-hardness-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set to Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-hardness-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-hardness-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-hardness-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaEnumActionEntry tools_force_actions[] =
{
  { "tools-force-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-force-set-to-default", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set to Default Value"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-force-minimum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Minimize"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-force-maximum", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Maximize"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-force-decrease", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-skip", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-percent", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const LigmaDoubleActionEntry tools_paint_select_pixel_size_actions[] =
{
  { "tools-paint-select-pixel-size-set", LIGMA_ICON_TOOL_PAINT_SELECT,
    "Set Paint Select Brush Size in Pixel", NULL, NULL,
    1.0, NULL }
};

static const LigmaEnumActionEntry tools_object_1_actions[] =
{
  { "tools-object-1-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Select Object 1 by Index", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-1-first", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "First Object 1", NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-1-last", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Last Object 1", NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-1-previous", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Previous Object 1", NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-1-next", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Next Object 1", NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry tools_object_2_actions[] =
{
  { "tools-object-2-set", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Select Object 2 by Index", NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-2-first", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "First Object 2", NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-2-last", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Last Object 2", NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-2-previous", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Previous Object 2", NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-2-next", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    "Next Object 2", NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};


void
tools_actions_setup (LigmaActionGroup *group)
{
  LigmaAction *action;
  GList      *list;

  ligma_action_group_add_actions (group, "tools-action",
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions));

  ligma_action_group_add_string_actions (group, "tools-action",
                                        tools_alternative_actions,
                                        G_N_ELEMENTS (tools_alternative_actions),
                                        tools_select_cmd_callback);

  action = ligma_action_group_get_action (group,
                                         "tools-by-color-select-short");
  ligma_action_set_accel_path (action, "<Actions>/tools/tools-by-color-select");

  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_color_average_radius_actions,
                                      G_N_ELEMENTS (tools_color_average_radius_actions),
                                      tools_color_average_radius_cmd_callback);

  ligma_action_group_add_double_actions (group, NULL,
                                        tools_paintbrush_pixel_size_actions,
                                        G_N_ELEMENTS (tools_paintbrush_pixel_size_actions),
                                        tools_paintbrush_pixel_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_size_actions,
                                      G_N_ELEMENTS (tools_paintbrush_size_actions),
                                      tools_paintbrush_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_aspect_ratio_actions,
                                      G_N_ELEMENTS (tools_paintbrush_aspect_ratio_actions),
                                      tools_paintbrush_aspect_ratio_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_angle_actions,
                                      G_N_ELEMENTS (tools_paintbrush_angle_actions),
                                      tools_paintbrush_angle_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_spacing_actions,
                                      G_N_ELEMENTS (tools_paintbrush_spacing_actions),
                                      tools_paintbrush_spacing_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_hardness_actions,
                                      G_N_ELEMENTS (tools_paintbrush_hardness_actions),
                                      tools_paintbrush_hardness_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_force_actions,
                                      G_N_ELEMENTS (tools_paintbrush_force_actions),
                                      tools_paintbrush_force_cmd_callback);

  ligma_action_group_add_double_actions (group, NULL,
                                        tools_ink_blob_pixel_size_actions,
                                        G_N_ELEMENTS (tools_ink_blob_pixel_size_actions),
                                        tools_ink_blob_pixel_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_size_actions,
                                      G_N_ELEMENTS (tools_ink_blob_size_actions),
                                      tools_ink_blob_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_aspect_actions,
                                      G_N_ELEMENTS (tools_ink_blob_aspect_actions),
                                      tools_ink_blob_aspect_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_angle_actions,
                                      G_N_ELEMENTS (tools_ink_blob_angle_actions),
                                      tools_ink_blob_angle_cmd_callback);

  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_rate_actions,
                                      G_N_ELEMENTS (tools_airbrush_rate_actions),
                                      tools_airbrush_rate_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_flow_actions,
                                      G_N_ELEMENTS (tools_airbrush_flow_actions),
                                      tools_airbrush_flow_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_radius_actions,
                                      G_N_ELEMENTS (tools_mybrush_radius_actions),
                                      tools_mybrush_radius_cmd_callback);
  ligma_action_group_add_double_actions (group, NULL,
                                        tools_mybrush_pixel_size_actions,
                                        G_N_ELEMENTS (tools_mybrush_pixel_size_actions),
                                        tools_mybrush_pixel_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_hardness_actions,
                                      G_N_ELEMENTS (tools_mybrush_hardness_actions),
                                      tools_mybrush_hardness_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_foreground_select_brush_size_actions,
                                      G_N_ELEMENTS (tools_foreground_select_brush_size_actions),
                                      tools_fg_select_brush_size_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_transform_preview_opacity_actions,
                                      G_N_ELEMENTS (tools_transform_preview_opacity_actions),
                                      tools_transform_preview_opacity_cmd_callback);

  ligma_action_group_add_double_actions (group, NULL,
                                        tools_warp_effect_pixel_size_actions,
                                        G_N_ELEMENTS (tools_warp_effect_pixel_size_actions),
                                        tools_warp_effect_pixel_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_size_actions,
                                      G_N_ELEMENTS (tools_warp_effect_size_actions),
                                      tools_warp_effect_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_hardness_actions,
                                      G_N_ELEMENTS (tools_warp_effect_hardness_actions),
                                      tools_warp_effect_hardness_cmd_callback);

  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_opacity_actions,
                                      G_N_ELEMENTS (tools_opacity_actions),
                                      tools_opacity_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_size_actions,
                                      G_N_ELEMENTS (tools_size_actions),
                                      tools_size_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_aspect_actions,
                                      G_N_ELEMENTS (tools_aspect_actions),
                                      tools_aspect_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_angle_actions,
                                      G_N_ELEMENTS (tools_angle_actions),
                                      tools_angle_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_spacing_actions,
                                      G_N_ELEMENTS (tools_spacing_actions),
                                      tools_spacing_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_hardness_actions,
                                      G_N_ELEMENTS (tools_hardness_actions),
                                      tools_hardness_cmd_callback);
  ligma_action_group_add_enum_actions (group, "tools-action",
                                      tools_force_actions,
                                      G_N_ELEMENTS (tools_force_actions),
                                      tools_force_cmd_callback);

  ligma_action_group_add_double_actions (group, NULL,
                                        tools_paint_select_pixel_size_actions,
                                        G_N_ELEMENTS (tools_paint_select_pixel_size_actions),
                                        tools_paint_select_pixel_size_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_object_1_actions,
                                      G_N_ELEMENTS (tools_object_1_actions),
                                      tools_object_1_cmd_callback);
  ligma_action_group_add_enum_actions (group, NULL,
                                      tools_object_2_actions,
                                      G_N_ELEMENTS (tools_object_2_actions),
                                      tools_object_2_cmd_callback);

  for (list = ligma_get_tool_info_iter (group->ligma);
       list;
       list = g_list_next (list))
    {
      LigmaToolInfo *tool_info = list->data;

      if (tool_info->menu_label)
        {
          LigmaStringActionEntry  entry;
          gchar                 *name;
          const gchar           *icon_name;
          const gchar           *identifier;

          name       = ligma_tool_info_get_action_name (tool_info);
          icon_name  = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info));
          identifier = ligma_object_get_name (tool_info);

          entry.name        = name;
          entry.icon_name   = icon_name;
          entry.label       = tool_info->menu_label;
          entry.accelerator = tool_info->menu_accel;
          entry.tooltip     = tool_info->tooltip;
          entry.help_id     = tool_info->help_id;
          entry.value       = identifier;

          ligma_action_group_add_string_actions (group, NULL,
                                                &entry, 1,
                                                tools_select_cmd_callback);

          g_free (name);
        }
    }
}

void
tools_actions_update (LigmaActionGroup *group,
                      gpointer         data)
{
  LigmaImage *image = action_data_get_image (data);

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("tools-gegl", image);

#undef SET_SENSITIVE
}
