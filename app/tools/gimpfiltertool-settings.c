/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiltertool-settings.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpsettingsbox.h"

#include "display/gimptoolgui.h"

#include "gimpfiltertool.h"
#include "gimpfiltertool-settings.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean gimp_filter_tool_settings_import (GimpSettingsBox *box,
                                                  GFile           *file,
                                                  GimpFilterTool  *filter_tool);
static gboolean gimp_filter_tool_settings_export (GimpSettingsBox *box,
                                                  GFile           *file,
                                                  GimpFilterTool  *filter_tool);


/*  public functions  */

GtkWidget *
gimp_filter_tool_get_settings_box (GimpFilterTool *filter_tool)
{
  GimpToolInfo *tool_info = GIMP_TOOL (filter_tool)->tool_info;
  GQuark        quark     = g_quark_from_static_string ("settings-folder");
  GType         type      = G_TYPE_FROM_INSTANCE (filter_tool->config);
  GFile        *settings_folder;
  GtkWidget    *box;
  GtkWidget    *label;
  GtkWidget    *combo;
  gchar        *import_title;
  gchar        *export_title;

  settings_folder = g_type_get_qdata (type, quark);

  import_title = g_strdup_printf (_("Import '%s' Settings"),
                                  gimp_tool_get_label (GIMP_TOOL (filter_tool)));
  export_title = g_strdup_printf (_("Export '%s' Settings"),
                                  gimp_tool_get_label (GIMP_TOOL (filter_tool)));

  box = gimp_settings_box_new (tool_info->gimp,
                               filter_tool->config,
                               filter_tool->settings,
                               import_title,
                               export_title,
                               gimp_tool_get_help_id (GIMP_TOOL (filter_tool)),
                               settings_folder,
                               NULL);

  g_free (import_title);
  g_free (export_title);

  g_signal_connect (box, "import",
                    G_CALLBACK (gimp_filter_tool_settings_import),
                    filter_tool);

  g_signal_connect (box, "export",
                    G_CALLBACK (gimp_filter_tool_settings_export),
                    filter_tool);

  g_signal_connect_swapped (box, "selected",
                            G_CALLBACK (gimp_filter_tool_set_config),
                            filter_tool);

  label = gtk_label_new_with_mnemonic (_("Pre_sets:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (box), label, 0);
  gtk_widget_show (label);

  combo = gimp_settings_box_get_combo (GIMP_SETTINGS_BOX (box));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  return box;
}

gboolean
gimp_filter_tool_real_settings_import (GimpFilterTool  *filter_tool,
                                       GInputStream    *input,
                                       GError         **error)
{
  return gimp_config_deserialize_stream (GIMP_CONFIG (filter_tool->config),
                                         input,
                                         NULL, error);
}

gboolean
gimp_filter_tool_real_settings_export (GimpFilterTool  *filter_tool,
                                       GOutputStream   *output,
                                       GError         **error)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);
  gchar    *header;
  gchar    *footer;
  gboolean  success;

  header = g_strdup_printf ("GIMP '%s' settings",
                            gimp_tool_get_label (tool));
  footer = g_strdup_printf ("end of '%s' settings",
                            gimp_tool_get_label (tool));

  success = gimp_config_serialize_to_stream (GIMP_CONFIG (filter_tool->config),
                                             output,
                                             header, footer,
                                             NULL, error);

  g_free (header);
  g_free (footer);

  return success;
}


/*  private functions  */

static gboolean
gimp_filter_tool_settings_import (GimpSettingsBox *box,
                                  GFile           *file,
                                  GimpFilterTool  *filter_tool)
{
  GimpFilterToolClass *tool_class = GIMP_FILTER_TOOL_GET_CLASS (filter_tool);
  GInputStream        *input;
  GError              *error      = NULL;

  g_return_val_if_fail (tool_class->settings_import != NULL, FALSE);

  if (GIMP_TOOL (filter_tool)->tool_info->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));
  if (! input)
    {
      gimp_message (GIMP_TOOL (filter_tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (filter_tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Could not open '%s' for reading: %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      return FALSE;
    }

  if (! tool_class->settings_import (filter_tool, input, &error))
    {
      gimp_message (GIMP_TOOL (filter_tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (filter_tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Error reading '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      g_object_unref (input);
      return FALSE;
    }

  g_object_unref (input);

  return TRUE;
}

static gboolean
gimp_filter_tool_settings_export (GimpSettingsBox *box,
                                  GFile           *file,
                                  GimpFilterTool  *filter_tool)
{
  GimpFilterToolClass *tool_class = GIMP_FILTER_TOOL_GET_CLASS (filter_tool);
  GOutputStream       *output;
  GError              *error      = NULL;

  g_return_val_if_fail (tool_class->settings_export != NULL, FALSE);

  if (GIMP_TOOL (filter_tool)->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      gimp_message_literal (GIMP_TOOL (filter_tool)->tool_info->gimp,
                            G_OBJECT (gimp_tool_gui_get_dialog (filter_tool->gui)),
                            GIMP_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
      return FALSE;
    }

  if (! tool_class->settings_export (filter_tool, output, &error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      gimp_message (GIMP_TOOL (filter_tool)->tool_info->gimp,
                    G_OBJECT (gimp_tool_gui_get_dialog (filter_tool->gui)),
                    GIMP_MESSAGE_ERROR,
                    _("Error writing '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
      g_object_unref (output);

      return FALSE;
    }

  g_object_unref (output);

  gimp_message (GIMP_TOOL (filter_tool)->tool_info->gimp,
                G_OBJECT (GIMP_TOOL (filter_tool)->display),
                GIMP_MESSAGE_INFO,
                _("Settings saved to '%s'"),
                gimp_file_get_utf8_name (file));

  return TRUE;
}
