/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpcontrollereditor.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "gimpactionview.h"
#include "gimpcontrollereditor.h"
#include "gimpcontrollerinfo.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimppropwidgets.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTROLLER_INFO
};

enum
{
  COLUMN_EVENT,
  COLUMN_BLURB,
  COLUMN_ACTION,
  NUM_COLUMNS
};


static void gimp_controller_editor_class_init (GimpControllerEditorClass *klass);
static void gimp_controller_editor_init       (GimpControllerEditor      *editor);

static GObject * gimp_controller_editor_constructor (GType               type,
                                                     guint               n_params,
                                                     GObjectConstructParam *params);
static void gimp_controller_editor_set_property   (GObject              *object,
                                                   guint                 property_id,
                                                   const GValue         *value,
                                                   GParamSpec           *pspec);
static void gimp_controller_editor_get_property   (GObject              *object,
                                                   guint                 property_id,
                                                   GValue               *value,
                                                   GParamSpec           *pspec);

static void gimp_controller_editor_finalize       (GObject              *object);

static void gimp_controller_editor_unmap          (GtkWidget            *widget);

static void gimp_controller_editor_sel_changed    (GtkTreeSelection     *sel,
                                                   GimpControllerEditor *editor);

static void gimp_controller_editor_row_activated  (GtkTreeView          *tv,
                                                   GtkTreePath          *path,
                                                   GtkTreeViewColumn    *column,
                                                   GimpControllerEditor *editor);

static void gimp_controller_editor_edit_clicked   (GtkWidget            *button,
                                                   GimpControllerEditor *editor);
static void gimp_controller_editor_delete_clicked (GtkWidget            *button,
                                                   GimpControllerEditor *editor);

static void gimp_controller_editor_edit_activated (GtkTreeView          *tv,
                                                   GtkTreePath          *path,
                                                   GtkTreeViewColumn    *column,
                                                   GimpControllerEditor *editor);
static void gimp_controller_editor_edit_response  (GtkWidget            *dialog,
                                                   gint                  response_id,
                                                   GimpControllerEditor *editor);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_controller_editor_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpControllerEditorClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_controller_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpControllerEditor),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_controller_editor_init
      };

      view_type = g_type_register_static (GTK_TYPE_VBOX,
                                          "GimpControllerEditor",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_controller_editor_class_init (GimpControllerEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_controller_editor_constructor;
  object_class->set_property = gimp_controller_editor_set_property;
  object_class->get_property = gimp_controller_editor_get_property;
  object_class->finalize     = gimp_controller_editor_finalize;

  widget_class->unmap        = gimp_controller_editor_unmap;

  g_object_class_install_property (object_class, PROP_CONTROLLER_INFO,
                                   g_param_spec_object ("controller-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTROLLER_INFO,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_controller_editor_init (GimpControllerEditor *editor)
{
  editor->info = NULL;
}

static GObject *
gimp_controller_editor_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject              *object;
  GimpControllerEditor *editor;
  GimpControllerInfo   *info;
  GimpController       *controller;
  GimpControllerClass  *controller_class;
  GtkListStore         *store;
  GtkWidget            *frame;
  GtkWidget            *vbox;
  GtkWidget            *table;
  GtkWidget            *hbox;
  GtkWidget            *button;
  GtkWidget            *tv;
  GtkWidget            *sw;
  GtkWidget            *entry;
  GParamSpec          **property_specs;
  guint                 n_property_specs;
  gint                  n_events;
  gint                  row;
  gint                  i;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_CONTROLLER_EDITOR (object);

  g_assert (GIMP_IS_CONTROLLER_INFO (editor->info));

  info             = editor->info;
  controller       = info->controller;
  controller_class = GIMP_CONTROLLER_GET_CLASS (controller);

  frame = gimp_frame_new (_("General"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  entry = gimp_prop_entry_new (G_OBJECT (info), "name", -1);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  button = gimp_prop_check_button_new (G_OBJECT (info), "debug-events",
                                       _("Dump events from this controller"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (info), "enabled",
                                       _("Enable this controller"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  frame = gimp_frame_new (controller_class->name);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Name:"), 0.0, 0.5,
                             gimp_prop_label_new (G_OBJECT (controller),
                                                  "name"),
                             1, TRUE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("State:"), 0.0, 0.5,
                             gimp_prop_label_new (G_OBJECT (controller),
                                                  "state"),
                             1, TRUE);

  property_specs =
    g_object_class_list_properties (G_OBJECT_CLASS (controller_class),
                                    &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      GtkWidget  *widget    = NULL;

      if (prop_spec->owner_type == GIMP_TYPE_CONTROLLER)
        continue;

      if (G_IS_PARAM_SPEC_STRING (prop_spec))
        {
          if (prop_spec->flags & G_PARAM_WRITABLE)
            widget = gimp_prop_entry_new (G_OBJECT (controller),
                                          prop_spec->name, -1);
          else
            widget = gimp_prop_label_new (G_OBJECT (controller),
                                          prop_spec->name);

          gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                     g_param_spec_get_nick (prop_spec),
                                     0.0, 0.5,
                                     widget,
                                     1, FALSE);
        }
      else if (G_IS_PARAM_SPEC_INT (prop_spec))
        {
          if (prop_spec->flags & G_PARAM_WRITABLE)
            {
              GimpIntStore *model      = NULL;
              gchar        *model_name = g_strdup_printf ("%s-values",
                                                          prop_spec->name);

              if (((i + 1) < n_property_specs)                       &&
                  ! strcmp (model_name, property_specs[i + 1]->name) &&
                  G_IS_PARAM_SPEC_OBJECT (property_specs[i + 1])     &&
                  g_type_is_a (property_specs[i + 1]->value_type,
                               GIMP_TYPE_INT_STORE))
                {
                  g_object_get (controller,
                                property_specs[i + 1]->name, &model,
                                NULL);
                }

              g_free (model_name);

              if (model)
                {
                  widget = gimp_prop_int_combo_box_new (G_OBJECT (controller),
                                                        prop_spec->name,
                                                        model);
                  g_object_unref (model);
                }
              else
                {
                  widget = gimp_prop_spin_button_new (G_OBJECT (controller),
                                                      prop_spec->name,
                                                      1, 8, 0);
                }

              gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                         g_param_spec_get_nick (prop_spec),
                                         0.0, 0.5,
                                         widget,
                                         1, TRUE);
            }
        }
    }

  g_free (property_specs);

  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (sw), tv);
  gtk_widget_show (tv);

  gtk_container_add (GTK_CONTAINER (vbox), sw);
  gtk_widget_show (sw);

  g_signal_connect (tv, "row-activated",
                    G_CALLBACK (gimp_controller_editor_row_activated),
                    editor);

  editor->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (editor->sel, "changed",
                    G_CALLBACK (gimp_controller_editor_sel_changed),
                    editor);

  n_events = gimp_controller_get_n_events (controller);

  for (i = 0; i < n_events; i++)
    {
      GtkTreeIter iter;
      const gchar *event_name;
      const gchar *event_blurb;
      const gchar *event_action;

      event_name  = gimp_controller_get_event_name  (controller, i);
      event_blurb = gimp_controller_get_event_blurb (controller, i);

      event_action = g_hash_table_lookup (info->mapping, event_name);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_EVENT,  event_name,
                          COLUMN_BLURB,  event_blurb,
                          COLUMN_ACTION, event_action,
                          -1);
    }

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv), 0,
                                               _("Event"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_BLURB,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv), 2,
                                               _("Action"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_ACTION,
                                               NULL);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->edit_button = gtk_button_new_from_stock (GIMP_STOCK_EDIT);
  gtk_box_pack_start (GTK_BOX (hbox), editor->edit_button, TRUE, TRUE, 0);
  gtk_widget_show (editor->edit_button);

  g_signal_connect (editor->edit_button, "clicked",
                    G_CALLBACK (gimp_controller_editor_edit_clicked),
                    editor);

  editor->delete_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_box_pack_start (GTK_BOX (hbox), editor->delete_button, TRUE, TRUE, 0);
  gtk_widget_show (editor->delete_button);

  g_signal_connect (editor->delete_button, "clicked",
                    G_CALLBACK (gimp_controller_editor_delete_clicked),
                    editor);

  gtk_widget_set_sensitive (editor->edit_button,   FALSE);
  gtk_widget_set_sensitive (editor->delete_button, FALSE);

  return object;
}

static void
gimp_controller_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpControllerEditor *editor = GIMP_CONTROLLER_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_INFO:
      editor->info = GIMP_CONTROLLER_INFO (g_value_dup_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_controller_editor_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpControllerEditor *editor = GIMP_CONTROLLER_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_INFO:
      g_value_set_object (value, editor->info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_controller_editor_finalize (GObject *object)
{
  GimpControllerEditor *editor = GIMP_CONTROLLER_EDITOR (object);

  if (editor->info)
    {
      g_object_unref (editor->info);
      editor->info = NULL;
    }

  if (editor->edit_dialog)
    gtk_widget_destroy (editor->edit_dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_controller_editor_unmap (GtkWidget *widget)
{
  GimpControllerEditor *editor = GIMP_CONTROLLER_EDITOR (widget);

  if (editor->edit_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->edit_dialog),
                         GTK_RESPONSE_CANCEL);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


/*  public functions  */

GtkWidget *
gimp_controller_editor_new (GimpControllerInfo *info)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_INFO (info), NULL);

  return g_object_new (GIMP_TYPE_CONTROLLER_EDITOR,
                       "controller-info", info,
                       NULL);
}


/*  private functions  */

static void
gimp_controller_editor_sel_changed (GtkTreeSelection     *sel,
                                    GimpControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      edit_sensitive   = FALSE;
  gboolean      delete_sensitive = FALSE;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gchar *action = NULL;

      gtk_tree_model_get (model, &iter,
                          COLUMN_ACTION, &action,
                          -1);

      if (action)
        {
          g_free (action);

          delete_sensitive = TRUE;
        }

      edit_sensitive = TRUE;
    }

  gtk_widget_set_sensitive (editor->edit_button,   edit_sensitive);
  gtk_widget_set_sensitive (editor->delete_button, delete_sensitive);
}

static void
gimp_controller_editor_row_activated (GtkTreeView          *tv,
                                      GtkTreePath          *path,
                                      GtkTreeViewColumn    *column,
                                      GimpControllerEditor *editor)
{
  if (GTK_WIDGET_IS_SENSITIVE (editor->edit_button))
    gtk_button_clicked (GTK_BUTTON (editor->edit_button));
}

static void
gimp_controller_editor_edit_clicked (GtkWidget            *button,
                                     GimpControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *event_name  = NULL;
  gchar        *action_name = NULL;

  if (gtk_tree_selection_get_selected (editor->sel, &model, &iter))
    gtk_tree_model_get (model, &iter,
                        COLUMN_EVENT,  &event_name,
                        COLUMN_ACTION, &action_name,
                        -1);

  if (event_name)
    {
      GtkWidget *scrolled_window;
      GtkWidget *view;

      editor->edit_dialog =
        gimp_dialog_new (_("Select Controller Event Action"),
                         "gimp-controller-action-dialog",
                         GTK_WIDGET (editor), 0,
                         gimp_standard_help_func,
                         GIMP_HELP_PREFS_INPUT_CONTROLLERS,

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

      g_object_add_weak_pointer (G_OBJECT (editor->edit_dialog),
                                 (gpointer) &editor->edit_dialog);

      gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                       "gimp-controller-action-dialog",
                                       editor->edit_dialog);

      g_signal_connect (editor->edit_dialog, "response",
                        G_CALLBACK (gimp_controller_editor_edit_response),
                        editor);

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                           GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 12);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (editor->edit_dialog)->vbox),
                         scrolled_window);
      gtk_widget_show (scrolled_window);

      view = gimp_action_view_new (gimp_ui_managers_from_name ("<Image>")->data,
                                   action_name, FALSE);
      gtk_widget_set_size_request (view, 300, 400);
      gtk_container_add (GTK_CONTAINER (scrolled_window), view);
      gtk_widget_show (view);

      g_signal_connect (view, "row-activated",
                        G_CALLBACK (gimp_controller_editor_edit_activated),
                        editor);

      editor->edit_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

      g_object_add_weak_pointer (G_OBJECT (editor->edit_sel),
                                 (gpointer) &editor->edit_sel);

      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
      gtk_widget_show (editor->edit_dialog);

      g_free (event_name);
      g_free (action_name);
    }
}

static void
gimp_controller_editor_delete_clicked (GtkWidget            *button,
                                       GimpControllerEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *event_name = NULL;

  if (gtk_tree_selection_get_selected (editor->sel, &model, &iter))
    gtk_tree_model_get (model, &iter,
                        COLUMN_EVENT, &event_name,
                        -1);

  if (event_name)
    {
      g_hash_table_remove (editor->info->mapping, event_name);
      g_free (event_name);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_ACTION, NULL,
                          -1);
    }
}

static void
gimp_controller_editor_edit_activated (GtkTreeView          *tv,
                                       GtkTreePath          *path,
                                       GtkTreeViewColumn    *column,
                                       GimpControllerEditor *editor)
{
  gtk_dialog_response (GTK_DIALOG (editor->edit_dialog), GTK_RESPONSE_OK);
}

static void
gimp_controller_editor_edit_response (GtkWidget            *dialog,
                                      gint                  response_id,
                                      GimpControllerEditor *editor)
{
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gchar        *event_name  = NULL;
      gchar        *action_name = NULL;

      if (gtk_tree_selection_get_selected (editor->edit_sel, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            GIMP_ACTION_VIEW_COLUMN_NAME, &action_name,
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
                              COLUMN_ACTION, action_name,
                              -1);
        }

      g_free (event_name);
      g_free (action_name);
    }

  gtk_widget_destroy (dialog);
}
