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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmadatafactory.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"
#include "core/ligmatoolpreset.h"

#include "widgets/ligmadataeditor.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmaeditor.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "dialogs/data-delete-dialog.h"

#include "tool-options-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   tool_options_show_preset_editor (Ligma           *ligma,
                                               LigmaEditor     *editor,
                                               LigmaToolPreset *preset);


/*  public functions  */

void
tool_options_save_new_preset_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    user_data)
{
  LigmaEditor  *editor  = LIGMA_EDITOR (user_data);
  Ligma        *ligma    = ligma_editor_get_ui_manager (editor)->ligma;
  LigmaContext *context = ligma_get_user_context (ligma);
  LigmaData    *data;

  data = ligma_data_factory_data_new (context->ligma->tool_preset_factory,
                                     context, _("Untitled"));

  tool_options_show_preset_editor (ligma, editor, LIGMA_TOOL_PRESET (data));
}

void
tool_options_save_preset_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaEditor     *editor    = LIGMA_EDITOR (data);
  Ligma           *ligma      = ligma_editor_get_ui_manager (editor)->ligma;
  LigmaContext    *context   = ligma_get_user_context (ligma);
  LigmaToolInfo   *tool_info = ligma_context_get_tool (context);
  LigmaToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (LigmaToolPreset *)
    ligma_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      ligma_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);

      tool_options_show_preset_editor (ligma, editor, preset);
    }
}

void
tool_options_restore_preset_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaEditor     *editor    = LIGMA_EDITOR (data);
  Ligma           *ligma      = ligma_editor_get_ui_manager (editor)->ligma;
  LigmaContext    *context   = ligma_get_user_context (ligma);
  LigmaToolInfo   *tool_info = ligma_context_get_tool (context);
  LigmaToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (LigmaToolPreset *)
    ligma_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      if (ligma_context_get_tool_preset (context) != preset)
        ligma_context_set_tool_preset (context, preset);
      else
        ligma_context_tool_preset_changed (context);
    }
}

void
tool_options_edit_preset_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaEditor     *editor    = LIGMA_EDITOR (data);
  Ligma           *ligma      = ligma_editor_get_ui_manager (editor)->ligma;
  LigmaContext    *context   = ligma_get_user_context (ligma);
  LigmaToolInfo   *tool_info = ligma_context_get_tool (context);
  LigmaToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (LigmaToolPreset *)
    ligma_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      tool_options_show_preset_editor (ligma, editor, preset);
    }
}

void
tool_options_delete_preset_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaEditor     *editor    = LIGMA_EDITOR (data);
  LigmaContext    *context   = ligma_get_user_context (ligma_editor_get_ui_manager (editor)->ligma);
  LigmaToolInfo   *tool_info = ligma_context_get_tool (context);
  LigmaToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (LigmaToolPreset *)
    ligma_container_get_child_by_index (tool_info->presets, index);

  if (preset &&
      ligma_data_is_deletable (LIGMA_DATA (preset)))
    {
      LigmaDataFactory *factory = context->ligma->tool_preset_factory;
      GtkWidget       *dialog;

      dialog = data_delete_dialog_new (factory, LIGMA_DATA (preset), NULL,
                                       GTK_WIDGET (editor));
      gtk_widget_show (dialog);
    }
}

void
tool_options_reset_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaEditor   *editor    = LIGMA_EDITOR (data);
  LigmaContext  *context   = ligma_get_user_context (ligma_editor_get_ui_manager (editor)->ligma);
  LigmaToolInfo *tool_info = ligma_context_get_tool (context);

  ligma_config_reset (LIGMA_CONFIG (tool_info->tool_options));
}

void
tool_options_reset_all_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaEditor *editor = LIGMA_EDITOR (data);
  GtkWidget  *dialog;

  dialog = ligma_message_dialog_new (_("Reset All Tool Options"),
                                    LIGMA_ICON_DIALOG_QUESTION,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    ligma_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Reset"),  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "tool options to default values?"));

  if (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      Ligma  *ligma = ligma_editor_get_ui_manager (editor)->ligma;
      GList *list;

      for (list = ligma_get_tool_info_iter (ligma);
           list;
           list = g_list_next (list))
        {
          LigmaToolInfo *tool_info = list->data;

          ligma_config_reset (LIGMA_CONFIG (tool_info->tool_options));
        }
    }

  gtk_widget_destroy (dialog);
}


/*  private functions  */

static void
tool_options_show_preset_editor (Ligma           *ligma,
                                 LigmaEditor     *editor,
                                 LigmaToolPreset *preset)
{
  GtkWidget *dockable;

  dockable =
    ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
                                               ligma,
                                               ligma_dialog_factory_get_singleton (),
                                               ligma_widget_get_monitor (GTK_WIDGET (editor)),
                                               "ligma-tool-preset-editor");

  ligma_data_editor_set_data (LIGMA_DATA_EDITOR (gtk_bin_get_child (GTK_BIN (dockable))),
                             LIGMA_DATA (preset));
}
