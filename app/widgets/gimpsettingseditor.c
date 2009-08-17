/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingseditor.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpsettingseditor.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_CONFIG,
  PROP_CONTAINER,
  PROP_FILENAME
};


static GObject * gimp_settings_editor_constructor   (GType                type,
                                                     guint                n_params,
                                                     GObjectConstructParam *params);
static void      gimp_settings_editor_finalize      (GObject             *object);
static void      gimp_settings_editor_set_property  (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void      gimp_settings_editor_get_property  (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static gboolean
            gimp_settings_editor_row_separator_func (GtkTreeModel        *model,
                                                     GtkTreeIter         *iter,
                                                     gpointer             data);
static void gimp_settings_editor_select_item        (GimpContainerView   *view,
                                                     GimpViewable        *viewable,
                                                     gpointer             insert_data,
                                                     GimpSettingsEditor  *editor);
static void gimp_settings_editor_import_clicked     (GtkWidget           *widget,
                                                     GimpSettingsEditor  *editor);
static void gimp_settings_editor_export_clicked     (GtkWidget           *widget,
                                                     GimpSettingsEditor  *editor);
static void gimp_settings_editor_delete_clicked     (GtkWidget           *widget,
                                                     GimpSettingsEditor  *editor);
static void gimp_settings_editor_name_edited        (GtkCellRendererText *cell,
                                                     const gchar         *path_str,
                                                     const gchar         *new_name,
                                                     GimpSettingsEditor  *editor);


G_DEFINE_TYPE (GimpSettingsEditor, gimp_settings_editor, GTK_TYPE_VBOX)

#define parent_class gimp_settings_editor_parent_class


static void
gimp_settings_editor_class_init (GimpSettingsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_settings_editor_constructor;
  object_class->finalize     = gimp_settings_editor_finalize;
  object_class->set_property = gimp_settings_editor_set_property;
  object_class->get_property = gimp_settings_editor_get_property;

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
}

static void
gimp_settings_editor_init (GimpSettingsEditor *editor)
{
  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static GObject *
gimp_settings_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject               *object;
  GimpSettingsEditor    *editor;
  GimpContainerTreeView *tree_view;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_SETTINGS_EDITOR (object);

  g_assert (GIMP_IS_GIMP (editor->gimp));
  g_assert (GIMP_IS_CONFIG (editor->config));
  g_assert (GIMP_IS_CONTAINER (editor->container));

  editor->view = gimp_container_tree_view_new (editor->container,
                                               gimp_get_user_context (editor->gimp),
                                               16, 0, FALSE);
  gtk_widget_set_size_request (editor->view, 200, 200);
  gtk_container_add (GTK_CONTAINER (editor), editor->view);
  gtk_widget_show (editor->view);

  tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

  gtk_tree_view_set_row_separator_func (tree_view->view,
                                        gimp_settings_editor_row_separator_func,
                                        editor->view, NULL);

  g_signal_connect (tree_view, "select-item",
                    G_CALLBACK (gimp_settings_editor_select_item),
                    editor);

  gimp_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (gimp_settings_editor_name_edited),
                                                editor);

  editor->import_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GTK_STOCK_OPEN,
                            _("Import settings from a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_import_clicked),
                            NULL,
                            editor);

  editor->export_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GTK_STOCK_SAVE,
                            _("Export the selected settings to a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_export_clicked),
                            NULL,
                            editor);

  editor->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GTK_STOCK_DELETE,
                            _("Delete the selected settings"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_delete_clicked),
                            NULL,
                            editor);

  gtk_widget_set_sensitive (editor->delete_button, FALSE);

  return object;
}

static void
gimp_settings_editor_finalize (GObject *object)
{
  GimpSettingsEditor *editor = GIMP_SETTINGS_EDITOR (object);

  if (editor->config)
    {
      g_object_unref (editor->config);
      editor->config = NULL;
    }

  if (editor->container)
    {
      g_object_unref (editor->container);
      editor->container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_settings_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpSettingsEditor *editor = GIMP_SETTINGS_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      editor->gimp = g_value_get_object (value); /* don't dup */
      break;

    case PROP_CONFIG:
      editor->config = g_value_dup_object (value);
      break;

    case PROP_CONTAINER:
      editor->container = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_settings_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpSettingsEditor *editor = GIMP_SETTINGS_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, editor->gimp);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, editor->config);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, editor->container);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_settings_editor_row_separator_func (GtkTreeModel *model,
                                         GtkTreeIter  *iter,
                                         gpointer      data)
{
  gchar *name = NULL;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_VIEW_COLUMN_NAME, &name,
                      -1);
  g_free (name);

  return name == NULL;
}

static void
gimp_settings_editor_select_item (GimpContainerView  *view,
                                  GimpViewable       *viewable,
                                  gpointer            insert_data,
                                  GimpSettingsEditor *editor)
{
  editor->selected_setting = G_OBJECT (viewable);

  gtk_widget_set_sensitive (editor->export_button,
                            editor->selected_setting != NULL);
  gtk_widget_set_sensitive (editor->delete_button,
                            editor->selected_setting != NULL);
}

static void
gimp_settings_editor_import_clicked (GtkWidget          *widget,
                                     GimpSettingsEditor *editor)
{
}

static void
gimp_settings_editor_export_clicked (GtkWidget          *widget,
                                     GimpSettingsEditor *editor)
{
}

static void
gimp_settings_editor_delete_clicked (GtkWidget          *widget,
                                     GimpSettingsEditor *editor)
{
  if (editor->selected_setting)
    {
      GimpObject *new = NULL;
      gint        index;

      index = gimp_container_get_child_index (editor->container,
                                              GIMP_OBJECT (editor->selected_setting));

      if (index != -1)
        {
          new = gimp_container_get_child_by_index (editor->container,
                                                   index + 1);

          if (! new && index > 0)
            new = gimp_container_get_child_by_index (editor->container,
                                                     index - 1);

          /*  don't select the separator  */
          if (new && ! gimp_object_get_name (new))
            new = NULL;
        }

      gimp_container_remove (editor->container,
                             GIMP_OBJECT (editor->selected_setting));

      if (new)
        gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                         GIMP_VIEWABLE (new));
    }
}

static void
gimp_settings_editor_name_edited (GtkCellRendererText *cell,
                                  const gchar         *path_str,
                                  const gchar         *new_name,
                                  GimpSettingsEditor  *editor)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpObject       *object;
      const gchar      *old_name;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_VIEW_COLUMN_RENDERER, &renderer,
                          -1);

      object = GIMP_OBJECT (renderer->viewable);

      old_name = gimp_object_get_name (object);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      name = g_strstrip (g_strdup (new_name));

      if (strlen (name) && strcmp (old_name, name))
        {
          guint t;

          g_object_get (object, "time", &t, NULL);

          if (t > 0)
            g_object_set (object, "time", 0, NULL);

          /*  set name after time so the object is reordered correctly  */
          gimp_object_take_name (object, name);
        }
      else
        {
          g_free (name);

          name = gimp_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              GIMP_CONTAINER_TREE_VIEW_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}


/*  public functions  */

GtkWidget *
gimp_settings_editor_new (Gimp          *gimp,
                          GObject       *config,
                          GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  return g_object_new (GIMP_TYPE_SETTINGS_EDITOR,
                       "gimp",      gimp,
                       "config",    config,
                       "container", container,
                       NULL);
}
