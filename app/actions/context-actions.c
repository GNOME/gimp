/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

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
  { "context-menu",            NULL,                      N_("_Context")    },
  { "context-colors-menu",     GIMP_STOCK_DEFAULT_COLORS, N_("_Colors")     },
  { "context-opacity-menu",    GIMP_STOCK_TRANSPARENCY,   N_("_Opacity")    },
  { "context-paint-mode-menu", GIMP_STOCK_TOOL_PENCIL,    N_("Paint _Mode") },
  { "context-tool-menu",       GIMP_STOCK_TOOLS,          N_("_Tool")       },
  { "context-brush-menu",      GIMP_STOCK_BRUSH,          N_("_Brush")      },
  { "context-pattern-menu",    GIMP_STOCK_PATTERN,        N_("_Pattern")    },
  { "context-palette-menu",    GIMP_STOCK_PALETTE,        N_("_Palette")    },
  { "context-gradient-menu",   GIMP_STOCK_GRADIENT,       N_("_Gradient")   },
  { "context-font-menu",       GIMP_STOCK_FONT,           N_("_Font")       },

  { "context-brush-shape-menu",    NULL,                  N_("_Shape")      },
  { "context-brush-radius-menu",   NULL,                  N_("_Radius")     },
  { "context-brush-spikes-menu",   NULL,                  N_("S_pikes")     },
  { "context-brush-hardness-menu", NULL,                  N_("_Hardness")   },
  { "context-brush-aspect-menu",   NULL,                  N_("_Aspect")     },
  { "context-brush-angle-menu",    NULL,                  N_("A_ngle")      },

  { "context-colors-default", GIMP_STOCK_DEFAULT_COLORS,
    N_("_Default Colors"), "D",
    N_("Set foreground color to black, background color to white"),
    G_CALLBACK (context_colors_default_cmd_callback),
    GIMP_HELP_TOOLBOX_DEFAULT_COLORS },

  { "context-colors-swap", GIMP_STOCK_SWAP_COLORS,
    N_("S_wap Colors"), "X",
    N_("Exchange foreground and background colors"),
    G_CALLBACK (context_colors_swap_cmd_callback),
    GIMP_HELP_TOOLBOX_SWAP_COLORS }
};

static GimpEnumActionEntry context_palette_foreground_actions[] =
{
  { "context-palette-foreground-set", GIMP_STOCK_PALETTE,
    "Foreground Palette color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-foreground-first", GIMP_STOCK_PALETTE,
    "Foreground Palette color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-foreground-last", GIMP_STOCK_PALETTE,
    "Foreground Palette color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-foreground-previous", GIMP_STOCK_PALETTE,
    "Foreground Palette color Previous", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next", GIMP_STOCK_PALETTE,
    "Foreground Palette color Next", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-foreground-previous-skip", GIMP_STOCK_PALETTE,
    "Foreground Palette color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next-skip", GIMP_STOCK_PALETTE,
    "Foreground Palette color Skip Forward", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_palette_background_actions[] =
{
  { "context-palette-background-set", GIMP_STOCK_PALETTE,
    "Background Palette color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-background-first", GIMP_STOCK_PALETTE,
    "Background Palette color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-background-last", GIMP_STOCK_PALETTE,
    "Background Palette color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-background-previous", GIMP_STOCK_PALETTE,
    "Background Palette color Previous", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next", GIMP_STOCK_PALETTE,
    "Background Palette color Next", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-background-previous-skip", GIMP_STOCK_PALETTE,
    "Background Palette color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next-skip", GIMP_STOCK_PALETTE,
    "Background Palette color Skip ahead", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_colormap_foreground_actions[] =
{
  { "context-colormap-foreground-set", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-foreground-first", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-foreground-last", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-foreground-previous", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Previous", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Next", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-foreground-previous-skip", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next-skip", GIMP_STOCK_COLORMAP,
    "Foreground Colormap color Skip Forward", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_colormap_background_actions[] =
{
  { "context-colormap-background-set", GIMP_STOCK_COLORMAP,
    "Background Colormap color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-background-first", GIMP_STOCK_COLORMAP,
    "Background Colormap color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-background-last", GIMP_STOCK_COLORMAP,
    "Background Colormap color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-background-previous", GIMP_STOCK_COLORMAP,
    "Background Colormap color Previous", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next", GIMP_STOCK_COLORMAP,
    "Background Colormap color Next", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-background-previous-skip", GIMP_STOCK_COLORMAP,
    "Background Colormap color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next-skip", GIMP_STOCK_COLORMAP,
    "Background Colormap color Skip ahead", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_swatch_foreground_actions[] =
{
  { "context-swatch-foreground-set", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-foreground-first", GIMP_STOCK_PALETTE,
    "Foreground Swatch color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-foreground-last", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-foreground-previous", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Previous", "9", NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Next", "0", NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-foreground-previous-skip", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next-skip", GIMP_STOCK_PALETTE,
    "Foreground Swatch color Skip Forward", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static GimpEnumActionEntry context_swatch_background_actions[] =
{
  { "context-swatch-background-set", GIMP_STOCK_PALETTE,
    "Background Swatch color Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-background-first", GIMP_STOCK_PALETTE,
    "Background Swatch color First", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-background-last", GIMP_STOCK_PALETTE,
    "Background Swatch color Last", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-background-previous", GIMP_STOCK_PALETTE,
    "Background Swatch color Previous", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next", GIMP_STOCK_PALETTE,
    "Background Swatch color Next", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-background-previous-skip", GIMP_STOCK_PALETTE,
    "Background Swatch color Skip Back", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next-skip", GIMP_STOCK_PALETTE,
    "Background Swatch color Skip ahead", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_red_actions[] =
{
  { "context-foreground-red-set", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-red-minimum", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-red-maximum", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-red-decrease", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-red-decrease-skip", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase-skip", GIMP_STOCK_CHANNEL_RED,
    "Foreground Red Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_green_actions[] =
{
  { "context-foreground-green-set", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-green-minimum", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-green-maximum", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-green-decrease", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-green-decrease-skip", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase-skip", GIMP_STOCK_CHANNEL_GREEN,
    "Foreground Green Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_blue_actions[] =
{
  { "context-foreground-blue-set", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-blue-minimum", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-blue-maximum", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-blue-decrease", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-blue-decrease-skip", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase-skip", GIMP_STOCK_CHANNEL_BLUE,
    "Foreground Blue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_red_actions[] =
{
  { "context-background-red-set", GIMP_STOCK_CHANNEL_RED,
    "Background Red Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-red-minimum", GIMP_STOCK_CHANNEL_RED,
    "Background Red Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-red-maximum", GIMP_STOCK_CHANNEL_RED,
    "Background Red Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-red-decrease", GIMP_STOCK_CHANNEL_RED,
    "Background Red Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase", GIMP_STOCK_CHANNEL_RED,
    "Background Red Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-red-decrease-skip", GIMP_STOCK_CHANNEL_RED,
    "Background Red Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase-skip", GIMP_STOCK_CHANNEL_RED,
    "Background Red Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_green_actions[] =
{
  { "context-background-green-set", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-green-minimum", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-green-maximum", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-green-decrease", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-green-decrease-skip", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase-skip", GIMP_STOCK_CHANNEL_GREEN,
    "Background Green Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_blue_actions[] =
{
  { "context-background-blue-set", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-blue-minimum", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-blue-maximum", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-blue-decrease", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-blue-decrease-skip", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase-skip", GIMP_STOCK_CHANNEL_BLUE,
    "Background Blue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_hue_actions[] =
{
  { "context-foreground-hue-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-hue-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-hue-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-hue-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-hue-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Hue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_saturation_actions[] =
{
  { "context-foreground-saturation-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-saturation-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-saturation-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-saturation-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-saturation-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Saturation Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_foreground_value_actions[] =
{
  { "context-foreground-value-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-value-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-value-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-value-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-value-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Foreground Value Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_hue_actions[] =
{
  { "context-background-hue-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-hue-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-hue-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-hue-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-hue-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Hue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_saturation_actions[] =
{
  { "context-background-saturation-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-saturation-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-saturation-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-saturation-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-saturation-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Saturation Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_background_value_actions[] =
{
  { "context-background-value-set", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-value-minimum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-value-maximum", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-value-decrease", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-value-decrease-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase-skip", GIMP_STOCK_TOOL_HUE_SATURATION,
    "Background Value Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_opacity_actions[] =
{
  { "context-opacity-set", GIMP_STOCK_TRANSPARENCY,
    "Set Transparency", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-opacity-transparent", GIMP_STOCK_TRANSPARENCY,
    "Completely Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-opacity-opaque", GIMP_STOCK_TRANSPARENCY,
    "Completely Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-opacity-decrease", GIMP_STOCK_TRANSPARENCY,
    "More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase", GIMP_STOCK_TRANSPARENCY,
    "More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-opacity-decrease-skip", GIMP_STOCK_TRANSPARENCY,
    "10% More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase-skip", GIMP_STOCK_TRANSPARENCY,
    "10% More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_paint_mode_actions[] =
{
  { "context-paint-mode-first", GIMP_STOCK_TOOL_PENCIL,
    "First Paint Mode", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-paint-mode-last", GIMP_STOCK_TOOL_PENCIL,
    "Last Paint Mode", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-paint-mode-previous", GIMP_STOCK_TOOL_PENCIL,
    "Previous Paint Mode", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-paint-mode-next", GIMP_STOCK_TOOL_PENCIL,
    "Next Paint Mode", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_tool_select_actions[] =
{
  { "context-tool-select-set", GIMP_STOCK_TOOLS,
    "Select Tool by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-tool-select-first", GIMP_STOCK_TOOLS,
    "First Tool", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-tool-select-last", GIMP_STOCK_TOOLS,
    "Last Tool", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-tool-select-previous", GIMP_STOCK_TOOLS,
    "Previous Tool", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-tool-select-next", GIMP_STOCK_TOOLS,
    "Next Tool", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_select_actions[] =
{
  { "context-brush-select-set", GIMP_STOCK_BRUSH,
    "Select Brush by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-select-first", GIMP_STOCK_BRUSH,
    "First Brush", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-select-last", GIMP_STOCK_BRUSH,
    "Last Brush", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-select-previous", GIMP_STOCK_BRUSH,
    "Previous Brush", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-select-next", GIMP_STOCK_BRUSH,
    "Next Brush", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_pattern_select_actions[] =
{
  { "context-pattern-select-set", GIMP_STOCK_PATTERN,
    "Select Pattern by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-pattern-select-first", GIMP_STOCK_PATTERN,
    "First Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-pattern-select-last", GIMP_STOCK_PATTERN,
    "Last Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-pattern-select-previous", GIMP_STOCK_PATTERN,
    "Previous Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-pattern-select-next", GIMP_STOCK_PATTERN,
    "Next Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_palette_select_actions[] =
{
  { "context-palette-select-set", GIMP_STOCK_PALETTE,
    "Select Palette by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-palette-select-first", GIMP_STOCK_PALETTE,
    "First Palette", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-select-last", GIMP_STOCK_PALETTE,
    "Last Palette", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-select-previous", GIMP_STOCK_PALETTE,
    "Previous Palette", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-select-next", GIMP_STOCK_PALETTE,
    "Next Palette", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_gradient_select_actions[] =
{
  { "context-gradient-select-set", GIMP_STOCK_GRADIENT,
    "Select Gradient by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-gradient-select-first", GIMP_STOCK_GRADIENT,
    "First Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-gradient-select-last", GIMP_STOCK_GRADIENT,
    "Last Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-gradient-select-previous", GIMP_STOCK_GRADIENT,
    "Previous Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-gradient-select-next", GIMP_STOCK_GRADIENT,
    "Next Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_font_select_actions[] =
{
  { "context-font-select-set", GIMP_STOCK_FONT,
    "Select Font by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-font-select-first", GIMP_STOCK_FONT,
    "First Font", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-font-select-last", GIMP_STOCK_FONT,
    "Last Font", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-font-select-previous", GIMP_STOCK_FONT,
    "Previous Font", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-font-select-next", GIMP_STOCK_FONT,
    "Next Font", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_shape_actions[] =
{
  { "context-brush-shape-circle", GIMP_STOCK_BRUSH,
    "Circular Brush", NULL, NULL,
    GIMP_BRUSH_GENERATED_CIRCLE, FALSE,
    NULL },
  { "context-brush-shape-square", GIMP_STOCK_BRUSH,
    "Square Brush", NULL, NULL,
    GIMP_BRUSH_GENERATED_SQUARE, FALSE,
    NULL },
  { "context-brush-shape-diamond", GIMP_STOCK_BRUSH,
    "Diamond Brush", NULL, NULL,
    GIMP_BRUSH_GENERATED_DIAMOND, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_radius_actions[] =
{
  { "context-brush-radius-set", GIMP_STOCK_BRUSH,
    "Set Brush Radius", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-radius-minimum", GIMP_STOCK_BRUSH,
    "Minumum Radius", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-radius-maximum", GIMP_STOCK_BRUSH,
    "Maximum Radius", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-radius-decrease-less", GIMP_STOCK_BRUSH,
    "Decrease Radius Less", NULL, NULL,
    GIMP_ACTION_SELECT_SMALL_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-less", GIMP_STOCK_BRUSH,
    "Increase Radius Less", NULL, NULL,
    GIMP_ACTION_SELECT_SMALL_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease", GIMP_STOCK_BRUSH,
    "Decrease Radius", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase", GIMP_STOCK_BRUSH,
    "Increase Radius", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-skip", GIMP_STOCK_BRUSH,
    "Decrease Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-skip", GIMP_STOCK_BRUSH,
    "Increase Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-percent", GIMP_STOCK_BRUSH,
    "Decrease Radius Relative", NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-percent", GIMP_STOCK_BRUSH,
    "Increase Radius Relative", NULL, NULL,
    GIMP_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_spikes_actions[] =
{
  { "context-brush-spikes-set", GIMP_STOCK_BRUSH,
    "Set Brush Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spikes-minimum", GIMP_STOCK_BRUSH,
    "Minumum Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spikes-maximum", GIMP_STOCK_BRUSH,
    "Maximum Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spikes-decrease", GIMP_STOCK_BRUSH,
    "Decrease Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase", GIMP_STOCK_BRUSH,
    "Increase Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spikes-decrease-skip", GIMP_STOCK_BRUSH,
    "Decrease Spikes More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase-skip", GIMP_STOCK_BRUSH,
    "Increase Spikes More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_hardness_actions[] =
{
  { "context-brush-hardness-set", GIMP_STOCK_BRUSH,
    "Set Brush Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-hardness-minimum", GIMP_STOCK_BRUSH,
    "Minumum Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-hardness-maximum", GIMP_STOCK_BRUSH,
    "Maximum Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-hardness-decrease", GIMP_STOCK_BRUSH,
    "Decrease Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase", GIMP_STOCK_BRUSH,
    "Increase Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-hardness-decrease-skip", GIMP_STOCK_BRUSH,
    "Decrease Hardness More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase-skip", GIMP_STOCK_BRUSH,
    "Increase Hardness More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_aspect_actions[] =
{
  { "context-brush-aspect-set", GIMP_STOCK_BRUSH,
    "Set Brush Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-aspect-minimum", GIMP_STOCK_BRUSH,
    "Minumum Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-aspect-maximum", GIMP_STOCK_BRUSH,
    "Maximum Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-aspect-decrease", GIMP_STOCK_BRUSH,
    "Decrease Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase", GIMP_STOCK_BRUSH,
    "Increase Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-aspect-decrease-skip", GIMP_STOCK_BRUSH,
    "Decrease Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase-skip", GIMP_STOCK_BRUSH,
    "Increase Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry context_brush_angle_actions[] =
{
  { "context-brush-angle-set", GIMP_STOCK_BRUSH,
    "Set Brush Angle", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-angle-minimum", GIMP_STOCK_BRUSH,
    "Horizontal", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-angle-maximum", GIMP_STOCK_BRUSH,
    "Vertical", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-angle-decrease", GIMP_STOCK_BRUSH,
    "Rotate Right", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase", GIMP_STOCK_BRUSH,
    "Rotate Left", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-angle-decrease-skip", GIMP_STOCK_BRUSH,
    "Rotate Right 15 degrees", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase-skip", GIMP_STOCK_BRUSH,
    "Rotate Left 15 degrees", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};


void
context_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 context_actions,
                                 G_N_ELEMENTS (context_actions));

  gimp_action_group_add_enum_actions (group,
                                      context_palette_foreground_actions,
                                      G_N_ELEMENTS (context_palette_foreground_actions),
                                      G_CALLBACK (context_palette_foreground_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_palette_background_actions,
                                      G_N_ELEMENTS (context_palette_background_actions),
                                      G_CALLBACK (context_palette_background_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_colormap_foreground_actions,
                                      G_N_ELEMENTS (context_colormap_foreground_actions),
                                      G_CALLBACK (context_colormap_foreground_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_colormap_background_actions,
                                      G_N_ELEMENTS (context_colormap_background_actions),
                                      G_CALLBACK (context_colormap_background_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_swatch_foreground_actions,
                                      G_N_ELEMENTS (context_swatch_foreground_actions),
                                      G_CALLBACK (context_swatch_foreground_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_swatch_background_actions,
                                      G_N_ELEMENTS (context_swatch_background_actions),
                                      G_CALLBACK (context_swatch_background_cmd_callback));


  gimp_action_group_add_enum_actions (group,
                                      context_foreground_red_actions,
                                      G_N_ELEMENTS (context_foreground_red_actions),
                                      G_CALLBACK (context_foreground_red_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_foreground_green_actions,
                                      G_N_ELEMENTS (context_foreground_green_actions),
                                      G_CALLBACK (context_foreground_green_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_foreground_blue_actions,
                                      G_N_ELEMENTS (context_foreground_blue_actions),
                                      G_CALLBACK (context_foreground_blue_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_foreground_hue_actions,
                                      G_N_ELEMENTS (context_foreground_hue_actions),
                                      G_CALLBACK (context_foreground_hue_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_foreground_saturation_actions,
                                      G_N_ELEMENTS (context_foreground_saturation_actions),
                                      G_CALLBACK (context_foreground_saturation_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_foreground_value_actions,
                                      G_N_ELEMENTS (context_foreground_value_actions),
                                      G_CALLBACK (context_foreground_value_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_background_red_actions,
                                      G_N_ELEMENTS (context_background_red_actions),
                                      G_CALLBACK (context_background_red_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_background_green_actions,
                                      G_N_ELEMENTS (context_background_green_actions),
                                      G_CALLBACK (context_background_green_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_background_blue_actions,
                                      G_N_ELEMENTS (context_background_blue_actions),
                                      G_CALLBACK (context_background_blue_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_background_hue_actions,
                                      G_N_ELEMENTS (context_background_hue_actions),
                                      G_CALLBACK (context_background_hue_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_background_saturation_actions,
                                      G_N_ELEMENTS (context_background_saturation_actions),
                                      G_CALLBACK (context_background_saturation_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_background_value_actions,
                                      G_N_ELEMENTS (context_background_value_actions),
                                      G_CALLBACK (context_background_value_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_opacity_actions,
                                      G_N_ELEMENTS (context_opacity_actions),
                                      G_CALLBACK (context_opacity_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_paint_mode_actions,
                                      G_N_ELEMENTS (context_paint_mode_actions),
                                      G_CALLBACK (context_paint_mode_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_tool_select_actions,
                                      G_N_ELEMENTS (context_tool_select_actions),
                                      G_CALLBACK (context_tool_select_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_select_actions,
                                      G_N_ELEMENTS (context_brush_select_actions),
                                      G_CALLBACK (context_brush_select_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_pattern_select_actions,
                                      G_N_ELEMENTS (context_pattern_select_actions),
                                      G_CALLBACK (context_pattern_select_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_palette_select_actions,
                                      G_N_ELEMENTS (context_palette_select_actions),
                                      G_CALLBACK (context_palette_select_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_gradient_select_actions,
                                      G_N_ELEMENTS (context_gradient_select_actions),
                                      G_CALLBACK (context_gradient_select_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_font_select_actions,
                                      G_N_ELEMENTS (context_font_select_actions),
                                      G_CALLBACK (context_font_select_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      context_brush_shape_actions,
                                      G_N_ELEMENTS (context_brush_shape_actions),
                                      G_CALLBACK (context_brush_shape_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_radius_actions,
                                      G_N_ELEMENTS (context_brush_radius_actions),
                                      G_CALLBACK (context_brush_radius_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_spikes_actions,
                                      G_N_ELEMENTS (context_brush_spikes_actions),
                                      G_CALLBACK (context_brush_spikes_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_hardness_actions,
                                      G_N_ELEMENTS (context_brush_hardness_actions),
                                      G_CALLBACK (context_brush_hardness_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_aspect_actions,
                                      G_N_ELEMENTS (context_brush_aspect_actions),
                                      G_CALLBACK (context_brush_aspect_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      context_brush_angle_actions,
                                      G_N_ELEMENTS (context_brush_angle_actions),
                                      G_CALLBACK (context_brush_angle_cmd_callback));
}

void
context_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
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

#if 0
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
#endif

#undef SET_SENSITIVE
}
