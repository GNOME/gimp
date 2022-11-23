/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasearchpopup.c
 * Copyright (C) 2015 Jehan <jehan at girinstud.io>
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

#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"

#include "ligmaaction.h"
#include "ligmapopup.h"
#include "ligmasearchpopup.h"
#include "ligmatoggleaction.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


enum
{
  COLUMN_ICON,
  COLUMN_MARKUP,
  COLUMN_TOOLTIP,
  COLUMN_ACTION,
  COLUMN_SENSITIVE,
  COLUMN_SECTION,
  N_COL
};

enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_CALLBACK,
  PROP_CALLBACK_DATA
};


struct _LigmaSearchPopupPrivate
{
  Ligma                    *ligma;
  GtkWidget               *keyword_entry;
  GtkWidget               *results_list;
  GtkWidget               *list_view;

  LigmaSearchPopupCallback  build_results;
  gpointer                 build_results_data;
};


static void       ligma_search_popup_constructed         (GObject            *object);
static void       ligma_search_popup_set_property        (GObject            *object,
                                                         guint               property_id,
                                                         const GValue       *value,
                                                         GParamSpec         *pspec);
static void       ligma_search_popup_get_property        (GObject            *object,
                                                         guint               property_id,
                                                         GValue             *value,
                                                         GParamSpec         *pspec);

static void       ligma_search_popup_size_allocate        (GtkWidget         *widget,
                                                          GtkAllocation     *allocation);

static void       ligma_search_popup_confirm              (LigmaPopup *popup);

/* Signal handlers on the search entry */
static gboolean   keyword_entry_key_press_event          (GtkWidget         *widget,
                                                          GdkEventKey       *event,
                                                          LigmaSearchPopup   *popup);
static gboolean   keyword_entry_key_release_event        (GtkWidget         *widget,
                                                          GdkEventKey       *event,
                                                          LigmaSearchPopup   *popup);

/* Signal handlers on the results list */
static gboolean   results_list_key_press_event           (GtkWidget         *widget,
                                                          GdkEventKey       *kevent,
                                                          LigmaSearchPopup   *popup);
static void       results_list_row_activated             (GtkTreeView       *treeview,
                                                          GtkTreePath       *path,
                                                          GtkTreeViewColumn *col,
                                                          LigmaSearchPopup   *popup);

/* Utils */
static void       ligma_search_popup_run_selected         (LigmaSearchPopup   *popup);
static void       ligma_search_popup_setup_results        (GtkWidget        **results_list,
                                                          GtkWidget        **list_view);

static gchar    * ligma_search_popup_find_accel_label     (LigmaAction        *action);
static gboolean   ligma_search_popup_view_accel_find_func (GtkAccelKey       *key,
                                                          GClosure          *closure,
                                                          gpointer           data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaSearchPopup, ligma_search_popup, LIGMA_TYPE_POPUP)

#define parent_class ligma_search_popup_parent_class

static gint window_height = 0;


static void
ligma_search_popup_class_init (LigmaSearchPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaPopupClass *popup_class  = LIGMA_POPUP_CLASS (klass);

  object_class->constructed   = ligma_search_popup_constructed;
  object_class->set_property  = ligma_search_popup_set_property;
  object_class->get_property  = ligma_search_popup_get_property;

  widget_class->size_allocate = ligma_search_popup_size_allocate;

  popup_class->confirm        = ligma_search_popup_confirm;

  /**
   * LigmaSearchPopup:ligma:
   *
   * The #Ligma object.
   */
  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  /**
   * LigmaSearchPopup:callback:
   *
   * The #LigmaSearchPopupCallback used to fill in results.
   */
  g_object_class_install_property (object_class, PROP_CALLBACK,
                                   g_param_spec_pointer ("callback", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  /**
   * LigmaSearchPopup:callback-data:
   *
   * The #GPointer fed as last parameter to the #LigmaSearchPopupCallback.
   */
  g_object_class_install_property (object_class, PROP_CALLBACK_DATA,
                                   g_param_spec_pointer ("callback-data", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_search_popup_init (LigmaSearchPopup *search_popup)
{
  search_popup->priv = ligma_search_popup_get_instance_private (search_popup);
}

/************ Public Functions ****************/

/**
 * ligma_search_popup_new:
 * @ligma:          #Ligma object.
 * @role:          the role to give to the #GtkWindow.
 * @title:         the #GtkWindow title.
 * @callback:      the #LigmaSearchPopupCallback used to fill in results.
 * @callback_data: data fed to @callback.
 *
 * Returns: a new #LigmaSearchPopup.
 */
GtkWidget *
ligma_search_popup_new (Ligma                    *ligma,
                       const gchar             *role,
                       const gchar             *title,
                       LigmaSearchPopupCallback  callback,
                       gpointer                 callback_data)
{
  GtkWidget *widget;

  widget = g_object_new (LIGMA_TYPE_SEARCH_POPUP,
                         "type",          GTK_WINDOW_TOPLEVEL,
                         "type-hint",     GDK_WINDOW_TYPE_HINT_DIALOG,
                         "decorated",     TRUE,
                         "modal",         TRUE,
                         "role",          role,
                         "title",         title,

                         "ligma",          ligma,
                         "callback",      callback,
                         "callback-data", callback_data,
                         NULL);
  gtk_window_set_modal (GTK_WINDOW (widget), FALSE);


  return widget;
}

/**
 * ligma_search_popup_add_result:
 * @popup:   the #LigmaSearchPopup.
 * @action:  a #LigmaAction to add in results list.
 * @section: the section to add @action.
 *
 * Adds @action in the @popup's results at @section.
 * The section only indicates relative order. If you want some items
 * to appear before other, simply use lower @section.
 */
void
ligma_search_popup_add_result (LigmaSearchPopup *popup,
                              LigmaAction      *action,
                              gint             section)
{
  GtkTreeIter   iter;
  GtkTreeIter   next_section;
  GtkListStore *store;
  GtkTreeModel *model;
  gchar        *markup;
  gchar        *action_name;
  gchar        *label;
  gchar        *escaped_label    = NULL;
  const gchar  *icon_name;
  gchar        *accel_string;
  gchar        *escaped_accel    = NULL;
  gboolean      has_shortcut     = FALSE;
  const gchar  *tooltip;
  gchar        *escaped_tooltip  = NULL;
  gboolean      has_tooltip      = FALSE;
  gboolean      sensitive        = FALSE;
  const gchar  *sensitive_reason = NULL;
  gchar        *escaped_reason   = NULL;

  label = g_strstrip (ligma_strip_uline (ligma_action_get_label (action)));

  if (! label || strlen (label) == 0)
    {
      g_free (label);
      return;
    }

  escaped_label = g_markup_escape_text (label, -1);

  if (LIGMA_IS_TOGGLE_ACTION (action))
    {
      if (ligma_toggle_action_get_active (LIGMA_TOGGLE_ACTION (action)))
        icon_name = "gtk-ok";
      else
        icon_name = "gtk-no";
    }
  else
    {
      icon_name = ligma_action_get_icon_name (action);
    }

  accel_string = ligma_search_popup_find_accel_label (action);
  if (accel_string)
    {
      escaped_accel = g_markup_escape_text (accel_string, -1);
      has_shortcut = TRUE;
    }

  tooltip = ligma_action_get_tooltip (action);
  if (tooltip != NULL)
    {
      escaped_tooltip = g_markup_escape_text (tooltip, -1);
      has_tooltip = TRUE;
    }

  sensitive = ligma_action_is_sensitive (action, &sensitive_reason);
  if (sensitive_reason != NULL)
    {
      escaped_reason = g_markup_escape_text (sensitive_reason, -1);
    }

  markup = g_strdup_printf ("%s"                                           /* Label           */
                            "<small>%s%s"                                  /* Shortcut        */
                            "%s<span weight='light'>%s</span>"             /* Tooltip         */
                            "%s<i><span weight='ultralight'>%s</span></i>" /* Inactive reason */
                            "</small>",
                            escaped_label,
                            has_shortcut ? " | " : "",
                            has_shortcut ? escaped_accel : "",
                            has_tooltip ? "\n" : "",
                            has_tooltip ? escaped_tooltip : "",
                            escaped_reason ? "\n" : "",
                            escaped_reason ? escaped_reason : "");

  action_name = g_markup_escape_text (ligma_action_get_name (action), -1);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (popup->priv->results_list));
  store = GTK_LIST_STORE (model);
  if (gtk_tree_model_get_iter_first (model, &next_section))
    {
      while (TRUE)
        {
          gint iter_section;

          gtk_tree_model_get (model, &next_section,
                              COLUMN_SECTION, &iter_section, -1);
          if (iter_section > section)
            {
              gtk_list_store_insert_before (store, &iter, &next_section);
              break;
            }
          else if (! gtk_tree_model_iter_next (model, &next_section))
            {
              gtk_list_store_append (store, &iter);
              break;
            }
        }
    }
  else
    {
      gtk_list_store_append (store, &iter);
    }

  gtk_list_store_set (store, &iter,
                      COLUMN_ICON,      icon_name,
                      COLUMN_MARKUP,    markup,
                      COLUMN_TOOLTIP,   action_name,
                      COLUMN_ACTION,    action,
                      COLUMN_SECTION,   section,
                      COLUMN_SENSITIVE, sensitive,
                      -1);

  g_free (accel_string);
  g_free (markup);
  g_free (action_name);
  g_free (label);
  g_free (escaped_accel);
  g_free (escaped_label);
  g_free (escaped_tooltip);
  g_free (escaped_reason);
}

/************ Private Functions ****************/

static void
ligma_search_popup_constructed (GObject *object)
{
  LigmaSearchPopup *popup = LIGMA_SEARCH_POPUP (object);
  GtkWidget       *main_vbox;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (popup), main_vbox);
  gtk_widget_show (main_vbox);

  popup->priv->keyword_entry = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (popup->priv->keyword_entry),
                                     GTK_ENTRY_ICON_PRIMARY, "edit-find");
  gtk_entry_set_icon_activatable (GTK_ENTRY (popup->priv->keyword_entry),
                                  GTK_ENTRY_ICON_PRIMARY, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox),
                      popup->priv->keyword_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (popup->priv->keyword_entry);

  ligma_search_popup_setup_results (&popup->priv->results_list,
                                   &popup->priv->list_view);
  gtk_box_pack_start (GTK_BOX (main_vbox),
                      popup->priv->list_view, TRUE, TRUE, 0);

  gtk_widget_set_events (GTK_WIDGET (object),
                         GDK_KEY_RELEASE_MASK  |
                         GDK_KEY_PRESS_MASK    |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_SCROLL_MASK       |
                         GDK_SMOOTH_SCROLL_MASK);

  g_signal_connect (popup->priv->keyword_entry, "key-press-event",
                    G_CALLBACK (keyword_entry_key_press_event),
                    popup);
  g_signal_connect (popup->priv->keyword_entry, "key-release-event",
                    G_CALLBACK (keyword_entry_key_release_event),
                    popup);

  g_signal_connect (popup->priv->results_list, "key-press-event",
                    G_CALLBACK (results_list_key_press_event),
                    popup);
  g_signal_connect (popup->priv->results_list, "row-activated",
                    G_CALLBACK (results_list_row_activated),
                    popup);
}

static void
ligma_search_popup_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaSearchPopup *search_popup = LIGMA_SEARCH_POPUP (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      search_popup->priv->ligma = g_value_get_object (value);
      break;
    case PROP_CALLBACK:
      search_popup->priv->build_results = g_value_get_pointer (value);
      break;
    case PROP_CALLBACK_DATA:
      search_popup->priv->build_results_data = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_search_popup_get_property (GObject      *object,
                                guint         property_id,
                                GValue       *value,
                                GParamSpec   *pspec)
{
  LigmaSearchPopup *search_popup = LIGMA_SEARCH_POPUP (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, search_popup->priv->ligma);
      break;
    case PROP_CALLBACK:
      g_value_set_pointer (value, search_popup->priv->build_results);
      break;
    case PROP_CALLBACK_DATA:
      g_value_set_pointer (value, search_popup->priv->build_results_data);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_search_popup_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  LigmaSearchPopup *popup = LIGMA_SEARCH_POPUP (widget);
  GdkRectangle     workarea;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gdk_monitor_get_workarea (ligma_widget_get_monitor (widget),
                            &workarea);

  if (window_height == 0)
    {
      /* Default to half the monitor */
      window_height = workarea.height / 2;
    }

  if (gtk_widget_get_visible (widget) &&
      gtk_widget_get_visible (popup->priv->list_view))
    {
      /* Save the window height when results are shown so that resizes
       * by the user are saved across searches.
       */
      window_height = MAX (workarea.height / 4, allocation->height);
    }
}

static void
ligma_search_popup_confirm (LigmaPopup *popup)
{
  LigmaSearchPopup *search_popup = LIGMA_SEARCH_POPUP (popup);

  ligma_search_popup_run_selected (search_popup);
}

static gboolean
keyword_entry_key_press_event (GtkWidget       *widget,
                               GdkEventKey     *event,
                               LigmaSearchPopup *popup)
{
  gboolean event_processed = FALSE;

  if (event->keyval == GDK_KEY_Down &&
      gtk_widget_get_visible (popup->priv->list_view))
    {
      GtkTreeView *tree_view = GTK_TREE_VIEW (popup->priv->results_list);
      GtkTreePath *path;

      /* When hitting the down key while editing, select directly the
       * second item, since the first could have run directly with
       * Enter. */
      path = gtk_tree_path_new_from_string ("1");
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree_view), path);
      gtk_tree_path_free (path);

      gtk_widget_grab_focus (GTK_WIDGET (popup->priv->results_list));
      event_processed = TRUE;
    }

  return event_processed;
}

static gboolean
keyword_entry_key_release_event (GtkWidget       *widget,
                                 GdkEventKey     *event,
                                 LigmaSearchPopup *popup)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW (popup->priv->results_list);
  GtkTreePath *path;
  gchar       *entry_text;
  gint         width;

  /* These keys are already managed by key bindings. */
  if (event->keyval == GDK_KEY_Escape   ||
      event->keyval == GDK_KEY_Return   ||
      event->keyval == GDK_KEY_KP_Enter ||
      event->keyval == GDK_KEY_ISO_Enter)
    {
      return FALSE;
    }

  gtk_window_get_size (GTK_WINDOW (popup), &width, NULL);
  entry_text = g_strstrip (gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1));

  if (strcmp (entry_text, "") != 0)
    {
      path = gtk_tree_path_new_from_string ("0");
      gtk_window_resize (GTK_WINDOW (popup),
                         width, window_height);
      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (tree_view)));
      gtk_widget_show_all (popup->priv->list_view);
      popup->priv->build_results (popup, entry_text,
                                  popup->priv->build_results_data);
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree_view), path);
      gtk_tree_path_free (path);
    }
  else if (strcmp (entry_text, "") == 0 && (event->keyval == GDK_KEY_Down))
    {
      path = gtk_tree_path_new_from_string ("0");
      gtk_window_resize (GTK_WINDOW (popup),
                         width, window_height);
      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (tree_view)));
      gtk_widget_show_all (popup->priv->list_view);
      popup->priv->build_results (popup, NULL,
                                  popup->priv->build_results_data);
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree_view), path);
      gtk_tree_path_free (path);
    }
  else
    {
      GtkTreeSelection *selection;
      GtkTreeModel     *model;
      GtkTreeIter       iter;

      selection = gtk_tree_view_get_selection (tree_view);
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_selection_unselect_path (selection, path);

          gtk_tree_path_free (path);
        }

      gtk_widget_hide (popup->priv->list_view);
      gtk_window_resize (GTK_WINDOW (popup), width, 1);
    }

  g_free (entry_text);

  return TRUE;
}

static gboolean
results_list_key_press_event (GtkWidget       *widget,
                              GdkEventKey     *kevent,
                              LigmaSearchPopup *popup)
{
  /* These keys are already managed by key bindings. */
  g_return_val_if_fail (kevent->keyval != GDK_KEY_Escape   &&
                        kevent->keyval != GDK_KEY_Return   &&
                        kevent->keyval != GDK_KEY_KP_Enter &&
                        kevent->keyval != GDK_KEY_ISO_Enter,
                        FALSE);

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
      {
        gboolean          event_processed = FALSE;
        GtkTreeSelection *selection;
        GtkTreeModel     *model;
        GtkTreeIter       iter;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (popup->priv->results_list));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
          {
            GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
            gchar       *path_str;

            path_str = gtk_tree_path_to_string (path);
            if (strcmp (path_str, "0") == 0)
              {
                gint start_pos;
                gint end_pos;

                gtk_editable_get_selection_bounds (GTK_EDITABLE (popup->priv->keyword_entry),
                                                   &start_pos, &end_pos);
                gtk_widget_grab_focus ((GTK_WIDGET (popup->priv->keyword_entry)));
                gtk_editable_select_region (GTK_EDITABLE (popup->priv->keyword_entry),
                                            start_pos, end_pos);

                event_processed = TRUE;
              }

            g_free (path_str);
            gtk_tree_path_free (path);
          }

        return event_processed;
      }
    case GDK_KEY_Down:
      {
        return FALSE;
      }
    default:
      {
        gint start_pos;
        gint end_pos;

        gtk_editable_get_selection_bounds (GTK_EDITABLE (popup->priv->keyword_entry),
                                           &start_pos, &end_pos);
        gtk_widget_grab_focus ((GTK_WIDGET (popup->priv->keyword_entry)));
        gtk_editable_select_region (GTK_EDITABLE (popup->priv->keyword_entry),
                                    start_pos, end_pos);
        gtk_widget_event (GTK_WIDGET (popup->priv->keyword_entry),
                          (GdkEvent *) kevent);
      }
    }

  return FALSE;
}

static void
results_list_row_activated (GtkTreeView       *treeview,
                            GtkTreePath       *path,
                            GtkTreeViewColumn *col,
                            LigmaSearchPopup   *popup)
{
  ligma_search_popup_run_selected (popup);
}

static void
ligma_search_popup_run_selected (LigmaSearchPopup *popup)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (popup->priv->results_list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      LigmaAction *action;

      gtk_tree_model_get (model, &iter, COLUMN_ACTION, &action, -1);

      if (ligma_action_is_sensitive (action, NULL))
        {
          /* Close the search popup on activation. */
          LIGMA_POPUP_CLASS (parent_class)->cancel (LIGMA_POPUP (popup));

          ligma_action_activate (action);
        }

      g_object_unref (action);
    }
}

static void
ligma_search_popup_setup_results (GtkWidget **results_list,
                                 GtkWidget **list_view)
{
  gint                wid1 = 100;
  GtkListStore       *store;
  GtkCellRenderer    *cell;
  GtkTreeViewColumn  *column;

  *list_view = gtk_scrolled_window_new (NULL, NULL);
  store = gtk_list_store_new (N_COL,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              LIGMA_TYPE_ACTION,
                              G_TYPE_BOOLEAN,
                              G_TYPE_INT);
  *results_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (*results_list), FALSE);
#ifdef LIGMA_UNSTABLE
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (*results_list),
                                    COLUMN_TOOLTIP);
#endif

  cell = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, cell,
                                                     "icon-name", COLUMN_ICON,
                                                     "sensitive", COLUMN_SENSITIVE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (*results_list), column);
  gtk_tree_view_column_set_min_width (column, 22);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, cell,
                                                     "markup",    COLUMN_MARKUP,
                                                     "sensitive", COLUMN_SENSITIVE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (*results_list), column);
  gtk_tree_view_column_set_max_width (column, wid1);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (*list_view),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (*list_view), *results_list);
  g_object_unref (G_OBJECT (store));
}

static gchar *
ligma_search_popup_find_accel_label (LigmaAction *action)
{
  guint            accel_key     = 0;
  GdkModifierType  accel_mask    = 0;
  GClosure        *accel_closure = NULL;
  gchar           *accel_string;
  GtkAccelGroup   *accel_group;
  LigmaUIManager   *manager;

  manager       = ligma_ui_managers_from_name ("<Image>")->data;
  accel_group   = ligma_ui_manager_get_accel_group (manager);
  accel_closure = ligma_action_get_accel_closure (action);

  if (accel_closure)
    {
      GtkAccelKey *key;

      key = gtk_accel_group_find (accel_group,
                                  ligma_search_popup_view_accel_find_func,
                                  accel_closure);
      if (key            &&
          key->accel_key &&
          key->accel_flags & GTK_ACCEL_VISIBLE)
        {
          accel_key  = key->accel_key;
          accel_mask = key->accel_mods;
        }
    }

  accel_string = gtk_accelerator_get_label (accel_key, accel_mask);

  if (strcmp (g_strstrip (accel_string), "") == 0)
    {
      /* The value returned by gtk_accelerator_get_label() must be
       * freed after use.
       */
      g_clear_pointer (&accel_string, g_free);
    }

  return accel_string;
}

static gboolean
ligma_search_popup_view_accel_find_func (GtkAccelKey *key,
                                        GClosure    *closure,
                                        gpointer     data)
{
  return (GClosure *) data == closure;
}
