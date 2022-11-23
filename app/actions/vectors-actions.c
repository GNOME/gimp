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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "actions.h"
#include "items-actions.h"
#include "vectors-actions.h"
#include "vectors-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry vectors_actions[] =
{
  { "vectors-popup", LIGMA_ICON_DIALOG_PATHS,
    NC_("vectors-action", "Paths Menu"), NULL, NULL, NULL,
    LIGMA_HELP_PATH_DIALOG },

  { "vectors-color-tag-menu", NULL,
    NC_("vectors-action", "Color Tag"), NULL, NULL, NULL,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-edit", LIGMA_ICON_TOOL_PATH,
    NC_("vectors-action", "Edit Pa_th"), NULL,
    NC_("vectors-action", "Edit the active path"),
    vectors_edit_cmd_callback,
    LIGMA_HELP_TOOL_VECTORS },

  { "vectors-edit-attributes", LIGMA_ICON_EDIT,
    NC_("vectors-action", "_Edit Path Attributes..."), NULL,
    NC_("vectors-action", "Edit path attributes"),
    vectors_edit_attributes_cmd_callback,
    LIGMA_HELP_PATH_EDIT },

  { "vectors-new", LIGMA_ICON_DOCUMENT_NEW,
    NC_("vectors-action", "_New Path..."), NULL,
    NC_("vectors-action", "Create a new path..."),
    vectors_new_cmd_callback,
    LIGMA_HELP_PATH_NEW },

  { "vectors-new-last-values", LIGMA_ICON_DOCUMENT_NEW,
    NC_("vectors-action", "_New Path with last values"), NULL,
    NC_("vectors-action", "Create a new path with last used values"),
    vectors_new_last_vals_cmd_callback,
    LIGMA_HELP_PATH_NEW },

  { "vectors-duplicate", LIGMA_ICON_OBJECT_DUPLICATE,
    NC_("vectors-action", "D_uplicate Path"), NULL,
    NC_("vectors-action", "Duplicate this path"),
    vectors_duplicate_cmd_callback,
    LIGMA_HELP_PATH_DUPLICATE },

  { "vectors-delete", LIGMA_ICON_EDIT_DELETE,
    NC_("vectors-action", "_Delete Path"), NULL,
    NC_("vectors-action", "Delete this path"),
    vectors_delete_cmd_callback,
    LIGMA_HELP_PATH_DELETE },

  { "vectors-merge-visible", NULL,
    NC_("vectors-action", "Merge _Visible Paths"), NULL, NULL,
    vectors_merge_visible_cmd_callback,
    LIGMA_HELP_PATH_MERGE_VISIBLE },

  { "vectors-raise", LIGMA_ICON_GO_UP,
    NC_("vectors-action", "_Raise Path"), NULL,
    NC_("vectors-action", "Raise this path"),
    vectors_raise_cmd_callback,
    LIGMA_HELP_PATH_RAISE },

  { "vectors-raise-to-top", LIGMA_ICON_GO_TOP,
    NC_("vectors-action", "Raise Path to _Top"), NULL,
    NC_("vectors-action", "Raise this path to the top"),
    vectors_raise_to_top_cmd_callback,
    LIGMA_HELP_PATH_RAISE_TO_TOP },

  { "vectors-lower", LIGMA_ICON_GO_DOWN,
    NC_("vectors-action", "_Lower Path"), NULL,
    NC_("vectors-action", "Lower this path"),
    vectors_lower_cmd_callback,
    LIGMA_HELP_PATH_LOWER },

  { "vectors-lower-to-bottom", LIGMA_ICON_GO_BOTTOM,
    NC_("vectors-action", "Lower Path to _Bottom"), NULL,
    NC_("vectors-action", "Lower this path to the bottom"),
    vectors_lower_to_bottom_cmd_callback,
    LIGMA_HELP_PATH_LOWER_TO_BOTTOM },

  { "vectors-fill", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("vectors-action", "Fill Pat_h..."), NULL,
    NC_("vectors-action", "Fill the path"),
    vectors_fill_cmd_callback,
    LIGMA_HELP_PATH_FILL },

  { "vectors-fill-last-values", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("vectors-action", "Fill Path"), NULL,
    NC_("vectors-action", "Fill the path with last values"),
    vectors_fill_last_vals_cmd_callback,
    LIGMA_HELP_PATH_FILL },

  { "vectors-stroke", LIGMA_ICON_PATH_STROKE,
    NC_("vectors-action", "Stro_ke Path..."), NULL,
    NC_("vectors-action", "Paint along the path"),
    vectors_stroke_cmd_callback,
    LIGMA_HELP_PATH_STROKE },

  { "vectors-stroke-last-values", LIGMA_ICON_PATH_STROKE,
    NC_("vectors-action", "Stro_ke Path"), NULL,
    NC_("vectors-action", "Paint along the path with last values"),
    vectors_stroke_last_vals_cmd_callback,
    LIGMA_HELP_PATH_STROKE },

  { "vectors-copy", LIGMA_ICON_EDIT_COPY,
    NC_("vectors-action", "Co_py Paths"), "", NULL,
    vectors_copy_cmd_callback,
    LIGMA_HELP_PATH_COPY },

  { "vectors-paste", LIGMA_ICON_EDIT_PASTE,
    NC_("vectors-action", "Paste Pat_h"), "", NULL,
    vectors_paste_cmd_callback,
    LIGMA_HELP_PATH_PASTE },

  { "vectors-export", LIGMA_ICON_DOCUMENT_SAVE,
    NC_("vectors-action", "E_xport Paths..."), "", NULL,
    vectors_export_cmd_callback,
    LIGMA_HELP_PATH_EXPORT },

  { "vectors-import", LIGMA_ICON_DOCUMENT_OPEN,
    NC_("vectors-action", "I_mport Path..."), "", NULL,
    vectors_import_cmd_callback,
    LIGMA_HELP_PATH_IMPORT }
};

static const LigmaToggleActionEntry vectors_toggle_actions[] =
{
  { "vectors-visible", LIGMA_ICON_VISIBLE,
    NC_("vectors-action", "Toggle Path _Visibility"), NULL, NULL,
    vectors_visible_cmd_callback,
    FALSE,
    LIGMA_HELP_PATH_VISIBLE },

  { "vectors-lock-content", LIGMA_ICON_LOCK_CONTENT,
    NC_("vectors-action", "L_ock Strokes of Path"), NULL, NULL,
    vectors_lock_content_cmd_callback,
    FALSE,
    LIGMA_HELP_PATH_LOCK_STROKES },

  { "vectors-lock-position", LIGMA_ICON_LOCK_POSITION,
    NC_("vectors-action", "L_ock Position of Path"), NULL, NULL,
    vectors_lock_position_cmd_callback,
    FALSE,
    LIGMA_HELP_PATH_LOCK_POSITION }
};

static const LigmaEnumActionEntry vectors_color_tag_actions[] =
{
  { "vectors-color-tag-none", LIGMA_ICON_EDIT_CLEAR,
    NC_("vectors-action", "None"), NULL,
    NC_("vectors-action", "Path Color Tag: Clear"),
    LIGMA_COLOR_TAG_NONE, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-blue", NULL,
    NC_("vectors-action", "Blue"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Blue"),
    LIGMA_COLOR_TAG_BLUE, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-green", NULL,
    NC_("vectors-action", "Green"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Green"),
    LIGMA_COLOR_TAG_GREEN, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-yellow", NULL,
    NC_("vectors-action", "Yellow"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Yellow"),
    LIGMA_COLOR_TAG_YELLOW, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-orange", NULL,
    NC_("vectors-action", "Orange"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Orange"),
    LIGMA_COLOR_TAG_ORANGE, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-brown", NULL,
    NC_("vectors-action", "Brown"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Brown"),
    LIGMA_COLOR_TAG_BROWN, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-red", NULL,
    NC_("vectors-action", "Red"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Red"),
    LIGMA_COLOR_TAG_RED, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-violet", NULL,
    NC_("vectors-action", "Violet"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Violet"),
    LIGMA_COLOR_TAG_VIOLET, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-gray", NULL,
    NC_("vectors-action", "Gray"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Gray"),
    LIGMA_COLOR_TAG_GRAY, FALSE,
    LIGMA_HELP_PATH_COLOR_TAG }
};

static const LigmaEnumActionEntry vectors_to_selection_actions[] =
{
  { "vectors-selection-replace", LIGMA_ICON_SELECTION_REPLACE,
    NC_("vectors-action", "Path to Sele_ction"), NULL,
    NC_("vectors-action", "Path to selection"),
    LIGMA_CHANNEL_OP_REPLACE, FALSE,
    LIGMA_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-from-vectors", LIGMA_ICON_SELECTION_REPLACE,
    NC_("vectors-action", "Fr_om Path"), "<shift>V",
    NC_("vectors-action", "Replace selection with path"),
    LIGMA_CHANNEL_OP_REPLACE, FALSE,
    LIGMA_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-add", LIGMA_ICON_SELECTION_ADD,
    NC_("vectors-action", "_Add to Selection"), NULL,
    NC_("vectors-action", "Add path to selection"),
    LIGMA_CHANNEL_OP_ADD, FALSE,
    LIGMA_HELP_PATH_SELECTION_ADD },

  { "vectors-selection-subtract", LIGMA_ICON_SELECTION_SUBTRACT,
    NC_("vectors-action", "_Subtract from Selection"), NULL,
    NC_("vectors-action", "Subtract path from selection"),
    LIGMA_CHANNEL_OP_SUBTRACT, FALSE,
    LIGMA_HELP_PATH_SELECTION_SUBTRACT },

  { "vectors-selection-intersect", LIGMA_ICON_SELECTION_INTERSECT,
    NC_("vectors-action", "_Intersect with Selection"), NULL,
    NC_("vectors-action", "Intersect path with selection"),
    LIGMA_CHANNEL_OP_INTERSECT, FALSE,
    LIGMA_HELP_PATH_SELECTION_INTERSECT }
};

static const LigmaEnumActionEntry vectors_selection_to_vectors_actions[] =
{
  { "vectors-selection-to-vectors", LIGMA_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "Selecti_on to Path"), NULL,
    NC_("vectors-action", "Selection to path"),
    FALSE, FALSE,
    LIGMA_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-short", LIGMA_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "To _Path"), NULL,
    NC_("vectors-action", "Selection to path"),
    FALSE, FALSE,
    LIGMA_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-advanced", LIGMA_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "Selection to Path (_Advanced)"), NULL,
    NC_("vectors-action", "Advanced options"),
    TRUE, FALSE,
    LIGMA_HELP_SELECTION_TO_PATH }
};

static const LigmaEnumActionEntry vectors_select_actions[] =
{
  { "vectors-select-top", NULL,
    NC_("vectors-action", "Select _Top Path"), NULL,
    NC_("vectors-action", "Select the topmost path"),
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    LIGMA_HELP_PATH_TOP },

  { "vectors-select-bottom", NULL,
    NC_("vectors-action", "Select _Bottom Path"), NULL,
    NC_("vectors-action", "Select the bottommost path"),
    LIGMA_ACTION_SELECT_LAST, FALSE,
    LIGMA_HELP_PATH_BOTTOM },

  { "vectors-select-previous", NULL,
    NC_("vectors-action", "Select _Previous Path"), NULL,
    NC_("vectors-action", "Select the path above the current path"),
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    LIGMA_HELP_PATH_PREVIOUS },

  { "vectors-select-next", NULL,
    NC_("vectors-action", "Select _Next Path"), NULL,
    NC_("vectors-action", "Select the vector below the current path"),
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    LIGMA_HELP_PATH_NEXT }
};

void
vectors_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "vectors-action",
                                 vectors_actions,
                                 G_N_ELEMENTS (vectors_actions));

  ligma_action_group_add_toggle_actions (group, "vectors-action",
                                        vectors_toggle_actions,
                                        G_N_ELEMENTS (vectors_toggle_actions));

  ligma_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_color_tag_actions,
                                      G_N_ELEMENTS (vectors_color_tag_actions),
                                      vectors_color_tag_cmd_callback);

  ligma_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_to_selection_actions,
                                      G_N_ELEMENTS (vectors_to_selection_actions),
                                      vectors_to_selection_cmd_callback);

  ligma_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_selection_to_vectors_actions,
                                      G_N_ELEMENTS (vectors_selection_to_vectors_actions),
                                      vectors_selection_to_vectors_cmd_callback);

  ligma_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_select_actions,
                                      G_N_ELEMENTS (vectors_select_actions),
                                      vectors_select_cmd_callback);

  items_actions_setup (group, "vectors");
}

void
vectors_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  LigmaImage    *image              = action_data_get_image (data);
  GList        *selected_vectors   = NULL;
  gint          n_selected_vectors = 0;

  gint          n_vectors     = 0;
  gboolean      mask_empty    = TRUE;
  gboolean      dr_writable   = FALSE;
  gboolean      dr_children   = FALSE;

  gboolean      have_prev     = FALSE; /* At least 1 selected path has a previous sibling. */
  gboolean      have_next     = FALSE; /* At least 1 selected path has a next sibling.     */

  if (image)
    {
      GList *drawables;
      GList *iter;

      n_vectors  = ligma_image_get_n_vectors (image);
      mask_empty = ligma_channel_is_empty (ligma_image_get_mask (image));

      selected_vectors = ligma_image_get_selected_vectors (image);
      n_selected_vectors = g_list_length (selected_vectors);

      for (iter = selected_vectors; iter; iter = iter->next)
        {
          GList *vectors_list;
          GList *iter2;

          vectors_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
          iter2 = g_list_find (vectors_list, iter->data);

          if (iter2)
            {
              if (g_list_previous (iter2))
                have_prev = TRUE;

              if (g_list_next (iter2))
                have_next = TRUE;
            }

          if (have_prev && have_next)
            break;
        }

      drawables = ligma_image_get_selected_drawables (image);

      if (g_list_length (drawables) == 1)
        {
          dr_writable = ! ligma_item_is_content_locked (LIGMA_ITEM (drawables->data), NULL);

          if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawables->data)))
            dr_children = TRUE;
        }

      g_list_free (drawables);
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("vectors-edit",            n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-edit-attributes", n_selected_vectors == 1);

  SET_SENSITIVE ("vectors-new",             image);
  SET_SENSITIVE ("vectors-new-last-values", image);
  SET_SENSITIVE ("vectors-duplicate",       n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-delete",          n_selected_vectors > 0);
  SET_SENSITIVE ("vectors-merge-visible",   n_vectors > 1);

  SET_SENSITIVE ("vectors-raise",           n_selected_vectors > 0 && have_prev);
  SET_SENSITIVE ("vectors-raise-to-top",    n_selected_vectors > 0 && have_prev);
  SET_SENSITIVE ("vectors-lower",           n_selected_vectors > 0 && have_next);
  SET_SENSITIVE ("vectors-lower-to-bottom", n_selected_vectors > 0 && have_next);

  SET_SENSITIVE ("vectors-copy",   n_selected_vectors > 0);
  SET_SENSITIVE ("vectors-paste",  image);
  SET_SENSITIVE ("vectors-export", n_selected_vectors > 0);
  SET_SENSITIVE ("vectors-import", image);

  SET_SENSITIVE ("vectors-selection-to-vectors",          image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-short",    image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-advanced", image && !mask_empty);
  SET_SENSITIVE ("vectors-fill",                          n_selected_vectors == 1 &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-fill-last-values",              n_selected_vectors == 1 &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-stroke",                        n_selected_vectors == 1 &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-stroke-last-values",            n_selected_vectors == 1 &&
                                                          dr_writable &&
                                                          !dr_children);

  SET_SENSITIVE ("vectors-selection-replace",      n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-selection-from-vectors", n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-selection-add",          n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-selection-subtract",     n_selected_vectors == 1);
  SET_SENSITIVE ("vectors-selection-intersect",    n_selected_vectors == 1);

  SET_SENSITIVE ("vectors-select-top",       n_selected_vectors > 0 && have_prev);
  SET_SENSITIVE ("vectors-select-bottom",    n_selected_vectors > 0 && have_next);
  SET_SENSITIVE ("vectors-select-previous",  n_selected_vectors > 0 && have_prev);
  SET_SENSITIVE ("vectors-select-next",      n_selected_vectors > 0 && have_next);

#undef SET_SENSITIVE
#undef SET_ACTIVE

  items_actions_update (group, "vectors", selected_vectors);
}
