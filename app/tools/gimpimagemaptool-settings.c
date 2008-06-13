/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagemaptool-settings.c
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
#include "core/gimpimagemapconfig.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpsettingsbox.h"

#include "gimpimagemapoptions.h"
#include "gimpimagemaptool.h"
#include "gimpimagemaptool-settings.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void  gimp_image_map_tool_import_activate    (GtkWidget        *widget,
                                                     GimpImageMapTool *tool);
static void  gimp_image_map_tool_export_activate    (GtkWidget        *widget,
                                                     GimpImageMapTool *tool);

static void  gimp_image_map_tool_settings_dialog    (GimpImageMapTool *im_tool,
                                                     const gchar      *title,
                                                     gboolean          save);

static gboolean gimp_image_map_tool_settings_import (GimpImageMapTool *tool,
                                                     const gchar      *filename);
static gboolean gimp_image_map_tool_settings_export (GimpImageMapTool *tool,
                                                     const gchar      *filename);


/*  public functions  */

gboolean
gimp_image_map_tool_add_settings_gui (GimpImageMapTool *image_map_tool)
{
  GimpImageMapToolClass *klass;
  GimpToolInfo          *tool_info;
  GtkWidget             *hbox;
  GtkWidget             *label;
  gchar                 *filename;

  klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

  tool_info = GIMP_TOOL (image_map_tool)->tool_info;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Pre_sets:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  image_map_tool->label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (image_map_tool->label_group, label);
  g_object_unref (image_map_tool->label_group);

  filename = gimp_tool_info_build_options_filename (tool_info, ".settings");

  image_map_tool->settings_box = gimp_settings_box_new (tool_info->gimp,
                                                        image_map_tool->config,
                                                        klass->recent_settings,
                                                        filename);
  gtk_box_pack_start (GTK_BOX (hbox), image_map_tool->settings_box,
                      TRUE, TRUE, 0);
  gtk_widget_show (image_map_tool->settings_box);

  g_free (filename);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 GIMP_SETTINGS_BOX (image_map_tool->settings_box)->combo);

  g_signal_connect (image_map_tool->settings_box, "import",
                    G_CALLBACK (gimp_image_map_tool_import_activate),
                    image_map_tool);

  g_signal_connect (image_map_tool->settings_box, "export",
                    G_CALLBACK (gimp_image_map_tool_export_activate),
                    image_map_tool);

  return TRUE;
}

gboolean
gimp_image_map_tool_real_settings_import (GimpImageMapTool  *tool,
                                          const gchar       *filename,
                                          GError           **error)
{
  gboolean success;

  if (GIMP_TOOL (tool)->tool_info->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  success = gimp_config_deserialize_file (GIMP_CONFIG (tool->config),
                                          filename,
                                          NULL, error);

  return success;
}

gboolean
gimp_image_map_tool_real_settings_export (GimpImageMapTool  *tool,
                                          const gchar       *filename,
                                          GError           **error)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  gchar                 *header;
  gchar                 *footer;
  gboolean               success;

  header = g_strdup_printf ("GIMP %s tool settings",   klass->settings_name);
  footer = g_strdup_printf ("end of %s tool settings", klass->settings_name);

  if (GIMP_TOOL (tool)->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  success = gimp_config_serialize_to_file (GIMP_CONFIG (tool->config),
                                           filename,
                                           header, footer,
                                           NULL, error);

  g_free (header);
  g_free (footer);

  return success;
}


/*  private functions  */

static void
gimp_image_map_tool_import_activate (GtkWidget        *widget,
                                     GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->import_dialog_title, FALSE);
}

static void
gimp_image_map_tool_export_activate (GtkWidget        *widget,
                                     GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->export_dialog_title, TRUE);
}

static void
settings_dialog_response (GtkWidget        *dialog,
                          gint              response_id,
                          GimpImageMapTool *tool)
{
  gboolean save;

  save = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "save"));

  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (save)
        gimp_image_map_tool_settings_export (tool, filename);
      else
        gimp_image_map_tool_settings_import (tool, filename);

      g_free (filename);
    }

  if (save)
    gtk_widget_set_sensitive (GIMP_SETTINGS_BOX (tool->settings_box)->import_item, TRUE);
  else
    gtk_widget_set_sensitive (GIMP_SETTINGS_BOX (tool->settings_box)->export_item, TRUE);

  gtk_widget_destroy (dialog);
}

static void
gimp_image_map_tool_settings_dialog (GimpImageMapTool *tool,
                                     const gchar      *title,
                                     gboolean          save)
{
  GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);
  GtkFileChooser      *chooser;
  const gchar         *settings_name;
  gchar               *folder;

  settings_name = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->settings_name;

  g_return_if_fail (settings_name != NULL);

  if (tool->settings_dialog)
    {
      gtk_window_present (GTK_WINDOW (tool->settings_dialog));
      return;
    }

  if (save)
    gtk_widget_set_sensitive (GIMP_SETTINGS_BOX (tool->settings_box)->import_item, FALSE);
  else
    gtk_widget_set_sensitive (GIMP_SETTINGS_BOX (tool->settings_box)->export_item, FALSE);

  tool->settings_dialog =
    gtk_file_chooser_dialog_new (title, GTK_WINDOW (tool->shell),
                                 save ?
                                 GTK_FILE_CHOOSER_ACTION_SAVE :
                                 GTK_FILE_CHOOSER_ACTION_OPEN,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
                                 GTK_RESPONSE_OK,

                                 NULL);

  chooser = GTK_FILE_CHOOSER (tool->settings_dialog);

  g_object_set_data (G_OBJECT (chooser), "save", GINT_TO_POINTER (save));

  gtk_window_set_role (GTK_WINDOW (chooser), "gimp-import-export-settings");
  gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_MOUSE);

  g_object_add_weak_pointer (G_OBJECT (chooser),
                             (gpointer) &tool->settings_dialog);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (chooser), TRUE);

  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

  if (save)
    gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (settings_dialog_response),
                    tool);
  g_signal_connect (chooser, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  folder = g_build_filename (gimp_directory (), settings_name, NULL);

  if (g_file_test (folder, G_FILE_TEST_IS_DIR))
    {
      gtk_file_chooser_add_shortcut_folder (chooser, folder, NULL);
    }
  else
    {
      g_free (folder);
      folder = g_strdup (g_get_home_dir ());
    }

  if (options->settings)
    gtk_file_chooser_set_filename (chooser, options->settings);
  else
    gtk_file_chooser_set_current_folder (chooser, folder);

  g_free (folder);

  gimp_help_connect (tool->settings_dialog, gimp_standard_help_func,
                     GIMP_TOOL (tool)->tool_info->help_id, NULL);

  gtk_widget_show (tool->settings_dialog);
}

static gboolean
gimp_image_map_tool_settings_import (GimpImageMapTool *tool,
                                     const gchar      *filename)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GError                *error      = NULL;

  g_return_val_if_fail (tool_class->settings_import != NULL, FALSE);

  if (! tool_class->settings_import (tool, filename, &error))
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp, G_OBJECT (tool->shell),
                    GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      return FALSE;
    }

  gimp_image_map_tool_preview (tool);

  g_object_set (GIMP_TOOL_GET_OPTIONS (tool),
                "settings", filename,
                NULL);

  return TRUE;
}

static gboolean
gimp_image_map_tool_settings_export (GimpImageMapTool *tool,
                                     const gchar      *filename)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GError                *error      = NULL;
  gchar                 *display_name;

  g_return_val_if_fail (tool_class->settings_export != NULL, FALSE);

  if (! tool_class->settings_export (tool, filename, &error))
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp, G_OBJECT (tool->shell),
                    GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      return FALSE;
    }

  display_name = g_filename_display_name (filename);
  gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                G_OBJECT (GIMP_TOOL (tool)->display),
                GIMP_MESSAGE_INFO,
                _("Settings saved to '%s'"),
                display_name);
  g_free (display_name);

  g_object_set (GIMP_TOOL_GET_OPTIONS (tool),
                "settings", filename,
                NULL);

  return TRUE;
}
