/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagemaptool-settings.c
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

#include <errno.h>

#include <glib/gstdio.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpsettingsbox.h"

#include "display/gimptoolgui.h"

#include "gimpimagemapoptions.h"
#include "gimpimagemaptool.h"
#include "gimpimagemaptool-settings.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean gimp_image_map_tool_settings_import (GimpSettingsBox  *box,
                                                     GFile            *file,
                                                     GimpImageMapTool *tool);
static gboolean gimp_image_map_tool_settings_export (GimpSettingsBox  *box,
                                                     GFile            *file,
                                                     GimpImageMapTool *tool);


/*  public functions  */

GtkWidget *
gimp_image_map_tool_real_get_settings_ui (GimpImageMapTool  *image_map_tool,
                                          GimpContainer     *settings,
                                          GFile             *settings_file,
                                          const gchar       *import_dialog_title,
                                          const gchar       *export_dialog_title,
                                          const gchar       *file_dialog_help_id,
                                          GFile             *default_folder,
                                          GtkWidget        **settings_box)
{
  GimpToolInfo *tool_info;
  GtkSizeGroup *label_group;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkWidget    *settings_combo;

  tool_info = GIMP_TOOL (image_map_tool)->tool_info;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label_group = gimp_image_map_tool_dialog_get_label_group (image_map_tool);

  label = gtk_label_new_with_mnemonic (_("Pre_sets:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (label_group, label);
  gtk_widget_show (label);

  *settings_box = gimp_settings_box_new (tool_info->gimp,
                                         image_map_tool->config,
                                         settings,
                                         settings_file,
                                         import_dialog_title,
                                         export_dialog_title,
                                         file_dialog_help_id,
                                         default_folder,
                                         NULL);
  gtk_box_pack_start (GTK_BOX (hbox), *settings_box, TRUE, TRUE, 0);
  gtk_widget_show (*settings_box);

  settings_combo = gimp_settings_box_get_combo (GIMP_SETTINGS_BOX (*settings_box));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), settings_combo);

  g_signal_connect (image_map_tool->settings_box, "import",
                    G_CALLBACK (gimp_image_map_tool_settings_import),
                    image_map_tool);

  g_signal_connect (image_map_tool->settings_box, "export",
                    G_CALLBACK (gimp_image_map_tool_settings_export),
                    image_map_tool);

  return hbox;
}

gboolean
gimp_image_map_tool_real_settings_import (GimpImageMapTool  *tool,
                                          GInputStream      *input,
                                          GError           **error)
{
  return gimp_config_deserialize_stream (GIMP_CONFIG (tool->config),
                                         input,
                                         NULL, error);
}

gboolean
gimp_image_map_tool_real_settings_export (GimpImageMapTool  *tool,
                                          GOutputStream     *output,
                                          GError           **error)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  gchar                 *header;
  gchar                 *footer;
  gboolean               success;

  header = g_strdup_printf ("GIMP %s tool settings",   klass->settings_name);
  footer = g_strdup_printf ("end of %s tool settings", klass->settings_name);

  success = gimp_config_serialize_to_stream (GIMP_CONFIG (tool->config),
                                             output,
                                             header, footer,
                                             NULL, error);

  g_free (header);
  g_free (footer);

  return success;
}


/*  private functions  */

static gboolean
gimp_image_map_tool_settings_import (GimpSettingsBox  *box,
                                     GFile            *file,
                                     GimpImageMapTool *tool)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GInputStream          *input;
  GError                *error      = NULL;

  g_return_val_if_fail (tool_class->settings_import != NULL, FALSE);

  if (GIMP_TOOL (tool)->tool_info->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));
  if (! input)
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Could not open '%s' for reading: %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      return FALSE;
    }

  if (! tool_class->settings_import (tool, input, &error))
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Error reading '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      g_object_unref (input);
      return FALSE;
    }

  g_object_unref (input);

  gimp_image_map_tool_preview (tool);

  g_object_set (GIMP_TOOL_GET_OPTIONS (tool),
                "settings", file,
                NULL);

  return TRUE;
}

static gboolean
gimp_image_map_tool_settings_export (GimpSettingsBox  *box,
                                     GFile            *file,
                                     GimpImageMapTool *tool)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GOutputStream         *output;
  GError                *error      = NULL;

  g_return_val_if_fail (tool_class->settings_export != NULL, FALSE);

  if (GIMP_TOOL (tool)->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Could not open '%s' for writing: %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      return FALSE;
    }

  if (! tool_class->settings_export (tool, output, &error))
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Error writing '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      g_object_unref (output);
      return FALSE;
    }

  g_object_unref (output);

  gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                G_OBJECT (GIMP_TOOL (tool)->display),
                GIMP_MESSAGE_INFO,
                _("Settings saved to '%s'"),
                gimp_file_get_utf8_name (file));

  g_object_set (GIMP_TOOL_GET_OPTIONS (tool),
                "settings", file,
                NULL);

  return TRUE;
}
