/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasettingseditor.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "operations/ligma-operation-config.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaviewable.h"

#include "ligmacontainertreestore.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmasettingseditor.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_CONFIG,
  PROP_CONTAINER
};


typedef struct _LigmaSettingsEditorPrivate LigmaSettingsEditorPrivate;

struct _LigmaSettingsEditorPrivate
{
  Ligma          *ligma;
  GObject       *config;
  LigmaContainer *container;
  GObject       *selected_setting;

  GtkWidget     *view;
  GtkWidget     *import_button;
  GtkWidget     *export_button;
  GtkWidget     *delete_button;
};

#define GET_PRIVATE(item) ((LigmaSettingsEditorPrivate *) ligma_settings_editor_get_instance_private ((LigmaSettingsEditor *) (item)))


static void   ligma_settings_editor_constructed    (GObject             *object);
static void   ligma_settings_editor_finalize       (GObject             *object);
static void   ligma_settings_editor_set_property   (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   ligma_settings_editor_get_property   (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gboolean
          ligma_settings_editor_row_separator_func (GtkTreeModel        *model,
                                                   GtkTreeIter         *iter,
                                                   gpointer             data);
static gboolean ligma_settings_editor_select_items (LigmaContainerView   *view,
                                                   GList               *viewables,
                                                   GList               *paths,
                                                   LigmaSettingsEditor  *editor);
static void   ligma_settings_editor_import_clicked (GtkWidget           *widget,
                                                   LigmaSettingsEditor  *editor);
static void   ligma_settings_editor_export_clicked (GtkWidget           *widget,
                                                   LigmaSettingsEditor  *editor);
static void   ligma_settings_editor_delete_clicked (GtkWidget           *widget,
                                                   LigmaSettingsEditor  *editor);
static void   ligma_settings_editor_name_edited    (GtkCellRendererText *cell,
                                                   const gchar         *path_str,
                                                   const gchar         *new_name,
                                                   LigmaSettingsEditor  *editor);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaSettingsEditor, ligma_settings_editor,
                            GTK_TYPE_BOX)

#define parent_class ligma_settings_editor_parent_class


static void
ligma_settings_editor_class_init (LigmaSettingsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_settings_editor_constructed;
  object_class->finalize     = ligma_settings_editor_finalize;
  object_class->set_property = ligma_settings_editor_set_property;
  object_class->get_property = ligma_settings_editor_get_property;

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
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_settings_editor_init (LigmaSettingsEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
ligma_settings_editor_constructed (GObject *object)
{
  LigmaSettingsEditor        *editor  = LIGMA_SETTINGS_EDITOR (object);
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (object);
  LigmaContainerTreeView     *tree_view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (private->ligma));
  ligma_assert (LIGMA_IS_CONFIG (private->config));
  ligma_assert (LIGMA_IS_CONTAINER (private->container));

  private->view = ligma_container_tree_view_new (private->container,
                                                ligma_get_user_context (private->ligma),
                                               16, 0);
  gtk_widget_set_size_request (private->view, 200, 200);
  gtk_box_pack_start (GTK_BOX (editor), private->view, TRUE, TRUE, 0);
  gtk_widget_show (private->view);

  tree_view = LIGMA_CONTAINER_TREE_VIEW (private->view);

  gtk_tree_view_set_row_separator_func (tree_view->view,
                                        ligma_settings_editor_row_separator_func,
                                        private->view, NULL);

  g_signal_connect (tree_view, "select-items",
                    G_CALLBACK (ligma_settings_editor_select_items),
                    editor);

  ligma_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (ligma_settings_editor_name_edited),
                                                editor);

  private->import_button =
    ligma_editor_add_button (LIGMA_EDITOR (tree_view),
                            LIGMA_ICON_DOCUMENT_OPEN,
                            _("Import presets from a file"),
                            NULL,
                            G_CALLBACK (ligma_settings_editor_import_clicked),
                            NULL,
                            editor);

  private->export_button =
    ligma_editor_add_button (LIGMA_EDITOR (tree_view),
                            LIGMA_ICON_DOCUMENT_SAVE,
                            _("Export the selected presets to a file"),
                            NULL,
                            G_CALLBACK (ligma_settings_editor_export_clicked),
                            NULL,
                            editor);

  private->delete_button =
    ligma_editor_add_button (LIGMA_EDITOR (tree_view),
                            LIGMA_ICON_EDIT_DELETE,
                            _("Delete the selected preset"),
                            NULL,
                            G_CALLBACK (ligma_settings_editor_delete_clicked),
                            NULL,
                            editor);

  gtk_widget_set_sensitive (private->delete_button, FALSE);
}

static void
ligma_settings_editor_finalize (GObject *object)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->config);
  g_clear_object (&private->container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_settings_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      private->ligma = g_value_get_object (value); /* don't dup */
      break;

    case PROP_CONFIG:
      private->config = g_value_dup_object (value);
      break;

    case PROP_CONTAINER:
      private->container = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_settings_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (object);

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

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_settings_editor_row_separator_func (GtkTreeModel *model,
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
ligma_settings_editor_select_items (LigmaContainerView  *view,
                                   GList              *viewables,
                                   GList              *paths,
                                   LigmaSettingsEditor *editor)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   sensitive;

  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  private->selected_setting = viewables ? G_OBJECT (viewables->data) : NULL;

  sensitive = (private->selected_setting != NULL &&
               ligma_object_get_name (private->selected_setting));

  gtk_widget_set_sensitive (private->export_button, sensitive);
  gtk_widget_set_sensitive (private->delete_button, sensitive);

  return TRUE;
}

static void
ligma_settings_editor_import_clicked (GtkWidget          *widget,
                                     LigmaSettingsEditor *editor)
{
}

static void
ligma_settings_editor_export_clicked (GtkWidget          *widget,
                                     LigmaSettingsEditor *editor)
{
}

static void
ligma_settings_editor_delete_clicked (GtkWidget          *widget,
                                     LigmaSettingsEditor *editor)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (editor);

  if (private->selected_setting)
    {
      LigmaObject *new;

      new = ligma_container_get_neighbor_of (private->container,
                                            LIGMA_OBJECT (private->selected_setting));

      /*  don't select the separator  */
      if (new && ! ligma_object_get_name (new))
        new = NULL;

      ligma_container_remove (private->container,
                             LIGMA_OBJECT (private->selected_setting));

      ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (private->view),
                                       LIGMA_VIEWABLE (new));

      ligma_operation_config_serialize (private->ligma, private->container, NULL);
    }
}

static void
ligma_settings_editor_name_edited (GtkCellRendererText *cell,
                                  const gchar         *path_str,
                                  const gchar         *new_name,
                                  LigmaSettingsEditor  *editor)
{
  LigmaSettingsEditorPrivate *private = GET_PRIVATE (editor);
  LigmaContainerTreeView     *tree_view;
  GtkTreePath               *path;
  GtkTreeIter                iter;

  tree_view = LIGMA_CONTAINER_TREE_VIEW (private->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaObject       *object;
      const gchar      *old_name;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      object = LIGMA_OBJECT (renderer->viewable);

      old_name = ligma_object_get_name (object);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      name = g_strstrip (g_strdup (new_name));

      if (strlen (name) && strcmp (old_name, name))
        {
          gint64 t;

          g_object_get (object,
                        "time", &t,
                        NULL);

          if (t > 0)
            g_object_set (object,
                          "time", (gint64) 0,
                          NULL);

          /*  set name after time so the object is reordered correctly  */
          ligma_object_take_name (object, name);

          ligma_operation_config_serialize (private->ligma, private->container,
                                           NULL);
        }
      else
        {
          g_free (name);

          name = ligma_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}


/*  public functions  */

GtkWidget *
ligma_settings_editor_new (Ligma          *ligma,
                          GObject       *config,
                          LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONFIG (config), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  return g_object_new (LIGMA_TYPE_SETTINGS_EDITOR,
                       "ligma",      ligma,
                       "config",    config,
                       "container", container,
                       NULL);
}
