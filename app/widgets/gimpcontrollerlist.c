/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmacontrollerlist.c
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#include "core/ligma.h"
#include "core/ligmacontainer.h"

#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmacontrollereditor.h"
#include "ligmacontrollerlist.h"
#include "ligmacontrollerinfo.h"
#include "ligmacontrollerkeyboard.h"
#include "ligmacontrollerwheel.h"
#include "ligmacontrollers.h"
#include "ligmadialogfactory.h"
#include "ligmahelp-ids.h"
#include "ligmamessagebox.h"
#include "ligmamessagedialog.h"
#include "ligmapropwidgets.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA
};

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_TYPE,
  N_COLUMNS
};


static void ligma_controller_list_constructed     (GObject            *object);
static void ligma_controller_list_finalize        (GObject            *object);
static void ligma_controller_list_set_property    (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void ligma_controller_list_get_property    (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void ligma_controller_list_src_sel_changed (GtkTreeSelection   *sel,
                                                  LigmaControllerList *list);
static void ligma_controller_list_row_activated   (GtkTreeView        *tv,
                                                  GtkTreePath        *path,
                                                  GtkTreeViewColumn  *column,
                                                  LigmaControllerList *list);

static gboolean ligma_controller_list_select_items(LigmaContainerView  *view,
                                                  GList              *viewables,
                                                  GList              *paths,
                                                  LigmaControllerList *list);
static void ligma_controller_list_activate_item   (LigmaContainerView  *view,
                                                  LigmaViewable       *viewable,
                                                  gpointer            insert_data,
                                                  LigmaControllerList *list);

static void ligma_controller_list_add_clicked     (GtkWidget          *button,
                                                  LigmaControllerList *list);
static void ligma_controller_list_remove_clicked  (GtkWidget          *button,
                                                  LigmaControllerList *list);

static void ligma_controller_list_edit_clicked    (GtkWidget          *button,
                                                  LigmaControllerList *list);
static void ligma_controller_list_edit_destroy    (GtkWidget          *widget,
                                                  LigmaControllerInfo *info);
static void ligma_controller_list_up_clicked      (GtkWidget          *button,
                                                  LigmaControllerList *list);
static void ligma_controller_list_down_clicked    (GtkWidget          *button,
                                                  LigmaControllerList *list);


G_DEFINE_TYPE (LigmaControllerList, ligma_controller_list, GTK_TYPE_BOX)

#define parent_class ligma_controller_list_parent_class


static void
ligma_controller_list_class_init (LigmaControllerListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_controller_list_constructed;
  object_class->finalize     = ligma_controller_list_finalize;
  object_class->set_property = ligma_controller_list_set_property;
  object_class->get_property = ligma_controller_list_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_controller_list_init (LigmaControllerList *list)
{
  GtkWidget         *hbox;
  GtkWidget         *sw;
  GtkWidget         *tv;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkWidget         *vbox;
  GtkWidget         *image;
  GtkIconSize        icon_size;
  gint               icon_width;
  gint               icon_height;
  GType             *controller_types;
  guint              n_controller_types;
  gint               i;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (list),
                                  GTK_ORIENTATION_VERTICAL);

  list->ligma = NULL;

  list->hbox = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (list), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  list->src = gtk_list_store_new (N_COLUMNS,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_GTYPE);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list->src));
  g_object_unref (list->src);

  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tv), FALSE);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Available Controllers"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "icon-name", COLUMN_ICON,
                                       NULL);

  g_object_get (cell, "stock-size", &icon_size, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_NAME,
                                       NULL);

  gtk_container_add (GTK_CONTAINER (sw), tv);
  gtk_widget_show (tv);

  g_signal_connect_object (tv, "row-activated",
                           G_CALLBACK (ligma_controller_list_row_activated),
                           G_OBJECT (list), 0);

  list->src_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gtk_tree_selection_set_mode (list->src_sel, GTK_SELECTION_BROWSE);

  g_signal_connect_object (list->src_sel, "changed",
                           G_CALLBACK (ligma_controller_list_src_sel_changed),
                           G_OBJECT (list), 0);

  controller_types = g_type_children (LIGMA_TYPE_CONTROLLER,
                                      &n_controller_types);

  for (i = 0; i < n_controller_types; i++)
    {
      LigmaControllerClass *controller_class;
      GtkTreeIter          iter;

      controller_class = g_type_class_ref (controller_types[i]);

      gtk_list_store_append (list->src, &iter);
      gtk_list_store_set (list->src, &iter,
                          COLUMN_ICON, controller_class->icon_name,
                          COLUMN_NAME, controller_class->name,
                          COLUMN_TYPE, controller_types[i],
                          -1);

      g_type_class_unref (controller_class);
    }

  g_free (controller_types);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  list->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), list->add_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (list->add_button, FALSE);
  gtk_widget_show (list->add_button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->add_button), image);
  gtk_widget_show (image);

  g_signal_connect (list->add_button, "clicked",
                    G_CALLBACK (ligma_controller_list_add_clicked),
                    list);

  g_object_add_weak_pointer (G_OBJECT (list->add_button),
                             (gpointer) &list->add_button);

  list->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), list->remove_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (list->remove_button, FALSE);
  gtk_widget_show (list->remove_button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->remove_button), image);
  gtk_widget_show (image);

  g_signal_connect (list->remove_button, "clicked",
                    G_CALLBACK (ligma_controller_list_remove_clicked),
                    list);

  g_object_add_weak_pointer (G_OBJECT (list->remove_button),
                             (gpointer) &list->remove_button);

  gtk_icon_size_lookup (icon_size, &icon_width, &icon_height);
  list->dest = ligma_container_tree_view_new (NULL, NULL, icon_height, 0);
  ligma_container_tree_view_set_main_column_title (LIGMA_CONTAINER_TREE_VIEW (list->dest),
                                                  _("Active Controllers"));
  gtk_tree_view_set_headers_visible (LIGMA_CONTAINER_TREE_VIEW (list->dest)->view,
                                     TRUE);
  gtk_box_pack_start (GTK_BOX (list->hbox), list->dest, TRUE, TRUE, 0);
  gtk_widget_show (list->dest);

  g_signal_connect_object (list->dest, "select-items",
                           G_CALLBACK (ligma_controller_list_select_items),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->dest, "activate-item",
                           G_CALLBACK (ligma_controller_list_activate_item),
                           G_OBJECT (list), 0);

  list->edit_button =
    ligma_editor_add_button (LIGMA_EDITOR (list->dest),
                            LIGMA_ICON_DOCUMENT_PROPERTIES,
                            _("Configure the selected controller"),
                            NULL,
                            G_CALLBACK (ligma_controller_list_edit_clicked),
                            NULL,
                            list);
  list->up_button =
    ligma_editor_add_button (LIGMA_EDITOR (list->dest),
                            LIGMA_ICON_GO_UP,
                            _("Move the selected controller up"),
                            NULL,
                            G_CALLBACK (ligma_controller_list_up_clicked),
                            NULL,
                            list);
  list->down_button =
    ligma_editor_add_button (LIGMA_EDITOR (list->dest),
                            LIGMA_ICON_GO_DOWN,
                            _("Move the selected controller down"),
                            NULL,
                            G_CALLBACK (ligma_controller_list_down_clicked),
                            NULL,
                            list);

  gtk_widget_set_sensitive (list->edit_button, FALSE);
  gtk_widget_set_sensitive (list->up_button,   FALSE);
  gtk_widget_set_sensitive (list->down_button, FALSE);
}

static void
ligma_controller_list_constructed (GObject *object)
{
  LigmaControllerList *list = LIGMA_CONTROLLER_LIST (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (list->ligma));

  ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (list->dest),
                                     ligma_controllers_get_list (list->ligma));

  ligma_container_view_set_context (LIGMA_CONTAINER_VIEW (list->dest),
                                   ligma_get_user_context (list->ligma));
}

static void
ligma_controller_list_finalize (GObject *object)
{
  LigmaControllerList *list = LIGMA_CONTROLLER_LIST (object);

  g_clear_object (&list->ligma);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_controller_list_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaControllerList *list = LIGMA_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      list->ligma = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_controller_list_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaControllerList *list = LIGMA_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, list->ligma);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
ligma_controller_list_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_CONTROLLER_LIST,
                       "ligma", ligma,
                       NULL);
}


/*  private functions  */

static void
ligma_controller_list_src_sel_changed (GtkTreeSelection   *sel,
                                      LigmaControllerList *list)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *tip = NULL;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gchar *name;

      gtk_tree_model_get (model, &iter,
                          COLUMN_NAME, &name,
                          COLUMN_TYPE, &list->src_gtype,
                          -1);

      if (list->add_button)
        {
          tip =
            g_strdup_printf (_("Add '%s' to the list of active controllers"),
                             name);
          gtk_widget_set_sensitive (list->add_button, TRUE);
        }

      g_free (name);
    }
  else
    {
      if (list->add_button)
        gtk_widget_set_sensitive (list->add_button, FALSE);
    }

  if (list->add_button)
    {
      ligma_help_set_help_data (list->add_button, tip, NULL);
      g_free (tip);
    }
}

static void
ligma_controller_list_row_activated (GtkTreeView        *tv,
                                    GtkTreePath        *path,
                                    GtkTreeViewColumn  *column,
                                    LigmaControllerList *list)
{
  if (gtk_widget_is_sensitive (list->add_button))
    gtk_button_clicked (GTK_BUTTON (list->add_button));
}

static gboolean
ligma_controller_list_select_items (LigmaContainerView  *view,
                                   GList              *viewables,
                                   GList              *paths,
                                   LigmaControllerList *list)
{
  gboolean selected;

  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  list->dest_info = viewables ? LIGMA_CONTROLLER_INFO (viewables->data) : NULL;

  selected = LIGMA_IS_CONTROLLER_INFO (list->dest_info);

  if (list->remove_button)
    {
      LigmaObject *object = LIGMA_OBJECT (list->dest_info);
      gchar      *tip    = NULL;

      gtk_widget_set_sensitive (list->remove_button, selected);

      if (selected)
        tip =
          g_strdup_printf (_("Remove '%s' from the list of active controllers"),
                           ligma_object_get_name (object));

      ligma_help_set_help_data (list->remove_button, tip, NULL);
      g_free (tip);
    }

  gtk_widget_set_sensitive (list->edit_button, selected);
  gtk_widget_set_sensitive (list->up_button,   selected);
  gtk_widget_set_sensitive (list->down_button, selected);

  return TRUE;
}

static void
ligma_controller_list_activate_item (LigmaContainerView  *view,
                                    LigmaViewable       *viewable,
                                    gpointer            insert_data,
                                    LigmaControllerList *list)
{
  if (gtk_widget_is_sensitive (list->edit_button))
    gtk_button_clicked (GTK_BUTTON (list->edit_button));
}

static void
ligma_controller_list_add_clicked (GtkWidget          *button,
                                  LigmaControllerList *list)
{
  LigmaControllerInfo *info;
  LigmaContainer      *container;

  if (list->src_gtype == LIGMA_TYPE_CONTROLLER_KEYBOARD &&
      ligma_controllers_get_keyboard (list->ligma) != NULL)
    {
      ligma_message_literal (list->ligma,
                            G_OBJECT (button), LIGMA_MESSAGE_WARNING,
                            _("There can only be one active keyboard "
                              "controller.\n\n"
                              "You already have a keyboard controller in "
                              "your list of active controllers."));
      return;
    }
  else if (list->src_gtype == LIGMA_TYPE_CONTROLLER_WHEEL &&
           ligma_controllers_get_wheel (list->ligma) != NULL)
    {
      ligma_message_literal (list->ligma,
                            G_OBJECT (button), LIGMA_MESSAGE_WARNING,
                            _("There can only be one active wheel "
                              "controller.\n\n"
                              "You already have a wheel controller in "
                              "your list of active controllers."));
      return;
    }

  info = ligma_controller_info_new (list->src_gtype);
  container = ligma_controllers_get_list (list->ligma);
  ligma_container_add (container, LIGMA_OBJECT (info));
  g_object_unref (info);

  ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (list->dest),
                                   LIGMA_VIEWABLE (info));
  ligma_controller_list_edit_clicked (list->edit_button, list);
}

static void
ligma_controller_list_remove_clicked (GtkWidget          *button,
                                     LigmaControllerList *list)
{
  GtkWidget   *dialog;
  const gchar *name;

#define RESPONSE_DISABLE 1

  dialog = ligma_message_dialog_new (_("Remove Controller?"),
                                    LIGMA_ICON_DIALOG_WARNING,
                                    GTK_WIDGET (list), GTK_DIALOG_MODAL,
                                    NULL, NULL,

                                    _("_Disable Controller"), RESPONSE_DISABLE,
                                    _("_Cancel"),             GTK_RESPONSE_CANCEL,
                                    _("_Remove Controller"),  GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           RESPONSE_DISABLE,
                                           -1);

  name = ligma_object_get_name (list->dest_info);
  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Remove Controller '%s'?"), name);

  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                             "%s",
                             _("Removing this controller from the list of "
                               "active controllers will permanently delete "
                               "all event mappings you have configured.\n\n"
                               "Selecting \"Disable Controller\" will disable "
                               "the controller without removing it."));

  switch (ligma_dialog_run (LIGMA_DIALOG (dialog)))
    {
    case RESPONSE_DISABLE:
      ligma_controller_info_set_enabled (list->dest_info, FALSE);
      break;

    case GTK_RESPONSE_OK:
      {
        GtkWidget     *editor_dialog;
        LigmaContainer *container;

        editor_dialog = g_object_get_data (G_OBJECT (list->dest_info),
                                           "ligma-controller-editor-dialog");

        if (editor_dialog)
          gtk_dialog_response (GTK_DIALOG (editor_dialog),
                               GTK_RESPONSE_DELETE_EVENT);

        container = ligma_controllers_get_list (list->ligma);
        ligma_container_remove (container, LIGMA_OBJECT (list->dest_info));
      }
      break;

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}

static void
ligma_controller_list_edit_clicked (GtkWidget          *button,
                                   LigmaControllerList *list)
{
  GtkWidget *dialog;
  GtkWidget *editor;

  dialog = g_object_get_data (G_OBJECT (list->dest_info),
                              "ligma-controller-editor-dialog");

  if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  dialog = ligma_dialog_new (_("Configure Input Controller"),
                            "ligma-controller-editor-dialog",
                            gtk_widget_get_toplevel (GTK_WIDGET (list)),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            ligma_standard_help_func,
                            LIGMA_HELP_PREFS_INPUT_CONTROLLERS,

                            _("_Close"), GTK_RESPONSE_CLOSE,

                            NULL);

  ligma_dialog_factory_add_foreign (ligma_dialog_factory_get_singleton (),
                                   "ligma-controller-editor-dialog",
                                   dialog,
                                   ligma_widget_get_monitor (button));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  editor = ligma_controller_editor_new (list->dest_info,
                                       ligma_get_user_context (list->ligma));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (list->dest_info), "ligma-controller-editor-dialog",
                     dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (ligma_controller_list_edit_destroy),
                           G_OBJECT (list->dest_info), 0);

  g_signal_connect_object (list, "destroy",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (list, "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);

  gtk_widget_show (dialog);
}

static void
ligma_controller_list_edit_destroy (GtkWidget          *widget,
                                   LigmaControllerInfo *info)
{
  g_object_set_data (G_OBJECT (info), "ligma-controller-editor-dialog", NULL);
}

static void
ligma_controller_list_up_clicked (GtkWidget          *button,
                                 LigmaControllerList *list)
{
  LigmaContainer *container;
  gint           index;

  container = ligma_controllers_get_list (list->ligma);

  index = ligma_container_get_child_index (container,
                                          LIGMA_OBJECT (list->dest_info));

  if (index > 0)
    ligma_container_reorder (container, LIGMA_OBJECT (list->dest_info),
                            index - 1);
}

static void
ligma_controller_list_down_clicked (GtkWidget          *button,
                                   LigmaControllerList *list)
{
  LigmaContainer *container;
  gint           index;

  container = ligma_controllers_get_list (list->ligma);

  index = ligma_container_get_child_index (container,
                                          LIGMA_OBJECT (list->dest_info));

  if (index < ligma_container_get_n_children (container) - 1)
    ligma_container_reorder (container, LIGMA_OBJECT (list->dest_info),
                            index + 1);
}
