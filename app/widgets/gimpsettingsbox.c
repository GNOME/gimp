/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasettingsbox.c
 * Copyright (C) 2008-2017 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "operations/ligma-operation-config.h"

#include "core/ligma.h"
#include "core/ligmalist.h"
#include "core/ligmamarshal.h"

#include "ligmacontainercombobox.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmasettingsbox.h"
#include "ligmasettingseditor.h"

#include "ligma-intl.h"


enum
{
  FILE_DIALOG_SETUP,
  IMPORT,
  EXPORT,
  SELECTED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_CONFIG,
  PROP_CONTAINER,
  PROP_HELP_ID,
  PROP_IMPORT_TITLE,
  PROP_EXPORT_TITLE,
  PROP_DEFAULT_FOLDER,
  PROP_LAST_FILE
};


typedef struct _LigmaSettingsBoxPrivate LigmaSettingsBoxPrivate;

struct _LigmaSettingsBoxPrivate
{
  GtkWidget     *combo;
  GtkWidget     *menu;
  GtkWidget     *import_item;
  GtkWidget     *export_item;
  GtkWidget     *file_dialog;
  GtkWidget     *editor_dialog;

  Ligma          *ligma;
  GObject       *config;
  LigmaContainer *container;

  gchar         *help_id;
  gchar         *import_title;
  gchar         *export_title;
  GFile         *default_folder;
  GFile         *last_file;
};

#define GET_PRIVATE(item) ((LigmaSettingsBoxPrivate *) ligma_settings_box_get_instance_private ((LigmaSettingsBox *) (item)))


static void      ligma_settings_box_constructed   (GObject           *object);
static void      ligma_settings_box_finalize      (GObject           *object);
static void      ligma_settings_box_set_property  (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void      ligma_settings_box_get_property  (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static GtkWidget *
                 ligma_settings_box_menu_item_add (LigmaSettingsBox   *box,
                                                  const gchar       *label,
                                                  GCallback          callback);
static gboolean
            ligma_settings_box_row_separator_func (GtkTreeModel      *model,
                                                  GtkTreeIter       *iter,
                                                  gpointer           data);
static gboolean
              ligma_settings_box_setting_selected (LigmaContainerView *view,
                                                  GList             *objects,
                                                  GList             *paths,
                                                  LigmaSettingsBox   *box);
static gboolean ligma_settings_box_menu_press     (GtkWidget         *widget,
                                                  GdkEventButton    *bevent,
                                                  LigmaSettingsBox   *box);
static void  ligma_settings_box_favorite_activate (GtkWidget         *widget,
                                                  LigmaSettingsBox   *box);
static void  ligma_settings_box_import_activate   (GtkWidget         *widget,
                                                  LigmaSettingsBox   *box);
static void  ligma_settings_box_export_activate   (GtkWidget         *widget,
                                                  LigmaSettingsBox   *box);
static void  ligma_settings_box_manage_activate   (GtkWidget         *widget,
                                                  LigmaSettingsBox   *box);

static void  ligma_settings_box_favorite_callback (GtkWidget         *query_box,
                                                  const gchar       *string,
                                                  gpointer           data);
static void  ligma_settings_box_file_dialog       (LigmaSettingsBox   *box,
                                                  const gchar       *title,
                                                  gboolean           save);
static void  ligma_settings_box_file_response     (GtkWidget         *dialog,
                                                  gint               response_id,
                                                  LigmaSettingsBox   *box);
static void  ligma_settings_box_toplevel_unmap    (GtkWidget         *toplevel,
                                                  GtkWidget         *dialog);
static void  ligma_settings_box_truncate_list     (LigmaSettingsBox   *box,
                                                  gint               max_recent);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaSettingsBox, ligma_settings_box, GTK_TYPE_BOX)

#define parent_class ligma_settings_box_parent_class

static guint settings_box_signals[LAST_SIGNAL] = { 0 };


static void
ligma_settings_box_class_init (LigmaSettingsBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  settings_box_signals[FILE_DIALOG_SETUP] =
    g_signal_new ("file-dialog-setup",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaSettingsBoxClass, file_dialog_setup),
                  NULL, NULL,
                  ligma_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  GTK_TYPE_FILE_CHOOSER_DIALOG,
                  G_TYPE_BOOLEAN);

  settings_box_signals[IMPORT] =
    g_signal_new ("import",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaSettingsBoxClass, import),
                  NULL, NULL,
                  ligma_marshal_BOOLEAN__OBJECT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_FILE);

  settings_box_signals[EXPORT] =
    g_signal_new ("export",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaSettingsBoxClass, export),
                  NULL, NULL,
                  ligma_marshal_BOOLEAN__OBJECT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_FILE);

  settings_box_signals[SELECTED] =
    g_signal_new ("selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaSettingsBoxClass, selected),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_CONFIG);

  object_class->constructed  = ligma_settings_box_constructed;
  object_class->finalize     = ligma_settings_box_finalize;
  object_class->set_property = ligma_settings_box_set_property;
  object_class->get_property = ligma_settings_box_get_property;

  klass->file_dialog_setup   = NULL;
  klass->import              = NULL;
  klass->export              = NULL;
  klass->selected            = NULL;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONFIG,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HELP_ID,
                                   g_param_spec_string ("help-id",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_IMPORT_TITLE,
                                   g_param_spec_string ("import-title",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EXPORT_TITLE,
                                   g_param_spec_string ("export-title",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DEFAULT_FOLDER,
                                   g_param_spec_object ("default-folder",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LAST_FILE,
                                   g_param_spec_object ("last-file",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_settings_box_init (LigmaSettingsBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (box), 6);
}

static void
ligma_settings_box_constructed (GObject *object)
{
  LigmaSettingsBox        *box     = LIGMA_SETTINGS_BOX (object);
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (object);
  GtkWidget              *hbox2;
  GtkWidget              *button;
  GtkWidget              *image;
  GtkWidget              *arrow;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (private->ligma));
  ligma_assert (LIGMA_IS_CONFIG (private->config));
  ligma_assert (LIGMA_IS_CONTAINER (private->container));

  private->combo = ligma_container_combo_box_new (private->container,
                                                 ligma_get_user_context (private->ligma),
                                                 16, 0);
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (private->combo),
                                        ligma_settings_box_row_separator_func,
                                        NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), private->combo, TRUE, TRUE, 0);
  gtk_widget_show (private->combo);

  ligma_help_set_help_data (private->combo, _("Pick a preset from the list"),
                           NULL);

  g_signal_connect_after (private->combo, "select-items",
                          G_CALLBACK (ligma_settings_box_setting_selected),
                          box);

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox2), TRUE);
  gtk_box_pack_start (GTK_BOX (box), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_LIST_ADD,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  ligma_help_set_help_data (button,
                           _("Save the current settings as named preset"),
                           NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_settings_box_favorite_activate),
                    box);

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  arrow = gtk_image_new_from_icon_name (LIGMA_ICON_MENU_LEFT,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  ligma_help_set_help_data (button, _("Manage presets"), NULL);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (ligma_settings_box_menu_press),
                    box);

  /*  Favorites menu  */

  private->menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (private->menu), button, NULL);

  private->import_item =
    ligma_settings_box_menu_item_add (box,
                                     _("_Import Current Settings from File..."),
                                     G_CALLBACK (ligma_settings_box_import_activate));

  private->export_item =
    ligma_settings_box_menu_item_add (box,
                                     _("_Export Current Settings to File..."),
                                     G_CALLBACK (ligma_settings_box_export_activate));

  ligma_settings_box_menu_item_add (box, NULL, NULL);

  ligma_settings_box_menu_item_add (box,
                                   _("_Manage Saved Presets..."),
                                   G_CALLBACK (ligma_settings_box_manage_activate));
}

static void
ligma_settings_box_finalize (GObject *object)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->config);
  g_clear_object (&private->container);
  g_clear_object (&private->last_file);
  g_clear_object (&private->default_folder);

  g_free (private->help_id);
  g_free (private->import_title);
  g_free (private->export_title);

  if (private->editor_dialog)
    gtk_widget_destroy (private->editor_dialog);

  if (private->file_dialog)
    gtk_widget_destroy (private->file_dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_settings_box_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      private->ligma = g_value_get_object (value); /* don't dup */
      break;

    case PROP_CONFIG:
      if (private->config)
        g_object_unref (private->config);
      private->config = g_value_dup_object (value);
      break;

    case PROP_CONTAINER:
      if (private->editor_dialog)
        gtk_dialog_response (GTK_DIALOG (private->editor_dialog),
                             GTK_RESPONSE_DELETE_EVENT);
      if (private->file_dialog)
        gtk_dialog_response (GTK_DIALOG (private->file_dialog),
                             GTK_RESPONSE_DELETE_EVENT);
      if (private->container)
        g_object_unref (private->container);
      private->container = g_value_dup_object (value);
      if (private->combo)
        ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (private->combo),
                                           private->container);
      break;

    case PROP_HELP_ID:
      g_free (private->help_id);
      private->help_id = g_value_dup_string (value);
      break;

    case PROP_IMPORT_TITLE:
      g_free (private->import_title);
      private->import_title = g_value_dup_string (value);
      break;

    case PROP_EXPORT_TITLE:
      g_free (private->export_title);
      private->export_title = g_value_dup_string (value);
      break;

    case PROP_DEFAULT_FOLDER:
      if (private->default_folder)
        g_object_unref (private->default_folder);
      private->default_folder = g_value_dup_object (value);
      break;

    case PROP_LAST_FILE:
      if (private->last_file)
        g_object_unref (private->last_file);
      private->last_file = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_settings_box_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, private->ligma);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, private->config);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, private->container);
      break;

    case PROP_HELP_ID:
      g_value_set_string (value, private->help_id);
      break;

    case PROP_IMPORT_TITLE:
      g_value_set_string (value, private->import_title);
      break;

    case PROP_EXPORT_TITLE:
      g_value_set_string (value, private->export_title);
      break;

    case PROP_DEFAULT_FOLDER:
      g_value_set_object (value, private->default_folder);
      break;

    case PROP_LAST_FILE:
      g_value_set_object (value, private->last_file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
ligma_settings_box_menu_item_add (LigmaSettingsBox *box,
                                 const gchar     *label,
                                 GCallback        callback)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  GtkWidget              *item;

  if (label)
    {
      item = gtk_menu_item_new_with_mnemonic (label);

      g_signal_connect (item, "activate",
                        callback,
                        box);
    }
  else
    {
      item = gtk_separator_menu_item_new ();
    }

  gtk_menu_shell_append (GTK_MENU_SHELL (private->menu), item);
  gtk_widget_show (item);

  return item;
}

static gboolean
ligma_settings_box_row_separator_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      data)
{
  gchar *name = NULL;

  gtk_tree_model_get (model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, &name,
                      -1);
  g_free (name);

  return name == NULL;
}

static gboolean
ligma_settings_box_setting_selected (LigmaContainerView *view,
                                    GList             *objects,
                                    GList             *paths,
                                    LigmaSettingsBox   *box)
{
  g_return_val_if_fail (g_list_length (objects) < 2, FALSE);

  if (objects)
    g_signal_emit (box, settings_box_signals[SELECTED], 0,
                   objects->data);

  return TRUE;
}

static gboolean
ligma_settings_box_menu_press (GtkWidget       *widget,
                              GdkEventButton  *bevent,
                              LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);

  if (bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_menu_popup_at_widget (GTK_MENU (private->menu), widget,
                                GDK_GRAVITY_WEST,
                                GDK_GRAVITY_NORTH_EAST,
                                (GdkEvent *) bevent);
    }

  return TRUE;
}

static void
ligma_settings_box_favorite_activate (GtkWidget       *widget,
                                     LigmaSettingsBox *box)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  GtkWidget *dialog;

  dialog = ligma_query_string_box (_("Save Settings as Named Preset"),
                                  toplevel,
                                  ligma_standard_help_func, NULL,
                                  _("Enter a name for the preset"),
                                  _("Saved Settings"),
                                  G_OBJECT (toplevel), "hide",
                                  ligma_settings_box_favorite_callback,
                                  box, NULL);
  gtk_widget_show (dialog);
}

static void
ligma_settings_box_import_activate (GtkWidget       *widget,
                                   LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);

  ligma_settings_box_file_dialog (box, private->import_title, FALSE);
}

static void
ligma_settings_box_export_activate (GtkWidget       *widget,
                                   LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);

  ligma_settings_box_file_dialog (box, private->export_title, TRUE);
}

static void
ligma_settings_box_manage_activate (GtkWidget       *widget,
                                   LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  GtkWidget              *toplevel;
  GtkWidget              *editor;
  GtkWidget              *content_area;

  if (private->editor_dialog)
    {
      gtk_window_present (GTK_WINDOW (private->editor_dialog));
      return;
    }

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  private->editor_dialog = ligma_dialog_new (_("Manage Saved Presets"),
                                            "ligma-settings-editor-dialog",
                                            toplevel, 0,
                                            NULL, NULL,

                                            _("_Close"), GTK_RESPONSE_CLOSE,

                                            NULL);

  g_object_add_weak_pointer (G_OBJECT (private->editor_dialog),
                             (gpointer) &private->editor_dialog);
  g_signal_connect_object (toplevel, "unmap",
                           G_CALLBACK (ligma_settings_box_toplevel_unmap),
                           private->editor_dialog, 0);

  g_signal_connect (private->editor_dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    box);

  editor = ligma_settings_editor_new (private->ligma,
                                     private->config,
                                     private->container);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (private->editor_dialog));
  gtk_box_pack_start (GTK_BOX (content_area), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  gtk_widget_show (private->editor_dialog);
}

static void
ligma_settings_box_favorite_callback (GtkWidget   *query_box,
                                     const gchar *string,
                                     gpointer     data)
{
  LigmaSettingsBox        *box     = LIGMA_SETTINGS_BOX (data);
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  LigmaConfig             *config;

  config = ligma_config_duplicate (LIGMA_CONFIG (private->config));
  ligma_object_set_name (LIGMA_OBJECT (config), string);
  ligma_container_add (private->container, LIGMA_OBJECT (config));
  g_object_unref (config);

  ligma_operation_config_serialize (private->ligma, private->container, NULL);
}

static void
ligma_settings_box_file_dialog (LigmaSettingsBox *box,
                               const gchar     *title,
                               gboolean         save)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  GtkWidget              *toplevel;
  GtkWidget              *dialog;

  if (private->file_dialog)
    {
      gtk_window_present (GTK_WINDOW (private->file_dialog));
      return;
    }

  if (save)
    gtk_widget_set_sensitive (private->import_item, FALSE);
  else
    gtk_widget_set_sensitive (private->export_item, FALSE);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  private->file_dialog = dialog =
    gtk_file_chooser_dialog_new (title, GTK_WINDOW (toplevel),
                                 save ?
                                 GTK_FILE_CHOOSER_ACTION_SAVE :
                                 GTK_FILE_CHOOSER_ACTION_OPEN,

                                 _("_Cancel"),            GTK_RESPONSE_CANCEL,
                                 save ?
                                 _("_Save") : _("_Open"), GTK_RESPONSE_OK,

                                 NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_set_data (G_OBJECT (dialog), "save", GINT_TO_POINTER (save));

  gtk_window_set_role (GTK_WINDOW (dialog), "ligma-import-export-settings");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  g_object_add_weak_pointer (G_OBJECT (dialog),
                             (gpointer) &private->file_dialog);
  g_signal_connect_object (toplevel, "unmap",
                           G_CALLBACK (ligma_settings_box_toplevel_unmap),
                           dialog, 0);

  if (save)
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                    TRUE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ligma_settings_box_file_response),
                    box);
  g_signal_connect (dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  if (private->default_folder &&
      g_file_query_file_type (private->default_folder,
                              G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gchar *uri = g_file_get_uri (private->default_folder);
      gtk_file_chooser_add_shortcut_folder_uri (GTK_FILE_CHOOSER (dialog),
                                                uri, NULL);
      g_free (uri);

      if (! private->last_file)
        gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                  private->default_folder,
                                                  NULL);
    }
  else if (! private->last_file)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                           g_get_home_dir ());
    }

  if (private->last_file)
    gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog),
                               private->last_file, NULL);

  ligma_help_connect (private->file_dialog, ligma_standard_help_func,
                     private->help_id, NULL, NULL);

  /*  allow callbacks to add widgets to the dialog  */
  g_signal_emit (box, settings_box_signals[FILE_DIALOG_SETUP], 0,
                 private->file_dialog, save);

  gtk_widget_show (private->file_dialog);
}

static void
ligma_settings_box_file_response (GtkWidget       *dialog,
                                 gint             response_id,
                                 LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  gboolean                save;

  save = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "save"));

  if (response_id == GTK_RESPONSE_OK)
    {
      GFile    *file;
      gboolean  success = FALSE;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (save)
        g_signal_emit (box, settings_box_signals[EXPORT], 0, file,
                       &success);
      else
        g_signal_emit (box, settings_box_signals[IMPORT], 0, file,
                       &success);

      if (success)
        {
          if (private->last_file)
            g_object_unref (private->last_file);
          private->last_file = file;

          g_object_notify (G_OBJECT (box), "last-file");
        }
      else
        {
          g_object_unref (file);
        }
    }

  if (save)
    gtk_widget_set_sensitive (private->import_item, TRUE);
  else
    gtk_widget_set_sensitive (private->export_item, TRUE);

  gtk_widget_destroy (dialog);
}

static void
ligma_settings_box_toplevel_unmap (GtkWidget *toplevel,
                                  GtkWidget *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_DELETE_EVENT);
}

static void
ligma_settings_box_truncate_list (LigmaSettingsBox *box,
                                 gint             max_recent)
{
  LigmaSettingsBoxPrivate *private = GET_PRIVATE (box);
  GList                  *list;
  gint                    n_recent = 0;

  list = LIGMA_LIST (private->container)->queue->head;
  while (list)
    {
      LigmaConfig *config = list->data;
      gint64      t;

      list = g_list_next (list);

      g_object_get (config,
                    "time", &t,
                    NULL);

      if (t > 0)
        {
          n_recent++;

          if (n_recent > max_recent)
            ligma_container_remove (private->container, LIGMA_OBJECT (config));
        }
      else
        {
          break;
        }
    }
}


/*  public functions  */

GtkWidget *
ligma_settings_box_new (Ligma          *ligma,
                       GObject       *config,
                       LigmaContainer *container,
                       const gchar   *import_title,
                       const gchar   *export_title,
                       const gchar   *help_id,
                       GFile         *default_folder,
                       GFile         *last_file)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONFIG (config), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (default_folder == NULL || G_IS_FILE (default_folder),
                        NULL);
  g_return_val_if_fail (last_file == NULL || G_IS_FILE (last_file), NULL);

  return g_object_new (LIGMA_TYPE_SETTINGS_BOX,
                       "ligma",           ligma,
                       "config",         config,
                       "container",      container,
                       "help-id",        help_id,
                       "import-title",   import_title,
                       "export-title",   export_title,
                       "default-folder", default_folder,
                       "last-file",      last_file,
                       NULL);
}

GtkWidget *
ligma_settings_box_get_combo (LigmaSettingsBox *box)
{
  g_return_val_if_fail (LIGMA_IS_SETTINGS_BOX (box), NULL);

  return GET_PRIVATE (box)->combo;
}

void
ligma_settings_box_add_current (LigmaSettingsBox *box,
                               gint             max_recent)
{
  LigmaSettingsBoxPrivate *private;
  LigmaConfig             *config = NULL;
  GList                  *list;

  g_return_if_fail (LIGMA_IS_SETTINGS_BOX (box));

  private = GET_PRIVATE (box);

  for (list = LIGMA_LIST (private->container)->queue->head;
       list;
       list = g_list_next (list))
    {
      gint64 t;

      config = list->data;

      g_object_get (config,
                    "time", &t,
                    NULL);

      if (t > 0 && ligma_config_is_equal_to (config,
                                            LIGMA_CONFIG (private->config)))
        {
          GDateTime *now = g_date_time_new_now_utc ();

          g_object_set (config,
                        "time", g_date_time_to_unix (now),
                        NULL);
          g_date_time_unref (now);

          break;
        }
    }

  if (! list)
    {
      GDateTime *now = g_date_time_new_now_utc ();

      config = ligma_config_duplicate (LIGMA_CONFIG (private->config));

      g_object_set (config,
                    "time", g_date_time_to_unix (now),
                    NULL);
      g_date_time_unref (now);

      ligma_container_insert (private->container, LIGMA_OBJECT (config), 0);
      g_object_unref (config);
    }

  ligma_settings_box_truncate_list (box, max_recent);

  ligma_operation_config_serialize (private->ligma, private->container, NULL);
}

void
ligma_settings_box_unset (LigmaSettingsBox *box)
{
  LigmaSettingsBoxPrivate *private;

  g_return_if_fail (LIGMA_IS_SETTINGS_BOX (box));

  private = GET_PRIVATE (box);

  ligma_container_view_select_items (LIGMA_CONTAINER_VIEW (private->combo), NULL);
}
