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
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
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


static GimpActionEntry edit_actions[] =
{
  { "edit-menu",          NULL, N_("_Edit")     },
  { "edit-paste-as-menu", NULL, N_("_Paste as") },
  { "edit-buffer-menu",   NULL, N_("_Buffer")   },

  { "undo-editor-popup",
    GTK_STOCK_UNDO, N_("Undo History Menu"), NULL, NULL, NULL,
    GIMP_HELP_UNDO_DIALOG },

  { "edit-undo", GTK_STOCK_UNDO,
    N_("_Undo"), "<control>Z",
    N_("Undo"),
    G_CALLBACK (edit_undo_cmd_callback),
    GIMP_HELP_EDIT_UNDO },

  { "edit-redo", GTK_STOCK_REDO,
    N_("_Redo"), "<control>Y",
    N_("Redo"),
    G_CALLBACK (edit_redo_cmd_callback),
    GIMP_HELP_EDIT_REDO },

  { "edit-undo-clear", GTK_STOCK_CLEAR,
    N_("_Clear Undo History"), "",
    N_("Clear undo history"),
    G_CALLBACK (edit_undo_clear_cmd_callback),
    GIMP_HELP_EDIT_UNDO_CLEAR },

  { "edit-cut", GTK_STOCK_CUT,
    N_("Cu_t"), "<control>X", NULL,
    G_CALLBACK (edit_cut_cmd_callback),
    GIMP_HELP_EDIT_CUT },

  { "edit-copy", GTK_STOCK_COPY,
    N_("_Copy"), "<control>C", NULL,
    G_CALLBACK (edit_copy_cmd_callback),
    GIMP_HELP_EDIT_COPY },

  { "edit-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    N_("Copy _Visible"), "", NULL,
    G_CALLBACK (edit_copy_visible_cmd_callback),
    GIMP_HELP_EDIT_COPY_VISIBLE },

  { "edit-paste", GTK_STOCK_PASTE,
    N_("_Paste"), "<control>V", NULL,
    G_CALLBACK (edit_paste_cmd_callback),
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-into", GIMP_STOCK_PASTE_INTO,
    N_("Paste _Into"), NULL, NULL,
    G_CALLBACK (edit_paste_into_cmd_callback),
    GIMP_HELP_EDIT_PASTE_INTO },

  { "edit-paste-as-new", GIMP_STOCK_PASTE_AS_NEW,
    N_("Paste as New"), NULL, NULL,
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-paste-as-new-short", GIMP_STOCK_PASTE_AS_NEW,
    N_("_New Image"), NULL, NULL,
    G_CALLBACK (edit_paste_as_new_cmd_callback),
    GIMP_HELP_EDIT_PASTE_AS_NEW },

  { "edit-named-cut", GTK_STOCK_CUT,
    N_("Cu_t Named..."), "<control><shift>X",NULL,
    G_CALLBACK (edit_named_cut_cmd_callback),
    GIMP_HELP_BUFFER_CUT },

  { "edit-named-copy", GTK_STOCK_COPY,
    N_("_Copy Named..."), "<control><shift>C", NULL,
    G_CALLBACK (edit_named_copy_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-copy-visible", NULL, /* GIMP_STOCK_COPY_VISIBLE, */
    N_("Copy _Visible Named..."), "", NULL,
    G_CALLBACK (edit_named_copy_visible_cmd_callback),
    GIMP_HELP_BUFFER_COPY },

  { "edit-named-paste", GTK_STOCK_PASTE,
    N_("_Paste Named..."), "<control><shift>V", NULL,
    G_CALLBACK (edit_named_paste_cmd_callback),
    GIMP_HELP_BUFFER_PASTE },

  { "edit-clear", GTK_STOCK_CLEAR,
    N_("Cl_ear"), "Delete", NULL,
    G_CALLBACK (edit_clear_cmd_callback),
    GIMP_HELP_EDIT_CLEAR }
};

static GimpEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with _FG Color"), "<control>comma", NULL,
    GIMP_FOREGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with B_G Color"), "<control>period", NULL,
    GIMP_BACKGROUND_FILL, FALSE,
    GIMP_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with P_attern"), "<control>semicolon", NULL,
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
  GimpImage    *gimage       = action_data_get_image (data);
  GimpDrawable *drawable     = NULL;
  gchar        *undo_name    = NULL;
  gchar        *redo_name    = NULL;
  gboolean      undo_enabled = FALSE;

  if (gimage)
    {
      GimpUndo *undo;
      GimpUndo *redo;

      drawable = gimp_image_active_drawable (gimage);

      undo_enabled = gimp_image_undo_is_enabled (gimage);

      if (undo_enabled)
        {
          undo = gimp_undo_stack_peek (gimage->undo_stack);
          redo = gimp_undo_stack_peek (gimage->redo_stack);

          if (undo)
            undo_name =
              g_strdup_printf (_("_Undo %s"),
                               gimp_object_get_name (GIMP_OBJECT (undo)));

          if (redo)
            redo_name =
              g_strdup_printf (_("_Redo %s"),
                               gimp_object_get_name (GIMP_OBJECT (redo)));
        }
    }


#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_LABEL ("edit-undo", undo_name ? undo_name : _("_Undo"));
  SET_LABEL ("edit-redo", redo_name ? redo_name : _("_Redo"));

  SET_SENSITIVE ("edit-undo",       undo_enabled && undo_name);
  SET_SENSITIVE ("edit-redo",       undo_enabled && redo_name);
  SET_SENSITIVE ("edit-undo-clear", undo_enabled && (undo_name || redo_name));

  g_free (undo_name);
  g_free (redo_name);

  SET_SENSITIVE ("edit-cut",          drawable);
  SET_SENSITIVE ("edit-copy",         drawable);
  SET_SENSITIVE ("edit-copy-visible", gimage);
  /*             "edit-paste" is always enabled  */
  SET_SENSITIVE ("edit-paste-into",   gimage);

  SET_SENSITIVE ("edit-named-cut",          drawable);
  SET_SENSITIVE ("edit-named-copy",         drawable);
  SET_SENSITIVE ("edit-named-copy-visible", drawable);
  SET_SENSITIVE ("edit-named-paste",        gimage);

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
