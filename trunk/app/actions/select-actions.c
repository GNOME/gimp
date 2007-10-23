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
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "select-actions.h"
#include "select-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry select_actions[] =
{
  { "selection-popup", GIMP_STOCK_TOOL_RECT_SELECT,
    N_("Selection Editor Menu"), NULL, NULL, NULL,
    GIMP_HELP_SELECTION_DIALOG },

  { "select-menu", NULL, N_("_Select") },

  { "select-all", GIMP_STOCK_SELECTION_ALL,
    N_("select|_All"), "<control>A",
    N_("Select everything"),
    G_CALLBACK (select_all_cmd_callback),
    GIMP_HELP_SELECTION_ALL },

  { "select-none", GIMP_STOCK_SELECTION_NONE,
    N_("select|_None"), "<control><shift>A",
    N_("Dismiss the selection"),
    G_CALLBACK (select_none_cmd_callback),
    GIMP_HELP_SELECTION_NONE },

  { "select-invert", GIMP_STOCK_INVERT,
    N_("_Invert"), "<control>I",
    N_("Invert the selection"),
    G_CALLBACK (select_invert_cmd_callback),
    GIMP_HELP_SELECTION_INVERT },

  { "select-float", GIMP_STOCK_FLOATING_SELECTION,
    N_("_Float"), "<control><shift>L",
    N_("Create a floating selection"),
    G_CALLBACK (select_float_cmd_callback),
    GIMP_HELP_SELECTION_FLOAT },

  { "select-feather", NULL,
    N_("Fea_ther..."), NULL,
    N_("Blur the selection border so that it fades out smoothly"),
    G_CALLBACK (select_feather_cmd_callback),
    GIMP_HELP_SELECTION_FEATHER },

  { "select-sharpen", NULL,
    N_("_Sharpen"), NULL,
    N_("Remove fuzzyness from the selection"),
    G_CALLBACK (select_sharpen_cmd_callback),
    GIMP_HELP_SELECTION_SHARPEN },

  { "select-shrink", GIMP_STOCK_SELECTION_SHRINK,
    N_("S_hrink..."), NULL,
    N_("Contract the selection"),
    G_CALLBACK (select_shrink_cmd_callback),
    GIMP_HELP_SELECTION_SHRINK },

  { "select-grow", GIMP_STOCK_SELECTION_GROW,
    N_("_Grow..."), NULL,
    N_("Enlarge the selection"),
    G_CALLBACK (select_grow_cmd_callback),
    GIMP_HELP_SELECTION_GROW },

  { "select-border", GIMP_STOCK_SELECTION_BORDER,
    N_("Bo_rder..."), NULL,
    N_("Replace the selection by its border"),
    G_CALLBACK (select_border_cmd_callback),
    GIMP_HELP_SELECTION_BORDER },

  { "select-save", GIMP_STOCK_SELECTION_TO_CHANNEL,
    N_("Save to _Channel"), NULL,
    N_("Save the selection to a channel"),
    G_CALLBACK (select_save_cmd_callback),
    GIMP_HELP_SELECTION_TO_CHANNEL },

  { "select-stroke", GIMP_STOCK_SELECTION_STROKE,
    N_("_Stroke Selection..."), NULL,
    N_("Paint along the selection outline"),
    G_CALLBACK (select_stroke_cmd_callback),
    GIMP_HELP_SELECTION_STROKE },

  { "select-stroke-last-values", GIMP_STOCK_SELECTION_STROKE,
    N_("_Stroke Selection"), NULL,
    N_("Stroke the selection with last used values"),
    G_CALLBACK (select_stroke_last_vals_cmd_callback),
    GIMP_HELP_SELECTION_STROKE }
};


void
select_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 select_actions,
                                 G_N_ELEMENTS (select_actions));
}

void
select_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpImage    *image   = action_data_get_image (data);
  GimpDrawable *drawable = NULL;
  gboolean      fs       = FALSE;
  gboolean      sel      = FALSE;

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      fs  = (gimp_image_floating_sel (image) != NULL);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (image));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("select-all",    drawable);
  SET_SENSITIVE ("select-none",   drawable && sel);
  SET_SENSITIVE ("select-invert", drawable);
  SET_SENSITIVE ("select-float",  drawable && sel);

  SET_SENSITIVE ("select-feather", drawable && sel);
  SET_SENSITIVE ("select-sharpen", drawable && sel);
  SET_SENSITIVE ("select-shrink",  drawable && sel);
  SET_SENSITIVE ("select-grow",    drawable && sel);
  SET_SENSITIVE ("select-border",  drawable && sel);

  SET_SENSITIVE ("select-save",               drawable && !fs);
  SET_SENSITIVE ("select-stroke",             drawable && sel);
  SET_SENSITIVE ("select-stroke-last-values", drawable && sel);

#undef SET_SENSITIVE
}
