/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingsbox.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpmarshal.h"

#include "gimpcontainercombobox.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpsettingsbox.h"
#include "gimpsettingseditor.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  FILE_DIALOG_SETUP,
  IMPORT,
  EXPORT,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GIMP,
  PROP_CONFIG,
  PROP_CONTAINER,
  PROP_FILENAME
};


static void      gimp_settings_box_constructed   (GObject           *object);
static void      gimp_settings_box_finalize      (GObject           *object);
static void      gimp_settings_box_set_property  (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void      gimp_settings_box_get_property  (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static void      gimp_settings_box_deserialize   (GimpSettingsBox   *box);
static void      gimp_settings_box_serialize     (GimpSettingsBox   *box);
static GtkWidget *
                 gimp_settings_box_menu_item_add (GimpSettingsBox   *box,
                                                  const gchar       *stock_id,
                                                  const gchar       *label,
                                                  GCallback          callback);
static gboolean
            gimp_settings_box_row_separator_func (GtkTreeModel      *model,
                                                  GtkTreeIter       *iter,
                                                  gpointer           data);
static void   gimp_settings_box_setting_selected (GimpContainerView *view,
                                                  GimpViewable      *object,
                                                  gpointer           insert_data,
                                                  GimpSettingsBox   *box);
static gboolean gimp_settings_box_menu_press     (GtkWidget         *widget,
                                                  GdkEventButton    *bevent,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_favorite_activate (GtkWidget         *widget,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_import_activate   (GtkWidget         *widget,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_export_activate   (GtkWidget         *widget,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_manage_activate   (GtkWidget         *widget,
                                                  GimpSettingsBox   *box);

static void  gimp_settings_box_favorite_callback (GtkWidget         *query_box,
                                                  const gchar       *string,
                                                  gpointer           data);
static void  gimp_settings_box_file_dialog       (GimpSettingsBox   *box,
                                                  const gchar       *title,
                                                  gboolean           save);
static void  gimp_settings_box_file_response     (GtkWidget         *dialog,
                                                  gint               response_id,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_manage_response   (GtkWidget         *widget,
                                                  gint               response_id,
                                                  GimpSettingsBox   *box);
static void  gimp_settings_box_toplevel_unmap    (GtkWidget         *toplevel,
                                                  GtkWidget         *dialog);
static void  gimp_settings_box_truncate_list     (GimpSettingsBox   *box,
                                                  gint               max_recent);


G_DEFINE_TYPE (GimpSettingsBox, gimp_settings_box, GTK_TYPE_BOX)

#define parent_class gimp_settings_box_parent_class

static guint settings_box_signals[LAST_SIGNAL] = { 0 };


static void
gimp_settings_box_class_init (GimpSettingsBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  settings_box_signals[FILE_DIALOG_SETUP] =
    g_signal_new ("file-dialog-setup",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpSettingsBoxClass, file_dialog_setup),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  GTK_TYPE_FILE_CHOOSER_DIALOG,
                  G_TYPE_BOOLEAN);

  settings_box_signals[IMPORT] =
    g_signal_new ("import",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpSettingsBoxClass, import),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__STRING,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_STRING);

  settings_box_signals[EXPORT] =
    g_signal_new ("export",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpSettingsBoxClass, export),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__STRING,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_STRING);

  object_class->constructed  = gimp_settings_box_constructed;
  object_class->finalize     = gimp_settings_box_finalize;
  object_class->set_property = gimp_settings_box_set_property;
  object_class->get_property = gimp_settings_box_get_property;

  klass->file_dialog_setup   = NULL;
  klass->import              = NULL;
  klass->export              = NULL;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONFIG,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FILENAME,
                                   g_param_spec_string ("filename",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_settings_box_init (GimpSettingsBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (box), 6);
}

static void
gimp_settings_box_constructed (GObject *object)
{
  GimpSettingsBox *box = GIMP_SETTINGS_BOX (object);
  GtkWidget       *hbox2;
  GtkWidget       *button;
  GtkWidget       *image;
  GtkWidget       *arrow;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (box->gimp));
  g_assert (GIMP_IS_CONFIG (box->config));
  g_assert (GIMP_IS_CONTAINER (box->container));
  g_assert (box->filename != NULL);

  if (gimp_container_get_n_children (box->container) == 0)
    gimp_settings_box_deserialize (box);

  box->combo = gimp_container_combo_box_new (box->container,
                                             gimp_get_user_context (box->gimp),
                                             16, 0);
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (box->combo),
                                        gimp_settings_box_row_separator_func,
                                        NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), box->combo, TRUE, TRUE, 0);
  gtk_widget_show (box->combo);

  gimp_help_set_help_data (box->combo, _("Pick a setting from the list"),
                           NULL);

  g_signal_connect_after (box->combo, "select-item",
                          G_CALLBACK (gimp_settings_box_setting_selected),
                          box);

  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (button, _("Add settings to favorites"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_settings_box_favorite_activate),
                    box);

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  arrow = gtk_image_new_from_stock (GIMP_STOCK_MENU_LEFT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (gimp_settings_box_menu_press),
                    box);

  /*  Favorites menu  */

  box->menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (box->menu), button, NULL);

  box->import_item =
    gimp_settings_box_menu_item_add (box,
                                     GTK_STOCK_OPEN,
                                     _("_Import Settings from File..."),
                                     G_CALLBACK (gimp_settings_box_import_activate));

  box->export_item =
    gimp_settings_box_menu_item_add (box,
                                     GTK_STOCK_SAVE,
                                     _("_Export Settings to File..."),
                                     G_CALLBACK (gimp_settings_box_export_activate));

  gimp_settings_box_menu_item_add (box, NULL, NULL, NULL);

  gimp_settings_box_menu_item_add (box,
                                   GTK_STOCK_EDIT,
                                   _("_Manage Settings..."),
                                   G_CALLBACK (gimp_settings_box_manage_activate));
}

static void
gimp_settings_box_finalize (GObject *object)
{
  GimpSettingsBox *box = GIMP_SETTINGS_BOX (object);

  if (box->config)
    {
      g_object_unref (box->config);
      box->config = NULL;
    }

  if (box->container)
    {
      g_object_unref (box->container);
      box->container = NULL;
    }

  if (box->filename)
    {
      g_free (box->filename);
      box->filename = NULL;
    }

  g_free (box->import_dialog_title);
  g_free (box->export_dialog_title);
  g_free (box->file_dialog_help_id);
  g_free (box->default_folder);
  g_free (box->last_filename);

  if (box->editor_dialog)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (toplevel)
        g_signal_handlers_disconnect_by_func (toplevel,
                                              gimp_settings_box_toplevel_unmap,
                                              box->editor_dialog);

      gtk_widget_destroy (box->editor_dialog);
    }

  if (box->file_dialog)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (toplevel)
        g_signal_handlers_disconnect_by_func (toplevel,
                                              gimp_settings_box_toplevel_unmap,
                                              box->file_dialog);

      gtk_widget_destroy (box->file_dialog);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_settings_box_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpSettingsBox *box = GIMP_SETTINGS_BOX (object);

  switch (property_id)
    {
    case PROP_GIMP:
      box->gimp = g_value_get_object (value); /* don't dup */
      break;

    case PROP_CONFIG:
      box->config = g_value_dup_object (value);
      break;

    case PROP_CONTAINER:
      box->container = g_value_dup_object (value);
      break;

    case PROP_FILENAME:
      box->filename = g_value_dup_string (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_settings_box_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpSettingsBox *box = GIMP_SETTINGS_BOX (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, box->gimp);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, box->config);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, box->container);
      break;

    case PROP_FILENAME:
      g_value_set_string (value, box->filename);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_settings_box_separator_add (GimpContainer *container)
{
  GimpObject *sep = g_object_new (gimp_container_get_children_type (container), NULL);

  gimp_container_add (container, sep);
  g_object_unref (sep);

  g_object_set_data (G_OBJECT (container), "separator", sep);
}

static void
gimp_settings_box_separator_remove (GimpContainer *container)
{
  GimpObject *sep = g_object_get_data (G_OBJECT (container), "separator");

  gimp_container_remove (container, sep);

  g_object_set_data (G_OBJECT (container), "separator", NULL);
}

static void
gimp_settings_box_deserialize (GimpSettingsBox *box)
{
  GError *error = NULL;

  if (box->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (box->filename));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (box->container),
                                      box->filename,
                                      NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message_literal (box->gimp, NULL, GIMP_MESSAGE_ERROR,
			      error->message);

      g_clear_error (&error);
    }

  gimp_settings_box_separator_add (box->container);
}

static void
gimp_settings_box_serialize (GimpSettingsBox *box)
{
  GError *error = NULL;

  gimp_settings_box_separator_remove (box->container);

  if (box->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (box->filename));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (box->container),
                                       box->filename,
                                       "settings",
                                       "end of settings",
                                       NULL, &error))
    {
      gimp_message_literal (box->gimp, NULL, GIMP_MESSAGE_ERROR,
			    error->message);
      g_clear_error (&error);
    }

  gimp_settings_box_separator_add (box->container);
}

static GtkWidget *
gimp_settings_box_menu_item_add (GimpSettingsBox *box,
                                 const gchar     *stock_id,
                                 const gchar     *label,
                                 GCallback        callback)
{
  GtkWidget *item;

  if (label)
    {
      GtkWidget *image;

      item = gtk_image_menu_item_new_with_mnemonic (label);
      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);

      g_signal_connect (item, "activate",
                        callback,
                        box);
    }
  else
    {
      item = gtk_separator_menu_item_new ();
    }

  gtk_menu_shell_append (GTK_MENU_SHELL (box->menu), item);
  gtk_widget_show (item);

  return item;
}

static gboolean
gimp_settings_box_row_separator_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      data)
{
  gchar *name = NULL;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, &name,
                      -1);
  g_free (name);

  return name == NULL;
}

static void
gimp_settings_box_setting_selected (GimpContainerView *view,
                                    GimpViewable      *object,
                                    gpointer           insert_data,
                                    GimpSettingsBox   *box)
{
  if (object)
    {
      gimp_config_copy (GIMP_CONFIG (object),
                        GIMP_CONFIG (box->config), 0);

      gimp_container_view_select_item (view, NULL);
    }
}

static void
gimp_settings_box_menu_position (GtkMenu  *menu,
                                 gint     *x,
                                 gint     *y,
                                 gboolean *push_in,
                                 gpointer  user_data)
{
  gimp_button_menu_position (user_data, menu, GTK_POS_LEFT, x, y);
}

static gboolean
gimp_settings_box_menu_press (GtkWidget       *widget,
                              GdkEventButton  *bevent,
                              GimpSettingsBox *box)
{
  if (bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_menu_popup (GTK_MENU (box->menu),
                      NULL, NULL,
                      gimp_settings_box_menu_position, widget,
                      bevent->button, bevent->time);
    }

  return TRUE;
}

static void
gimp_settings_box_favorite_activate (GtkWidget       *widget,
                                     GimpSettingsBox *box)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  GtkWidget *dialog;

  dialog = gimp_query_string_box (_("Add Settings to Favorites"),
                                  toplevel,
                                  gimp_standard_help_func, NULL,
                                  _("Enter a name for the settings"),
                                  _("Saved Settings"),
                                  G_OBJECT (toplevel), "hide",
                                  gimp_settings_box_favorite_callback, box);
  gtk_widget_show (dialog);
}

static void
gimp_settings_box_import_activate (GtkWidget       *widget,
                                   GimpSettingsBox *box)
{
  gimp_settings_box_file_dialog (box, box->import_dialog_title, FALSE);
}

static void
gimp_settings_box_export_activate (GtkWidget       *widget,
                                   GimpSettingsBox *box)
{
  gimp_settings_box_file_dialog (box, box->export_dialog_title, TRUE);
}

static void
gimp_settings_box_manage_activate (GtkWidget       *widget,
                                   GimpSettingsBox *box)
{
  GtkWidget *toplevel;
  GtkWidget *editor;
  GtkWidget *content_area;

  if (box->editor_dialog)
    {
      gtk_window_present (GTK_WINDOW (box->editor_dialog));
      return;
    }

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  box->editor_dialog = gimp_dialog_new (_("Manage Saved Settings"),
                                        "gimp-settings-editor-dialog",
                                        toplevel, 0,
                                        NULL, NULL,
                                        GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE,
                                        NULL);

  g_object_add_weak_pointer (G_OBJECT (box->editor_dialog),
                             (gpointer) &box->editor_dialog);
  g_signal_connect (toplevel, "unmap",
                    G_CALLBACK (gimp_settings_box_toplevel_unmap),
                    box->editor_dialog);

  g_signal_connect (box->editor_dialog, "response",
                    G_CALLBACK (gimp_settings_box_manage_response),
                    box);

  editor = gimp_settings_editor_new (box->gimp,
                                     box->config,
                                     box->container);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (box->editor_dialog));
  gtk_box_pack_start (GTK_BOX (content_area), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  gtk_widget_show (box->editor_dialog);
}

static void
gimp_settings_box_favorite_callback (GtkWidget   *query_box,
                                     const gchar *string,
                                     gpointer     data)
{
  GimpSettingsBox *box = GIMP_SETTINGS_BOX (data);
  GimpConfig      *config;

  config = gimp_config_duplicate (GIMP_CONFIG (box->config));
  gimp_object_set_name (GIMP_OBJECT (config), string);
  gimp_container_add (box->container, GIMP_OBJECT (config));
  g_object_unref (config);

  gimp_settings_box_serialize (box);
}

static void
gimp_settings_box_file_dialog (GimpSettingsBox *box,
                               const gchar     *title,
                               gboolean         save)
{
  GtkWidget *toplevel;
  GtkWidget *dialog;

  if (box->file_dialog)
    {
      gtk_window_present (GTK_WINDOW (box->file_dialog));
      return;
    }

  if (save)
    gtk_widget_set_sensitive (box->import_item, FALSE);
  else
    gtk_widget_set_sensitive (box->export_item, FALSE);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  box->file_dialog = dialog =
    gtk_file_chooser_dialog_new (title, GTK_WINDOW (toplevel),
                                 save ?
                                 GTK_FILE_CHOOSER_ACTION_SAVE :
                                 GTK_FILE_CHOOSER_ACTION_OPEN,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
                                 GTK_RESPONSE_OK,

                                 NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_set_data (G_OBJECT (dialog), "save", GINT_TO_POINTER (save));

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-import-export-settings");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &box->file_dialog);
  g_signal_connect (toplevel, "unmap",
                    G_CALLBACK (gimp_settings_box_toplevel_unmap),
                    dialog);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (save)
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                    TRUE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_settings_box_file_response),
                    box);
  g_signal_connect (dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  if (box->default_folder &&
      g_file_test (box->default_folder, G_FILE_TEST_IS_DIR))
    {
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                            box->default_folder, NULL);

      if (! box->last_filename)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                             box->default_folder);
    }
  else if (! box->last_filename)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                           g_get_home_dir ());
    }

  if (box->last_filename)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
                                   box->last_filename);

  gimp_help_connect (box->file_dialog, gimp_standard_help_func,
                     box->file_dialog_help_id, NULL);

  /*  allow callbacks to add widgets to the dialog  */
  g_signal_emit (box, settings_box_signals[FILE_DIALOG_SETUP], 0,
                 box->file_dialog, save);

  gtk_widget_show (box->file_dialog);
}

static void
gimp_settings_box_file_response (GtkWidget       *dialog,
                                 gint             response_id,
                                 GimpSettingsBox *box)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));
  gboolean   save;

  if (toplevel)
    g_signal_handlers_disconnect_by_func (toplevel,
                                          gimp_settings_box_toplevel_unmap,
                                          dialog);

  save = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "save"));

  if (response_id == GTK_RESPONSE_OK)
    {
      gchar    *filename;
      gboolean  success = FALSE;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (save)
        g_signal_emit (box, settings_box_signals[EXPORT], 0, filename,
                       &success);
      else
        g_signal_emit (box, settings_box_signals[IMPORT], 0, filename,
                       &success);

      if (success)
        {
          g_free (box->last_filename);
          box->last_filename = g_strdup (filename);
        }

      g_free (filename);
    }

  if (save)
    gtk_widget_set_sensitive (box->import_item, TRUE);
  else
    gtk_widget_set_sensitive (box->export_item, TRUE);

  gtk_widget_destroy (dialog);
}

static void
gimp_settings_box_manage_response (GtkWidget       *dialog,
                                   gint             response_id,
                                   GimpSettingsBox *box)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  if (toplevel)
    g_signal_handlers_disconnect_by_func (toplevel,
                                          gimp_settings_box_toplevel_unmap,
                                          dialog);

  gtk_widget_destroy (dialog);
}

static void
gimp_settings_box_toplevel_unmap (GtkWidget *toplevel,
                                  GtkWidget *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_DELETE_EVENT);
}

static void
gimp_settings_box_truncate_list (GimpSettingsBox *box,
                                 gint             max_recent)
{
  GList *list;
  gint   n_recent = 0;

  list = GIMP_LIST (box->container)->list;
  while (list)
    {
      GimpConfig *config = list->data;
      guint       t;

      list = g_list_next (list);

      g_object_get (config,
                    "time", &t,
                    NULL);

      if (t > 0)
        {
          n_recent++;

          if (n_recent > max_recent)
            gimp_container_remove (box->container, GIMP_OBJECT (config));
        }
      else
        {
          break;
        }
    }
}


/*  public functions  */

GtkWidget *
gimp_settings_box_new (Gimp          *gimp,
                       GObject       *config,
                       GimpContainer *container,
                       const gchar   *filename,
                       const gchar   *import_dialog_title,
                       const gchar   *export_dialog_title,
                       const gchar   *file_dialog_help_id,
                       const gchar   *default_folder,
                       const gchar   *last_filename)
{
  GimpSettingsBox *box;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (filename != NULL, NULL);

  box = g_object_new (GIMP_TYPE_SETTINGS_BOX,
                      "gimp",      gimp,
                      "config",    config,
                      "container", container,
                      "filename",  filename,
                      NULL);

  box->import_dialog_title = g_strdup (import_dialog_title);
  box->export_dialog_title = g_strdup (export_dialog_title);
  box->file_dialog_help_id = g_strdup (file_dialog_help_id);
  box->default_folder      = g_strdup (default_folder);
  box->last_filename       = g_strdup (last_filename);

  return GTK_WIDGET (box);
}

void
gimp_settings_box_add_current (GimpSettingsBox *box,
                               gint             max_recent)
{
  GimpConfig *config = NULL;
  GList      *list;

  g_return_if_fail (GIMP_IS_SETTINGS_BOX (box));

  for (list = GIMP_LIST (box->container)->list; list; list = g_list_next (list))
    {
      guint t;

      config = list->data;

      g_object_get (config,
                    "time", &t,
                    NULL);

      if (t > 0 && gimp_config_is_equal_to (config, GIMP_CONFIG (box->config)))
        {
          g_object_set (config,
                        "time", (guint) time (NULL),
                        NULL);
          break;
        }
    }

  if (! list)
    {
      config = gimp_config_duplicate (GIMP_CONFIG (box->config));
      g_object_set (config,
                    "time", (guint) time (NULL),
                    NULL);

      gimp_container_insert (box->container, GIMP_OBJECT (config), 0);
      g_object_unref (config);
    }

  gimp_settings_box_truncate_list (box, max_recent);

  gimp_settings_box_serialize (box);
}
