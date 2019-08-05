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

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "actions.h"
#include "items-actions.h"
#include "vectors-actions.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry vectors_actions[] =
{
  { "vectors-popup", GIMP_ICON_DIALOG_PATHS,
    NC_("vectors-action", "Paths Menu"), NULL, NULL, NULL,
    GIMP_HELP_PATH_DIALOG },

  { "vectors-color-tag-menu", NULL,
    NC_("vectors-action", "Color Tag"), NULL, NULL, NULL,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-edit", GIMP_ICON_TOOL_PATH,
    NC_("vectors-action", "Edit Pa_th"), NULL,
    NC_("vectors-action", "Edit the active path"),
    vectors_edit_cmd_callback,
    GIMP_HELP_TOOL_VECTORS },

  { "vectors-edit-attributes", GIMP_ICON_EDIT,
    NC_("vectors-action", "_Edit Path Attributes..."), NULL,
    NC_("vectors-action", "Edit path attributes"),
    vectors_edit_attributes_cmd_callback,
    GIMP_HELP_PATH_EDIT },

  { "vectors-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("vectors-action", "_New Path..."), NULL,
    NC_("vectors-action", "Create a new path..."),
    vectors_new_cmd_callback,
    GIMP_HELP_PATH_NEW },

  { "vectors-new-last-values", GIMP_ICON_DOCUMENT_NEW,
    NC_("vectors-action", "_New Path with last values"), NULL,
    NC_("vectors-action", "Create a new path with last used values"),
    vectors_new_last_vals_cmd_callback,
    GIMP_HELP_PATH_NEW },

  { "vectors-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("vectors-action", "D_uplicate Path"), NULL,
    NC_("vectors-action", "Duplicate this path"),
    vectors_duplicate_cmd_callback,
    GIMP_HELP_PATH_DUPLICATE },

  { "vectors-delete", GIMP_ICON_EDIT_DELETE,
    NC_("vectors-action", "_Delete Path"), NULL,
    NC_("vectors-action", "Delete this path"),
    vectors_delete_cmd_callback,
    GIMP_HELP_PATH_DELETE },

  { "vectors-merge-visible", NULL,
    NC_("vectors-action", "Merge _Visible Paths"), NULL, NULL,
    vectors_merge_visible_cmd_callback,
    GIMP_HELP_PATH_MERGE_VISIBLE },

  { "vectors-raise", GIMP_ICON_GO_UP,
    NC_("vectors-action", "_Raise Path"), NULL,
    NC_("vectors-action", "Raise this path"),
    vectors_raise_cmd_callback,
    GIMP_HELP_PATH_RAISE },

  { "vectors-raise-to-top", GIMP_ICON_GO_TOP,
    NC_("vectors-action", "Raise Path to _Top"), NULL,
    NC_("vectors-action", "Raise this path to the top"),
    vectors_raise_to_top_cmd_callback,
    GIMP_HELP_PATH_RAISE_TO_TOP },

  { "vectors-lower", GIMP_ICON_GO_DOWN,
    NC_("vectors-action", "_Lower Path"), NULL,
    NC_("vectors-action", "Lower this path"),
    vectors_lower_cmd_callback,
    GIMP_HELP_PATH_LOWER },

  { "vectors-lower-to-bottom", GIMP_ICON_GO_BOTTOM,
    NC_("vectors-action", "Lower Path to _Bottom"), NULL,
    NC_("vectors-action", "Lower this path to the bottom"),
    vectors_lower_to_bottom_cmd_callback,
    GIMP_HELP_PATH_LOWER_TO_BOTTOM },

  { "vectors-fill", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("vectors-action", "Fill Pat_h..."), NULL,
    NC_("vectors-action", "Fill the path"),
    vectors_fill_cmd_callback,
    GIMP_HELP_PATH_FILL },

  { "vectors-fill-last-values", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("vectors-action", "Fill Path"), NULL,
    NC_("vectors-action", "Fill the path with last values"),
    vectors_fill_last_vals_cmd_callback,
    GIMP_HELP_PATH_FILL },

  { "vectors-stroke", GIMP_ICON_PATH_STROKE,
    NC_("vectors-action", "Stro_ke Path..."), NULL,
    NC_("vectors-action", "Paint along the path"),
    vectors_stroke_cmd_callback,
    GIMP_HELP_PATH_STROKE },

  { "vectors-stroke-last-values", GIMP_ICON_PATH_STROKE,
    NC_("vectors-action", "Stro_ke Path"), NULL,
    NC_("vectors-action", "Paint along the path with last values"),
    vectors_stroke_last_vals_cmd_callback,
    GIMP_HELP_PATH_STROKE },

  { "vectors-copy", GIMP_ICON_EDIT_COPY,
    NC_("vectors-action", "Co_py Path"), "", NULL,
    vectors_copy_cmd_callback,
    GIMP_HELP_PATH_COPY },

  { "vectors-paste", GIMP_ICON_EDIT_PASTE,
    NC_("vectors-action", "Paste Pat_h"), "", NULL,
    vectors_paste_cmd_callback,
    GIMP_HELP_PATH_PASTE },

  { "vectors-export", GIMP_ICON_DOCUMENT_SAVE,
    NC_("vectors-action", "E_xport Path..."), "", NULL,
    vectors_export_cmd_callback,
    GIMP_HELP_PATH_EXPORT },

  { "vectors-import", GIMP_ICON_DOCUMENT_OPEN,
    NC_("vectors-action", "I_mport Path..."), "", NULL,
    vectors_import_cmd_callback,
    GIMP_HELP_PATH_IMPORT }
};

static const GimpToggleActionEntry vectors_toggle_actions[] =
{
  { "vectors-visible", GIMP_ICON_VISIBLE,
    NC_("vectors-action", "Toggle Path _Visibility"), NULL, NULL,
    vectors_visible_cmd_callback,
    FALSE,
    GIMP_HELP_PATH_VISIBLE },

  { "vectors-linked", GIMP_ICON_LINKED,
    NC_("vectors-action", "Toggle Path _Linked State"), NULL, NULL,
    vectors_linked_cmd_callback,
    FALSE,
    GIMP_HELP_PATH_LINKED },

  { "vectors-lock-content", NULL /* GIMP_ICON_LOCK */,
    NC_("vectors-action", "L_ock Strokes of Path"), NULL, NULL,
    vectors_lock_content_cmd_callback,
    FALSE,
    GIMP_HELP_PATH_LOCK_STROKES },

  { "vectors-lock-position", GIMP_ICON_TOOL_MOVE,
    NC_("vectors-action", "L_ock Position of Path"), NULL, NULL,
    vectors_lock_position_cmd_callback,
    FALSE,
    GIMP_HELP_PATH_LOCK_POSITION }
};

static const GimpEnumActionEntry vectors_color_tag_actions[] =
{
  { "vectors-color-tag-none", GIMP_ICON_EDIT_CLEAR,
    NC_("vectors-action", "None"), NULL,
    NC_("vectors-action", "Path Color Tag: Clear"),
    GIMP_COLOR_TAG_NONE, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-blue", NULL,
    NC_("vectors-action", "Blue"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Blue"),
    GIMP_COLOR_TAG_BLUE, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-green", NULL,
    NC_("vectors-action", "Green"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Green"),
    GIMP_COLOR_TAG_GREEN, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-yellow", NULL,
    NC_("vectors-action", "Yellow"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Yellow"),
    GIMP_COLOR_TAG_YELLOW, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-orange", NULL,
    NC_("vectors-action", "Orange"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Orange"),
    GIMP_COLOR_TAG_ORANGE, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-brown", NULL,
    NC_("vectors-action", "Brown"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Brown"),
    GIMP_COLOR_TAG_BROWN, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-red", NULL,
    NC_("vectors-action", "Red"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Red"),
    GIMP_COLOR_TAG_RED, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-violet", NULL,
    NC_("vectors-action", "Violet"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Violet"),
    GIMP_COLOR_TAG_VIOLET, FALSE,
    GIMP_HELP_PATH_COLOR_TAG },

  { "vectors-color-tag-gray", NULL,
    NC_("vectors-action", "Gray"), NULL,
    NC_("vectors-action", "Path Color Tag: Set to Gray"),
    GIMP_COLOR_TAG_GRAY, FALSE,
    GIMP_HELP_PATH_COLOR_TAG }
};

static const GimpEnumActionEntry vectors_to_selection_actions[] =
{
  { "vectors-selection-replace", GIMP_ICON_SELECTION_REPLACE,
    NC_("vectors-action", "Path to Sele_ction"), NULL,
    NC_("vectors-action", "Path to selection"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-from-vectors", GIMP_ICON_SELECTION_REPLACE,
    NC_("vectors-action", "Fr_om Path"), "<shift>V",
    NC_("vectors-action", "Replace selection with path"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_PATH_SELECTION_REPLACE },

  { "vectors-selection-add", GIMP_ICON_SELECTION_ADD,
    NC_("vectors-action", "_Add to Selection"), NULL,
    NC_("vectors-action", "Add path to selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_PATH_SELECTION_ADD },

  { "vectors-selection-subtract", GIMP_ICON_SELECTION_SUBTRACT,
    NC_("vectors-action", "_Subtract from Selection"), NULL,
    NC_("vectors-action", "Subtract path from selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_PATH_SELECTION_SUBTRACT },

  { "vectors-selection-intersect", GIMP_ICON_SELECTION_INTERSECT,
    NC_("vectors-action", "_Intersect with Selection"), NULL,
    NC_("vectors-action", "Intersect path with selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_PATH_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry vectors_selection_to_vectors_actions[] =
{
  { "vectors-selection-to-vectors", GIMP_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "Selecti_on to Path"), NULL,
    NC_("vectors-action", "Selection to path"),
    FALSE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-short", GIMP_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "To _Path"), NULL,
    NC_("vectors-action", "Selection to path"),
    FALSE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH },

  { "vectors-selection-to-vectors-advanced", GIMP_ICON_SELECTION_TO_PATH,
    NC_("vectors-action", "Selection to Path (_Advanced)"), NULL,
    NC_("vectors-action", "Advanced options"),
    TRUE, FALSE,
    GIMP_HELP_SELECTION_TO_PATH }
};

static const GimpEnumActionEntry vectors_select_actions[] =
{
  { "vectors-select-top", NULL,
    NC_("vectors-action", "Select _Top Path"), NULL,
    NC_("vectors-action", "Select the topmost path"),
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_PATH_TOP },

  { "vectors-select-bottom", NULL,
    NC_("vectors-action", "Select _Bottom Path"), NULL,
    NC_("vectors-action", "Select the bottommost path"),
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_PATH_BOTTOM },

  { "vectors-select-previous", NULL,
    NC_("vectors-action", "Select _Previous Path"), NULL,
    NC_("vectors-action", "Select the path above the current path"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_PATH_PREVIOUS },

  { "vectors-select-next", NULL,
    NC_("vectors-action", "Select _Next Path"), NULL,
    NC_("vectors-action", "Select the vector below the current path"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_PATH_NEXT }
};

void
vectors_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "vectors-action",
                                 vectors_actions,
                                 G_N_ELEMENTS (vectors_actions));

  gimp_action_group_add_toggle_actions (group, "vectors-action",
                                        vectors_toggle_actions,
                                        G_N_ELEMENTS (vectors_toggle_actions));

  gimp_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_color_tag_actions,
                                      G_N_ELEMENTS (vectors_color_tag_actions),
                                      vectors_color_tag_cmd_callback);

  gimp_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_to_selection_actions,
                                      G_N_ELEMENTS (vectors_to_selection_actions),
                                      vectors_to_selection_cmd_callback);

  gimp_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_selection_to_vectors_actions,
                                      G_N_ELEMENTS (vectors_selection_to_vectors_actions),
                                      vectors_selection_to_vectors_cmd_callback);

  gimp_action_group_add_enum_actions (group, "vectors-action",
                                      vectors_select_actions,
                                      G_N_ELEMENTS (vectors_select_actions),
                                      vectors_select_cmd_callback);

  items_actions_setup (group, "vectors");
}

void
vectors_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage    *image         = action_data_get_image (data);
  GimpVectors  *vectors       = NULL;
  GimpDrawable *drawable      = NULL;
  gint          n_vectors     = 0;
  gboolean      mask_empty    = TRUE;
  gboolean      dr_writable   = FALSE;
  gboolean      dr_children   = FALSE;
  GList        *next          = NULL;
  GList        *prev          = NULL;

  if (image)
    {
      n_vectors  = gimp_image_get_n_vectors (image);
      mask_empty = gimp_channel_is_empty (gimp_image_get_mask (image));

      vectors = gimp_image_get_active_vectors (image);

      if (vectors)
        {
          GList *vectors_list;
          GList *list;

          vectors_list = gimp_item_get_container_iter (GIMP_ITEM (vectors));

          list = g_list_find (vectors_list, vectors);

          if (list)
            {
              prev = g_list_previous (list);
              next = g_list_next (list);
            }
        }

      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          dr_writable = ! gimp_item_is_content_locked (GIMP_ITEM (drawable));

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            dr_children = TRUE;
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("vectors-edit",            vectors);
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

  SET_SENSITIVE ("vectors-selection-to-vectors",          image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-short",    image && !mask_empty);
  SET_SENSITIVE ("vectors-selection-to-vectors-advanced", image && !mask_empty);
  SET_SENSITIVE ("vectors-fill",                          vectors &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-fill-last-values",              vectors &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-stroke",                        vectors &&
                                                          dr_writable &&
                                                          !dr_children);
  SET_SENSITIVE ("vectors-stroke-last-values",            vectors &&
                                                          dr_writable &&
                                                          !dr_children);

  SET_SENSITIVE ("vectors-selection-replace",      vectors);
  SET_SENSITIVE ("vectors-selection-from-vectors", vectors);
  SET_SENSITIVE ("vectors-selection-add",          vectors);
  SET_SENSITIVE ("vectors-selection-subtract",     vectors);
  SET_SENSITIVE ("vectors-selection-intersect",    vectors);

  SET_SENSITIVE ("vectors-select-top",       vectors && prev);
  SET_SENSITIVE ("vectors-select-bottom",    vectors && next);
  SET_SENSITIVE ("vectors-select-previous",  vectors && prev);
  SET_SENSITIVE ("vectors-select-next",      vectors && next);

#undef SET_SENSITIVE
#undef SET_ACTIVE

  items_actions_update (group, "vectors", GIMP_ITEM (vectors));
}
