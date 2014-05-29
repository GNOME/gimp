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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawableundo.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "tools/tool_manager.h"

#include "actions.h"
#include "edit-actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   edit_actions_foreground_changed (GimpContext     *context,
                                               const GimpRGB   *color,
                                               GimpActionGroup *group);
static void   edit_actions_background_changed (GimpContext     *context,
                                               const GimpRGB   *color,
                                               GimpActionGroup *group);
static void   edit_actions_pattern_changed    (GimpContext     *context,
                                               GimpPattern     *pattern,
                                               GimpActionGroup *group);


static const GimpActionEntry edit_actions[] =
{
  { "edit-menu",          NULL, NC_("edit-action", "_Edit")     },
  { "edit-paste-as-menu", NULL, NC_("edit-action", "Paste _as") },
  { "edit-buffer-menu",   NULL, NC_("edit-action", "_Buffer")   },

  { "undo-popup",
    "edit-undo", NC_("edit-action", "Undo History Menu"), NULL, NULL, NULL,
    GIMP_HELP_UNDO_DIALOG },

  { "edit-undo", "edit-undo",
    NC_("edit-action", "_Undo"), "<primary>Z",
    NC_("edit-action", "Undo the last operation"),
    G_CALLBACK (edit_undo_cmd_callback),
    GIMP_HELP_EDIT_UNDO },

  { "edit-redo", "edit-redo",
    NC_("edit-action", "_Redo"), "<primary>Y",
    NC_("edit-action", "Redo the last operation that was undone"),
    G_CALLBACK (edit_redo_cmd_callback),
    GIMP_HELP_EDIT_REDO },

  { "edit-strong-undo", "edit-undo",
    NC_("edit-action", "Strong Undo"), "<primary><shift>Z",
    NC_("edit-action", "Undo the last operation, skipping visibility changes"),
    G_CALLBACK (edit_strong_undo_cmd_callback),
    GIMP_HELP_EDIT_STRONG_UNDO },

  { "edit-strong-redo", "edit-redo",
    NC_("edit-action", "Strong Redo"), "<primary><shift>Y",
    NC_("edit-action",
        "Redo the last operation that was undone, skipping visibility changes"),
    G_CALLBACK (edit_strong_redo_cmd_callback),
    GIMP_HELP_EDIT_STRONG_REDO },

  { "edit-undo-clear", "edit-clear",
    NC_("edit-action", "_Clear Undo History"), NULL,
    NC_("edit-action", "Remove all operations from the undo history"),
    G_CALLBACK (edit_undo_clear_cmd_callback),
    GIMP_HELP_EDIT_UNDO_CLEAR },

  { "edit-fade", "edit-undo",
    NC_("edit-action", "_Fade..."), NULL,
    NC_("edit-action",
        "Modify paint mode and opacity of the last pixel manipulation"),
    G_CALLBACK (edit_fade_cmd_callback),
    GIMP_HELP_EDIT_FADE },

  { "edit-cut", "edit-cut",
    NC_("edit-action", "Cu_t"), "<primary>X",
    NC_("edit-action", "Move the selected pixels to the clipboard"),
    G_CALLBACK (edit_cut_cmd_callback),
    GIMP_HELP_EDIT_CUT },

  { "edit-copy", "edit-copy",
    NC_("edit-action", "_Copy"), "<primary>C",
    NC_("edit-action", "Copy the selected pixels to the clipboard"),
    G_CALLBACK (edit_copy_cmd_callback),
    GIMP_HELP_EDIT_COPY },

  { "edit-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible"), "<primary><shift>C",
    NC_("edit-action", "Copy what is visible in the selected region"),
    G_CALLBACK (edit_copy_visible_cmd_callback),
    GIMP_HELP_EDIT_COPY_VISIBLE },

  { "edit-paste", "edit-paste",
    NC_("edit-action", "_Paste"), "<primary>V",
    NC_("edit-action", "Paste the content of the clipboard"),
    G_CALLBACK (edit_paste_cmd_callback),
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-into", GIMP_STOCK_PASTE_INTO,
    NC_("edit-action", "Paste _Into"), NULL,
    NC_("edit-action",
        "Paste the content of the clipboard into the current selection"),
    G_CALLBACK (edit_paste_into_cmd_callback),
    GIMP_HELP_EDIT_PASTE_INTO },

  { "edit-paste-as-new", GIMP_STOCK_PASTE_AS_NEW,
    NC_("edit-action", "From _Clipboard"), "<primary><shift>V",
    NC_("edit-action", "Create a new image from the content of the clipboard"),
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-paste-as-new-short", GIMP_STOCK_PASTE_AS_NEW,
    NC_("edit-action", "_New Image"), NULL,
    NC_("edit-action", "Create a new image from the content of the clipboard"),
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-paste-as-new-layer", NULL,
    NC_("edit-action", "New _Layer"), NULL,
    NC_("edit-action", "Create a new layer from the content of the clipboard"),
    G_CALLBACK (edit_paste_as_new_layer_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW_LAYER },

  { "edit-named-cut", "edit-cut",
    NC_("edit-action", "Cu_t Named..."), NULL,
    NC_("edit-action", "Move the selected pixels to a named buffer"),
    G_CALLBACK (edit_named_cut_cmd_callback),
    GIMP_HELP_BUFFER_CUT },

  { "edit-named-copy", "edit-copy",
    NC_("edit-action", "_Copy Named..."), NULL,
    NC_("edit-action", "Copy the selected pixels to a named buffer"),
    G_CALLBACK (edit_named_copy_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    NC_("edit-action", "Copy _Visible Named..."), "",
    NC_("edit-action",
        "Copy what is visible in the selected region to a named buffer"),
    G_CALLBACK (edit_named_copy_visible_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-paste", "edit-paste",
    NC_("edit-action", "_Paste Named..."), NULL,
    NC_("edit-action", "Paste the content of a named buffer"),
    G_CALLBACK (edit_named_paste_cmd_callback),
    GIMP_HELP_BUFFER_PASTE },

  { "edit-clear", "edit-clear",
    NC_("edit-action", "Cl_ear"), "Delete",
    NC_("edit-action", "Clear the selected pixels"),
    G_CALLBACK (edit_clear_cmd_callback),
    GIMP_HELP_EDIT_CLEAR }
};

static const GimpEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", GIMP_STOCK_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with _FG Color"), "<primary>comma",
    NC_("edit-action", "Fill the selection using the foreground color"),
    GIMP_FOREGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", GIMP_STOCK_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill with B_G Color"), "<primary>period",
    NC_("edit-action", "Fill the selection using the background color"),
    GIMP_BACKGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", GIMP_STOCK_TOOL_BUCKET_FILL,
    NC_("edit-action", "Fill _with Pattern"), "<primary>semicolon",
    NC_("edit-action", "Fill the selection using the active pattern"),
    GIMP_PATTERN_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_PATTERN }
};


void
edit_actions_setup (GimpActionGroup *group)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  GimpRGB      color;
  GimpPattern *pattern;
  GtkAction   *action;

  gimp_action_group_add_actions (group, "edit-action",
                                 edit_actions,
                                 G_N_ELEMENTS (edit_actions));

  gimp_action_group_add_enum_actions (group, "edit-action",
                                      edit_fill_actions,
                                      G_N_ELEMENTS (edit_fill_actions),
                                      G_CALLBACK (edit_fill_cmd_callback));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "edit-paste-as-new-short");
  gtk_action_set_accel_path (action, "<Actions>/edit/edit-paste-as-new");

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "edit-fill-pattern");
  g_object_set (action, "context", context, NULL);

  g_signal_connect_object (context, "foreground-changed",
                           G_CALLBACK (edit_actions_foreground_changed),
                           group, 0);
  g_signal_connect_object (context, "background-changed",
                           G_CALLBACK (edit_actions_background_changed),
                           group, 0);
  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (edit_actions_pattern_changed),
                           group, 0);

  gimp_context_get_foreground (context, &color);
  edit_actions_foreground_changed (context, &color, group);

  gimp_context_get_background (context, &color);
  edit_actions_background_changed (context, &color, group);

  pattern = gimp_context_get_pattern (context);
  edit_actions_pattern_changed (context, pattern, group);

#define SET_ALWAYS_SHOW_IMAGE(action,show) \
        gimp_action_group_set_action_always_show_image (group, action, show)

  SET_ALWAYS_SHOW_IMAGE ("edit-fill-fg",      TRUE);
  SET_ALWAYS_SHOW_IMAGE ("edit-fill-bg",      TRUE);
  SET_ALWAYS_SHOW_IMAGE ("edit-fill-pattern", TRUE);

#undef SET_ALWAYS_SHOW_IMAGE
}

void
edit_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpImage    *image        = action_data_get_image (data);
  GimpDisplay  *display      = action_data_get_display (data);
  GimpDrawable *drawable     = NULL;
  gchar        *undo_name    = NULL;
  gchar        *redo_name    = NULL;
  gchar        *fade_name    = NULL;
  gboolean      writable     = FALSE;
  gboolean      children     = FALSE;
  gboolean      undo_enabled = FALSE;
  gboolean      fade_enabled = FALSE;

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          writable = ! gimp_item_is_content_locked (GIMP_ITEM (drawable));

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            children = TRUE;
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
              tool_undo = tool_manager_get_undo_desc_active (image->gimp,
                                                             display);
              tool_redo = tool_manager_get_redo_desc_active (image->gimp,
                                                             display);
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

          undo = gimp_image_undo_get_fadeable (image);

          if (GIMP_IS_DRAWABLE_UNDO (undo) &&
              GIMP_DRAWABLE_UNDO (undo)->applied_buffer)
            {
              fade_enabled = TRUE;
            }

          if (fade_enabled)
            {
              fade_name =
                g_strdup_printf (_("_Fade %s..."),
                                 gimp_object_get_name (undo));
            }
        }
    }


#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_LABEL ("edit-undo", undo_name ? undo_name : _("_Undo"));
  SET_LABEL ("edit-redo", redo_name ? redo_name : _("_Redo"));
  SET_LABEL ("edit-fade", fade_name ? fade_name : _("_Fade..."));

  SET_SENSITIVE ("edit-undo",        undo_enabled && undo_name);
  SET_SENSITIVE ("edit-redo",        undo_enabled && redo_name);
  SET_SENSITIVE ("edit-strong-undo", undo_enabled && undo_name);
  SET_SENSITIVE ("edit-strong-redo", undo_enabled && redo_name);
  SET_SENSITIVE ("edit-undo-clear",  undo_enabled && (undo_name || redo_name));
  SET_SENSITIVE ("edit-fade",        fade_enabled && fade_name);

  g_free (undo_name);
  g_free (redo_name);
  g_free (fade_name);

  SET_SENSITIVE ("edit-cut",                writable && !children);
  SET_SENSITIVE ("edit-copy",               drawable);
  SET_SENSITIVE ("edit-copy-visible",       image);
  SET_SENSITIVE ("edit-paste",              !image || (!drawable ||
                                                       (writable && !children)));
  SET_SENSITIVE ("edit-paste-as-new-layer", image);
  SET_SENSITIVE ("edit-paste-into",         image && (!drawable ||
                                                      (writable  && !children)));

  SET_SENSITIVE ("edit-named-cut",          writable && !children);
  SET_SENSITIVE ("edit-named-copy",         drawable);
  SET_SENSITIVE ("edit-named-copy-visible", drawable);
  SET_SENSITIVE ("edit-named-paste",        TRUE);

  SET_SENSITIVE ("edit-clear",              writable && !children);
  SET_SENSITIVE ("edit-fill-fg",            writable && !children);
  SET_SENSITIVE ("edit-fill-bg",            writable && !children);
  SET_SENSITIVE ("edit-fill-pattern",       writable && !children);

#undef SET_LABEL
#undef SET_SENSITIVE
}


/*  private functions  */

static void
edit_actions_foreground_changed (GimpContext     *context,
                                 const GimpRGB   *color,
                                 GimpActionGroup *group)
{
  gimp_action_group_set_action_color (group, "edit-fill-fg", color, FALSE);
}

static void
edit_actions_background_changed (GimpContext     *context,
                                 const GimpRGB   *color,
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
