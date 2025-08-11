/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingseditor.c
 * Copyright (C) 2008-2025 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "operations/gimp-operation-config.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcontainerlistview.h"
#include "gimpcontainerview.h"
#include "gimpsettingseditor.h"

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
  GQuark         name_changed_handler;

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

static void   gimp_settings_editor_selection_changed
                                                  (GimpContainerView   *view,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_import_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_export_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_delete_clicked (GtkWidget           *widget,
                                                   GimpSettingsEditor  *editor);
static void   gimp_settings_editor_name_changed   (GimpSettings        *setting,
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

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (private->gimp));
  gimp_assert (GIMP_IS_CONFIG (private->config));
  gimp_assert (GIMP_IS_CONTAINER (private->container));

  private->view = gimp_container_list_view_new (private->container,
                                                gimp_get_user_context (private->gimp),
                                                GIMP_VIEW_SIZE_SMALL, 0);
  gtk_widget_set_size_request (private->view, 200, 200);
  gtk_box_pack_start (GTK_BOX (editor), private->view, TRUE, TRUE, 0);
  gtk_widget_show (private->view);

  g_signal_connect (private->view, "selection-changed",
                    G_CALLBACK (gimp_settings_editor_selection_changed),
                    editor);

  private->name_changed_handler =
    gimp_container_add_handler (private->container, "name-changed",
                                G_CALLBACK (gimp_settings_editor_name_changed),
                                editor);

  private->import_button =
    gimp_editor_add_button (GIMP_EDITOR (private->view),
                            GIMP_ICON_DOCUMENT_OPEN,
                            _("Import presets from a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_import_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->export_button =
    gimp_editor_add_button (GIMP_EDITOR (private->view),
                            GIMP_ICON_DOCUMENT_SAVE,
                            _("Export the selected presets to a file"),
                            NULL,
                            G_CALLBACK (gimp_settings_editor_export_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (private->view),
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

  gimp_container_remove_handler (private->container,
                                 private->name_changed_handler);

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

static void
gimp_settings_editor_selection_changed (GimpContainerView  *view,
                                        GimpSettingsEditor *editor)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   sensitive;

  private->selected_setting =
    G_OBJECT (gimp_container_view_get_1_selected (view));

  sensitive = (private->selected_setting != NULL);

  gtk_widget_set_sensitive (private->export_button, sensitive);
  gtk_widget_set_sensitive (private->delete_button, sensitive);
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

      gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (private->view),
                                          GIMP_VIEWABLE (new));

      gimp_operation_config_serialize (private->gimp, private->container, NULL);
    }
}

static void
gimp_settings_editor_name_changed (GimpSettings      *settings,
                                  GimpSettingsEditor *editor)
{
  GimpSettingsEditorPrivate *private = GET_PRIVATE (editor);

  gimp_operation_config_serialize (private->gimp, private->container,
                                   NULL);
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
