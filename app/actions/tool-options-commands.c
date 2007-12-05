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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpresets.h"

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
                                  gtk_widget_get_toplevel (GTK_WIDGET (editor)),
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

  options = gimp_tool_presets_get_options (tool_info->presets, value);

  if (options)
    {
      gchar *name = g_strdup (gimp_object_get_name (GIMP_OBJECT (options)));

      gimp_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (options),
                        GIMP_CONFIG_PARAM_SERIALIZE);
      gimp_object_take_name (GIMP_OBJECT (options), name);
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

  options = gimp_tool_presets_get_options (tool_info->presets, value);

  if (options)
    gimp_config_sync (G_OBJECT (options),
                      G_OBJECT (tool_info->tool_options),
                      GIMP_CONFIG_PARAM_SERIALIZE);
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

  options = gimp_tool_presets_get_options (tool_info->presets, value);

  if (options)
    {
      GtkWidget *dialog;

      dialog = gimp_query_string_box (_("Rename Saved Tool Options"),
                                      gtk_widget_get_toplevel (GTK_WIDGET (editor)),
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

  options = gimp_tool_presets_get_options (tool_info->presets, value);

  if (options)
    gimp_container_remove (GIMP_CONTAINER (tool_info->presets),
                           GIMP_OBJECT (options));
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

void
tool_options_reset_all_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpEditor *editor = GIMP_EDITOR (data);
  GtkWidget  *dialog;

  dialog = gimp_message_dialog_new (_("Reset Tool Options"),
                                    GIMP_STOCK_QUESTION,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GIMP_STOCK_RESET, GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "tool options to default values?"));

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      Gimp  *gimp = editor->ui_manager->gimp;
      GList *list;

      for (list = GIMP_LIST (gimp->tool_info_list)->list;
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = list->data;

          gimp_tool_options_reset (tool_info->tool_options);
        }
    }

  gtk_widget_destroy (dialog);
}


/*  private functions  */

static void
tool_options_save_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (data);
  GimpConfig   *copy;

  copy = gimp_config_duplicate (GIMP_CONFIG (tool_info->tool_options));

  if (name && strlen (name))
    gimp_object_set_name (GIMP_OBJECT (copy), name);
  else
    gimp_object_set_static_name (GIMP_OBJECT (copy), _("Saved Options"));

  gimp_container_insert (GIMP_CONTAINER (tool_info->presets),
                         GIMP_OBJECT (copy), -1);
  g_object_unref (copy);
}

static void
tool_options_rename_callback (GtkWidget   *widget,
                              const gchar *name,
                              gpointer     data)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (data);

  if (name && strlen (name))
    gimp_object_set_name (GIMP_OBJECT (options), name);
  else
    gimp_object_set_static_name (GIMP_OBJECT (options), _("Saved Options"));
}
