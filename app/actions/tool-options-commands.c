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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "widgets/gimpeditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"

#include "tool-options-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   tool_options_save_callback   (GtkWidget   *widget,
                                            const gchar *name,
                                            gpointer     data);
static void   tool_options_rename_callback (GtkWidget   *widget,
                                            const gchar *name,
                                            gpointer     data);


/*  public functions  */

void
tool_options_save_new_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpEditor   *editor    = GIMP_EDITOR (data);
  GimpContext  *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo *tool_info = gimp_context_get_tool (context);
  GtkWidget    *dialog;

  context   = gimp_get_user_context (editor->ui_manager->gimp);
  tool_info = gimp_context_get_tool (context);

  dialog = gimp_query_string_box (_("Save Tool Options"),
                                  GTK_WIDGET (editor),
                                  gimp_standard_help_func,
                                  GIMP_HELP_TOOL_OPTIONS_DIALOG,
                                  _("Enter a name for the saved options"),
                                  _("Saved Options"),
                                  NULL, NULL,
                                  tool_options_save_callback, tool_info);
  gtk_widget_show (dialog);
}

void
tool_options_save_to_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpEditor      *editor    = GIMP_EDITOR (data);
  GimpContext     *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo    *tool_info = gimp_context_get_tool (context);
  GimpToolOptions *options;

  options = (GimpToolOptions *)
    gimp_container_get_child_by_index (tool_info->options_presets, value);

  if (options)
    {
      gchar *name;

      name = g_strdup (gimp_object_get_name (GIMP_OBJECT (options)));

      gimp_config_sync (GIMP_CONFIG (tool_info->tool_options),
                        GIMP_CONFIG (options), 0);
      gimp_object_set_name (GIMP_OBJECT (options), name);

      g_free (name);
    }
}

void
tool_options_restore_from_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpEditor      *editor    = GIMP_EDITOR (data);
  GimpContext     *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo    *tool_info = gimp_context_get_tool (context);
  GimpToolOptions *options;

  options = (GimpToolOptions *)
    gimp_container_get_child_by_index (tool_info->options_presets, value);

  if (options)
    gimp_config_sync (GIMP_CONFIG (options),
                      GIMP_CONFIG (tool_info->tool_options), 0);
}

void
tool_options_rename_saved_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpEditor      *editor    = GIMP_EDITOR (data);
  GimpContext     *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo    *tool_info = gimp_context_get_tool (context);
  GimpToolOptions *options;

  options = (GimpToolOptions *)
    gimp_container_get_child_by_index (tool_info->options_presets, value);

  if (options)
    {
      GtkWidget *dialog;

      dialog = gimp_query_string_box (_("Rename Saved Tool Options"),
                                      NULL /* FIXME */,
                                      gimp_standard_help_func,
                                      GIMP_HELP_TOOL_OPTIONS_DIALOG,
                                      _("Enter a new name for the saved options"),
                                      GIMP_OBJECT (options)->name,
                                      NULL, NULL,
                                      tool_options_rename_callback, options);
      gtk_widget_show (dialog);
    }
}

void
tool_options_delete_saved_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpEditor      *editor    = GIMP_EDITOR (data);
  GimpContext     *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo    *tool_info = gimp_context_get_tool (context);
  GimpToolOptions *options;

  options = (GimpToolOptions *)
    gimp_container_get_child_by_index (tool_info->options_presets, value);

  if (options)
    gimp_container_remove (tool_info->options_presets, GIMP_OBJECT (options));
}

void
tool_options_reset_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpEditor   *editor    = GIMP_EDITOR (data);
  GimpContext  *context   = gimp_get_user_context (editor->ui_manager->gimp);
  GimpToolInfo *tool_info = gimp_context_get_tool (context);

  gimp_tool_options_reset (tool_info->tool_options);
}

static void
tool_options_reset_all_response (GtkWidget *dialog,
                                 gint       response_id,
                                 Gimp      *gimp)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      GList *list;

      for (list = GIMP_LIST (gimp->tool_info_list)->list;
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = list->data;

          gimp_tool_options_reset (tool_info->tool_options);
        }
    }
}

void
tool_options_reset_all_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpEditor *editor = GIMP_EDITOR (data);
  GtkWidget  *dialog;

  dialog = gimp_message_dialog_new (_("Reset Tool Options"), GIMP_STOCK_QUESTION,
                                    GTK_WIDGET (editor), 0,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GIMP_STOCK_RESET, GTK_RESPONSE_OK,

                                    NULL);

  g_signal_connect_object (editor, "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (tool_options_reset_all_response),
                    editor->ui_manager->gimp);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "tool options to default values?"));

  gtk_widget_show (dialog);
}


/*  private functions  */

static void
tool_options_save_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (data);
  GimpConfig   *copy;

  if (! name || ! strlen (name))
    name = _("Saved Options");

  copy = gimp_config_duplicate (GIMP_CONFIG (tool_info->tool_options));

  gimp_object_set_name (GIMP_OBJECT (copy), name);

  gimp_container_insert (tool_info->options_presets, GIMP_OBJECT (copy), -1);
  g_object_unref (copy);
}

static void
tool_options_rename_callback (GtkWidget   *widget,
                              const gchar *name,
                              gpointer     data)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (data);

  if (! name || ! strlen (name))
    name = _("Saved Options");

  gimp_object_set_name (GIMP_OBJECT (options), name);
}
