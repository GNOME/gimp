/* The GIMP -- an image manipulation program
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

static GimpActionEntry context_actions[] =
{
  { "context-menu",          NULL,                      N_("_Context")  },
  { "context-colors-menu",   GIMP_STOCK_DEFAULT_COLORS, N_("_Colors")   },
  { "context-opacity-menu",  GIMP_STOCK_TRANSPARENCY,   N_("_Opacity")  },
  { "context-brush-menu",    GIMP_STOCK_BRUSH,          N_("_Brush")    },
  { "context-pattern-menu",  GIMP_STOCK_PATTERN,        N_("_Pattern")  },
  { "context-palette-menu",  GIMP_STOCK_PALETTE,        N_("_Palette")  },
  { "context-gradient-menu", GIMP_STOCK_GRADIENT,       N_("_Gradient") },
  { "context-font-menu",     GIMP_STOCK_FONT,           N_("_Font")     },

  { "context-brush-shape-menu",    NULL,                N_("_Shape")    },
  { "context-brush-radius-menu",   NULL,                N_("_Radius")   },
  { "context-brush-spikes-menu",   NULL,                N_("S_pikes")   },
  { "context-brush-hardness-menu", NULL,                N_("_Hardness") },
  { "context-brush-aspect-menu",   NULL,                N_("_Aspect")   },
  { "context-brush-angle-menu",    NULL,                N_("A_ngle")    },

  { "context-colors-default", GIMP_STOCK_DEFAULT_COLORS,
    N_("_Default Colors"), "D", NULL,
    G_CALLBACK (context_colors_default_cmd_callback),
    GIMP_HELP_TOOLBOX_DEFAULT_COLORS },

  { "context-colors-swap", GIMP_STOCK_SWAP_COLORS,
    N_("S_wap Colors"), "X", NULL,
    G_CALLBACK (context_colors_swap_cmd_callback),
    GIMP_HELP_TOOLBOX_SWAP_COLORS }
};

static GimpEnumActionEntry context_foreground_red_actions[] =
{
  { "context-foreground-red-set", GTK_STOCK_JUMP_TO,
    "Foreground Red Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-foreground-red-minimum", GTK_STOCK_GOTO_FIRST,
    "Foreground Red Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-foreground-red-maximum", GTK_STOCK_GOTO_LAST,
    "Foreground Red Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-foreground-red-decrease", GTK_STOCK_REMOVE,
    "Foreground Red Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-foreground-red-increase", GTK_STOCK_ADD,
    "Foreground Red Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-foreground-red-decrease-skip", GTK_STOCK_REMOVE,
    "Foreground Red Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-foreground-red-increase-skip", GTK_STOCK_ADD,
    "Foreground Red Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_foreground_green_actions[] =
{
  { "context-foreground-green-set", GTK_STOCK_JUMP_TO,
    "Foreground Green Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-foreground-green-minimum", GTK_STOCK_GOTO_FIRST,
    "Foreground Green Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-foreground-green-maximum", GTK_STOCK_GOTO_LAST,
    "Foreground Green Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-foreground-green-decrease", GTK_STOCK_REMOVE,
    "Foreground Green Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-foreground-green-increase", GTK_STOCK_ADD,
    "Foreground Green Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-foreground-green-decrease-skip", GTK_STOCK_REMOVE,
    "Foreground Green Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-foreground-green-increase-skip", GTK_STOCK_ADD,
    "Foreground Green Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_foreground_blue_actions[] =
{
  { "context-foreground-blue-set", GTK_STOCK_JUMP_TO,
    "Foreground Blue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-foreground-blue-minimum", GTK_STOCK_GOTO_FIRST,
    "Foreground Blue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-foreground-blue-maximum", GTK_STOCK_GOTO_LAST,
    "Foreground Blue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-foreground-blue-decrease", GTK_STOCK_REMOVE,
    "Foreground Blue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-foreground-blue-increase", GTK_STOCK_ADD,
    "Foreground Blue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-foreground-blue-decrease-skip", GTK_STOCK_REMOVE,
    "Foreground Blue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-foreground-blue-increase-skip", GTK_STOCK_ADD,
    "Foreground Blue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_background_red_actions[] =
{
  { "context-background-red-set", GTK_STOCK_JUMP_TO,
    "Background Red Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-background-red-minimum", GTK_STOCK_GOTO_FIRST,
    "Background Red Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-background-red-maximum", GTK_STOCK_GOTO_LAST,
    "Background Red Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-background-red-decrease", GTK_STOCK_REMOVE,
    "Background Red Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-background-red-increase", GTK_STOCK_ADD,
    "Background Red Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-background-red-decrease-skip", GTK_STOCK_REMOVE,
    "Background Red Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-background-red-increase-skip", GTK_STOCK_ADD,
    "Background Red Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_background_green_actions[] =
{
  { "context-background-green-set", GTK_STOCK_JUMP_TO,
    "Background Green Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-background-green-minimum", GTK_STOCK_GOTO_FIRST,
    "Background Green Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-background-green-maximum", GTK_STOCK_GOTO_LAST,
    "Background Green Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-background-green-decrease", GTK_STOCK_REMOVE,
    "Background Green Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-background-green-increase", GTK_STOCK_ADD,
    "Background Green Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-background-green-decrease-skip", GTK_STOCK_REMOVE,
    "Background Green Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-background-green-increase-skip", GTK_STOCK_ADD,
    "Background Green Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_background_blue_actions[] =
{
  { "context-background-blue-set", GTK_STOCK_JUMP_TO,
    "Background Blue Set", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-background-blue-minimum", GTK_STOCK_GOTO_FIRST,
    "Background Blue Minimum", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-background-blue-maximum", GTK_STOCK_GOTO_LAST,
    "Background Blue Maximum", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-background-blue-decrease", GTK_STOCK_REMOVE,
    "Background Blue Decrease", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-background-blue-increase", GTK_STOCK_ADD,
    "Background Blue Increase", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-background-blue-decrease-skip", GTK_STOCK_REMOVE,
    "Background Blue Decrease 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-background-blue-increase-skip", GTK_STOCK_ADD,
    "Background Blue Increase 10%", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_opacity_actions[] =
{
  { "context-opacity-set", GTK_STOCK_JUMP_TO,
    "Set Transparency", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-opacity-transparent", GTK_STOCK_GOTO_FIRST,
    "Completely Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-opacity-opaque", GTK_STOCK_GOTO_LAST,
    "Completely Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-opacity-decrease", GTK_STOCK_REMOVE,
    "More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-opacity-increase", GTK_STOCK_ADD,
    "More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-opacity-decrease-skip", GTK_STOCK_REMOVE,
    "10% More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-opacity-increase-skip", GTK_STOCK_ADD,
    "10% More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL }
};

static GimpEnumActionEntry context_tool_select_actions[] =
{
  { "context-tool-select-first", GTK_STOCK_GOTO_FIRST,
    "First Tool", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-tool-select-last", GTK_STOCK_GOTO_LAST,
    "Last Tool", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-tool-select-previous", GTK_STOCK_GO_BACK,
    "Previous Tool", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-tool-select-next", GTK_STOCK_GO_FORWARD,
    "Next Tool", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_brush_select_actions[] =
{
  { "context-brush-select-first", GTK_STOCK_GOTO_FIRST,
    "First Brush", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-select-last", GTK_STOCK_GOTO_LAST,
    "Last Brush", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-select-previous", GTK_STOCK_GO_BACK,
    "Previous Brush", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-select-next", GTK_STOCK_GO_FORWARD,
    "Next Brush", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_pattern_select_actions[] =
{
  { "context-pattern-select-first", GTK_STOCK_GOTO_FIRST,
    "First Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-pattern-select-last", GTK_STOCK_GOTO_LAST,
    "Last Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-pattern-select-previous", GTK_STOCK_GO_BACK,
    "Previous Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-pattern-select-next", GTK_STOCK_GO_FORWARD,
    "Next Pattern", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_palette_select_actions[] =
{
  { "context-palette-select-first", GTK_STOCK_GOTO_FIRST,
    "First Palette", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-palette-select-last", GTK_STOCK_GOTO_LAST,
    "Last Palette", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-palette-select-previous", GTK_STOCK_GO_BACK,
    "Previous Palette", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-palette-select-next", GTK_STOCK_GO_FORWARD,
    "Next Palette", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_gradient_select_actions[] =
{
  { "context-gradient-select-first", GTK_STOCK_GOTO_FIRST,
    "First Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-gradient-select-last", GTK_STOCK_GOTO_LAST,
    "Last Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-gradient-select-previous", GTK_STOCK_GO_BACK,
    "Previous Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-gradient-select-next", GTK_STOCK_GO_FORWARD,
    "Next Gradient", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_font_select_actions[] =
{
  { "context-font-select-first", GTK_STOCK_GOTO_FIRST,
    "First Font", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-font-select-last", GTK_STOCK_GOTO_LAST,
    "Last Font", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-font-select-previous", GTK_STOCK_GO_BACK,
    "Previous Font", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-font-select-next", GTK_STOCK_GO_FORWARD,
    "Next Font", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL }
};

static GimpEnumActionEntry context_brush_shape_actions[] =
{
  { "context-brush-shape-circle", GIMP_STOCK_SHAPE_CIRCLE,
    "Circle", NULL, NULL,
    GIMP_BRUSH_GENERATED_CIRCLE,
    NULL },
  { "context-brush-shape-square", GIMP_STOCK_SHAPE_SQUARE,
    "Square", NULL, NULL,
    GIMP_BRUSH_GENERATED_SQUARE,
    NULL },
  { "context-brush-shape-diamond", GIMP_STOCK_SHAPE_DIAMOND,
    "Diamond", NULL, NULL,
    GIMP_BRUSH_GENERATED_DIAMOND,
    NULL }
};

static GimpEnumActionEntry context_brush_radius_actions[] =
{
  { "context-brush-radius-set", GTK_STOCK_JUMP_TO,
    "Set Brush Radius", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-brush-radius-minimum", GTK_STOCK_GOTO_FIRST,
    "Minumum Radius", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-radius-maximum", GTK_STOCK_GOTO_LAST,
    "Maximum Radius", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-radius-decrease", GTK_STOCK_GO_BACK,
    "Decrease Radius", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-radius-increase", GTK_STOCK_GO_FORWARD,
    "Increase Radius", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-brush-radius-decrease-skip", GTK_STOCK_GO_BACK,
    "Decrease Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-brush-radius-increase-skip", GTK_STOCK_GO_FORWARD,
    "Increase Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL },
};

static GimpEnumActionEntry context_brush_spikes_actions[] =
{
  { "context-brush-spikes-set", GTK_STOCK_JUMP_TO,
    "Set Brush Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-brush-spikes-minimum", GTK_STOCK_GOTO_FIRST,
    "Minumum Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-spikes-maximum", GTK_STOCK_GOTO_LAST,
    "Maximum Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-spikes-decrease", GTK_STOCK_GO_BACK,
    "Decrease Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-spikes-increase", GTK_STOCK_GO_FORWARD,
    "Increase Spikes", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-brush-spikes-decrease-skip", GTK_STOCK_GO_BACK,
    "Decrease Spikes More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-brush-spikes-increase-skip", GTK_STOCK_GO_FORWARD,
    "Increase Spikes More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL },
};

static GimpEnumActionEntry context_brush_hardness_actions[] =
{
  { "context-brush-hardness-set", GTK_STOCK_JUMP_TO,
    "Set Brush Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-brush-hardness-minimum", GTK_STOCK_GOTO_FIRST,
    "Minumum Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-hardness-maximum", GTK_STOCK_GOTO_LAST,
    "Maximum Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-hardness-decrease", GTK_STOCK_GO_BACK,
    "Decrease Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-hardness-increase", GTK_STOCK_GO_FORWARD,
    "Increase Hardness", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-brush-hardness-decrease-skip", GTK_STOCK_GO_BACK,
    "Decrease Hardness More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-brush-hardness-increase-skip", GTK_STOCK_GO_FORWARD,
    "Increase Hardness More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL },
};

static GimpEnumActionEntry context_brush_aspect_actions[] =
{
  { "context-brush-aspect-set", GTK_STOCK_JUMP_TO,
    "Set Brush Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-brush-aspect-minimum", GTK_STOCK_GOTO_FIRST,
    "Minumum Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-aspect-maximum", GTK_STOCK_GOTO_LAST,
    "Maximum Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-aspect-decrease", GTK_STOCK_GO_BACK,
    "Decrease Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-aspect-increase", GTK_STOCK_GO_FORWARD,
    "Increase Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-brush-aspect-decrease-skip", GTK_STOCK_GO_BACK,
    "Decrease Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-brush-aspect-increase-skip", GTK_STOCK_GO_FORWARD,
    "Increase Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL },
};

static GimpEnumActionEntry context_brush_angle_actions[] =
{
  { "context-brush-angle-set", GTK_STOCK_JUMP_TO,
    "Set Brush Angle", NULL, NULL,
    GIMP_ACTION_SELECT_SET,
    NULL },
  { "context-brush-angle-minimum", GIMP_STOCK_FLIP_HORIZONTAL,
    "Horizontal", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST,
    NULL },
  { "context-brush-angle-maximum", GIMP_STOCK_FLIP_VERTICAL,
    "Vertical", NULL, NULL,
    GIMP_ACTION_SELECT_LAST,
    NULL },
  { "context-brush-angle-decrease", GIMP_STOCK_ROTATE_90,
    "Rotate Right", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS,
    NULL },
  { "context-brush-angle-increase", GIMP_STOCK_ROTATE_270,
    "Rotate Left", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT,
    NULL },
  { "context-brush-angle-decrease-skip", GIMP_STOCK_ROTATE_90,
    "Rotate Right 15 degrees", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS,
    NULL },
  { "context-brush-angle-increase-skip", GIMP_STOCK_ROTATE_270,
    "Rotate Left 15 degrees", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT,
    NULL },
};


void
context_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 context_actions,
                                 G_N_ELEMENTS (context_actions));

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
                                      context_opacity_actions,
                                      G_N_ELEMENTS (context_opacity_actions),
                                      G_CALLBACK (context_opacity_cmd_callback));

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
  GimpContext *context;
  gboolean     generated = FALSE;
  gdouble      radius    = 0.0;
  gint         spikes    = 0;
  gdouble      hardness  = 0.0;
  gdouble      aspect    = 0.0;
  gdouble      angle     = 0.0;

  context = action_data_get_context (data);

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

  SET_SENSITIVE ("brush-radius-maximum",       generated && radius < 4096.0);
  SET_SENSITIVE ("brush-radius-increase",      generated && radius < 4096.0);
  SET_SENSITIVE ("brush-radius-increase-skip", generated && radius < 4096.0);

  SET_SENSITIVE ("brush-angle-minimum",       generated);
  SET_SENSITIVE ("brush-angle-decrease",      generated);
  SET_SENSITIVE ("brush-angle-decrease-skip", generated);

  SET_SENSITIVE ("brush-angle-maximum",       generated);
  SET_SENSITIVE ("brush-angle-increase",      generated);
  SET_SENSITIVE ("brush-angle-increase-skip", generated);
#endif

#undef SET_SENSITIVE
}
