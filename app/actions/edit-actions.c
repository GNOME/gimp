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

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalist.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaundostack.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "tools/tool_manager.h"

#include "actions.h"
#include "edit-actions.h"
#include "edit-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   edit_actions_foreground_changed (LigmaContext     *context,
                                               const LigmaRGB   *color,
                                               LigmaActionGroup *group);
static void   edit_actions_background_changed (LigmaContext     *context,
                                               const LigmaRGB   *color,
                                               LigmaActionGroup *group);
static void   edit_actions_pattern_changed    (LigmaContext     *context,
                                               LigmaPattern     *pattern,
                                               LigmaActionGroup *group);


static const LigmaActionEntry edit_actions[] =
{
  { "edit-menu",          NULL, NC_("edit-action", "_Edit")     },
  { "edit-paste-as-menu", NULL, NC_("edit-action", "Paste _as") },
  { "edit-buffer-menu",   NULL, NC_("edit-action", "_Buffer")   },

  { "undo-popup",
    "edit-undo", NC_("edit-action", "Undo History Menu"), NULL, NULL, NULL,
    LIGMA_HELP_UNDO_DIALOG },

  { "edit-undo", LIGMA_ICON_EDIT_UNDO,
    NC_("edit-action", "_Undo"), "<primary>Z",
    NC_("edit-action", "Undo the last operation"),
    edit_undo_cmd_callback,
    LIGMA_HELP_EDIT_UNDO },

  { "edit-redo", LIGMA_ICON_EDIT_REDO,
    NC_("edit-action", "_Redo"), "<primary>Y",
    NC_("edit-action", "Redo the last operation that was undone"),
    edit_redo_cmd_callback,
    LIGMA_HELP_EDIT_REDO },

  { "edit-strong-undo", LIGMA_ICON_EDIT_UNDO,
    NC_("edit-action", "Strong Undo"), "<primary><shift>Z",
    NC_("edit-action", "Undo the last operation, skipping visibility changes"),
    edit_strong_undo_cmd_callback,
    LIGMA_HELP_EDIT_STRONG_UNDO },

  { "edit-strong-redo", LIGMA_ICON_EDIT_REDO,
    NC_("edit-action", "Strong Redo"), "<primary><shift>Y",
    NC_("edit-action",
        "Redo the last operation that was undone, skipping visibility changes"),
    edit_strong_redo_cmd_callback,
    LIGMA_HELP_EDIT_STRONG_REDO },

  { "edit-undo-clear", LIGMA_ICON_SHRED,
    NC_("edit-action", "_Clear Undo History"), NULL,
    NC_("edit-action", "Remove all operations from the undo history"),
    edit_undo_clear_cmd_callback,
    LIGMA_HELP_EDIT_UNDO_CLEAR },

  { "edit-cut", LIGMA_ICON_EDIT_CUT,
    NC_("edit-action", "Cu_t"), "<primary>X",
    NC_("edit-action", "Move the selected pixels to the clipboard"),
    edit_cut_cmd_callback,
    LIGMA_HELP_EDIT_CUT },

  { "edit-copy", LIGMA_ICON_EDIT_COPY,
    NC_("edit-action", "_Copy"), "<primary>C",
    NC_("edit-action", "Copy the selected pixels to the clipboard"),
    edit_copy_cmd_callback,
    LIGMA_HELP_EDIT_COPY },

  { "edit-copy-visible", NULL, /* LIGMA_ICON_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible"), "<primary><shift>C",
    NC_("edit-action", "Copy what is visible in the selected region"),
    edit_copy_visible_cmd_callback,
    LIGMA_HELP_EDIT_COPY_VISIBLE },

  { "edit-paste-as-new-image", LIGMA_ICON_EDIT_PASTE_AS_NEW,
    NC_("edit-action", "From _Clipboard"), "<primary><shift>V",
    NC_("edit-action", "Create a new image from the content of the clipboard"),
    edit_paste_as_new_image_cmd_callback,
    LIGMA_HELP_EDIT_PASTE_AS_NEW_IMAGE },

  { "edit-paste-as-new-image-short", LIGMA_ICON_EDIT_PASTE_AS_NEW,
    NC_("edit-action", "Paste as _New Image"), NULL,
    NC_("edit-action", "Create a new image from the content of the clipboard"),
    edit_paste_as_new_image_cmd_callback,
    LIGMA_HELP_EDIT_PASTE_AS_NEW_IMAGE },

  { "edit-named-cut", LIGMA_ICON_EDIT_CUT,
    NC_("edit-action", "Cu_t Named..."), NULL,
    NC_("edit-action", "Move the selected pixels to a named buffer"),
    edit_named_cut_cmd_callback,
    LIGMA_HELP_BUFFER_CUT },

  { "edit-named-copy", LIGMA_ICON_EDIT_COPY,
    NC_("edit-action", "_Copy Named..."), NULL,
    NC_("edit-action", "Copy the selected pixels to a named buffer"),
    edit_named_copy_cmd_callback,
    LIGMA_HELP_BUFFER_COPY },

  { "edit-named-copy-visible", NULL, /* LIGMA_ICON_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible Named..."), "",
    NC_("edit-action",
        "Copy what is visible in the selected region to a named buffer"),
    edit_named_copy_visible_cmd_callback,
    LIGMA_HELP_BUFFER_COPY },

  { "edit-named-paste", LIGMA_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste Named..."), NULL,
    NC_("edit-action", "Paste the content of a named buffer"),
    edit_named_paste_cmd_callback,
    LIGMA_HELP_BUFFER_PASTE },

  { "edit-clear", LIGMA_ICON_EDIT_CLEAR,
    NC_("edit-action", "Cl_ear"), "Delete",
    NC_("edit-action", "Clear the selected pixels"),
    edit_clear_cmd_callback,
    LIGMA_HELP_EDIT_CLEAR }
};

static const LigmaEnumActionEntry edit_paste_actions[] =
{
  { "edit-paste", LIGMA_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste"), "<primary>V",
    NC_("edit-action", "Paste the content of the clipboard"),
    LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING, FALSE,
    LIGMA_HELP_EDIT_PASTE },

  { "edit-paste-in-place", LIGMA_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste In P_lace"), "<primary><alt>V",
    NC_("edit-action",
        "Paste the content of the clipboard at its original position"),
    LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, FALSE,
    LIGMA_HELP_EDIT_PASTE_IN_PLACE },

  { "edit-paste-merged", LIGMA_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste as Single Layer"), NULL,
    NC_("edit-action", "Paste the content of the clipboard as a single layer"),
    LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, FALSE,
    LIGMA_HELP_EDIT_PASTE },

  { "edit-paste-merged-in-place", LIGMA_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste as Single Layer In P_lace"), NULL,
    NC_("edit-action",
        "Paste the content of the clipboard at its original position as a single layer"),
    LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, FALSE,
    LIGMA_HELP_EDIT_PASTE_IN_PLACE },

  { "edit-paste-into", LIGMA_ICON_EDIT_PASTE_INTO,
    NC_("edit-action", "Paste _Into Selection"), NULL,
    NC_("edit-action",
        "Paste the content of the clipboard into the current selection"),
    LIGMA_PASTE_TYPE_FLOATING_INTO, FALSE,
    LIGMA_HELP_EDIT_PASTE_INTO },

  { "edit-paste-into-in-place", LIGMA_ICON_EDIT_PASTE_INTO,
    NC_("edit-action", "Paste Int_o Selection In Place"), NULL,
    NC_("edit-action",
        "Paste the content of the clipboard into the current selection "
        "at its original position"),
    LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE, FALSE,
    LIGMA_HELP_EDIT_PASTE_INTO_IN_PLACE }
};

static const LigmaEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with _FG Color"), "<primary>comma",
    NC_("edit-action", "Fill the selection using the foreground color"),
    LIGMA_FILL_FOREGROUND, FALSE,
    LIGMA_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with B_G Color"), "<primary>period",
    NC_("edit-action", "Fill the selection using the background color"),
    LIGMA_FILL_BACKGROUND, FALSE,
    LIGMA_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", LIGMA_ICON_PATTERN,
    NC_("edit-action", "Fill _with Pattern"), "<primary>semicolon",
    NC_("edit-action", "Fill the selection using the active pattern"),
    LIGMA_FILL_PATTERN, FALSE,
    LIGMA_HELP_EDIT_FILL_PATTERN }
};


void
edit_actions_setup (LigmaActionGroup *group)
{
  LigmaContext *context = ligma_get_user_context (group->ligma);
  LigmaRGB      color;
  LigmaPattern *pattern;
  LigmaAction  *action;

  ligma_action_group_add_actions (group, "edit-action",
                                 edit_actions,
                                 G_N_ELEMENTS (edit_actions));

  ligma_action_group_add_enum_actions (group, "edit-action",
                                      edit_paste_actions,
                                      G_N_ELEMENTS (edit_paste_actions),
                                      edit_paste_cmd_callback);

  ligma_action_group_add_enum_actions (group, "edit-action",
                                      edit_fill_actions,
                                      G_N_ELEMENTS (edit_fill_actions),
                                      edit_fill_cmd_callback);

  action = ligma_action_group_get_action (group,
                                         "edit-paste-as-new-image-short");
  ligma_action_set_accel_path (action,
                              "<Actions>/edit/edit-paste-as-new-image");

  ligma_action_group_set_action_context (group, "edit-fill-fg", context);
  ligma_action_group_set_action_context (group, "edit-fill-bg", context);
  ligma_action_group_set_action_context (group, "edit-fill-pattern", context);

  g_signal_connect_object (context, "foreground-changed",
                           G_CALLBACK (edit_actions_foreground_changed),
                           group, 0);
  g_signal_connect_object (context, "background-changed",
                           G_CALLBACK (edit_actions_background_changed),
                           group, 0);
  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (edit_actions_pattern_changed),
                           group, 0);

  ligma_context_get_foreground (context, &color);
  edit_actions_foreground_changed (context, &color, group);

  ligma_context_get_background (context, &color);
  edit_actions_background_changed (context, &color, group);

  pattern = ligma_context_get_pattern (context);
  edit_actions_pattern_changed (context, pattern, group);
}

void
edit_actions_update (LigmaActionGroup *group,
                     gpointer         data)
{
  LigmaImage    *image        = action_data_get_image (data);
  LigmaDisplay  *display      = action_data_get_display (data);
  GList        *drawables    = NULL;
  gchar        *undo_name    = NULL;
  gchar        *redo_name    = NULL;
  gboolean      undo_enabled = FALSE;

  gboolean      have_no_groups = FALSE; /* At least 1 selected layer is not a group.         */
  gboolean      have_writable  = FALSE; /* At least 1 selected layer has no contents lock.   */

  if (image)
    {
      GList *iter;

      drawables = ligma_image_get_selected_drawables (image);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (! ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            have_no_groups = TRUE;

          if (! ligma_item_is_content_locked (LIGMA_ITEM (iter->data), NULL))
            have_writable = TRUE;

          if (have_no_groups && have_writable)
            break;
        }

      undo_enabled = ligma_image_undo_is_enabled (image);

      if (undo_enabled)
        {
          LigmaUndoStack *undo_stack = ligma_image_get_undo_stack (image);
          LigmaUndoStack *redo_stack = ligma_image_get_redo_stack (image);
          LigmaUndo      *undo       = ligma_undo_stack_peek (undo_stack);
          LigmaUndo      *redo       = ligma_undo_stack_peek (redo_stack);
          const gchar   *tool_undo  = NULL;
          const gchar   *tool_redo  = NULL;

          if (display)
            {
              tool_undo = tool_manager_can_undo_active (image->ligma, display);
              tool_redo = tool_manager_can_redo_active (image->ligma, display);
            }

          if (tool_undo)
            undo_name = g_strdup_printf (_("_Undo %s"), tool_undo);
          else if (undo)
            undo_name = g_strdup_printf (_("_Undo %s"),
                                         ligma_object_get_name (undo));

          if (tool_redo)
            redo_name = g_strdup_printf (_("_Redo %s"), tool_redo);
          else if (redo)
            redo_name = g_strdup_printf (_("_Redo %s"),
                                         ligma_object_get_name (redo));
        }
    }


#define SET_LABEL(action,label) \
        ligma_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_LABEL ("edit-undo", undo_name ? undo_name : _("_Undo"));
  SET_LABEL ("edit-redo", redo_name ? redo_name : _("_Redo"));

  SET_SENSITIVE ("edit-undo",        undo_enabled && undo_name);
  SET_SENSITIVE ("edit-redo",        undo_enabled && redo_name);
  SET_SENSITIVE ("edit-strong-undo", undo_enabled && undo_name);
  SET_SENSITIVE ("edit-strong-redo", undo_enabled && redo_name);
  SET_SENSITIVE ("edit-undo-clear",  undo_enabled && (undo_name || redo_name));

  g_free (undo_name);
  g_free (redo_name);

  SET_SENSITIVE ("edit-cut",                         have_writable && have_no_groups);
  SET_SENSITIVE ("edit-copy",                        drawables);
  SET_SENSITIVE ("edit-copy-visible",                image);
  /*             "edit-paste" is always active */
  SET_SENSITIVE ("edit-paste-in-place",              image);
  SET_SENSITIVE ("edit-paste-into",                  image);
  SET_SENSITIVE ("edit-paste-into-in-place",         image);

  SET_SENSITIVE ("edit-named-cut",          have_writable && have_no_groups);
  SET_SENSITIVE ("edit-named-copy",         drawables);
  SET_SENSITIVE ("edit-named-copy-visible", drawables);
  /*             "edit-named-paste" is always active */

  SET_SENSITIVE ("edit-clear",              have_writable && have_no_groups);
  SET_SENSITIVE ("edit-fill-fg",            have_writable && have_no_groups);
  SET_SENSITIVE ("edit-fill-bg",            have_writable && have_no_groups);
  SET_SENSITIVE ("edit-fill-pattern",       have_writable && have_no_groups);

#undef SET_LABEL
#undef SET_SENSITIVE

  g_list_free (drawables);
}


/*  private functions  */

static void
edit_actions_foreground_changed (LigmaContext     *context,
                                 const LigmaRGB   *color,
                                 LigmaActionGroup *group)
{
  ligma_action_group_set_action_color (group, "edit-fill-fg", color, FALSE);
}

static void
edit_actions_background_changed (LigmaContext     *context,
                                 const LigmaRGB   *color,
                                 LigmaActionGroup *group)
{
  ligma_action_group_set_action_color (group, "edit-fill-bg", color, FALSE);
}

static void
edit_actions_pattern_changed (LigmaContext     *context,
                              LigmaPattern     *pattern,
                              LigmaActionGroup *group)
{
  ligma_action_group_set_action_viewable (group, "edit-fill-pattern",
                                         LIGMA_VIEWABLE (pattern));
}
