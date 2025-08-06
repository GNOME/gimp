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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "context-actions.h"
#include "context-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static const GimpActionEntry context_actions[] =
{
  { "context-colors-default", GIMP_ICON_COLORS_DEFAULT,
    NC_("context-action", "_Default Colors"), NULL, { "D", NULL },
    NC_("context-action",
        "Set foreground color to black, background color to white"),
    context_colors_default_cmd_callback,
    GIMP_HELP_TOOLBOX_DEFAULT_COLORS },

  { "context-colors-swap", GIMP_ICON_COLORS_SWAP,
    NC_("context-action", "S_wap Colors"), NULL, { "X", NULL },
    NC_("context-action", "Exchange foreground and background colors"),
    context_colors_swap_cmd_callback,
    GIMP_HELP_TOOLBOX_SWAP_COLORS }
};

static GimpEnumActionEntry context_palette_foreground_actions[] =
{
  { "context-palette-foreground-set", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Palette"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-foreground-first", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-foreground-last", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-foreground-previous", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-foreground-previous-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_palette_background_actions[] =
{
  { "context-palette-background-set", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Palette"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-background-first", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use First Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-background-last", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-background-previous", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-background-previous-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Skip Back Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Skip Forward Palette Color"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_colormap_foreground_actions[] =
{
  { "context-colormap-foreground-set", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Set Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-foreground-first", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use First Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-foreground-last", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Last Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-foreground-previous", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Previous Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Next Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-foreground-previous-skip", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Back Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next-skip", GIMP_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Forward Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_colormap_background_actions[] =
{
  { "context-colormap-background-set", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Set Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-background-first", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Use First Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-background-last", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Use Last Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-background-previous", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Use Previous Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Use Next Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-background-previous-skip", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Back Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next-skip", GIMP_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Forward Color From Colormap"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_swatch_foreground_actions[] =
{
  { "context-swatch-foreground-set", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-foreground-first", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-foreground-last", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-foreground-previous", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Color From Swatch"), NULL, { "9", NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Color From Swatch"), NULL, { "0", NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-foreground-previous-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_swatch_background_actions[] =
{
  { "context-swatch-background-set", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-background-first", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use First Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-background-last", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-background-previous", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Color From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-background-previous-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Back From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next-skip", GIMP_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Forward From Swatch"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_red_actions[] =
{
  { "context-foreground-red-set", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-red-minimum", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-red-maximum", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-red-decrease", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-red-decrease-skip", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase-skip", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_green_actions[] =
{
  { "context-foreground-green-set", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-green-minimum", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-green-maximum", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-green-decrease", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-green-decrease-skip", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase-skip", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_blue_actions[] =
{
  { "context-foreground-blue-set", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-blue-minimum", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-blue-maximum", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-blue-decrease", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-blue-decrease-skip", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase-skip", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_red_actions[] =
{
  { "context-background-red-set", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-red-minimum", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-red-maximum", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-red-decrease", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-red-decrease-skip", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase-skip", GIMP_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_green_actions[] =
{
  { "context-background-green-set", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-green-minimum", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-green-maximum", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-green-decrease", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-green-decrease-skip", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase-skip", GIMP_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_blue_actions[] =
{
  { "context-background-blue-set", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-blue-minimum", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-blue-maximum", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-blue-decrease", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-blue-decrease-skip", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase-skip", GIMP_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_hue_actions[] =
{
  { "context-foreground-hue-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-hue-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-hue-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-hue-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-hue-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_saturation_actions[] =
{
  { "context-foreground-saturation-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-saturation-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-saturation-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-saturation-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-saturation-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_value_actions[] =
{
  { "context-foreground-value-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-value-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-value-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-value-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-value-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_hue_actions[] =
{
  { "context-background-hue-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-hue-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-hue-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-hue-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-hue-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_saturation_actions[] =
{
  { "context-background-saturation-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-saturation-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-saturation-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-saturation-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-saturation-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_value_actions[] =
{
  { "context-background-value-set", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-value-minimum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-value-maximum", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-value-decrease", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 1%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-value-decrease-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase-skip", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 10%"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_opacity_actions[] =
{
  { "context-opacity-set", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Set Transparency"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-opacity-transparent", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-opacity-opaque", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-opacity-decrease", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-opacity-decrease-skip", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase-skip", GIMP_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_paint_mode_actions[] =
{
  { "context-paint-mode-first", GIMP_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-paint-mode-last", GIMP_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-paint-mode-previous", GIMP_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-paint-mode-next", GIMP_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_tool_select_actions[] =
{
  { "context-tool-select-set", GIMP_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Choose by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-tool-select-first", GIMP_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-tool-select-last", GIMP_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-tool-select-previous", GIMP_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-tool-select-next", GIMP_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_select_actions[] =
{
  { "context-brush-select-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Select by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-select-first", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-select-last", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-select-previous", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-select-next", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_pattern_select_actions[] =
{
  { "context-pattern-select-set", GIMP_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Select by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-pattern-select-first", GIMP_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-pattern-select-last", GIMP_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-pattern-select-previous", GIMP_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-pattern-select-next", GIMP_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_palette_select_actions[] =
{
  { "context-palette-select-set", GIMP_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Select by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-palette-select-first", GIMP_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-select-last", GIMP_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-select-previous", GIMP_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-select-next", GIMP_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_gradient_select_actions[] =
{
  { "context-gradient-select-set", GIMP_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Select by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-gradient-select-first", GIMP_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-gradient-select-last", GIMP_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-gradient-select-previous", GIMP_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-gradient-select-next", GIMP_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_font_select_actions[] =
{
  { "context-font-select-set", GIMP_ICON_FONT,
    NC_("context-action", "Font Selection: Select by Index"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-font-select-first", GIMP_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-font-select-last", GIMP_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-font-select-previous", GIMP_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-font-select-next", GIMP_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_spacing_actions[] =
{
  { "context-brush-spacing-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spacing-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spacing-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spacing-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spacing-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 10"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 10"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_shape_actions[] =
{
  { "context-brush-shape-circle", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Circular"), NULL, { NULL }, NULL,
    GIMP_BRUSH_GENERATED_CIRCLE, FALSE,
    NULL },
  { "context-brush-shape-square", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Square"), NULL, { NULL }, NULL,
    GIMP_BRUSH_GENERATED_SQUARE, FALSE,
    NULL },
  { "context-brush-shape-diamond", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Diamond"), NULL, { NULL }, NULL,
    GIMP_BRUSH_GENERATED_DIAMOND, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_radius_actions[] =
{
  { "context-brush-radius-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-radius-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-radius-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-radius-decrease-less", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SMALL_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-less", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SMALL_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 10"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 10"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-percent", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease Relative"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-percent", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase Relative"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_spikes_actions[] =
{
  { "context-brush-spikes-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spikes-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spikes-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spikes-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spikes-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 4"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 4"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_hardness_actions[] =
{
  { "context-brush-hardness-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-hardness-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-hardness-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-hardness-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.01"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.01"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-hardness-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_aspect_actions[] =
{
  { "context-brush-aspect-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-aspect-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-aspect-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-aspect-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-aspect-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 1"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_angle_actions[] =
{
  { "context-brush-angle-set", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-angle-minimum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Horizontal"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-angle-maximum", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Vertical"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-angle-decrease", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 1°"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 1°"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-angle-decrease-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 15°"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase-skip", GIMP_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 15°"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpToggleActionEntry context_toggle_actions[] =
{
  { "context-dynamics-toggle", NULL,
    NC_("context-action", "_Enable/Disable Dynamics"), NULL, { NULL },
    NC_("context-action", "Apply or ignore the dynamics when painting"),
    context_toggle_dynamics_cmd_callback,
    FALSE,
    NULL },
};


void
context_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "context-action",
                                 context_actions,
                                 G_N_ELEMENTS (context_actions));

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_palette_foreground_actions,
                                      G_N_ELEMENTS (context_palette_foreground_actions),
                                      context_palette_foreground_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_palette_background_actions,
                                      G_N_ELEMENTS (context_palette_background_actions),
                                      context_palette_background_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_foreground_actions,
                                      G_N_ELEMENTS (context_colormap_foreground_actions),
                                      context_colormap_foreground_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_background_actions,
                                      G_N_ELEMENTS (context_colormap_background_actions),
                                      context_colormap_background_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_foreground_actions,
                                      G_N_ELEMENTS (context_swatch_foreground_actions),
                                      context_swatch_foreground_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_background_actions,
                                      G_N_ELEMENTS (context_swatch_background_actions),
                                      context_swatch_background_cmd_callback);


  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_red_actions,
                                      G_N_ELEMENTS (context_foreground_red_actions),
                                      context_foreground_red_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_green_actions,
                                      G_N_ELEMENTS (context_foreground_green_actions),
                                      context_foreground_green_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_blue_actions,
                                      G_N_ELEMENTS (context_foreground_blue_actions),
                                      context_foreground_blue_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_hue_actions,
                                      G_N_ELEMENTS (context_foreground_hue_actions),
                                      context_foreground_hue_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_saturation_actions,
                                      G_N_ELEMENTS (context_foreground_saturation_actions),
                                      context_foreground_saturation_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_value_actions,
                                      G_N_ELEMENTS (context_foreground_value_actions),
                                      context_foreground_value_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_red_actions,
                                      G_N_ELEMENTS (context_background_red_actions),
                                      context_background_red_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_green_actions,
                                      G_N_ELEMENTS (context_background_green_actions),
                                      context_background_green_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_blue_actions,
                                      G_N_ELEMENTS (context_background_blue_actions),
                                      context_background_blue_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_hue_actions,
                                      G_N_ELEMENTS (context_background_hue_actions),
                                      context_background_hue_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_saturation_actions,
                                      G_N_ELEMENTS (context_background_saturation_actions),
                                      context_background_saturation_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_background_value_actions,
                                      G_N_ELEMENTS (context_background_value_actions),
                                      context_background_value_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_opacity_actions,
                                      G_N_ELEMENTS (context_opacity_actions),
                                      context_opacity_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_paint_mode_actions,
                                      G_N_ELEMENTS (context_paint_mode_actions),
                                      context_paint_mode_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_tool_select_actions,
                                      G_N_ELEMENTS (context_tool_select_actions),
                                      context_tool_select_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_select_actions,
                                      G_N_ELEMENTS (context_brush_select_actions),
                                      context_brush_select_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_pattern_select_actions,
                                      G_N_ELEMENTS (context_pattern_select_actions),
                                      context_pattern_select_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_palette_select_actions,
                                      G_N_ELEMENTS (context_palette_select_actions),
                                      context_palette_select_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_gradient_select_actions,
                                      G_N_ELEMENTS (context_gradient_select_actions),
                                      context_gradient_select_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_font_select_actions,
                                      G_N_ELEMENTS (context_font_select_actions),
                                      context_font_select_cmd_callback);

  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spacing_actions,
                                      G_N_ELEMENTS (context_brush_spacing_actions),
                                      context_brush_spacing_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_shape_actions,
                                      G_N_ELEMENTS (context_brush_shape_actions),
                                      context_brush_shape_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_radius_actions,
                                      G_N_ELEMENTS (context_brush_radius_actions),
                                      context_brush_radius_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spikes_actions,
                                      G_N_ELEMENTS (context_brush_spikes_actions),
                                      context_brush_spikes_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_hardness_actions,
                                      G_N_ELEMENTS (context_brush_hardness_actions),
                                      context_brush_hardness_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_aspect_actions,
                                      G_N_ELEMENTS (context_brush_aspect_actions),
                                      context_brush_aspect_cmd_callback);
  gimp_action_group_add_enum_actions (group, "context-action",
                                      context_brush_angle_actions,
                                      G_N_ELEMENTS (context_brush_angle_actions),
                                      context_brush_angle_cmd_callback);

  gimp_action_group_add_toggle_actions (group, "context-action",
                                        context_toggle_actions,
                                        G_N_ELEMENTS (context_toggle_actions));
}

void
context_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
#if 0
  GimpContext *context   = action_data_get_context (data);
  gboolean     generated = FALSE;
  gdouble      radius    = 0.0;
  gint         spikes    = 0;
  gdouble      hardness  = 0.0;
  gdouble      aspect    = 0.0;
  gdouble      angle     = 0.0;

  if (context)
    {
      GimpBrush *brush = gimp_context_get_brush (context);

      if (GIMP_IS_BRUSH_GENERATED (brush))
        {
          GimpBrushGenerated *gen = GIMP_BRUSH_GENERATED (brush);

          generated = TRUE;

          radius   = gimp_brush_generated_get_radius       (gen);
          spikes   = gimp_brush_generated_get_spikes       (gen);
          hardness = gimp_brush_generated_get_hardness     (gen);
          aspect   = gimp_brush_generated_get_aspect_ratio (gen);
          angle    = gimp_brush_generated_get_angle        (gen);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, "context-" action, (condition) != 0)

  SET_SENSITIVE ("brush-radius-minimum",       generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease",      generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease-skip", generated && radius > 1.0);

  SET_SENSITIVE ("brush-radius-maximum",       generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase",      generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase-skip", generated && radius < 4000.0);

  SET_SENSITIVE ("brush-angle-minimum",       generated);
  SET_SENSITIVE ("brush-angle-decrease",      generated);
  SET_SENSITIVE ("brush-angle-decrease-skip", generated);

  SET_SENSITIVE ("brush-angle-maximum",       generated);
  SET_SENSITIVE ("brush-angle-increase",      generated);
  SET_SENSITIVE ("brush-angle-increase-skip", generated);
#undef SET_SENSITIVE

#endif
}
