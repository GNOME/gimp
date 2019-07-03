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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"
#include "core/gimptoolpreset.h"

#include "widgets/gimpdataeditor.h"

#include "tool-preset-editor-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
tool_preset_editor_save_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpDataEditor *editor  = GIMP_DATA_EDITOR (data);
  GimpContext    *context = editor->context;
  GimpToolPreset *preset;
  GimpToolInfo   *tool_info;

  preset    = GIMP_TOOL_PRESET (gimp_data_editor_get_data (editor));
  tool_info = gimp_context_get_tool (gimp_get_user_context (context->gimp));

  if (tool_info && preset)
    {
      GimpToolInfo *preset_tool;

      preset_tool =  gimp_context_get_tool (GIMP_CONTEXT (preset->tool_options));

      if (tool_info != preset_tool)
        {
          gimp_message (context->gimp,
                        G_OBJECT (editor), GIMP_MESSAGE_WARNING,
                        _("Can't save '%s' tool options to an "
                          "existing '%s' tool preset."),
                        tool_info->label,
                        preset_tool->label);
          return;
        }

      gimp_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);
    }
}

void
tool_preset_editor_restore_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpDataEditor *editor  = GIMP_DATA_EDITOR (data);
  GimpContext    *context = editor->context;
  GimpToolPreset *preset;

  preset = GIMP_TOOL_PRESET (gimp_data_editor_get_data (editor));

  if (preset)
    gimp_context_tool_preset_changed (gimp_get_user_context (context->gimp));
}
