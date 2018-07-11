/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpcontrollerlist.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpcontrollereditor.h"
#include "gimpcontrollerlist.h"
#include "gimpcontrollerinfo.h"
#include "gimpcontrollerkeyboard.h"
#include "gimpcontrollermouse.h"
#include "gimpcontrollerwheel.h"
#include "gimpcontrollers.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"
#include "gimppropwidgets.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP
};

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_TYPE,
  N_COLUMNS
};


static void gimp_controller_list_constructed     (GObject            *object);
static void gimp_controller_list_finalize        (GObject            *object);
static void gimp_controller_list_set_property    (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void gimp_controller_list_get_property    (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void gimp_controller_list_src_sel_changed (GtkTreeSelection   *sel,
                                                  GimpControllerList *list);
static void gimp_controller_list_row_activated   (GtkTreeView        *tv,
                                                  GtkTreePath        *path,
                                                  GtkTreeViewColumn  *column,
                                                  GimpControllerList *list);

static void gimp_controller_list_select_item     (GimpContainerView  *view,
                                                  GimpViewable       *viewable,
                                                  gpointer            insert_data,
                                                  GimpControllerList *list);
static void gimp_controller_list_activate_item   (GimpContainerView  *view,
                                                  GimpViewable       *viewable,
                                                  gpointer            insert_data,
                                                  GimpControllerList *list);

static void gimp_controller_list_add_clicked     (GtkWidget          *button,
                                                  GimpControllerList *list);
static void gimp_controller_list_remove_clicked  (GtkWidget          *button,
                                                  GimpControllerList *list);

static void gimp_controller_list_edit_clicked    (GtkWidget          *button,
                                                  GimpControllerList *list);
static void gimp_controller_list_edit_destroy    (GtkWidget          *widget,
                                                  GimpControllerInfo *info);
static void gimp_controller_list_up_clicked      (GtkWidget          *button,
                                                  GimpControllerList *list);
static void gimp_controller_list_down_clicked    (GtkWidget          *button,
                                                  GimpControllerList *list);


G_DEFINE_TYPE (GimpControllerList, gimp_controller_list, GTK_TYPE_BOX)

#define parent_class gimp_controller_list_parent_class


static void
gimp_controller_list_class_init (GimpControllerListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_controller_list_constructed;
  object_class->finalize     = gimp_controller_list_finalize;
  object_class->set_property = gimp_controller_list_set_property;
  object_class->get_property = gimp_controller_list_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_controller_list_init (GimpControllerList *list)
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

  list->gimp = NULL;

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
                           G_CALLBACK (gimp_controller_list_row_activated),
                           G_OBJECT (list), 0);

  list->src_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gtk_tree_selection_set_mode (list->src_sel, GTK_SELECTION_BROWSE);

  g_signal_connect_object (list->src_sel, "changed",
                           G_CALLBACK (gimp_controller_list_src_sel_changed),
                           G_OBJECT (list), 0);

  controller_types = g_type_children (GIMP_TYPE_CONTROLLER,
                                      &n_controller_types);

  for (i = 0; i < n_controller_types; i++)
    {
      GimpControllerClass *controller_class;
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

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->add_button), image);
  gtk_widget_show (image);

  g_signal_connect (list->add_button, "clicked",
                    G_CALLBACK (gimp_controller_list_add_clicked),
                    list);

  g_object_add_weak_pointer (G_OBJECT (list->add_button),
                             (gpointer) &list->add_button);

  list->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), list->remove_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (list->remove_button, FALSE);
  gtk_widget_show (list->remove_button);

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->remove_button), image);
  gtk_widget_show (image);

  g_signal_connect (list->remove_button, "clicked",
                    G_CALLBACK (gimp_controller_list_remove_clicked),
                    list);

  g_object_add_weak_pointer (G_OBJECT (list->remove_button),
                             (gpointer) &list->remove_button);

  gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (list)),
                                     icon_size, &icon_width, &icon_height);

  list->dest = gimp_container_tree_view_new (NULL, NULL, icon_height, 0);
  gimp_container_tree_view_set_main_column_title (GIMP_CONTAINER_TREE_VIEW (list->dest),
                                                  _("Active Controllers"));
  gtk_tree_view_set_headers_visible (GIMP_CONTAINER_TREE_VIEW (list->dest)->view,
                                     TRUE);
  gtk_box_pack_start (GTK_BOX (list->hbox), list->dest, TRUE, TRUE, 0);
  gtk_widget_show (list->dest);

  g_signal_connect_object (list->dest, "select-item",
                           G_CALLBACK (gimp_controller_list_select_item),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->dest, "activate-item",
                           G_CALLBACK (gimp_controller_list_activate_item),
                           G_OBJECT (list), 0);

  list->edit_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GIMP_ICON_DOCUMENT_PROPERTIES,
                            _("Configure the selected controller"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_edit_clicked),
                            NULL,
                            list);
  list->up_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GIMP_ICON_GO_UP,
                            _("Move the selected controller up"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_up_clicked),
                            NULL,
                            list);
  list->down_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GIMP_ICON_GO_DOWN,
                            _("Move the selected controller down"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_down_clicked),
                            NULL,
                            list);

  gtk_widget_set_sensitive (list->edit_button, FALSE);
  gtk_widget_set_sensitive (list->up_button,   FALSE);
  gtk_widget_set_sensitive (list->down_button, FALSE);
}

static void
gimp_controller_list_constructed (GObject *object)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (list->gimp));

  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (list->dest),
                                     gimp_controllers_get_list (list->gimp));

  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (list->dest),
                                   gimp_get_user_context (list->gimp));
}

static void
gimp_controller_list_finalize (GObject *object)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);

  g_clear_object (&list->gimp);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_controller_list_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_GIMP:
      list->gimp = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_controller_list_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, list->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_controller_list_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_CONTROLLER_LIST,
                       "gimp", gimp,
                       NULL);
}


/*  private functions  */

static void
gimp_controller_list_src_sel_changed (GtkTreeSelection   *sel,
                                      GimpControllerList *list)
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
      gimp_help_set_help_data (list->add_button, tip, NULL);
      g_free (tip);
    }
}

static void
gimp_controller_list_row_activated (GtkTreeView        *tv,
                                    GtkTreePath        *path,
                                    GtkTreeViewColumn  *column,
                                    GimpControllerList *list)
{
  if (gtk_widget_is_sensitive (list->add_button))
    gtk_button_clicked (GTK_BUTTON (list->add_button));
}

static void
gimp_controller_list_select_item (GimpContainerView  *view,
                                  GimpViewable       *viewable,
                                  gpointer            insert_data,
                                  GimpControllerList *list)
{
  gboolean selected;

  list->dest_info = GIMP_CONTROLLER_INFO (viewable);

  selected = GIMP_IS_CONTROLLER_INFO (list->dest_info);

  if (list->remove_button)
    {
      GimpObject *object = GIMP_OBJECT (list->dest_info);
      gchar      *tip    = NULL;

      gtk_widget_set_sensitive (list->remove_button, selected);

      if (selected)
        tip =
          g_strdup_printf (_("Remove '%s' from the list of active controllers"),
                           gimp_object_get_name (object));

      gimp_help_set_help_data (list->remove_button, tip, NULL);
      g_free (tip);
    }

  gtk_widget_set_sensitive (list->edit_button, selected);
  gtk_widget_set_sensitive (list->up_button,   selected);
  gtk_widget_set_sensitive (list->down_button, selected);
}

static void
gimp_controller_list_activate_item (GimpContainerView  *view,
                                    GimpViewable       *viewable,
                                    gpointer            insert_data,
                                    GimpControllerList *list)
{
  if (gtk_widget_is_sensitive (list->edit_button))
    gtk_button_clicked (GTK_BUTTON (list->edit_button));
}

static void
gimp_controller_list_add_clicked (GtkWidget          *button,
                                  GimpControllerList *list)
{
  GimpControllerInfo *info;
  GimpContainer      *container;

  if (list->src_gtype == GIMP_TYPE_CONTROLLER_KEYBOARD &&
      gimp_controllers_get_keyboard (list->gimp) != NULL)
    {
      gimp_message_literal (list->gimp,
                            G_OBJECT (button), GIMP_MESSAGE_WARNING,
                            _("There can only be one active keyboard "
                              "controller.\n\n"
                              "You already have a keyboard controller in "
                              "your list of active controllers."));
      return;
    }
  else if (list->src_gtype == GIMP_TYPE_CONTROLLER_WHEEL &&
           gimp_controllers_get_wheel (list->gimp) != NULL)
    {
      gimp_message_literal (list->gimp,
                            G_OBJECT (button), GIMP_MESSAGE_WARNING,
                            _("There can only be one active wheel "
                              "controller.\n\n"
                              "You already have a wheel controller in "
                              "your list of active controllers."));
      return;
    }
  else if (list->src_gtype == GIMP_TYPE_CONTROLLER_MOUSE &&
           gimp_controllers_get_mouse (list->gimp) != NULL)
    {
      gimp_message_literal (list->gimp,
                            G_OBJECT (button), GIMP_MESSAGE_WARNING,
                            _("There can only be one active mouse "
                              "controller.\n\n"
                              "You already have a mouse controller in "
                              "your list of active controllers."));
      return;
    }

  info = gimp_controller_info_new (list->src_gtype);
  container = gimp_controllers_get_list (list->gimp);
  gimp_container_add (container, GIMP_OBJECT (info));
  g_object_unref (info);

  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (list->dest),
                                   GIMP_VIEWABLE (info));
  gimp_controller_list_edit_clicked (list->edit_button, list);
}

static void
gimp_controller_list_remove_clicked (GtkWidget          *button,
                                     GimpControllerList *list)
{
  GtkWidget   *dialog;
  const gchar *name;

#define RESPONSE_DISABLE 1

  dialog = gimp_message_dialog_new (_("Remove Controller?"),
                                    GIMP_ICON_DIALOG_WARNING,
                                    GTK_WIDGET (list), GTK_DIALOG_MODAL,
                                    NULL, NULL,

                                    _("_Disable Controller"), RESPONSE_DISABLE,
                                    _("_Cancel"),             GTK_RESPONSE_CANCEL,
                                    _("_Remove Controller"),  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           RESPONSE_DISABLE,
                                           -1);

  name = gimp_object_get_name (list->dest_info);
  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Remove Controller '%s'?"), name);

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             "%s",
                             _("Removing this controller from the list of "
                               "active controllers will permanently delete "
                               "all event mappings you have configured.\n\n"
                               "Selecting \"Disable Controller\" will disable "
                               "the controller without removing it."));

  switch (gimp_dialog_run (GIMP_DIALOG (dialog)))
    {
    case RESPONSE_DISABLE:
      gimp_controller_info_set_enabled (list->dest_info, FALSE);
      break;

    case GTK_RESPONSE_OK:
      {
        GtkWidget     *editor_dialog;
        GimpContainer *container;

        editor_dialog = g_object_get_data (G_OBJECT (list->dest_info),
                                           "gimp-controller-editor-dialog");

        if (editor_dialog)
          gtk_dialog_response (GTK_DIALOG (editor_dialog),
                               GTK_RESPONSE_DELETE_EVENT);

        container = gimp_controllers_get_list (list->gimp);
        gimp_container_remove (container, GIMP_OBJECT (list->dest_info));
      }
      break;

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}

static void
gimp_controller_list_edit_clicked (GtkWidget          *button,
                                   GimpControllerList *list)
{
  GtkWidget *dialog;
  GtkWidget *editor;

  dialog = g_object_get_data (G_OBJECT (list->dest_info),
                              "gimp-controller-editor-dialog");

  if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  dialog = gimp_dialog_new (_("Configure Input Controller"),
                            "gimp-controller-editor-dialog",
                            gtk_widget_get_toplevel (GTK_WIDGET (list)),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            gimp_standard_help_func,
                            GIMP_HELP_PREFS_INPUT_CONTROLLERS,

                            _("_Close"), GTK_RESPONSE_CLOSE,

                            NULL);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_get_singleton (),
                                   "gimp-controller-editor-dialog",
                                   dialog,
                                   gtk_widget_get_screen (button),
                                   gimp_widget_get_monitor (button));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  editor = gimp_controller_editor_new (list->dest_info,
                                       gimp_get_user_context (list->gimp));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (list->dest_info), "gimp-controller-editor-dialog",
                     dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (gimp_controller_list_edit_destroy),
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
gimp_controller_list_edit_destroy (GtkWidget          *widget,
                                   GimpControllerInfo *info)
{
  g_object_set_data (G_OBJECT (info), "gimp-controller-editor-dialog", NULL);
}

static void
gimp_controller_list_up_clicked (GtkWidget          *button,
                                 GimpControllerList *list)
{
  GimpContainer *container;
  gint           index;

  container = gimp_controllers_get_list (list->gimp);

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (list->dest_info));

  if (index > 0)
    gimp_container_reorder (container, GIMP_OBJECT (list->dest_info),
                            index - 1);
}

static void
gimp_controller_list_down_clicked (GtkWidget          *button,
                                   GimpControllerList *list)
{
  GimpContainer *container;
  gint           index;

  container = gimp_controllers_get_list (list->gimp);

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (list->dest_info));

  if (index < gimp_container_get_n_children (container) - 1)
    gimp_container_reorder (container, GIMP_OBJECT (list->dest_info),
                            index + 1);
}
