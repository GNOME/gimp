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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatoolpreset.h"

#include "widgets/ligmadataeditor.h"

#include "tool-preset-editor-commands.h"

#include "ligma-intl.h"


/*  public functions  */

void
tool_preset_editor_save_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaDataEditor *editor  = LIGMA_DATA_EDITOR (data);
  LigmaContext    *context = editor->context;
  LigmaToolPreset *preset;
  LigmaToolInfo   *tool_info;

  preset    = LIGMA_TOOL_PRESET (ligma_data_editor_get_data (editor));
  tool_info = ligma_context_get_tool (ligma_get_user_context (context->ligma));

  if (tool_info && preset)
    {
      LigmaToolInfo *preset_tool;

      preset_tool =  ligma_context_get_tool (LIGMA_CONTEXT (preset->tool_options));

      if (tool_info != preset_tool)
        {
          ligma_message (context->ligma,
                        G_OBJECT (editor), LIGMA_MESSAGE_WARNING,
                        _("Can't save '%s' tool options to an "
                          "existing '%s' tool preset."),
                        tool_info->label,
                        preset_tool->label);
          return;
        }

      ligma_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);
    }
}

void
tool_preset_editor_restore_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaDataEditor *editor  = LIGMA_DATA_EDITOR (data);
  LigmaContext    *context = editor->context;
  LigmaToolPreset *preset;

  preset = LIGMA_TOOL_PRESET (ligma_data_editor_get_data (editor));

  if (preset)
    ligma_context_tool_preset_changed (ligma_get_user_context (context->ligma));
}
