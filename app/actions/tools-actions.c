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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "tools-actions.h"
#include "tools-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry tools_actions[] =
{
  { "tools-menu",           NULL, NC_("tools-action", "_Tools")           },
  { "tools-select-menu",    NULL, NC_("tools-action", "_Selection Tools") },
  { "tools-paint-menu",     NULL, NC_("tools-action", "_Paint Tools")     },
  { "tools-transform-menu", NULL, NC_("tools-action", "_Transform Tools") },
  { "tools-color-menu",     NULL, NC_("tools-action", "_Color Tools")     },
};

static const GimpStringActionEntry tools_alternative_actions[] =
{
  { "tools-by-color-select-short", GIMP_ICON_TOOL_BY_COLOR_SELECT,
    NC_("tools-action", "_By Color"), NULL,
    NC_("tools-action", "Select regions with similar colors"),
    "gimp-by-color-select-tool",
    GIMP_HELP_TOOL_BY_COLOR_SELECT },

  { "tools-rotate-arbitrary", GIMP_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), "",
    NC_("tools-action", "Rotate drawable by an arbitrary angle"),
    "gimp-rotate-layer",
    GIMP_HELP_TOOL_ROTATE },

  { "tools-rotate-image-arbitrary", GIMP_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), "",
    NC_("tools-action", "Rotate image by an arbitrary angle"),
    "gimp-rotate-image",
    GIMP_HELP_TOOL_ROTATE }
};

static const GimpEnumActionEntry tools_color_average_radius_actions[] =
{
  { "tools-color-average-radius-set", GIMP_ICON_TOOL_COLOR_PICKER,
    "Set Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_size_actions[] =
{
  { "tools-paintbrush-size-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_aspect_ratio_actions[] =
{
  { "tools-paintbrush-aspect-ratio-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Aspect Ratio", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_angle_actions[] =
{
  { "tools-paintbrush-angle-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Angle", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_spacing_actions[] =
{
  { "tools-paintbrush-spacing-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Spacing", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_hardness_actions[] =
{
  { "tools-paintbrush-hardness-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_paintbrush_force_actions[] =
{
  { "tools-paintbrush-force-set", GIMP_ICON_TOOL_PAINTBRUSH,
    "Set Brush Force", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_ink_blob_size_actions[] =
{
  { "tools-ink-blob-size-set", GIMP_ICON_TOOL_INK,
    "Set Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_ink_blob_aspect_actions[] =
{
  { "tools-ink-blob-aspect-set", GIMP_ICON_TOOL_INK,
    "Set Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_ink_blob_angle_actions[] =
{
  { "tools-ink-blob-angle-set", GIMP_ICON_TOOL_INK,
    "Set Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_airbrush_rate_actions[] =
{
  { "tools-airbrush-rate-set", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-rate-minimum", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Minimum"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-rate-maximum", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Maximum"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease-skip", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase-skip", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry tools_airbrush_flow_actions[] =
{
  { "tools-airbrush-flow-set", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-flow-minimum", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Minimum"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-flow-maximum", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Maximum"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease-skip", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase-skip", GIMP_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry tools_mybrush_radius_actions[] =
{
  { "tools-mypaint-brush-radius-set", GIMP_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Radius", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_mybrush_hardness_actions[] =
{
  { "tools-mypaint-brush-hardness-set", GIMP_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_foreground_select_brush_size_actions[] =
{
  { "tools-foreground-select-brush-size-set",
    GIMP_ICON_TOOL_FOREGROUND_SELECT,
    "Set Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_transform_preview_opacity_actions[] =
{
  { "tools-transform-preview-opacity-set", GIMP_ICON_TOOL_PERSPECTIVE,
    "Set Transform Tool Preview Opacity", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_warp_effect_size_actions[] =
{
  { "tools-warp-effect-size-set", GIMP_ICON_TOOL_WARP,
    "Set Warp Effect Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_warp_effect_hardness_actions[] =
{
  { "tools-warp-effect-hardness-set", GIMP_ICON_TOOL_WARP,
    "Set Warp Effect Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const GimpEnumActionEntry tools_opacity_actions[] =
{
  { "tools-opacity-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-opacity-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set to Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-opacity-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-opacity-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-opacity-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 1"), "less", NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 1"), "greater", NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 10"), "<primary>less", NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 10"), "<primary>greater", NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_size_actions[] =
{
  { "tools-size-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-size-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set to Default Value"), "backslash", NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-size-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-size-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-size-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 1"), "bracketleft", NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 1"), "bracketright", NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 10"), "<shift>bracketleft", NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 10"), "<shift>bracketright", NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_aspect_actions[] =
{
  { "tools-aspect-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-aspect-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set To Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-aspect-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-aspect-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-aspect-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 0.1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 0.1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_angle_actions[] =
{
  { "tools-angle-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-angle-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set Angle To Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-angle-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-angle-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-angle-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 1°"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 1°"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 15°"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 15°"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_spacing_actions[] =
{
  { "tools-spacing-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-spacing-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set To Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-spacing-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-spacing-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-spacing-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_hardness_actions[] =
{
  { "tools-hardness-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-hardness-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set to Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-hardness-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-hardness-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-hardness-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_force_actions[] =
{
  { "tools-force-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-force-set-to-default", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set to Default Value"), NULL, NULL,
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-force-minimum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Minimize"), NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-force-maximum", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Maximize"), NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-force-decrease", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 1"), NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-skip", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 10"), NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-percent", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase Relative"), NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_object_1_actions[] =
{
  { "tools-object-1-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Select Object 1 by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-1-first", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "First Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-1-last", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Last Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-1-previous", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Previous Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-1-next", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Next Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry tools_object_2_actions[] =
{
  { "tools-object-2-set", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Select Object 2 by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-2-first", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "First Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-2-last", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Last Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-2-previous", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Previous Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-2-next", GIMP_ICON_DIALOG_TOOL_OPTIONS,
    "Next Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};


void
tools_actions_setup (GimpActionGroup *group)
{
  GimpAction *action;
  GList      *list;

  gimp_action_group_add_actions (group, "tools-action",
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions));

  gimp_action_group_add_string_actions (group, "tools-action",
                                        tools_alternative_actions,
                                        G_N_ELEMENTS (tools_alternative_actions),
                                        tools_select_cmd_callback);

  action = gimp_action_group_get_action (group,
                                         "tools-by-color-select-short");
  gimp_action_set_accel_path (action, "<Actions>/tools/tools-by-color-select");

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_color_average_radius_actions,
                                      G_N_ELEMENTS (tools_color_average_radius_actions),
                                      tools_color_average_radius_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_size_actions,
                                      G_N_ELEMENTS (tools_paintbrush_size_actions),
                                      tools_paintbrush_size_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_aspect_ratio_actions,
                                      G_N_ELEMENTS (tools_paintbrush_aspect_ratio_actions),
                                      tools_paintbrush_aspect_ratio_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_angle_actions,
                                      G_N_ELEMENTS (tools_paintbrush_angle_actions),
                                      tools_paintbrush_angle_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_spacing_actions,
                                      G_N_ELEMENTS (tools_paintbrush_spacing_actions),
                                      tools_paintbrush_spacing_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_hardness_actions,
                                      G_N_ELEMENTS (tools_paintbrush_hardness_actions),
                                      tools_paintbrush_hardness_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_force_actions,
                                      G_N_ELEMENTS (tools_paintbrush_force_actions),
                                      tools_paintbrush_force_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_size_actions,
                                      G_N_ELEMENTS (tools_ink_blob_size_actions),
                                      tools_ink_blob_size_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_aspect_actions,
                                      G_N_ELEMENTS (tools_ink_blob_aspect_actions),
                                      tools_ink_blob_aspect_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_angle_actions,
                                      G_N_ELEMENTS (tools_ink_blob_angle_actions),
                                      tools_ink_blob_angle_cmd_callback);

  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_rate_actions,
                                      G_N_ELEMENTS (tools_airbrush_rate_actions),
                                      tools_airbrush_rate_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_flow_actions,
                                      G_N_ELEMENTS (tools_airbrush_flow_actions),
                                      tools_airbrush_flow_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_radius_actions,
                                      G_N_ELEMENTS (tools_mybrush_radius_actions),
                                      tools_mybrush_radius_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_hardness_actions,
                                      G_N_ELEMENTS (tools_mybrush_hardness_actions),
                                      tools_mybrush_hardness_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_foreground_select_brush_size_actions,
                                      G_N_ELEMENTS (tools_foreground_select_brush_size_actions),
                                      tools_fg_select_brush_size_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_transform_preview_opacity_actions,
                                      G_N_ELEMENTS (tools_transform_preview_opacity_actions),
                                      tools_transform_preview_opacity_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_size_actions,
                                      G_N_ELEMENTS (tools_warp_effect_size_actions),
                                      tools_warp_effect_size_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_hardness_actions,
                                      G_N_ELEMENTS (tools_warp_effect_hardness_actions),
                                      tools_warp_effect_hardness_cmd_callback);

  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_opacity_actions,
                                      G_N_ELEMENTS (tools_opacity_actions),
                                      tools_opacity_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_size_actions,
                                      G_N_ELEMENTS (tools_size_actions),
                                      tools_size_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_aspect_actions,
                                      G_N_ELEMENTS (tools_aspect_actions),
                                      tools_aspect_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_angle_actions,
                                      G_N_ELEMENTS (tools_angle_actions),
                                      tools_angle_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_spacing_actions,
                                      G_N_ELEMENTS (tools_spacing_actions),
                                      tools_spacing_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_hardness_actions,
                                      G_N_ELEMENTS (tools_hardness_actions),
                                      tools_hardness_cmd_callback);
  gimp_action_group_add_enum_actions (group, "tools-action",
                                      tools_force_actions,
                                      G_N_ELEMENTS (tools_force_actions),
                                      tools_force_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_object_1_actions,
                                      G_N_ELEMENTS (tools_object_1_actions),
                                      tools_object_1_cmd_callback);
  gimp_action_group_add_enum_actions (group, NULL,
                                      tools_object_2_actions,
                                      G_N_ELEMENTS (tools_object_2_actions),
                                      tools_object_2_cmd_callback);

  for (list = gimp_get_tool_info_iter (group->gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      if (tool_info->menu_label)
        {
          GimpStringActionEntry  entry;
          gchar                 *name;
          const gchar           *icon_name;
          const gchar           *identifier;

          name       = gimp_tool_info_get_action_name (tool_info);
          icon_name  = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info));
          identifier = gimp_object_get_name (tool_info);

          entry.name        = name;
          entry.icon_name   = icon_name;
          entry.label       = tool_info->menu_label;
          entry.accelerator = tool_info->menu_accel;
          entry.tooltip     = tool_info->tooltip;
          entry.help_id     = tool_info->help_id;
          entry.value       = identifier;

          gimp_action_group_add_string_actions (group, NULL,
                                                &entry, 1,
                                                tools_select_cmd_callback);

          g_free (name);
        }
    }
}

void
tools_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
}
