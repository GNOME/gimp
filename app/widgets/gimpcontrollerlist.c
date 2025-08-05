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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpcontrollercategory.h"
#include "gimpcontrollereditor.h"
#include "gimpcontrollerlist.h"
#include "gimpcontrollerinfo.h"
#include "gimpcontrollerkeyboard.h"
#include "gimpcontrollermanager.h"
#include "gimpcontrollerwheel.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"
#include "gimppropwidgets.h"
#include "gimprow.h"
#include "gimprow-utils.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTROLLER_MANAGER,
  N_PROPS
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

static GimpControllerCategory *
            gimp_controller_list_get_selected_category
                                                 (GimpControllerList  *list);
static void gimp_controller_list_category_row_selected
                                                 (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);
static void gimp_controller_list_category_row_activated
                                                 (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);

static GimpControllerInfo *
            gimp_controller_list_get_selected_controller
                                                 (GimpControllerList  *list);
static void gimp_controller_list_row_selected    (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);
static void gimp_controller_list_row_activated   (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);

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

  g_object_class_install_property (object_class, PROP_CONTROLLER_MANAGER,
                                   g_param_spec_object ("controller-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTROLLER_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_controller_list_init (GimpControllerList      *list)
{
  GtkWidget *hbox;
  GtkWidget *sw;
  GtkWidget *vbox;
  GtkWidget *image;
  GtkWidget *label;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (list),
                                  GTK_ORIENTATION_VERTICAL);

  list->controller_manager = NULL;

  list->hbox = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (list), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (list->hbox), vbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Available Controllers"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  list->available_controllers = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list->available_controllers),
                                             FALSE);
  gtk_widget_show (list->available_controllers);
  gtk_container_add (GTK_CONTAINER (sw), list->available_controllers);

  g_signal_connect_object (list->available_controllers, "row-activated",
                           G_CALLBACK (gimp_controller_list_category_row_activated),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->available_controllers, "row-selected",
                           G_CALLBACK (gimp_controller_list_category_row_selected),
                           G_OBJECT (list), 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  g_set_weak_pointer (&list->add_button, gtk_button_new ());
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

  g_set_weak_pointer (&list->remove_button, gtk_button_new ());
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (list->hbox), vbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Active Controllers"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  list->active_controllers = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list->active_controllers),
                                             FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), list->active_controllers, TRUE, TRUE, 0);
  gtk_widget_show (list->active_controllers);

  g_signal_connect_object (list->active_controllers, "row-selected",
                           G_CALLBACK (gimp_controller_list_row_selected),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->active_controllers, "row-activated",
                           G_CALLBACK (gimp_controller_list_row_activated),
                           G_OBJECT (list), 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  list->edit_button = gtk_button_new_from_icon_name (GIMP_ICON_DOCUMENT_PROPERTIES, GTK_ICON_SIZE_BUTTON);
  gimp_help_set_help_data (list->edit_button, _("Configure the selected controller"), NULL);
  gtk_widget_show (list->edit_button);
  gtk_box_pack_start (GTK_BOX (hbox), list->edit_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->edit_button, "clicked",
                           G_CALLBACK (gimp_controller_list_edit_clicked),
                           list, 0);

  list->up_button = gtk_button_new_from_icon_name (GIMP_ICON_GO_UP, GTK_ICON_SIZE_BUTTON);
  gimp_help_set_help_data (list->up_button, _("Move the selected controller up"), NULL);
  gtk_widget_show (list->up_button);
  gtk_box_pack_start (GTK_BOX (hbox), list->up_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->up_button, "clicked",
                           G_CALLBACK (gimp_controller_list_up_clicked),
                           list, 0);

  list->down_button = gtk_button_new_from_icon_name (GIMP_ICON_GO_DOWN, GTK_ICON_SIZE_BUTTON);
  gimp_help_set_help_data (list->down_button, _("Move the selected controller down"), NULL);
  gtk_widget_show (list->down_button);
  gtk_box_pack_start (GTK_BOX (hbox), list->down_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->down_button, "clicked",
                           G_CALLBACK (gimp_controller_list_down_clicked),
                           list, 0);

  gtk_widget_set_sensitive (list->edit_button, FALSE);
  gtk_widget_set_sensitive (list->up_button,   FALSE);
  gtk_widget_set_sensitive (list->down_button, FALSE);
}

static void
gimp_controller_list_constructed (GObject *object)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);
  GListModel         *categories;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  categories = gimp_controller_manager_get_categories (list->controller_manager);
  gtk_list_box_bind_model (GTK_LIST_BOX (list->available_controllers),
                           categories,
                           gimp_row_create_for_context,
                           gimp_get_user_context (gimp_controller_manager_get_gimp (list->controller_manager)),
                           NULL);

  gtk_list_box_bind_model (GTK_LIST_BOX (list->active_controllers),
                           G_LIST_MODEL (list->controller_manager),
                           gimp_row_create_for_context,
                           gimp_get_user_context (gimp_controller_manager_get_gimp (list->controller_manager)),
                           NULL);
}

static void
gimp_controller_list_finalize (GObject *object)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (object);

  g_clear_object (&list->controller_manager);

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
    case PROP_CONTROLLER_MANAGER:
      list->controller_manager = g_value_dup_object (value);
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
    case PROP_CONTROLLER_MANAGER:
      g_value_set_object (value, list->controller_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_controller_list_new (GimpControllerManager *controller_manager)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (controller_manager), NULL);

  return g_object_new (GIMP_TYPE_CONTROLLER_LIST,
                       "controller-manager", controller_manager,
                       NULL);
}


/*  private functions  */

static GimpControllerCategory *
gimp_controller_list_get_selected_category (GimpControllerList *list)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (list->available_controllers));
  if (row == NULL)
    return NULL;

  return GIMP_CONTROLLER_CATEGORY (gimp_row_get_viewable (GIMP_ROW (row)));
}

static void
gimp_controller_list_category_row_selected (GtkListBox    *self,
                                            GtkListBoxRow *row,
                                            gpointer       user_data)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (user_data);
  gchar              *tip = NULL;

  if (row != NULL)
    {
      GimpControllerCategory *category;

      category = gimp_controller_list_get_selected_category (list);
      if (list->add_button)
        {
          tip =
            g_strdup_printf (_("Add '%s' to the list of active controllers"),
                             gimp_object_get_name (category));
          gtk_widget_set_sensitive (list->add_button, TRUE);
        }
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
gimp_controller_list_category_row_activated (GtkListBox    *self,
                                             GtkListBoxRow *row,
                                             gpointer       user_data)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (user_data);

  if (gtk_widget_is_sensitive (list->add_button))
    gtk_button_clicked (GTK_BUTTON (list->add_button));
}

static GimpControllerInfo *
gimp_controller_list_get_selected_controller (GimpControllerList *list)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (list->active_controllers));
  if (row == NULL)
    return NULL;

  return GIMP_CONTROLLER_INFO (gimp_row_get_viewable (GIMP_ROW (row)));
}

static void
gimp_controller_list_row_selected (GtkListBox    *self,
                                   GtkListBoxRow *row,
                                   gpointer       user_data)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (user_data);
  GimpControllerInfo *info;
  gboolean            selected;

  info = gimp_controller_list_get_selected_controller (list);
  selected = (info != NULL);

  if (list->remove_button)
    {
      gchar *tip = NULL;

      gtk_widget_set_sensitive (list->remove_button, selected);

      if (selected)
        tip =
          g_strdup_printf (_("Remove '%s' from the list of active controllers"),
                           gimp_object_get_name (GIMP_OBJECT (info)));

      gimp_help_set_help_data (list->remove_button, tip, NULL);
      g_free (tip);
    }

  gtk_widget_set_sensitive (list->edit_button, selected);
  gtk_widget_set_sensitive (list->up_button,   selected);
  gtk_widget_set_sensitive (list->down_button, selected);
}

static void
gimp_controller_list_row_activated (GtkListBox    *self,
                                    GtkListBoxRow *row,
                                    gpointer       user_data)
{
  GimpControllerList *list = GIMP_CONTROLLER_LIST (user_data);

  if (gtk_widget_is_sensitive (list->edit_button))
    gtk_button_clicked (GTK_BUTTON (list->edit_button));
}

static void
gimp_controller_list_add_clicked (GtkWidget          *button,
                                  GimpControllerList *list)
{
  GimpControllerInfo     *info;
  Gimp                   *gimp;
  GimpControllerCategory *category;
  GType                   gtype;
  gint                    position;
  GtkListBoxRow          *row;

  gimp = gimp_controller_manager_get_gimp (list->controller_manager);

  category = gimp_controller_list_get_selected_category (list);
  g_return_if_fail (category != NULL);

  gtype = gimp_controller_category_get_gtype (category);
  if (gtype == GIMP_TYPE_CONTROLLER_KEYBOARD &&
      gimp_controller_manager_get_keyboard (list->controller_manager) != NULL)
    {
      gimp_message_literal (gimp,
                            G_OBJECT (button), GIMP_MESSAGE_WARNING,
                            _("There can only be one active keyboard "
                              "controller.\n\n"
                              "You already have a keyboard controller in "
                              "your list of active controllers."));
      return;
    }
  else if (gtype == GIMP_TYPE_CONTROLLER_WHEEL &&
           gimp_controller_manager_get_wheel (list->controller_manager) != NULL)
    {
      gimp_message_literal (gimp,
                            G_OBJECT (button), GIMP_MESSAGE_WARNING,
                            _("There can only be one active wheel "
                              "controller.\n\n"
                              "You already have a wheel controller in "
                              "your list of active controllers."));
      return;
    }

  info = gimp_controller_info_new (gtype);
  gimp_container_add (GIMP_CONTAINER (list->controller_manager),
                      GIMP_OBJECT (info));
  g_object_unref (info);

  /* Make sure it's selected for when we press "edit" */
  position =
    gimp_container_get_child_index (GIMP_CONTAINER (list->controller_manager),
                                    GIMP_OBJECT (info));

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       position);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);

  gimp_controller_list_edit_clicked (list->edit_button, list);
}

static void
gimp_controller_list_remove_clicked (GtkWidget          *button,
                                     GimpControllerList *list)
{
  GtkWidget          *dialog;
  GimpControllerInfo *info;
  const gchar        *name;

#define RESPONSE_DISABLE 1

  dialog = gimp_message_dialog_new (_("Remove Controller?"),
                                    GIMP_ICON_DIALOG_WARNING,
                                    GTK_WIDGET (list), GTK_DIALOG_MODAL,
                                    NULL, NULL,

                                    _("_Disable Controller"), RESPONSE_DISABLE,
                                    _("_Cancel"),             GTK_RESPONSE_CANCEL,
                                    _("_Remove Controller"),  GTK_RESPONSE_OK,

                                    NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           RESPONSE_DISABLE,
                                           -1);

  info = gimp_controller_list_get_selected_controller (list);
  name = gimp_object_get_name (info);
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
      gimp_controller_info_set_enabled (info, FALSE);
      break;

    case GTK_RESPONSE_OK:
      {
        GtkWidget     *editor_dialog;

        editor_dialog = g_object_get_data (G_OBJECT (info),
                                           "gimp-controller-editor-dialog");

        if (editor_dialog)
          gtk_dialog_response (GTK_DIALOG (editor_dialog),
                               GTK_RESPONSE_DELETE_EVENT);

        gimp_container_remove (GIMP_CONTAINER (list->controller_manager),
                               GIMP_OBJECT (info));
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
  GimpControllerInfo *info;
  GtkWidget          *dialog;
  GtkWidget          *editor;
  Gimp               *gimp;

  info = gimp_controller_list_get_selected_controller (list);
  dialog = g_object_get_data (G_OBJECT (info),
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
                                   gimp_widget_get_monitor (button));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  gimp = gimp_controller_manager_get_gimp (list->controller_manager);
  editor = gimp_controller_editor_new (info,
                                       gimp_get_user_context (gimp));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (info), "gimp-controller-editor-dialog",
                     dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (gimp_controller_list_edit_destroy),
                           G_OBJECT (info), 0);

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
  GimpControllerInfo *info;
  GtkListBoxRow      *row;
  gint                old_position;

  info = gimp_controller_list_get_selected_controller (list);

  old_position =
    gimp_container_get_child_index (GIMP_CONTAINER (list->controller_manager),
                                    GIMP_OBJECT (info));
  if (old_position == 0)
    return;

  gimp_container_reorder (GIMP_CONTAINER (list->controller_manager),
                          GIMP_OBJECT (info),
                          old_position - 1);

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       old_position - 1);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);
}

static void
gimp_controller_list_down_clicked (GtkWidget          *button,
                                   GimpControllerList *list)
{
  GimpControllerInfo *info;
  GtkListBoxRow      *row;
  gint                old_position;
  gint                n_controllers;

  info = gimp_controller_list_get_selected_controller (list);

  old_position =
    gimp_container_get_child_index (GIMP_CONTAINER (list->controller_manager),
                                    GIMP_OBJECT (info));

  n_controllers =
    gimp_container_get_n_children (GIMP_CONTAINER (list->controller_manager));

  if (old_position == n_controllers - 1)
    return;

  gimp_container_reorder (GIMP_CONTAINER (list->controller_manager),
                          GIMP_OBJECT (info),
                          old_position + 1);

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       old_position + 1);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);
}
