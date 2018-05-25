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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpreset.h"

#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpeditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "dialogs/data-delete-dialog.h"

#include "tool-options-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   tool_options_show_preset_editor (Gimp           *gimp,
                                               GimpEditor     *editor,
                                               GimpToolPreset *preset);


/*  public functions  */

void
tool_options_save_new_preset_cmd_callback (GtkAction *action,
                                           gpointer   user_data)
{
  GimpEditor  *editor  = GIMP_EDITOR (user_data);
  Gimp        *gimp    = gimp_editor_get_ui_manager (editor)->gimp;
  GimpContext *context = gimp_get_user_context (gimp);
  GimpData    *data;

  data = gimp_data_factory_data_new (context->gimp->tool_preset_factory,
                                     context, _("Untitled"));

  tool_options_show_preset_editor (gimp, editor, GIMP_TOOL_PRESET (data));
}

void
tool_options_save_preset_cmd_callback (GtkAction *action,
                                       gint       value,
                                       gpointer   data)
{
  GimpEditor     *editor    = GIMP_EDITOR (data);
  Gimp           *gimp      = gimp_editor_get_ui_manager (editor)->gimp;
  GimpContext    *context   = gimp_get_user_context (gimp);
  GimpToolInfo   *tool_info = gimp_context_get_tool (context);
  GimpToolPreset *preset;

  preset = (GimpToolPreset *)
    gimp_container_get_child_by_index (tool_info->presets, value);

  if (preset)
    {
      gimp_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);

      tool_options_show_preset_editor (gimp, editor, preset);
    }
}

void
tool_options_restore_preset_cmd_callback (GtkAction *action,
                                          gint       value,
                                          gpointer   data)
{
  GimpEditor     *editor    = GIMP_EDITOR (data);
  Gimp           *gimp      = gimp_editor_get_ui_manager (editor)->gimp;
  GimpContext    *context   = gimp_get_user_context (gimp);
  GimpToolInfo   *tool_info = gimp_context_get_tool (context);
  GimpToolPreset *preset;

  preset = (GimpToolPreset *)
    gimp_container_get_child_by_index (tool_info->presets, value);

  if (preset)
    {
      if (gimp_context_get_tool_preset (context) != preset)
        gimp_context_set_tool_preset (context, preset);
      else
        gimp_context_tool_preset_changed (context);
    }
}

void
tool_options_edit_preset_cmd_callback (GtkAction *action,
                                       gint       value,
                                       gpointer   data)
{
  GimpEditor     *editor    = GIMP_EDITOR (data);
  Gimp           *gimp      = gimp_editor_get_ui_manager (editor)->gimp;
  GimpContext    *context   = gimp_get_user_context (gimp);
  GimpToolInfo   *tool_info = gimp_context_get_tool (context);
  GimpToolPreset *preset;

  preset = (GimpToolPreset *)
    gimp_container_get_child_by_index (tool_info->presets, value);

  if (preset)
    {
      tool_options_show_preset_editor (gimp, editor, preset);
    }
}

void
tool_options_delete_preset_cmd_callback (GtkAction *action,
                                         gint       value,
                                         gpointer   data)
{
  GimpEditor     *editor    = GIMP_EDITOR (data);
  GimpContext    *context   = gimp_get_user_context (gimp_editor_get_ui_manager (editor)->gimp);
  GimpToolInfo   *tool_info = gimp_context_get_tool (context);
  GimpToolPreset *preset;

  preset = (GimpToolPreset *)
    gimp_container_get_child_by_index (tool_info->presets, value);

  if (preset &&
      gimp_data_is_deletable (GIMP_DATA (preset)))
    {
      GimpDataFactory *factory = context->gimp->tool_preset_factory;
      GtkWidget       *dialog;

      dialog = data_delete_dialog_new (factory, GIMP_DATA (preset), NULL,
                                       GTK_WIDGET (editor));
      gtk_widget_show (dialog);
    }
}

void
tool_options_reset_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpEditor   *editor    = GIMP_EDITOR (data);
  GimpContext  *context   = gimp_get_user_context (gimp_editor_get_ui_manager (editor)->gimp);
  GimpToolInfo *tool_info = gimp_context_get_tool (context);

  gimp_config_reset (GIMP_CONFIG (tool_info->tool_options));
}

void
tool_options_reset_all_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpEditor *editor = GIMP_EDITOR (data);
  GtkWidget  *dialog;

  dialog = gimp_message_dialog_new (_("Reset All Tool Options"),
                                    GIMP_ICON_DIALOG_QUESTION,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Reset"),  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
      Gimp  *gimp = gimp_editor_get_ui_manager (editor)->gimp;
      GList *list;

      for (list = gimp_get_tool_info_iter (gimp);
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = list->data;

          gimp_config_reset (GIMP_CONFIG (tool_info->tool_options));
        }
    }

  gtk_widget_destroy (dialog);
}


/*  private functions  */

static void
tool_options_show_preset_editor (Gimp           *gimp,
                                 GimpEditor     *editor,
                                 GimpToolPreset *preset)
{
  GtkWidget *dockable;

  dockable =
    gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
                                               gimp,
                                               gimp_dialog_factory_get_singleton (),
                                               gimp_widget_get_monitor (GTK_WIDGET (editor)),
                                               "gimp-tool-preset-editor");

  gimp_data_editor_set_data (GIMP_DATA_EDITOR (gtk_bin_get_child (GTK_BIN (dockable))),
                             GIMP_DATA (preset));
}
