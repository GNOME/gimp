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
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "tools/tool_manager.h"

#include "actions.h"
#include "edit-actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   edit_actions_foreground_changed (GimpContext     *context,
                                               GeglColor       *color,
                                               GimpActionGroup *group);
static void   edit_actions_background_changed (GimpContext     *context,
                                               GeglColor       *color,
                                               GimpActionGroup *group);
static void   edit_actions_pattern_changed    (GimpContext     *context,
                                               GimpPattern     *pattern,
                                               GimpActionGroup *group);


static const GimpActionEntry edit_actions[] =
{
  { "edit-undo", GIMP_ICON_EDIT_UNDO,
    NC_("edit-action", "_Undo"), NULL, { "<primary>Z", NULL },
    NC_("edit-action", "Undo the last operation"),
    edit_undo_cmd_callback,
    GIMP_HELP_EDIT_UNDO },

  { "edit-redo", GIMP_ICON_EDIT_REDO,
    NC_("edit-action", "_Redo"), NULL, { "<primary>Y", NULL },
    NC_("edit-action", "Redo the last operation that was undone"),
    edit_redo_cmd_callback,
    GIMP_HELP_EDIT_REDO },

  { "edit-strong-undo", GIMP_ICON_EDIT_UNDO,
    NC_("edit-action", "Strong Undo"), NULL, { "<primary><shift>Z", NULL },
    NC_("edit-action", "Undo the last operation, skipping visibility changes"),
    edit_strong_undo_cmd_callback,
    GIMP_HELP_EDIT_STRONG_UNDO },

  { "edit-strong-redo", GIMP_ICON_EDIT_REDO,
    NC_("edit-action", "Strong Redo"), NULL, { "<primary><shift>Y", NULL },
    NC_("edit-action",
        "Redo the last operation that was undone, skipping visibility changes"),
    edit_strong_redo_cmd_callback,
    GIMP_HELP_EDIT_STRONG_REDO },

  { "edit-undo-clear", GIMP_ICON_SHRED,
    NC_("edit-action", "_Clear Undo History"), NULL, { NULL },
    NC_("edit-action", "Remove all operations from the undo history"),
    edit_undo_clear_cmd_callback,
    GIMP_HELP_EDIT_UNDO_CLEAR },

  { "edit-cut", GIMP_ICON_EDIT_CUT,
    NC_("edit-action", "Cu_t"), NULL, { "<primary>X", "Cut", NULL },
    NC_("edit-action", "Move the selected pixels to the clipboard"),
    edit_cut_cmd_callback,
    GIMP_HELP_EDIT_CUT },

  { "edit-copy", GIMP_ICON_EDIT_COPY,
    NC_("edit-action", "_Copy"), NULL, { "<primary>C", "Copy", NULL },
    NC_("edit-action", "Copy the selected pixels to the clipboard"),
    edit_copy_cmd_callback,
    GIMP_HELP_EDIT_COPY },

  { "edit-copy-visible", NULL, /* GIMP_ICON_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible"), NULL, { "<primary><shift>C", "<shift>Copy", NULL },
    NC_("edit-action", "Copy what is visible in the selected region"),
    edit_copy_visible_cmd_callback,
    GIMP_HELP_EDIT_COPY_VISIBLE },

  { "edit-paste-as-new-image", GIMP_ICON_EDIT_PASTE_AS_NEW,
    NC_("edit-action", "Paste as _New Image"),
    NC_("edit-action", "From _Clipboard"),
    { "<primary><shift>V", "<shift>Paste", NULL },
    NC_("edit-action", "Create a new image from the content of the clipboard"),
    edit_paste_as_new_image_cmd_callback,
    GIMP_HELP_EDIT_PASTE_AS_NEW_IMAGE },

  { "edit-named-cut", GIMP_ICON_EDIT_CUT,
    NC_("edit-action", "Cu_t Named..."), NULL, { NULL },
    NC_("edit-action", "Move the selected pixels to a named buffer"),
    edit_named_cut_cmd_callback,
    GIMP_HELP_BUFFER_CUT },

  { "edit-named-copy", GIMP_ICON_EDIT_COPY,
    NC_("edit-action", "_Copy Named..."), NULL, { NULL },
    NC_("edit-action", "Copy the selected pixels to a named buffer"),
    edit_named_copy_cmd_callback,
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-copy-visible", NULL, /* GIMP_ICON_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible Named..."), NULL, { NULL },
    NC_("edit-action",
        "Copy what is visible in the selected region to a named buffer"),
    edit_named_copy_visible_cmd_callback,
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-paste", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste Named..."), NULL, { NULL },
    NC_("edit-action", "Paste the content of a named buffer"),
    edit_named_paste_cmd_callback,
    GIMP_HELP_BUFFER_PASTE },

  { "edit-clear", GIMP_ICON_EDIT_CLEAR,
    NC_("edit-action", "Cl_ear"), NULL, { "Delete", NULL },
    NC_("edit-action", "Clear the selected pixels"),
    edit_clear_cmd_callback,
    GIMP_HELP_EDIT_CLEAR }
};

static const GimpEnumActionEntry edit_paste_actions[] =
{
  { "edit-paste", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste"), NULL, { "<primary>V", "Paste", NULL },
    NC_("edit-action", "Paste the content of the clipboard"),
    GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING, FALSE,
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-in-place", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste In P_lace"), NULL, { "<primary><alt>V", "<alt>Paste", NULL },
    NC_("edit-action",
        "Paste the content of the clipboard at its original position"),
    GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, FALSE,
    GIMP_HELP_EDIT_PASTE_IN_PLACE },

  { "edit-paste-merged", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "_Paste as Single Layer"), NULL, { NULL },
    NC_("edit-action", "Paste the content of the clipboard as a single layer"),
    GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, FALSE,
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-merged-in-place", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste as Single Layer In P_lace"), NULL, { NULL },
    NC_("edit-action",
        "Paste the content of the clipboard at its original position as a single layer"),
    GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, FALSE,
    GIMP_HELP_EDIT_PASTE_IN_PLACE },

  { "edit-paste-into", GIMP_ICON_EDIT_PASTE_INTO,
    NC_("edit-action", "Paste as Floating Data _Into Selection"), NULL, { NULL },
    NC_("edit-action",
        "Paste the content of the clipboard into the current selection"),
    GIMP_PASTE_TYPE_FLOATING_INTO, FALSE,
    GIMP_HELP_EDIT_PASTE_INTO },

  { "edit-paste-into-in-place", GIMP_ICON_EDIT_PASTE_INTO,
    NC_("edit-action", "Paste as Floating Data Int_o Selection In Place"), NULL, { NULL },
    NC_("edit-action",
        "Paste the content of the clipboard into the current selection "
        "at its original position"),
    GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE, FALSE,
    GIMP_HELP_EDIT_PASTE_INTO_IN_PLACE },

  { "edit-paste-float", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste as _Floating Data"), NULL, { NULL },
    NC_("edit-action", "Paste the content of the clipboard as Floating Data"),
    GIMP_PASTE_TYPE_FLOATING, FALSE,
    GIMP_HELP_EDIT_PASTE_FLOAT },

  { "edit-paste-float-in-place", GIMP_ICON_EDIT_PASTE,
    NC_("edit-action", "Paste as Floa_ting Data In Place"), NULL, { NULL },
    NC_("edit-action", "Paste the content of the clipboard as Floating Data at its original position"),
    GIMP_PASTE_TYPE_FLOATING_IN_PLACE, FALSE,
    GIMP_HELP_EDIT_PASTE_FLOAT_IN_PLACE }
};

static const GimpEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with _FG Color"), NULL, { "<primary>comma", NULL },
    NC_("edit-action", "Fill the selection using the foreground color"),
    GIMP_FILL_FOREGROUND, FALSE,
    GIMP_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with B_G Color"), NULL, { "<primary>period", NULL },
    NC_("edit-action", "Fill the selection using the background color"),
    GIMP_FILL_BACKGROUND, FALSE,
    GIMP_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", GIMP_ICON_PATTERN,
    NC_("edit-action", "Fill _with Pattern"), NULL, { "<primary>semicolon", NULL },
    NC_("edit-action", "Fill the selection using the active pattern"),
    GIMP_FILL_PATTERN, FALSE,
    GIMP_HELP_EDIT_FILL_PATTERN }
};


void
edit_actions_setup (GimpActionGroup *group)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  GeglColor   *color;
  GimpPattern *pattern;

  gimp_action_group_add_actions (group, "edit-action",
                                 edit_actions,
                                 G_N_ELEMENTS (edit_actions));

  gimp_action_group_add_enum_actions (group, "edit-action",
                                      edit_paste_actions,
                                      G_N_ELEMENTS (edit_paste_actions),
                                      edit_paste_cmd_callback);

  gimp_action_group_add_enum_actions (group, "edit-action",
                                      edit_fill_actions,
                                      G_N_ELEMENTS (edit_fill_actions),
                                      edit_fill_cmd_callback);

  g_signal_connect_object (context, "foreground-changed",
                           G_CALLBACK (edit_actions_foreground_changed),
                           group, 0);
  g_signal_connect_object (context, "background-changed",
                           G_CALLBACK (edit_actions_background_changed),
                           group, 0);
  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (edit_actions_pattern_changed),
                           group, 0);

  color = gimp_context_get_foreground (context);
  edit_actions_foreground_changed (context, color, group);

  color = gimp_context_get_background (context);
  edit_actions_background_changed (context, color, group);

  pattern = gimp_context_get_pattern (context);
  edit_actions_pattern_changed (context, pattern, group);
}

void
edit_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpImage    *image        = action_data_get_image (data);
  GimpDisplay  *display      = action_data_get_display (data);
  GList        *drawables    = NULL;
  gchar        *undo_name    = NULL;
  gchar        *redo_name    = NULL;
  gboolean      undo_enabled = FALSE;

  gboolean      have_no_groups = FALSE; /* At least 1 selected layer is not a group.         */
  gboolean      have_writable  = FALSE; /* At least 1 selected layer has no contents lock.   */

  if (image)
    {
      GList *iter;

      drawables = gimp_image_get_selected_drawables (image);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (! gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
            have_no_groups = TRUE;

          if (! gimp_item_is_content_locked (GIMP_ITEM (iter->data), NULL))
            have_writable = TRUE;

          if (have_no_groups && have_writable)
            break;
        }

      undo_enabled = gimp_image_undo_is_enabled (image);

      if (undo_enabled)
        {
          GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
          GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
          GimpUndo      *undo       = gimp_undo_stack_peek (undo_stack);
          GimpUndo      *redo       = gimp_undo_stack_peek (redo_stack);
          const gchar   *tool_undo  = NULL;
          const gchar   *tool_redo  = NULL;

          if (display)
            {
              tool_undo = tool_manager_can_undo_active (image->gimp, display);
              tool_redo = tool_manager_can_redo_active (image->gimp, display);
            }

          if (tool_undo)
            undo_name = g_strdup_printf (_("_Undo %s"), tool_undo);
          else if (undo)
            undo_name = g_strdup_printf (_("_Undo %s"),
                                         gimp_object_get_name (undo));

          if (tool_redo)
            redo_name = g_strdup_printf (_("_Redo %s"), tool_redo);
          else if (redo)
            redo_name = g_strdup_printf (_("_Redo %s"),
                                         gimp_object_get_name (redo));
        }
    }


#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

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
edit_actions_foreground_changed (GimpContext     *context,
                                 GeglColor       *color,
                                 GimpActionGroup *group)
{
  gimp_action_group_set_action_color (group, "edit-fill-fg", color, FALSE);
}

static void
edit_actions_background_changed (GimpContext     *context,
                                 GeglColor       *color,
                                 GimpActionGroup *group)
{
  gimp_action_group_set_action_color (group, "edit-fill-bg", color, FALSE);
}

static void
edit_actions_pattern_changed (GimpContext     *context,
                              GimpPattern     *pattern,
                              GimpActionGroup *group)
{
  gimp_action_group_set_action_viewable (group, "edit-fill-pattern",
                                         GIMP_VIEWABLE (pattern));
}
