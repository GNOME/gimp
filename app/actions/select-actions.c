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
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "select-actions.h"
#include "gui/select-commands.h"
#include "gui/tools-commands.h"
#include "gui/vectors-commands.h"

#include "gimp-intl.h"


static GimpActionEntry select_actions[] =
{
  { "select-menu", NULL,
    N_("_Select") },

  { "select-all", GIMP_STOCK_SELECTION_ALL,
    N_("_All"), "<control>A", NULL,
    G_CALLBACK (select_all_cmd_callback),
    GIMP_HELP_SELECTION_ALL },

  { "select-none", GIMP_STOCK_SELECTION_NONE,
    N_("_None"), "<control><shift>A", NULL,
    G_CALLBACK (select_none_cmd_callback),
    GIMP_HELP_SELECTION_NONE },

  { "select-invert", GIMP_STOCK_INVERT,
    N_("_Invert"), "<control>I", NULL,
    G_CALLBACK (select_invert_cmd_callback),
    GIMP_HELP_SELECTION_INVERT },

  { "select-from-vectors", GIMP_STOCK_SELECTION_REPLACE,
    N_("Fr_om Path"), "<shift>V", NULL,
    G_CALLBACK (select_from_vectors_cmd_callback),
    NULL /* FIXME */ },

  { "select-float", GIMP_STOCK_FLOATING_SELECTION,
    N_("_Float"), "<control><shift>L", NULL,
    G_CALLBACK (select_float_cmd_callback),
    GIMP_HELP_SELECTION_FLOAT },

  { "select-feather", NULL,
    N_("Fea_ther..."), NULL, NULL,
    G_CALLBACK (select_feather_cmd_callback),
    GIMP_HELP_SELECTION_FEATHER },

  { "select-sharpen", NULL,
    N_("_Sharpen"), NULL, NULL,
    G_CALLBACK (select_sharpen_cmd_callback),
    GIMP_HELP_SELECTION_SHARPEN },

  { "select-shrink", GIMP_STOCK_SELECTION_SHRINK,
    N_("S_hrink..."), NULL, NULL,
    G_CALLBACK (select_shrink_cmd_callback),
    GIMP_HELP_SELECTION_SHRINK },

  { "select-grow", GIMP_STOCK_SELECTION_GROW,
    N_("_Grow..."), NULL, NULL,
    G_CALLBACK (select_grow_cmd_callback),
    GIMP_HELP_SELECTION_GROW },

  { "select-border", GIMP_STOCK_SELECTION_BORDER,
    N_("Bo_rder..."), NULL, NULL,
    G_CALLBACK (select_border_cmd_callback),
    GIMP_HELP_SELECTION_BORDER },

  { "select-toggle-qmask", GIMP_STOCK_QMASK_ON,
    N_("Toggle _QuickMask"), "<shift>Q", NULL,
    G_CALLBACK (select_toggle_quickmask_cmd_callback),
    GIMP_HELP_QMASK_TOGGLE },

  { "select-save", GIMP_STOCK_SELECTION_TO_CHANNEL,
    N_("Save to _Channel"), NULL, NULL,
    G_CALLBACK (select_save_cmd_callback),
    GIMP_HELP_SELECTION_TO_CHANNEL },

  { "select-to-vectors", GIMP_STOCK_SELECTION_TO_PATH,
    N_("To _Path"), NULL, NULL,
    G_CALLBACK (vectors_selection_to_vectors_cmd_callback),
    GIMP_HELP_SELECTION_TO_PATH }
};

static GimpStringActionEntry select_tool_actions[] =
{
  { "select-by-color", GIMP_STOCK_TOOL_BY_COLOR_SELECT,
    N_("_By Color"), "<shift>O", NULL,
    "gimp-by-color-select-tool",
    GIMP_HELP_TOOL_BY_COLOR_SELECT }
};


void
select_actions_setup (GimpActionGroup *group,
                      gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 select_actions,
                                 G_N_ELEMENTS (select_actions),
                                 data);

  gimp_action_group_add_string_actions (group,
                                        select_tool_actions,
                                        G_N_ELEMENTS (select_tool_actions),
                                        G_CALLBACK (tools_select_cmd_callback),
                                        data);
}

void
select_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpDisplay      *gdisp   = NULL;
  GimpDisplayShell *shell   = NULL;
  GimpImage        *gimage  = NULL;
  GimpVectors      *vectors = NULL;
  gboolean          fs      = FALSE;
  gboolean          lp      = FALSE;
  gboolean          sel     = FALSE;

  if (GIMP_IS_DISPLAY_SHELL (data))
    {
      shell = GIMP_DISPLAY_SHELL (data);
      gdisp = shell->gdisp;
    }
  else if (GIMP_IS_DISPLAY (data))
    {
      gdisp = GIMP_DISPLAY (data);
      shell = GIMP_DISPLAY_SHELL (gdisp->shell);
    }

  if (gdisp)
    {
      gimage = gdisp->gimage;

      fs  = (gimp_image_floating_sel (gimage) != NULL);
      lp  = ! gimp_image_is_empty (gimage);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (gimage));

      vectors = gimp_image_get_active_vectors (gimage);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("select-all",          lp);
  SET_SENSITIVE ("select-none",         lp && sel);
  SET_SENSITIVE ("select-invert",       lp && sel);
  SET_SENSITIVE ("select-from-vectors", lp && vectors);
  SET_SENSITIVE ("select-float",        lp && sel);

  SET_SENSITIVE ("select-by-color",     lp);

  SET_SENSITIVE ("select-feather",      lp && sel);
  SET_SENSITIVE ("select-sharpen",      lp && sel);
  SET_SENSITIVE ("select-shrink",       lp && sel);
  SET_SENSITIVE ("select-grow",         lp && sel);
  SET_SENSITIVE ("select-border",       lp && sel);

  SET_SENSITIVE ("select-toggle-qmask", gdisp);

  SET_SENSITIVE ("select-save",         sel && !fs);
  SET_SENSITIVE ("select-to-vectors",   sel && !fs);

#undef SET_SENSITIVE
}
