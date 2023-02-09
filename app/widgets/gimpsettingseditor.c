/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingseditor.c
 * Copyright (C) 2008-2017 Michael Natterer <mitch@gimp.org>
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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "operations/gimp-operation-config.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreestore.h"
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
  PROP_CONTAINER
};


typedef struct _GimpSettingsEditorPrivate GimpSettingsEditorPrivate;

struct _GimpSettingsEditorPrivate
{
  Gimp          *gimp;
  GObject       *config;
  GimpContainer *container;
  GObject       *selected_setting;

  GtkWidget     *view;
  GtkWidget     *import_button;
  GtkWidget     *export_button;
  GtkWidget     *delete_button;
};

#define GET_PRIVATE(item) ((GimpSettingsEditorPrivate *) gimp_settings_editor_get_instance_private ((GimpSettingsEditor *) (item)))


static void   gimp_settings_editor_constructed    (GObject             *object);
static void   gimp_settings_editor_finalize       (GObject             *object);
static void   gimp_settings_editor_set_property   (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   gimp_settings_editor_get_property   (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gboolean
          gimp_settings_editor_row_separator_func (GtkTreeModel        *model,
                                                   GtkTreeIter         *iter,
                                                   gpointer             data);
static gboolean gimp_settings_editor_select_items (GimpContainerView   *view,
                                                   GList               *viewables,
                                                   GList               *paths,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_import_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_export_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_delete_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_name_edited    (GtkCellRendererText *cell,
                                                   const gchar         *path_str,
                                                   const gchar         *new_name,
                                                   GimpSettingsEditor  *editor);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSettingsEditor, gimp_settings_editor,
                            GTK_TYPE_BOX)

#define parent_class gimp_settings_editor_parent_class


static void
gimp_settings_editor_class_init (GimpSettingsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_settings_editor_constructed;
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
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
gimp_settings_editor_constructed (GObject *object)
{
  GimpSettingsEditor        *editor  = GIMP_SETTINGS_EDITOR (object);
  GimpSettingsEditorPrivate *private = GET_PRIVATE (object);
  GimpContainerTreeView     *tree_view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (private->gimp));
  gimp_assert (GIMP_IS_CONFIG (private->config));
  gimp_assert (GIMP_IS_CONTAINER (private->container));

  private->view = gimp_container_tree_view_new (private->container,
                                                gimp_get_user_context (private->gimp),
                                               16, 0);
  gtk_widget_set_size_request (private->view, 200, 200);
  gtk_box_pack_start (GTK_BOX (editor), private->view, TRUE, TRUE, 0);
  gtk_widget_show (private->view);

  tree_view = GIMP_CONTAINER_TREE_VIEW (private->view);

  gtk_tree_view_set_row_separator_func (tree_view->view,
                                        gimp_settings_editor_row_separator_func,
                                        private->view, NULL);

  g_signal_connect (tree_view, "select-items",
                    G_CALLBACK (gimp_settings_editor_select_items),
                    editor);

  gimp_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (gimp_settings_editor_name_edited),
                                                editor);

  private->import_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GIMP_ICON_DOCUMENT_OPEN,
                            _("Import presets from a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_import_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->export_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GIMP_ICON_DOCUMENT_SAVE,
                            _("Export the selected presets to a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_export_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view),
                            GIMP_ICON_EDIT_DELETE,
                            _("Delete the selected preset"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_delete_clicked),
                            NULL,
                            G_OBJECT (editor));

  gtk_widget_set_sensitive (private->delete_button, FALSE);
}

static void
gimp_settings_editor_finalize (GObject *object)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->config);
  g_clear_object (&private->container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_settings_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value); /* don't dup */
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
gimp_settings_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
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
gimp_settings_editor_row_separator_func (GtkTreeModel *model,
                                         GtkTreeIter  *iter,
                                         gpointer      data)
{
  gchar *name = NULL;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, &name,
                      -1);
  g_free (name);

  return name == NULL;
}

static gboolean
gimp_settings_editor_select_items (GimpContainerView  *view,
                                   GList              *viewables,
                                   GList              *paths,
                                   GimpSettingsEditor *editor)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   sensitive;

  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  private->selected_setting = viewables ? G_OBJECT (viewables->data) : NULL;

  sensitive = (private->selected_setting != NULL &&
               gimp_object_get_name (private->selected_setting));

  gtk_widget_set_sensitive (private->export_button, sensitive);
  gtk_widget_set_sensitive (private->delete_button, sensitive);

  return TRUE;
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
  GimpSettingsEditorPrivate *private = GET_PRIVATE (editor);

  if (private->selected_setting)
    {
      GimpObject *new;

      new = gimp_container_get_neighbor_of (private->container,
                                            GIMP_OBJECT (private->selected_setting));

      /*  don't select the separator  */
      if (new && ! gimp_object_get_name (new))
        new = NULL;

      gimp_container_remove (private->container,
                             GIMP_OBJECT (private->selected_setting));

      gimp_container_view_select_item (GIMP_CONTAINER_VIEW (private->view),
                                       GIMP_VIEWABLE (new));

      gimp_operation_config_serialize (private->gimp, private->container, NULL);
    }
}

static void
gimp_settings_editor_name_edited (GtkCellRendererText *cell,
                                  const gchar         *path_str,
                                  const gchar         *new_name,
                                  GimpSettingsEditor  *editor)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (editor);
  GimpContainerTreeView     *tree_view;
  GtkTreePath               *path;
  GtkTreeIter                iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (private->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpObject       *object;
      const gchar      *old_name;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      object = GIMP_OBJECT (renderer->viewable);

      old_name = gimp_object_get_name (object);

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
          gimp_object_take_name (object, name);

          gimp_operation_config_serialize (private->gimp, private->container,
                                           NULL);
        }
      else
        {
          g_free (name);

          name = gimp_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, name,
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
