/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactionview.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimpaction.h"
#include "gimpactiongroup.h"
#include "gimpactionview.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  COLUMN_ACTION,
  COLUMN_STOCK_ID,
  COLUMN_LABEL,
  COLUMN_NAME,
  COLUMN_SHORTCUT,
  COLUMN_MENU_ITEM,
  NUM_COLUMNS
};


/*  local function prototypes  */

static void     gimp_action_view_class_init  (GimpActionViewClass *klass);
static void     gimp_action_view_init        (GimpActionView      *view);

static gchar  * gimp_action_view_get_shortcut    (GtkAccelGroup   *group,
                                                  GClosure        *accel_closure);
static gboolean gimp_action_view_accel_find_func (GtkAccelKey     *key,
                                                  GClosure        *closure,
                                                  gpointer         data);
static void     gimp_action_view_accel_changed   (GtkAccelGroup   *accel_group,
                                                  guint            unused1,
                                                  GdkModifierType  unused2,
                                                  GClosure        *accel_closure,
                                                  GimpActionView  *view);


GType
gimp_action_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpActionViewClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_action_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpActionView),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_action_view_init
      };

      view_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
                                          "GimpActionView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_action_view_class_init (GimpActionViewClass *klass)
{
}

static void
gimp_action_view_init (GimpActionView *view)
{
}

GtkWidget *
gimp_action_view_new (GimpUIManager *manager,
                      gboolean       show_shortcuts)
{
  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkTreeStore      *store;
  GtkAccelGroup     *accel_group;
  GList             *list;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  store = gtk_tree_store_new (NUM_COLUMNS,
                              GTK_TYPE_ACTION,     /* COLUMN_ACTION    */
                              G_TYPE_STRING,       /* COLUMN_STOCK_ID  */
                              G_TYPE_STRING,       /* COLUMN_LABEL     */
                              G_TYPE_STRING,       /* COLUMN_NAME      */
                              G_TYPE_STRING,       /* COLUMN_SHORTCUT  */
                              GTK_TYPE_MENU_ITEM); /* COLUMN_MENU_ITEM */

  accel_group = gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (manager));

  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
       list;
       list = g_list_next (list))
    {
      GimpActionGroup *group = list->data;
      GList           *actions;
      GList           *list2;
      GtkTreeIter      group_iter;

      gtk_tree_store_append (store, &group_iter, NULL);

      gtk_tree_store_set (store, &group_iter,
                          COLUMN_ACTION,    NULL,
                          COLUMN_STOCK_ID,  group->stock_id,
                          COLUMN_LABEL,     group->label,
                          COLUMN_NAME,      NULL,
                          COLUMN_SHORTCUT,  NULL,
                          COLUMN_MENU_ITEM, NULL,
                          -1);

      actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));

      actions = g_list_sort (actions, (GCompareFunc) gimp_action_name_compare);

      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          GtkAction   *action = list2->data;
          const gchar *name   = gtk_action_get_name (action);

          if (! strstr (name, "-menu") &&
              ! strstr (name, "-popup"))
            {
              GtkTreeIter  action_iter;
              gchar       *stock_id;
              gchar       *label;
              gchar       *stripped;
              gchar       *shortcut  = NULL;
              GtkWidget   *menu_item = NULL;

              g_object_get (action,
                            "stock-id", &stock_id,
                            "label",    &label,
                            NULL);

              stripped = gimp_strip_uline (label);

              if (show_shortcuts)
                {
                  gtk_action_set_accel_group (action, accel_group);

                  menu_item = gtk_action_create_menu_item (action);

                  if (GTK_IS_MENU_ITEM (menu_item) &&
                      GTK_IS_ACCEL_LABEL (GTK_BIN (menu_item)->child))
                    {
                      GtkWidget *accel_label = GTK_BIN (menu_item)->child;
                      GClosure  *accel_closure;

                      g_object_get (accel_label,
                                    "accel-closure", &accel_closure,
                                    NULL);

                      if (accel_closure)
                        shortcut =
                          gimp_action_view_get_shortcut (accel_group,
                                                         accel_closure);

                      g_object_ref (menu_item);
                      gtk_object_sink (GTK_OBJECT (menu_item));
                    }
                  else if (menu_item)
                    {
                      gtk_object_sink (GTK_OBJECT (menu_item));
                      menu_item = NULL;
                    }
                }

              gtk_tree_store_append (store, &action_iter, &group_iter);

              gtk_tree_store_set (store, &action_iter,
                                  COLUMN_ACTION,    action,
                                  COLUMN_STOCK_ID,  stock_id,
                                  COLUMN_LABEL,     stripped,
                                  COLUMN_NAME,      name,
                                  COLUMN_SHORTCUT,  shortcut,
                                  COLUMN_MENU_ITEM, menu_item,
                                  -1);

              g_free (stock_id);
              g_free (label);
              g_free (stripped);
              g_free (shortcut);

              if (menu_item)
                g_object_unref (menu_item);
            }
        }

      g_list_free (actions);
    }

  view = g_object_new (GIMP_TYPE_ACTION_VIEW,
                       "model",      store,
                       "rules-hint", TRUE,
                       NULL);

  g_object_unref (store);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Action"));

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "stock-id", COLUMN_STOCK_ID,
                                       NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_LABEL,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  if (show_shortcuts)
    {
      g_signal_connect_object (accel_group, "accel_changed",
                               G_CALLBACK (gimp_action_view_accel_changed),
                               view, 0);

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Shortcut"));

      cell = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "text", COLUMN_SHORTCUT,
                                           NULL);

      gtk_tree_view_append_column (view, column);
    }
  else
    {
      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Name"));

      cell = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "text", COLUMN_NAME,
                                           NULL);

      gtk_tree_view_append_column (view, column);
    }

  return GTK_WIDGET (view);
}


/*  private functions  */

static gchar *
gimp_action_view_get_shortcut (GtkAccelGroup *accel_group,
                               GClosure      *accel_closure)
{
  GtkAccelKey *accel_key;

  accel_key = gtk_accel_group_find (accel_group,
                                    gimp_action_view_accel_find_func,
                                    accel_closure);

  if (accel_key            &&
      accel_key->accel_key &&
      accel_key->accel_flags & GTK_ACCEL_VISIBLE)
    {
      return gimp_get_accel_string (accel_key->accel_key,
                                    accel_key->accel_mods);
    }

  return NULL;
}

static gboolean
gimp_action_view_accel_find_func (GtkAccelKey *key,
                                  GClosure    *closure,
                                  gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
gimp_action_view_accel_changed (GtkAccelGroup   *accel_group,
                                guint            unused1,
                                GdkModifierType  unused2,
                                GClosure        *accel_closure,
                                GimpActionView  *view)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! model)
    return;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GtkTreeIter child_iter;
      gboolean    child_valid;

      for (child_valid = gtk_tree_model_iter_children (model, &child_iter,
                                                       &iter);
           child_valid;
           child_valid = gtk_tree_model_iter_next (model, &child_iter))
        {
          GtkWidget *menu_item;

          gtk_tree_model_get (model, &child_iter,
                              COLUMN_MENU_ITEM, &menu_item,
                              -1);

          if (menu_item)
            {
              GClosure *item_closure;

              g_object_get (GTK_BIN (menu_item)->child,
                            "accel-closure", &item_closure,
                            NULL);

              g_object_unref (menu_item);

              if (accel_closure == item_closure)
                {
                  gchar *shortcut;

                  shortcut = gimp_action_view_get_shortcut (accel_group,
                                                            accel_closure);

                  gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                                      COLUMN_SHORTCUT, shortcut,
                                      -1);
                  g_free (shortcut);

                  return;
                }
            }
        }
    }
}
