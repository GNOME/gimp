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
  { "edit-menu",          NULL, N_("_Edit")     },
  { "edit-paste-as-menu", NULL, N_("_Paste as") },
  { "edit-buffer-menu",   NULL, N_("_Buffer")   },

  { "undo-popup",
    GTK_STOCK_UNDO, N_("Undo History Menu"), NULL, NULL, NULL,
    GIMP_HELP_UNDO_DIALOG },

  { "edit-undo", GTK_STOCK_UNDO,
    N_("_Undo"), "<control>Z",
    N_("Undo the last operation"),
    G_CALLBACK (edit_undo_cmd_callback),
    GIMP_HELP_EDIT_UNDO },

  { "edit-redo", GTK_STOCK_REDO,
    N_("_Redo"), "<control>Y",
    N_("Redo the last operation that was undone"),
    G_CALLBACK (edit_redo_cmd_callback),
    GIMP_HELP_EDIT_REDO },

  { "edit-strong-undo", GTK_STOCK_UNDO,
    N_("Strong Undo"), "<control><shift>Z",
    N_("Undo the last operation, skipping visibility changes"),
    G_CALLBACK (edit_strong_undo_cmd_callback),
    GIMP_HELP_EDIT_STRONG_UNDO },

  { "edit-strong-redo", GTK_STOCK_REDO,
    N_("Strong Redo"), "<control><shift>Y",
    N_("Redo the last operation that was undone, skipping visibility changes"),
    G_CALLBACK (edit_strong_redo_cmd_callback),
    GIMP_HELP_EDIT_STRONG_REDO },

  { "edit-undo-clear", GTK_STOCK_CLEAR,
    N_("_Clear Undo History"), "",
    N_("Remove all operations from the undo history"),
    G_CALLBACK (edit_undo_clear_cmd_callback),
    GIMP_HELP_EDIT_UNDO_CLEAR },

  { "edit-fade", GTK_STOCK_UNDO,
    N_("_Fade..."), "",
    N_("Modify paint mode and opacity of the last pixel manipulation"),
    G_CALLBACK (edit_fade_cmd_callback),
    GIMP_HELP_EDIT_FADE },

  { "edit-cut", GTK_STOCK_CUT,
    N_("Cu_t"), "<control>X",
    N_("Move the selected pixels to the clipboard"),
    G_CALLBACK (edit_cut_cmd_callback),
    GIMP_HELP_EDIT_CUT },

  { "edit-copy", GTK_STOCK_COPY,
    N_("_Copy"), "<control>C",
    N_("Copy the selected pixels to the clipboard"),
    G_CALLBACK (edit_copy_cmd_callback),
    GIMP_HELP_EDIT_COPY },

  { "edit-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    N_("Copy _Visible"), "<control><shift>C",
    N_("Copy the selected region to the clipboard"),
    G_CALLBACK (edit_copy_visible_cmd_callback),
    GIMP_HELP_EDIT_COPY_VISIBLE },

  { "edit-paste", GTK_STOCK_PASTE,
    N_("_Paste"), "<control>V",
    N_("Paste the content of the clipboard"),
    G_CALLBACK (edit_paste_cmd_callback),
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-into", GIMP_STOCK_PASTE_INTO,
    N_("Paste _Into"), NULL,
    N_("Paste the content of the clipboard into the current selection"),
    G_CALLBACK (edit_paste_into_cmd_callback),
    GIMP_HELP_EDIT_PASTE_INTO },

  { "edit-paste-as-new", GIMP_STOCK_PASTE_AS_NEW,
    N_("Paste as New"), "<control><shift>V",
    N_("Create a new image from the content of the clipboard"),
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-paste-as-new-short", GIMP_STOCK_PASTE_AS_NEW,
    N_("_New Image"), NULL,
    N_("Create a new image from the content of the clipboard"),
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-named-cut", GTK_STOCK_CUT,
    N_("Cu_t Named..."), "",
    N_("Move the selected pixels to a named buffer"),
    G_CALLBACK (edit_named_cut_cmd_callback),
    GIMP_HELP_BUFFER_CUT },

  { "edit-named-copy", GTK_STOCK_COPY,
    N_("_Copy Named..."), "",
    N_("Copy the selected pixels to a named buffer"),
    G_CALLBACK (edit_named_copy_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    N_("Copy _Visible Named..."), "",
    N_("Copy the selected region to a named buffer"),
    G_CALLBACK (edit_named_copy_visible_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-paste", GTK_STOCK_PASTE,
    N_("_Paste Named..."), "",
    N_("Paste the content of a named buffer"),
    G_CALLBACK (edit_named_paste_cmd_callback),
    GIMP_HELP_BUFFER_PASTE },

  { "edit-clear", GTK_STOCK_CLEAR,
    N_("Cl_ear"), "Delete",
    N_("Clear the selected pixels"),
    G_CALLBACK (edit_clear_cmd_callback),
    GIMP_HELP_EDIT_CLEAR }
};

static const GimpEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with _FG Color"), "<control>comma",
    N_("Fill the selection using the foreground color"),
    GIMP_FOREGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with B_G Color"), "<control>period",
    N_("Fill the selection using the background color"),
    GIMP_BACKGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with P_attern"), "<control>semicolon",
    N_("Fill the selection using the active pattern"),
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

  gimp_action_group_add_actions (group,
                                 edit_actions,
                                 G_N_ELEMENTS (edit_actions));

  gimp_action_group_add_enum_actions (group,
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
}

void
edit_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpImage    *image       = action_data_get_image (data);
  GimpDrawable *drawable     = NULL;
  gchar        *undo_name    = NULL;
  gchar        *redo_name    = NULL;
  gchar        *fade_name    = NULL;
  gboolean      undo_enabled = FALSE;
  gboolean      fade_enabled = FALSE;

  if (image)
    {
      GimpUndo *undo;
      GimpUndo *redo;

      drawable = gimp_image_get_active_drawable (image);

      undo_enabled = gimp_image_undo_is_enabled (image);

      if (undo_enabled)
        {
          undo = gimp_undo_stack_peek (image->undo_stack);
          redo = gimp_undo_stack_peek (image->redo_stack);

          if (undo)
            undo_name =
              g_strdup_printf (_("_Undo %s"),
                               gimp_object_get_name (GIMP_OBJECT (undo)));

          if (redo)
            redo_name =
              g_strdup_printf (_("_Redo %s"),
                               gimp_object_get_name (GIMP_OBJECT (redo)));

          undo = gimp_image_undo_get_fadeable (image);

          if (GIMP_IS_DRAWABLE_UNDO (undo) &&
              GIMP_DRAWABLE_UNDO (undo)->src2_tiles)
            fade_enabled = TRUE;

          if (fade_enabled)
            fade_name =
              g_strdup_printf (_("_Fade %s..."),
                               gimp_object_get_name (GIMP_OBJECT (undo)));
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

  SET_SENSITIVE ("edit-cut",          drawable);
  SET_SENSITIVE ("edit-copy",         drawable);
  SET_SENSITIVE ("edit-copy-visible", image);
  /*             "edit-paste" is always enabled  */
  SET_SENSITIVE ("edit-paste-into",   image);

  SET_SENSITIVE ("edit-named-cut",          drawable);
  SET_SENSITIVE ("edit-named-copy",         drawable);
  SET_SENSITIVE ("edit-named-copy-visible", drawable);
  SET_SENSITIVE ("edit-named-paste",        image);

  SET_SENSITIVE ("edit-clear",        drawable);
  SET_SENSITIVE ("edit-fill-fg",      drawable);
  SET_SENSITIVE ("edit-fill-bg",      drawable);
  SET_SENSITIVE ("edit-fill-pattern", drawable);

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
