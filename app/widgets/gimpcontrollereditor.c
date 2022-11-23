/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmacontrollereditor.c
 * Copyright (C) 2004-2008 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#define LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libligmawidgets/ligmacontroller.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"

#include "ligmaaction.h"
#include "ligmaactioneditor.h"
#include "ligmaactionview.h"
#include "ligmacontrollereditor.h"
#include "ligmacontrollerinfo.h"
#include "ligmadialogfactory.h"
#include "ligmahelp-ids.h"
#include "ligmauimanager.h"
#include "ligmaviewabledialog.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CONTROLLER_INFO,
  PROP_CONTEXT
};

enum
{
  COLUMN_EVENT,
  COLUMN_BLURB,
  COLUMN_ICON_NAME,
  COLUMN_ACTION,
  N_COLUMNS
};


static void ligma_controller_editor_constructed    (GObject              *object);
static void ligma_controller_editor_finalize       (GObject              *object);
static void ligma_controller_editor_set_property   (GObject              *object,
                                                   guint                 property_id,
                                                   const GValue         *value,
                                                   GParamSpec           *pspec);
static void ligma_controller_editor_get_property   (GObject              *object,
                                                   guint                 property_id,
                                                   GValue               *value,
                                                   GParamSpec           *pspec);


static void ligma_controller_editor_unmap          (GtkWidget            *widget);

static void ligma_controller_editor_sel_changed    (GtkTreeSelection     *sel,
                                                   LigmaControllerEditor *editor);

static void ligma_controller_editor_row_activated  (GtkTreeView          *tv,
                                                   GtkTreePath          *path,
                                                   GtkTreeViewColumn    *column,
                                                   LigmaControllerEditor *editor);

static void ligma_controller_editor_grab_toggled   (GtkWidget            *button,
                                                   LigmaControllerEditor *editor);
static void ligma_controller_editor_edit_clicked   (GtkWidget            *button,
                                                   LigmaControllerEditor *editor);
static void ligma_controller_editor_delete_clicked (GtkWidget            *button,
                                                   LigmaControllerEditor *editor);

static void ligma_controller_editor_edit_activated (GtkTreeView          *tv,
                                                   GtkTreePath          *path,
                                                   GtkTreeViewColumn    *column,
                                                   LigmaControllerEditor *editor);
static void ligma_controller_editor_edit_response  (GtkWidget            *dialog,
                                                   gint                  response_id,
                                                   LigmaControllerEditor *editor);

static GtkWidget *  ligma_controller_string_view_new (LigmaController *controller,
                                                     GParamSpec     *pspec);
static GtkWidget *  ligma_controller_int_view_new    (LigmaController *controller,
                                                     GParamSpec     *pspec);


G_DEFINE_TYPE (LigmaControllerEditor, ligma_controller_editor, GTK_TYPE_BOX)

#define parent_class ligma_controller_editor_parent_class


static void
ligma_controller_editor_class_init (LigmaControllerEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = ligma_controller_editor_constructed;
  object_class->finalize     = ligma_controller_editor_finalize;
  object_class->set_property = ligma_controller_editor_set_property;
  object_class->get_property = ligma_controller_editor_get_property;

  widget_class->unmap        = ligma_controller_editor_unmap;

  g_object_class_install_property (object_class, PROP_CONTROLLER_INFO,
                                   g_param_spec_object ("controller-info",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTROLLER_INFO,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_controller_editor_init (LigmaControllerEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  editor->info = NULL;
}

static void
ligma_controller_editor_constructed (GObject *object)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (object);
  LigmaControllerInfo   *info;
  LigmaController       *controller;
  LigmaControllerClass  *controller_class;
  LigmaUIManager        *ui_manager;
  GtkListStore         *store;
  GtkWidget            *frame;
  GtkWidget            *vbox;
  GtkWidget            *grid;
  GtkWidget            *hbox;
  GtkWidget            *button;
  GtkWidget            *tv;
  GtkWidget            *sw;
  GtkWidget            *entry;
  GtkTreeViewColumn    *column;
  GtkCellRenderer      *cell;
  GParamSpec          **property_specs;
  guint                 n_property_specs;
  gint                  n_events;
  gint                  row;
  gint                  i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_CONTROLLER_INFO (editor->info));

  info             = editor->info;
  controller       = info->controller;
  controller_class = LIGMA_CONTROLLER_GET_CLASS (controller);

  frame = ligma_frame_new (_("General"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  entry = ligma_prop_entry_new (G_OBJECT (info), "name", -1);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (G_OBJECT (info), "debug-events",
                                       _("_Dump events from this controller"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (G_OBJECT (info), "enabled",
                                       _("_Enable this controller"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  frame = ligma_frame_new (controller_class->name);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  row = 0;

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Name:"), 0.0, 0.5,
                            ligma_prop_label_new (G_OBJECT (controller),
                                                 "name"),
                            1);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("State:"), 0.0, 0.5,
                            ligma_prop_label_new (G_OBJECT (controller),
                                                 "state"),
                            1);

  property_specs =
    g_object_class_list_properties (G_OBJECT_CLASS (controller_class),
                                    &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *pspec = property_specs[i];
      GtkWidget  *widget;

      if (pspec->owner_type == LIGMA_TYPE_CONTROLLER)
        continue;

      if (G_IS_PARAM_SPEC_STRING (pspec))
        {
          widget = ligma_controller_string_view_new (controller, pspec);

          ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    g_param_spec_get_nick (pspec),
                                    0.0, 0.5,
                                    widget,
                                    1);
        }
      else if (G_IS_PARAM_SPEC_INT (pspec))
        {
          widget = ligma_controller_int_view_new (controller, pspec);
          gtk_widget_set_halign (widget, GTK_ALIGN_START);

          ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    g_param_spec_get_nick (pspec),
                                    0.0, 0.5,
                                    widget,
                                    1);
        }
    }

  g_free (property_specs);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (sw, 400, 300);
  gtk_container_add (GTK_CONTAINER (sw), tv);
  gtk_widget_show (tv);

  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  g_signal_connect (tv, "row-activated",
                    G_CALLBACK (ligma_controller_editor_row_activated),
                    editor);

  editor->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (editor->sel, "changed",
                    G_CALLBACK (ligma_controller_editor_sel_changed),
                    editor);

  ui_manager = ligma_ui_managers_from_name ("<Image>")->data;

  n_events = ligma_controller_get_n_events (controller);

  for (i = 0; i < n_events; i++)
    {
      GtkTreeIter iter;
      const gchar *event_name;
      const gchar *event_blurb;
      const gchar *event_action;
      const gchar *icon_name = NULL;

      event_name  = ligma_controller_get_event_name  (controller, i);
      event_blurb = ligma_controller_get_event_blurb (controller, i);

      event_action = g_hash_table_lookup (info->mapping, event_name);

      if (event_action)
        {
          LigmaAction *action;

          action = ligma_ui_manager_find_action (ui_manager, NULL, event_action);

          if (action)
            icon_name = ligma_action_get_icon_name (action);
        }

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_EVENT,     event_name,
                          COLUMN_BLURB,     event_blurb,
                          COLUMN_ICON_NAME, icon_name,
                          COLUMN_ACTION,    event_action,
                          -1);
    }

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv), 0,
                                               _("Event"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_BLURB,
                                               NULL);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Action"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "icon-name", COLUMN_ICON_NAME,
                                       NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_ACTION,
                                       NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->grab_button = gtk_toggle_button_new_with_mnemonic (_("_Grab event"));
  gtk_box_pack_start (GTK_BOX (hbox), editor->grab_button, TRUE, TRUE, 0);
  gtk_widget_show (editor->grab_button);

  g_signal_connect (editor->grab_button, "toggled",
                    G_CALLBACK (ligma_controller_editor_grab_toggled),
                    editor);

  ligma_help_set_help_data (editor->grab_button,
                           _("Select the next event arriving from "
                             "the controller"),
                           NULL);

  editor->edit_button = gtk_button_new_with_mnemonic (_("_Edit event"));
  gtk_box_pack_start (GTK_BOX (hbox), editor->edit_button, TRUE, TRUE, 0);
  gtk_widget_show (editor->edit_button);

  g_signal_connect (editor->edit_button, "clicked",
                    G_CALLBACK (ligma_controller_editor_edit_clicked),
                    editor);

  editor->delete_button = gtk_button_new_with_mnemonic (_("_Clear event"));
  gtk_box_pack_start (GTK_BOX (hbox), editor->delete_button, TRUE, TRUE, 0);
  gtk_widget_show (editor->delete_button);

  g_signal_connect (editor->delete_button, "clicked",
                    G_CALLBACK (ligma_controller_editor_delete_clicked),
                    editor);

  gtk_widget_set_sensitive (editor->edit_button,   FALSE);
  gtk_widget_set_sensitive (editor->delete_button, FALSE);
}

static void
ligma_controller_editor_finalize (GObject *object)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (object);

  if (editor->info)
    {
      ligma_controller_info_set_event_snooper (editor->info, NULL, NULL);

      g_clear_object (&editor->info);
    }

  g_clear_object (&editor->context);

  if (editor->edit_dialog)
    gtk_widget_destroy (editor->edit_dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_controller_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_INFO:
      editor->info = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      editor->context = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_controller_editor_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_INFO:
      g_value_set_object (value, editor->info);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_controller_editor_unmap (GtkWidget *widget)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (widget);

  if (editor->edit_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->edit_dialog),
                         GTK_RESPONSE_CANCEL);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


/*  public functions  */

GtkWidget *
ligma_controller_editor_new (LigmaControllerInfo *info,
                            LigmaContext        *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTROLLER_INFO (info), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_CONTROLLER_EDITOR,
                       "controller-info", info,
                       "context",         context,
                       NULL);
}


/*  private functions  */

static void
ligma_controller_editor_sel_changed (GtkTreeSelection     *sel,
                                    LigmaControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *edit_help        = NULL;
  gchar        *delete_help      = NULL;
  gboolean      edit_sensitive   = FALSE;
  gboolean      delete_sensitive = FALSE;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gchar *event  = NULL;
      gchar *action = NULL;

      gtk_tree_model_get (model, &iter,
                          COLUMN_BLURB,  &event,
                          COLUMN_ACTION, &action,
                          -1);

      if (action)
        {
          g_free (action);

          delete_sensitive = TRUE;
          if (event)
            delete_help =
              g_strdup_printf (_("Remove the action assigned to '%s'"), event);
        }

      edit_sensitive = TRUE;
      if (event)
        edit_help = g_strdup_printf (_("Assign an action to '%s'"), event);

      g_free (event);
    }

  ligma_help_set_help_data (editor->edit_button, edit_help, NULL);
  gtk_widget_set_sensitive (editor->edit_button, edit_sensitive);
  g_free (edit_help);

  ligma_help_set_help_data (editor->delete_button, delete_help, NULL);
  gtk_widget_set_sensitive (editor->delete_button, delete_sensitive);
  g_free (delete_help);

  ligma_controller_info_set_event_snooper (editor->info, NULL, NULL);
}

static void
ligma_controller_editor_row_activated (GtkTreeView          *tv,
                                      GtkTreePath          *path,
                                      GtkTreeViewColumn    *column,
                                      LigmaControllerEditor *editor)
{
  if (gtk_widget_is_sensitive (editor->edit_button))
    gtk_button_clicked (GTK_BUTTON (editor->edit_button));
}

static gboolean
ligma_controller_editor_snooper (LigmaControllerInfo        *info,
                                LigmaController            *controller,
                                const LigmaControllerEvent *event,
                                gpointer                   user_data)
{
  LigmaControllerEditor *editor = LIGMA_CONTROLLER_EDITOR (user_data);
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;
  const gchar          *event_name;

  gtk_tree_selection_get_selected (editor->sel, &model, &iter);

  event_name = ligma_controller_get_event_name (info->controller,
                                               event->any.event_id);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar *list_name;

      gtk_tree_model_get (model, &iter,
                          COLUMN_EVENT, &list_name,
                          -1);

      if (! strcmp (list_name, event_name))
        {
          GtkTreeView *view;
          GtkTreePath *path;

          view = gtk_tree_selection_get_tree_view (editor->sel);

          gtk_tree_selection_select_iter (editor->sel, &iter);

          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0.0, 0.0);
          gtk_tree_view_set_cursor (view, path, NULL, FALSE);
          gtk_tree_path_free (path);

          gtk_widget_grab_focus (GTK_WIDGET (view));

          g_free (list_name);
          break;
        }

      g_free (list_name);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->grab_button), FALSE);

  return TRUE;
}

static void
ligma_controller_editor_grab_toggled (GtkWidget            *button,
                                     LigmaControllerEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      ligma_controller_info_set_event_snooper (editor->info,
                                              ligma_controller_editor_snooper,
                                              editor);
    }
  else
    {
      ligma_controller_info_set_event_snooper (editor->info, NULL, NULL);
    }
}

static void
ligma_controller_editor_edit_clicked (GtkWidget            *button,
                                     LigmaControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *event_name  = NULL;
  gchar        *event_blurb = NULL;
  gchar        *action_name = NULL;

  ligma_controller_info_set_event_snooper (editor->info, NULL, NULL);

  if (gtk_tree_selection_get_selected (editor->sel, &model, &iter))
    gtk_tree_model_get (model, &iter,
                        COLUMN_EVENT,  &event_name,
                        COLUMN_BLURB,  &event_blurb,
                        COLUMN_ACTION, &action_name,
                        -1);

  if (event_name)
    {
      GtkWidget *view;
      gchar     *title;

      title = g_strdup_printf (_("Select Action for Event '%s'"),
                               event_blurb);

      editor->edit_dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, editor->info), editor->context,
                                  _("Select Controller Event Action"),
                                  "ligma-controller-action-dialog",
                                  LIGMA_ICON_EDIT,
                                  title,
                                  gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                                  ligma_standard_help_func,
                                  LIGMA_HELP_PREFS_INPUT_CONTROLLERS,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_OK"),     GTK_RESPONSE_OK,

                                  NULL);

      g_free (title);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (editor->edit_dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_object_add_weak_pointer (G_OBJECT (editor->edit_dialog),
                                 (gpointer) &editor->edit_dialog);

      ligma_dialog_factory_add_foreign (ligma_dialog_factory_get_singleton (),
                                       "ligma-controller-action-dialog",
                                       editor->edit_dialog,
                                       ligma_widget_get_monitor (button));

      g_signal_connect (editor->edit_dialog, "response",
                        G_CALLBACK (ligma_controller_editor_edit_response),
                        editor);

      view = ligma_action_editor_new (ligma_ui_managers_from_name ("<Image>")->data,
                                     action_name, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (view), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (editor->edit_dialog))),
                          view, TRUE, TRUE, 0);
      gtk_widget_show (view);

      g_signal_connect (LIGMA_ACTION_EDITOR (view)->view, "row-activated",
                        G_CALLBACK (ligma_controller_editor_edit_activated),
                        editor);

      editor->edit_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (LIGMA_ACTION_EDITOR (view)->view));

      g_object_add_weak_pointer (G_OBJECT (editor->edit_sel),
                                 (gpointer) &editor->edit_sel);

      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
      gtk_widget_show (editor->edit_dialog);

      g_free (event_name);
      g_free (event_blurb);
      g_free (action_name);
    }
}

static void
ligma_controller_editor_delete_clicked (GtkWidget            *button,
                                       LigmaControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *event_name = NULL;

  ligma_controller_info_set_event_snooper (editor->info, NULL, NULL);

  if (gtk_tree_selection_get_selected (editor->sel, &model, &iter))
    gtk_tree_model_get (model, &iter,
                        COLUMN_EVENT, &event_name,
                        -1);

  if (event_name)
    {
      g_hash_table_remove (editor->info->mapping, event_name);
      g_free (event_name);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_ICON_NAME, NULL,
                          COLUMN_ACTION,    NULL,
                          -1);
    }
}

static void
ligma_controller_editor_edit_activated (GtkTreeView          *tv,
                                       GtkTreePath          *path,
                                       GtkTreeViewColumn    *column,
                                       LigmaControllerEditor *editor)
{
  gtk_dialog_response (GTK_DIALOG (editor->edit_dialog), GTK_RESPONSE_OK);
}

static void
ligma_controller_editor_edit_response (GtkWidget            *dialog,
                                      gint                  response_id,
                                      LigmaControllerEditor *editor)
{
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gchar        *event_name  = NULL;
      gchar        *icon_name   = NULL;
      gchar        *action_name = NULL;

      if (gtk_tree_selection_get_selected (editor->edit_sel, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            LIGMA_ACTION_VIEW_COLUMN_ICON_NAME, &icon_name,
                            LIGMA_ACTION_VIEW_COLUMN_NAME,      &action_name,
                            -1);

      if (gtk_tree_selection_get_selected (editor->sel, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            COLUMN_EVENT, &event_name,
                            -1);

      if (event_name && action_name)
        {
          g_hash_table_insert (editor->info->mapping,
                               g_strdup (event_name),
                               g_strdup (action_name));

          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              COLUMN_ICON_NAME, icon_name,
                              COLUMN_ACTION,    action_name,
                              -1);
        }

      g_free (event_name);
      g_free (icon_name);
      g_free (action_name);

      ligma_controller_editor_sel_changed (editor->sel, editor);
    }

  gtk_widget_destroy (dialog);
}

static GtkWidget *
ligma_controller_string_view_new (LigmaController *controller,
                                 GParamSpec     *pspec)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (G_IS_PARAM_SPEC_STRING (pspec), NULL);

  if (pspec->flags & G_PARAM_WRITABLE)
    {
      GtkTreeModel *model      = NULL;
      gchar        *model_name = g_strdup_printf ("%s-values", pspec->name);
      GParamSpec   *model_spec;

      model_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (controller),
                                                 model_name);

      if (G_IS_PARAM_SPEC_OBJECT (model_spec) &&
          g_type_is_a (model_spec->value_type, GTK_TYPE_LIST_STORE))
        {
          g_object_get (controller,
                        model_name, &model,
                        NULL);
        }

      g_free (model_name);

      if (model)
        {
          widget = ligma_prop_string_combo_box_new (G_OBJECT (controller),
                                                   pspec->name, model, 0, 1);
          g_object_unref (model);
        }
      else
        {
          widget = ligma_prop_entry_new (G_OBJECT (controller), pspec->name, -1);
        }
    }
  else
    {
      widget = ligma_prop_label_new (G_OBJECT (controller), pspec->name);
    }

  return widget;
}


static GtkWidget *
ligma_controller_int_view_new (LigmaController *controller,
                              GParamSpec     *pspec)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (G_IS_PARAM_SPEC_INT (pspec), NULL);

  if (pspec->flags & G_PARAM_WRITABLE)
    {
      LigmaIntStore *model      = NULL;
      gchar        *model_name = g_strdup_printf ("%s-values", pspec->name);
      GParamSpec   *model_spec;

      model_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (controller),
                                                 model_name);

      if (G_IS_PARAM_SPEC_OBJECT (model_spec) &&
          g_type_is_a (model_spec->value_type, LIGMA_TYPE_INT_STORE))
        {
          g_object_get (controller,
                        model_name, &model,
                        NULL);
        }

      g_free (model_name);

      if (model)
        {
          widget = ligma_prop_int_combo_box_new (G_OBJECT (controller),
                                                pspec->name, model);
          g_object_unref (model);
        }
      else
        {
          widget = ligma_prop_spin_button_new (G_OBJECT (controller),
                                              pspec->name, 1, 8, 0);
        }
    }
  else
    {
      widget = ligma_prop_label_new (G_OBJECT (controller), pspec->name);
    }

  return widget;
}
