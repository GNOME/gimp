/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingseditor.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpsettingseditor.h"
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


static GObject * gimp_settings_editor_constructor   (GType              type,
                                                     guint              n_params,
                                                     GObjectConstructParam *params);
static void      gimp_settings_editor_finalize      (GObject           *object);
static void      gimp_settings_editor_set_property  (GObject           *object,
                                                     guint              property_id,
                                                     const GValue      *value,
                                                     GParamSpec        *pspec);
static void      gimp_settings_editor_get_property  (GObject           *object,
                                                     guint              property_id,
                                                     GValue            *value,
                                                     GParamSpec        *pspec);


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
  GObject            *object;
  GimpSettingsEditor *editor;
  GtkWidget          *view;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_SETTINGS_EDITOR (object);

  g_assert (GIMP_IS_GIMP (editor->gimp));
  g_assert (GIMP_IS_CONFIG (editor->config));
  g_assert (GIMP_IS_CONTAINER (editor->container));

  view = gimp_container_tree_view_new (editor->container,
                                       gimp_get_user_context (editor->gimp),
                                       16, 0);
  gtk_container_add (GTK_CONTAINER (editor), view);
  gtk_widget_show (view);

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
