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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimpeditor.h"

#include "tools/tool_options.h"

#include "tool-options-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   tool_options_dialog_destroy           (GtkWidget    *widget,
                                                     gpointer      data);
static void   tool_options_dialog_tool_changed      (GimpContext  *context,
                                                     GimpToolInfo *tool_info,
                                                     gpointer      data);
static void   tool_options_dialog_drop_tool         (GtkWidget    *widget,
                                                     GimpViewable *viewable,
                                                     gpointer      data);
static void   tool_options_dialog_save_callback     (GtkWidget    *widget,
                                                     GimpContext  *context);
static void   tool_options_dialog_restore_callback  (GtkWidget    *widget,
                                                     GimpContext  *context);
static void   tool_options_dialog_reset_callback    (GtkWidget    *widget,
                                                     GimpContext  *context);


/*  private variables  */

static GtkWidget *options_shell         = NULL;
static GtkWidget *options_vbox          = NULL;

static GtkWidget *options_save_button   = NULL;
static GtkWidget *options_revert_button = NULL;
static GtkWidget *options_reset_button  = NULL;

static GimpToolOptions *visible_tool_options = NULL;


/*  public functions  */

GtkWidget *
tool_options_dialog_create (Gimp *gimp)
{
  GimpContext *user_context;
  GtkWidget   *editor;
  GtkWidget   *scrolled_win;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (options_shell)
    return options_shell;

  user_context = gimp_get_user_context (gimp);

  editor = g_object_new (GIMP_TYPE_EDITOR, NULL);

  gtk_widget_set_size_request (editor, -1, 200);

  options_shell = editor;

  g_signal_connect (options_shell, "destroy",
                    G_CALLBACK (tool_options_dialog_destroy),
                    NULL);

  options_save_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_SAVE,
                            _("Save current settings as default values"),
                            NULL,
                            G_CALLBACK (tool_options_dialog_save_callback),
                            NULL,
                            user_context);

  options_revert_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_REVERT_TO_SAVED,
                            _("Restore saved default values"),
                            NULL,
                            G_CALLBACK (tool_options_dialog_restore_callback),
                            NULL,
                            user_context);

  options_reset_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_STOCK_RESET,
                            _("Reset to factory defaults"),
                            NULL,
                            G_CALLBACK (tool_options_dialog_reset_callback),
                            NULL,
                            user_context);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (editor), scrolled_win);
  gtk_widget_show (scrolled_win);

  /*  The vbox containing the tool options  */
  options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (options_vbox), 2);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
                                         options_vbox);
  gtk_widget_show (options_vbox);

  /*  dnd stuff  */
  gtk_drag_dest_set (options_shell,
                     GTK_DEST_DEFAULT_ALL,
                     NULL, 0,
                     GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_add (options_shell,
                              GIMP_TYPE_TOOL_INFO,
                              tool_options_dialog_drop_tool,
                              user_context);

  g_signal_connect_object (user_context, "tool_changed",
                           G_CALLBACK (tool_options_dialog_tool_changed),
                           options_shell,
                           0);

  tool_options_dialog_tool_changed (user_context,
                                    gimp_context_get_tool (user_context),
                                    options_shell);

  return editor;
}


/*  private functions  */

static void
tool_options_dialog_destroy (GtkWidget *widget,
                             gpointer   data)
{
  GList *options;
  GList *list;

  options = gtk_container_get_children (GTK_CONTAINER (options_vbox));

  for (list = options; list; list = g_list_next (list))
    {
      g_object_ref (list->data);
      gtk_container_remove (GTK_CONTAINER (options_vbox),
                            GTK_WIDGET (list->data));
    }

  g_list_free (options);

  options_shell = NULL;
}

static void
tool_options_dialog_tool_changed (GimpContext  *context,
				  GimpToolInfo *tool_info,
				  gpointer      data)
{
  GtkWidget *options_gui;

  if (visible_tool_options &&
      (! tool_info || tool_info->tool_options != visible_tool_options))
    {
      options_gui = g_object_get_data (G_OBJECT (visible_tool_options),
                                       "gimp-tool-options-gui");

      if (options_gui)
        gtk_widget_hide (options_gui);

      visible_tool_options = NULL;
    }

  if (tool_info && tool_info->tool_options)
    {
      options_gui = g_object_get_data (G_OBJECT (tool_info->tool_options),
                                       "gimp-tool-options-gui");

      if (! options_gui->parent)
        gtk_box_pack_start (GTK_BOX (options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_show (options_gui);

      visible_tool_options = tool_info->tool_options;

      gtk_widget_set_sensitive (options_save_button,   TRUE);
      gtk_widget_set_sensitive (options_revert_button, TRUE);
      gtk_widget_set_sensitive (options_reset_button,  TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (options_save_button,   FALSE);
      gtk_widget_set_sensitive (options_revert_button, FALSE);
      gtk_widget_set_sensitive (options_reset_button,  FALSE);
    }
}

static void
tool_options_dialog_drop_tool (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
tool_options_dialog_save_callback (GtkWidget   *widget,
                                   GimpContext *context)
{
  GimpToolInfo *tool_info;
  GError       *error = NULL;

  tool_info = gimp_context_get_tool (context);

  if (! tool_info)
    return;

  if (! gimp_tool_options_serialize (tool_info->tool_options, "user", &error))
    {
      g_message ("EEK: %s\n", error->message);
      g_clear_error (&error);
    }
}

static void
tool_options_dialog_restore_callback (GtkWidget   *widget,
                                      GimpContext *context)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_context_get_tool (context);

  if (! tool_info)
    return;

  /*  Need to reset the tool-options since only the changes
   *  from the default values are written to disk.
   */
  g_object_freeze_notify (G_OBJECT (tool_info->tool_options));

  gimp_tool_options_reset (tool_info->tool_options);
  gimp_tool_options_deserialize (tool_info->tool_options, "user", NULL);

  g_object_thaw_notify (G_OBJECT (tool_info->tool_options));
}

static void
tool_options_dialog_reset_callback (GtkWidget   *widget,
				    GimpContext *context)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_context_get_tool (context);

  if (! tool_info)
    return;

  gimp_tool_options_reset (tool_info->tool_options);
}
