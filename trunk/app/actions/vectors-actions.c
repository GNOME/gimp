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

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "vectors-actions.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry vectors_actions[] =
{
  { "vectors-popup", GIMP_STOCK_PATHS,
    N_("Paths Menu"), NULL, NULL, NULL,
    GIMP_HELP_PATH_DIALOG },

  { "vectors-path-tool", GIMP_STOCK_TOOL_PATH,
    N_("Path _Tool"), NULL, NULL,
    G_CALLBACK (vectors_vectors_tool_cmd_callback),
    GIMP_HELP_TOOL_VECTORS },

  { "vectors-edit-attributes", GTK_STOCK_EDIT,
    N_("_Edit Path Attributes..."), NULL,
    N_("Edit path attributes"),
    G_CALLBACK (vectors_edit_attributes_cmd_callback),
    GIMP_HELP_PATH_EDIT },

  { "vectors-new", GTK_STOCK_NEW,
    N_("_New Path..."), "",
    N_("New path..."),
    G_CALLBACK (vectors_new_cmd_callback),
    GIMP_HELP_PATH_NEW },

  { "vectors-new-last-values", GTK_STOCK_NEW,
    N_("_New Path"), "",
    N_("New path with last values"),
    G_CALLBACK (vectors_new_last_vals_cmd_callback),
    GIMP_HELP_PATH_NEW },

  { "vectors-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Path"), NULL,
    N_("Duplicate path"),
    G_CALLBACK (vectors_duplicate_cmd_callback),
    GIMP_HELP_PATH_DUPLICATE },

  { "vectors-delete", GTK_STOCK_DELETE,
    N_("_Delete Path"), "",
    N_("Delete path"),
    G_CALLBACK (vectors_delete_cmd_callback),
    GIMP_HELP_PATH_DELETE },

  { "vectors-merge-visible", NULL,
    N_("Merge _Visible Paths"), NULL, NULL,
    G_CALLBACK (vectors_merge_visible_cmd_callback),
    GIMP_HELP_PATH_MERGE_VISIBLE },

  { "vectors-raise", GTK_STOCK_GO_UP,
    N_("_Raise Path"), "",
    N_("Raise path"),
    G_CALLBACK (vectors_raise_cmd_callback),
    GIMP_HELP_PATH_RAISE },

  { "vectors-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Raise Path to _Top"), "",
    N_("Raise path to top"),
    G_CALLBACK (vectors_raise_to_top_cmd_callback),
    GIMP_HELP_PATH_RAISE_TO_TOP },

  { "vectors-lower", GTK_STOCK_GO_DOWN,
    N_("_Lower Path"), "",
    N_("Lower path"),
    G_CALLBACK (vectors_lower_cmd_callback),
    GIMP_HELP_PATH_LOWER },

  { "vectors-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Lower Path to _Bottom"), "",
    N_("Lower path to bottom"),
    G_CALLBACK (vectors_lower_to_bottom_cmd_callback),
    GIMP_HELP_PATH_LOWER_TO_BOTTOM },

  { "vectors-stroke", GIMP_STOCK_PATH_STROKE,
    N_("Stro_ke Path..."), NULL,
    N_("Paint along the path"),
    G_CALLBACK (vectors_stroke_cmd_callback),
    GIMP_HELP_PATH_STROKE },

  { "vectors-stroke-last-values", GIMP_STOCK_PATH_STROKE,
    N_("Stro_ke Path"), NULL,
    N_("Paint along the path with last values"),
    G_CALLBACK (vectors_stroke_last_vals_cmd_callback),
    GIMP_HELP_PATH_STROKE },

  { "vectors-copy", GTK_STOCK_COPY,
    N_("Co_py Path"), "", NULL,
    G_CALLBACK (vectors_copy_cmd_callback),
    GIMP_HELP_PATH_COPY },

  { "vectors-paste", GTK_STOCK_PASTE,
    N_("Paste Pat_h"), "", NULL,
    G_CALLBACK (vectors_paste_cmd_callback),
    GIMP_HELP_PATH_PASTE },

  { "vectors-export", GTK_STOCK_SAVE,
    N_("E_xport Path..."), "", NULL,
    G_CALLBACK (vectors_export_cmd_callback),
    GIMP_HELP_PATH_EXPORT },

  { "vectors-import", GTK_STOCK_OPEN,
    N_("I_mport Path..."), "", NULL,
    G_CALLBACK (vectors_import_cmd_callback),
    GIMP_HELP_PATH_IMPORT }
};

static const GimpToggleActionEntry vectors_toggle_actions[] =
{
  { "vectors-visible", GIMP_STOCK_VISIBLE,
    N_("_Visible"), NULL, NULL,
    G_CALLBACK (vectors_visible_cmd_callback),
    FALSE,
    GIMP_HELP_PATH_VISIBLE },

  { "vectors-linked", GIMP_STOCK_LINKED,
    N_("_Linked"), NULL, NULL,
    G_CALLBACK (vectors_linked_cmd_callback),
    FALSE,
    GIMP_HELP_PATH_LINKED }
};

static const GimpEnumActionEntry vectors_to_selection_actions[] =
{
  { "vectors-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("Path to Sele_ction"), NULL,
    N_("Path to selection"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-from-vectors", GIMP_STOCK_SELECTION_REPLACE,
    N_("Fr_om Path"), "<shift>V", NULL,
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("_Add to Selection"), NULL,
    N_("Add"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_PATH_SELECTION_ADD },

  { "vectors-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL,
    N_("Subtract"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_PATH_SELECTION_SUBTRACT },

  { "vectors-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL,
    N_("Intersect"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_PATH_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry vectors_selection_to_vectors_actions[] =
{
  { "vectors-selection-to-vectors", GIMP_STOCK_SELECTION_TO_PATH,
    N_("Selecti_on to Path"), NULL,
    N_("Selection to path"),
    FALSE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-short", GIMP_STOCK_SELECTION_TO_PATH,
    N_("To _Path"), NULL,
    N_("Selection to path"),
    FALSE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-advanced", GIMP_STOCK_SELECTION_TO_PATH,
    N_("Selection to Path (_Advanced)"), NULL,
    N_("Advanced options"),
    TRUE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH }
};


void
vectors_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 vectors_actions,
                                 G_N_ELEMENTS (vectors_actions));

  gimp_action_group_add_toggle_actions (group,
                                        vectors_toggle_actions,
                                        G_N_ELEMENTS (vectors_toggle_actions));

  gimp_action_group_add_enum_actions (group,
                                      vectors_to_selection_actions,
                                      G_N_ELEMENTS (vectors_to_selection_actions),
                                      G_CALLBACK (vectors_to_selection_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      vectors_selection_to_vectors_actions,
                                      G_N_ELEMENTS (vectors_selection_to_vectors_actions),
                                      G_CALLBACK (vectors_selection_to_vectors_cmd_callback));
}

void
vectors_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage   *image     = action_data_get_image (data);
  GimpVectors *vectors    = NULL;
  gint         n_vectors  = 0;
  gboolean     mask_empty = TRUE;
  gboolean     global_buf = FALSE;
  gboolean     visible    = FALSE;
  gboolean     linked     = FALSE;
  GList       *next       = NULL;
  GList       *prev       = NULL;

  if (image)
    {
      n_vectors  = gimp_container_num_children (image->vectors);
      mask_empty = gimp_channel_is_empty (gimp_image_get_mask (image));
      global_buf = FALSE;

      vectors = gimp_image_get_active_vectors (image);

      if (vectors)
        {
          GimpItem *item = GIMP_ITEM (vectors);
          GList    *list;

          visible = gimp_item_get_visible (item);
          linked  = gimp_item_get_linked  (item);

          list = g_list_find (GIMP_LIST (image->vectors)->list, vectors);

          if (list)
            {
              prev = g_list_previous (list);
              next = g_list_next (list);
            }
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("vectors-path-tool",       vectors);
  SET_SENSITIVE ("vectors-edit-attributes", vectors);

  SET_SENSITIVE ("vectors-new",             image);
  SET_SENSITIVE ("vectors-new-last-values", image);
  SET_SENSITIVE ("vectors-duplicate",       vectors);
  SET_SENSITIVE ("vectors-delete",          vectors);
  SET_SENSITIVE ("vectors-merge-visible",   n_vectors > 1);

  SET_SENSITIVE ("vectors-raise",           vectors && prev);
  SET_SENSITIVE ("vectors-raise-to-top",    vectors && prev);
  SET_SENSITIVE ("vectors-lower",           vectors && next);
  SET_SENSITIVE ("vectors-lower-to-bottom", vectors && next);

  SET_SENSITIVE ("vectors-copy",   vectors);
  SET_SENSITIVE ("vectors-paste",  image);
  SET_SENSITIVE ("vectors-export", vectors);
  SET_SENSITIVE ("vectors-import", image);

  SET_SENSITIVE ("vectors-visible", vectors);
  SET_SENSITIVE ("vectors-linked",  vectors);

  SET_ACTIVE ("vectors-visible", visible);
  SET_ACTIVE ("vectors-linked",  linked);

  SET_SENSITIVE ("vectors-selection-to-vectors",          image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-short",    image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-advanced", image && !mask_empty);
  SET_SENSITIVE ("vectors-stroke",                        vectors);
  SET_SENSITIVE ("vectors-stroke-last-values",            vectors);

  SET_SENSITIVE ("vectors-selection-replace",      vectors);
  SET_SENSITIVE ("vectors-selection-from-vectors", vectors);
  SET_SENSITIVE ("vectors-selection-add",          vectors);
  SET_SENSITIVE ("vectors-selection-subtract",     vectors);
  SET_SENSITIVE ("vectors-selection-intersect",    vectors);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
