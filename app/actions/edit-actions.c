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

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"

#include "edit-actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   edit_actions_buffer_changed     (Gimp            *gimp,
                                               GimpActionGroup *group);
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
  { "edit-menu",        NULL, N_("_Edit")  },
  { "edit-buffer-menu", NULL, N_("Buffer") },

  { "edit-undo", GTK_STOCK_UNDO,
    N_("_Undo"), "<control>Z", NULL,
    G_CALLBACK (edit_undo_cmd_callback),
    GIMP_HELP_EDIT_UNDO },

  { "edit-redo", GTK_STOCK_REDO,
    N_("_Redo"), "<control>Y", NULL,
    G_CALLBACK (edit_redo_cmd_callback),
    GIMP_HELP_EDIT_REDO },

  { "edit-cut", GTK_STOCK_CUT,
    N_("Cu_t"), "<control>X", NULL,
    G_CALLBACK (edit_cut_cmd_callback),
    GIMP_HELP_EDIT_CUT },

  { "edit-copy", GTK_STOCK_COPY,
    N_("_Copy"), "<control>C", NULL,
    G_CALLBACK (edit_copy_cmd_callback),
    GIMP_HELP_EDIT_COPY },

  { "edit-paste", GTK_STOCK_PASTE,
    N_("_Paste"), "<control>V", NULL,
    G_CALLBACK (edit_paste_cmd_callback),
    GIMP_HELP_EDIT_PASTE },

  { "edit-paste-into", GIMP_STOCK_PASTE_INTO,
    N_("Paste _Into"), NULL, NULL,
    G_CALLBACK (edit_paste_into_cmd_callback),
    GIMP_HELP_EDIT_PASTE_INTO },

  { "edit-paste-as-new", GIMP_STOCK_PASTE_AS_NEW,
    N_("Paste as _New"), NULL, NULL,
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

  { "edit-named-paste", GTK_STOCK_PASTE,
    N_("_Paste Named..."), "<control><shift>V", NULL,
    G_CALLBACK (edit_named_paste_cmd_callback),
    GIMP_HELP_BUFFER_PASTE },

  { "edit-clear", GTK_STOCK_CLEAR,
    N_("Cl_ear"), "<control>K", NULL,
    G_CALLBACK (edit_clear_cmd_callback),
    GIMP_HELP_EDIT_CLEAR },

  { "edit-stroke", GIMP_STOCK_SELECTION_STROKE,
    N_("_Stroke Selection..."), NULL, NULL,
    G_CALLBACK (edit_stroke_cmd_callback),
    GIMP_HELP_SELECTION_STROKE }
};

static GimpEnumActionEntry edit_fill_actions[] =
{
  { "edit-fill-fg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with _FG Color"), "<control>comma", NULL,
    GIMP_FOREGROUND_FILL,
    GIMP_HELP_EDIT_FILL_FG },

  { "edit-fill-bg", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with B_G Color"), "<control>period", NULL,
    GIMP_BACKGROUND_FILL,
    GIMP_HELP_EDIT_FILL_BG },

  { "edit-fill-pattern", GIMP_STOCK_TOOL_BUCKET_FILL,
    N_("Fill with P_attern"), NULL, NULL,
    GIMP_PATTERN_FILL,
    GIMP_HELP_EDIT_FILL_PATTERN }
};


void
edit_actions_setup (GimpActionGroup *group)
{
  GimpContext *user_context;
  GimpRGB      color;

  gimp_action_group_add_actions (group,
                                 edit_actions,
                                 G_N_ELEMENTS (edit_actions));

  gimp_action_group_add_enum_actions (group,
                                      edit_fill_actions,
                                      G_N_ELEMENTS (edit_fill_actions),
                                      G_CALLBACK (edit_fill_cmd_callback));

  g_signal_connect_object (group->gimp, "buffer_changed",
                           G_CALLBACK (edit_actions_buffer_changed),
                           group, 0);
  edit_actions_buffer_changed (group->gimp, group);

  user_context = gimp_get_user_context (group->gimp);

  g_signal_connect_object (user_context, "foreground_changed",
                           G_CALLBACK (edit_actions_foreground_changed),
                           group, 0);
  g_signal_connect_object (user_context, "background_changed",
                           G_CALLBACK (edit_actions_background_changed),
                           group, 0);
  g_signal_connect_object (user_context, "pattern_changed",
                           G_CALLBACK (edit_actions_pattern_changed),
                           group, 0);

  gimp_context_get_foreground (user_context, &color);
  edit_actions_foreground_changed (user_context, &color, group);

  gimp_context_get_background (user_context, &color);
  edit_actions_background_changed (user_context, &color, group);
}

void
edit_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpDisplay      *gdisp    = NULL;
  GimpDisplayShell *shell    = NULL;
  GimpImage        *gimage   = NULL;
  GimpDrawable     *drawable = NULL;
  gboolean          sel      = FALSE;

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

      sel = ! gimp_channel_is_empty (gimp_image_get_mask (gimage));

      drawable = gimp_image_active_drawable (gimage);
    }

#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  {
    gchar *undo_name = NULL;
    gchar *redo_name = NULL;

    if (gdisp && gimp_image_undo_is_enabled (gimage))
      {
        GimpUndo *undo;
        GimpUndo *redo;

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

    SET_LABEL ("edit-undo", undo_name ? undo_name : _("_Undo"));
    SET_LABEL ("edit-redo", redo_name ? redo_name : _("_Redo"));

    SET_SENSITIVE ("edit-undo", undo_name);
    SET_SENSITIVE ("edit-redo", redo_name);

    g_free (undo_name);
    g_free (redo_name);
  }

  SET_SENSITIVE ("edit-cut",        drawable);
  SET_SENSITIVE ("edit-copy",       drawable);
  SET_SENSITIVE ("edit-paste",      gdisp && group->gimp->global_buffer);
  SET_SENSITIVE ("edit-paste-into", gdisp && group->gimp->global_buffer);

  SET_SENSITIVE ("edit-named-cut",  drawable);
  SET_SENSITIVE ("edit-named-paste", drawable);

  SET_SENSITIVE ("edit-clear",        drawable);
  SET_SENSITIVE ("edit-fill-fg",      drawable);
  SET_SENSITIVE ("edit-fill-bg",      drawable);
  SET_SENSITIVE ("edit-fill-pattern", drawable);
  SET_SENSITIVE ("edit-stroke",       drawable && sel);

#undef SET_LABEL
#undef SET_SENSITIVE
}


/*  private functions  */

static void
edit_actions_buffer_changed (Gimp            *gimp,
                             GimpActionGroup *group)
{
  gboolean buf = (gimp->global_buffer != NULL);

  gimp_action_group_set_action_sensitive (group, "edit-paste",        buf);
  gimp_action_group_set_action_sensitive (group, "edit-paste-into",   buf);
  gimp_action_group_set_action_sensitive (group, "edit-paste-as-new", buf);
}

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
