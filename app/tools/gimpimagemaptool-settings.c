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
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"

#include "gimpimagemapoptions.h"
#include "gimpimagemaptool.h"
#include "gimpimagemaptool-settings.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_image_map_tool_recent_selected  (GimpContainerView *view,
                                                    GimpViewable      *object,
                                                    gpointer           insert_data,
                                                    GimpImageMapTool  *tool);

static void   gimp_image_map_tool_favorite_clicked (GtkWidget         *widget,
                                                    GimpImageMapTool  *tool);
static void   gimp_image_map_tool_load_clicked     (GtkWidget         *widget,
                                                    GimpImageMapTool  *tool);
static void   gimp_image_map_tool_save_clicked     (GtkWidget         *widget,
                                                    GimpImageMapTool  *tool);

static void   gimp_image_map_tool_settings_dialog  (GimpImageMapTool  *im_tool,
                                                    const gchar       *title,
                                                    gboolean           save);

static void  gimp_image_map_tool_favorite_callback (GtkWidget         *query_box,
                                                    const gchar       *string,
                                                    gpointer           data);
static gboolean gimp_image_map_tool_settings_load  (GimpImageMapTool  *tool,
                                                    const gchar       *filename);
static gboolean gimp_image_map_tool_settings_save  (GimpImageMapTool  *tool,
                                                    const gchar       *filename);


/*  public functions  */

gboolean
gimp_image_map_tool_add_settings_gui (GimpImageMapTool *image_map_tool)
{
  GimpImageMapToolClass *klass;
  GimpToolInfo          *tool_info;
  GtkWidget             *hbox;
  GtkWidget             *label;
  GtkWidget             *combo;
  GtkWidget             *button;

  klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

  tool_info = GIMP_TOOL (image_map_tool)->tool_info;

  if (gimp_container_num_children (klass->recent_settings) == 0)
    {
      gchar  *filename;
      GError *error = NULL;

      filename = gimp_tool_info_build_options_filename (tool_info,
                                                        ".settings");

      if (tool_info->gimp->be_verbose)
        g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

      if (! gimp_config_deserialize_file (GIMP_CONFIG (klass->recent_settings),
                                          filename,
                                          NULL, &error))
        {
          if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
            gimp_message (tool_info->gimp, NULL, GIMP_MESSAGE_ERROR,
                          "%s", error->message);

          g_clear_error (&error);
        }

      gimp_list_reverse (GIMP_LIST (klass->recent_settings));

      g_free (filename);
    }

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Recent Settings:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_container_combo_box_new (klass->recent_settings,
                                        GIMP_CONTEXT (tool_info->tool_options),
                                        16, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_help_set_help_data (combo, _("Pick a setting from the list"),
                           NULL);

  g_signal_connect_after (combo, "select-item",
                          G_CALLBACK (gimp_image_map_tool_recent_selected),
                          image_map_tool);

  /*  The load/save hbox  */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("Save to _Favorites"));
  gtk_box_pack_start (GTK_BOX (hbox), button,
                      FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_image_map_tool_favorite_clicked),
                    image_map_tool);

  image_map_tool->load_button = g_object_new (GIMP_TYPE_BUTTON,
                                              "label",         GTK_STOCK_OPEN,
                                              "use-stock",     TRUE,
                                              "use-underline", TRUE,
                                              NULL);
  gtk_box_pack_start (GTK_BOX (hbox), image_map_tool->load_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->load_button);

  g_signal_connect (image_map_tool->load_button, "clicked",
                    G_CALLBACK (gimp_image_map_tool_load_clicked),
                    image_map_tool);

  if (klass->load_button_tip)
    gimp_help_set_help_data (image_map_tool->load_button,
                             klass->load_button_tip, NULL);

  image_map_tool->save_button = g_object_new (GIMP_TYPE_BUTTON,
                                              "label",         GTK_STOCK_SAVE,
                                              "use-stock",     TRUE,
                                              "use-underline", TRUE,
                                              NULL);
  gtk_box_pack_start (GTK_BOX (hbox), image_map_tool->save_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->save_button);

  g_signal_connect (image_map_tool->save_button, "clicked",
                    G_CALLBACK (gimp_image_map_tool_save_clicked),
                    image_map_tool);

  if (klass->save_button_tip)
    gimp_help_set_help_data (image_map_tool->save_button,
                             klass->save_button_tip, NULL);

  return TRUE;
}

void
gimp_image_map_tool_add_recent_settings (GimpImageMapTool *image_map_tool)
{
  GimpTool      *tool   = GIMP_TOOL (image_map_tool);
  GimpContainer *recent;
  GimpConfig    *current;
  GimpConfig    *config = NULL;
  GList         *list;
  time_t         now;
  struct tm      tm;
  gchar          buf[64];
  gchar         *name;
  gchar         *filename;
  GError        *error = NULL;

  recent  = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool)->recent_settings;
  current = GIMP_CONFIG (image_map_tool->config);

  for (list = GIMP_LIST (recent)->list; list; list = g_list_next (list))
    {
      config = list->data;

      if (gimp_config_is_equal_to (config, current))
        {
          gimp_container_reorder (recent, GIMP_OBJECT (config), 0);
          break;
        }

      config = NULL;
    }

  if (! config)
    {
      config = gimp_config_duplicate (current);
      gimp_container_insert (recent, GIMP_OBJECT (config), 0);
      g_object_unref (config);
    }

  now = time (NULL);
  tm = *localtime (&now);
  strftime (buf, sizeof (buf), "%Y-%m-%d %T", &tm);

  name = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  gimp_object_set_name (GIMP_OBJECT (config), name);
  g_free (name);

  filename = gimp_tool_info_build_options_filename (tool->tool_info,
                                                    ".settings");

  if (tool->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (recent),
                                       filename,
                                       "tool settings",
                                       "end of tool settings",
                                       NULL, &error))
    {
      gimp_message (tool->tool_info->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
    }

  g_free (filename);
}

gboolean
gimp_image_map_tool_real_settings_load (GimpImageMapTool *tool,
                                        const gchar      *filename,
                                        GError          **error)
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
gimp_image_map_tool_real_settings_save (GimpImageMapTool *tool,
                                        const gchar      *filename,
                                        GError          **error)
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
gimp_image_map_tool_recent_selected (GimpContainerView *view,
                                     GimpViewable      *object,
                                     gpointer           insert_data,
                                     GimpImageMapTool  *tool)
{
  if (object)
    {
      gimp_config_copy (GIMP_CONFIG (object),
                        GIMP_CONFIG (tool->config), 0);

      gimp_container_view_select_item (view, NULL);
    }
}

static void
gimp_image_map_tool_favorite_clicked (GtkWidget        *widget,
                                      GimpImageMapTool *tool)
{
  GtkWidget *dialog;

  dialog = gimp_query_string_box (_("Save Settings to Favorites"),
                                  tool->shell,
                                  gimp_standard_help_func,
                                  NULL, /*GIMP_HELP_TOOL_OPTIONS_DIALOG, */
                                  _("Enter a name for the saved settings"),
                                  _("Saved Settings"),
                                  G_OBJECT (tool->shell), "hide",
                                  gimp_image_map_tool_favorite_callback, tool);
  gtk_widget_show (dialog);
}

static void
gimp_image_map_tool_load_clicked (GtkWidget        *widget,
                                  GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->load_dialog_title, FALSE);
}

static void
gimp_image_map_tool_save_clicked (GtkWidget        *widget,
                                  GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->save_dialog_title, TRUE);
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
        gimp_image_map_tool_settings_save (tool, filename);
      else
        gimp_image_map_tool_settings_load (tool, filename);

      g_free (filename);
    }

  if (save)
    gtk_widget_set_sensitive (tool->load_button, TRUE);
  else
    gtk_widget_set_sensitive (tool->save_button, TRUE);

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
    gtk_widget_set_sensitive (tool->load_button, FALSE);
  else
    gtk_widget_set_sensitive (tool->save_button, FALSE);

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

  gtk_window_set_role (GTK_WINDOW (chooser), "gimp-load-save-settings");
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

static void
gimp_image_map_tool_favorite_callback (GtkWidget   *query_box,
                                       const gchar *string,
                                       gpointer     data)
{
  GimpImageMapTool *tool = GIMP_IMAGE_MAP_TOOL (data);
  GimpConfig       *config;

  config = gimp_config_duplicate (GIMP_CONFIG (tool->config));
  gimp_object_set_name (GIMP_OBJECT (config), string);
  gimp_container_insert (GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->recent_settings,
                         GIMP_OBJECT (config), 0);
  g_object_unref (config);
}

static gboolean
gimp_image_map_tool_settings_load (GimpImageMapTool *tool,
                                   const gchar      *filename)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GError                *error      = NULL;

  g_return_val_if_fail (tool_class->settings_load != NULL, FALSE);

  if (! tool_class->settings_load (tool, filename, &error))
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
gimp_image_map_tool_settings_save (GimpImageMapTool *tool,
                                   const gchar      *filename)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);
  GError                *error      = NULL;
  gchar                 *display_name;

  g_return_val_if_fail (tool_class->settings_save != NULL, FALSE);

  if (! tool_class->settings_save (tool, filename, &error))
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
