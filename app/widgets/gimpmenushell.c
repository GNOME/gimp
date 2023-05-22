/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenushell.c
 * Copyright (C) 2023 Jehan
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimpmenu.h"
#include "gimpmenumodel.h"
#include "gimpmenushell.h"
#include "gimpuimanager.h"


enum
{
  MODEL_DELETED,
  LAST_SIGNAL
};

#define GET_PRIVATE(obj) (gimp_menu_shell_get_private ((GimpMenuShell *) (obj)))

typedef struct _GimpMenuShellPrivate GimpMenuShellPrivate;

struct _GimpMenuShellPrivate
{
  GimpUIManager  *manager;
  GimpMenuModel  *model;
  gchar          *path_prefix;

  GRegex         *path_regex;
};


static GimpMenuShellPrivate *
                gimp_menu_shell_get_private             (GimpMenuShell        *menu_shell);
static void     gimp_menu_shell_private_finalize        (GimpMenuShellPrivate *priv);

static void     gimp_menu_shell_model_changed           (GMenuModel           *self,
                                                         gint                  position,
                                                         gint                  removed,
                                                         gint                  added,
                                                         GimpMenuShell        *shell);

static void     gimp_menu_shell_append_model_drop_top   (GimpMenuShell        *shell,
                                                         GimpMenuModel        *model);


G_DEFINE_INTERFACE (GimpMenuShell, gimp_menu_shell, GTK_TYPE_CONTAINER)

static guint signals[LAST_SIGNAL];

static void
gimp_menu_shell_default_init (GimpMenuShellInterface *iface)
{
  signals[MODEL_DELETED] =
    g_signal_new ("model-deleted",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMenuShellInterface, model_deleted),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("manager",
                                                            NULL, NULL,
                                                            GIMP_TYPE_UI_MANAGER,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("model",
                                                            NULL, NULL,
                                                            GIMP_TYPE_MENU_MODEL,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_READWRITE));
}


/* Public functions. */

void
gimp_menu_shell_fill (GimpMenuShell *shell,
                      GimpMenuModel *model,
                      gboolean       drop_top_submenu)
{
  g_return_if_fail (GTK_IS_CONTAINER (shell));

  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (drop_top_submenu)
    gimp_menu_shell_append_model_drop_top (shell, model);
  else
    gimp_menu_shell_append (shell, model);
}


/* Protected functions. */

void
gimp_menu_shell_append (GimpMenuShell *shell,
                        GimpMenuModel *model)
{
  GimpMenuShellPrivate *priv = GET_PRIVATE (shell);

  g_free (priv->path_prefix);
  priv->path_prefix = g_strdup (gimp_menu_model_get_path (GIMP_MENU_MODEL (model)));

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            G_CALLBACK (gimp_menu_shell_model_changed),
                                            shell);
      g_signal_emit (shell, signals[MODEL_DELETED], 0);
    }

  if (priv->model != model)
    {
      g_clear_object (&priv->model);
      priv->model = g_object_ref (model);
    }

  if (model)
    {
      GIMP_MENU_SHELL_GET_INTERFACE (shell)->append (shell, model);

      g_signal_connect_object (priv->model, "items-changed",
                               G_CALLBACK (gimp_menu_shell_model_changed),
                               shell, 0);
    }
}

void
gimp_menu_shell_init (GimpMenuShell *shell)
{
  GimpMenuShellPrivate *priv;

  g_return_if_fail (GIMP_IS_MENU_SHELL (shell));

  priv = GET_PRIVATE (shell);

  priv->path_prefix   = NULL;
  priv->model         = NULL;
  priv->path_regex    = g_regex_new ("/+", 0, 0, NULL);
}

/**
 * gimp_menu_shell_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpMenuShell. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
gimp_menu_shell_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, GIMP_MENU_SHELL_PROP_MANAGER, "manager");
  g_object_class_override_property (klass, GIMP_MENU_SHELL_PROP_MODEL,   "model");
}

void
gimp_menu_shell_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpMenuShellPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_MENU_SHELL_PROP_MANAGER:
      g_value_set_object (value, priv->manager);
      break;
    case GIMP_MENU_SHELL_PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_menu_shell_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpMenuShellPrivate *priv  = GET_PRIVATE (object);
  GimpMenuShell        *shell = GIMP_MENU_SHELL (object);


  switch (property_id)
    {
    case GIMP_MENU_SHELL_PROP_MANAGER:
      g_set_weak_pointer (&priv->manager, g_value_get_object (value));
      break;
    case GIMP_MENU_SHELL_PROP_MODEL:
      gimp_menu_shell_append (shell, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpUIManager *
gimp_menu_shell_get_manager (GimpMenuShell *shell)
{
  return GET_PRIVATE (shell)->manager;
}


/*  Private functions  */

static GimpMenuShellPrivate *
gimp_menu_shell_get_private (GimpMenuShell *menu_shell)
{
  GimpMenuShellPrivate *priv;
  static GQuark         private_key = 0;

  g_return_val_if_fail (GIMP_IS_MENU_SHELL (menu_shell), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-menu_shell-priv");

  priv = g_object_get_qdata ((GObject *) menu_shell, private_key);

  if (! priv)
    {
      priv = g_slice_new0 (GimpMenuShellPrivate);

      g_object_set_qdata_full ((GObject *) menu_shell, private_key, priv,
                               (GDestroyNotify) gimp_menu_shell_private_finalize);
    }

  return priv;
}

static void
gimp_menu_shell_private_finalize (GimpMenuShellPrivate *priv)
{
  g_clear_weak_pointer (&priv->manager);
  g_free (priv->path_prefix);
  g_clear_pointer (&priv->path_regex, g_regex_unref);
  g_clear_object (&priv->model);

  g_slice_free (GimpMenuShellPrivate, priv);
}

static void
gimp_menu_shell_model_changed (GMenuModel    *model,
                               gint           position,
                               gint           removed,
                               gint           added,
                               GimpMenuShell *shell)
{
  GimpMenuShellPrivate *priv = GET_PRIVATE (shell);

  /* Lazy approach to handle model changes. A better implementation would only
   * remove or delete the changed items using:
   * GIMP_MENU_SHELL_GET_INTERFACE (shell)->add_ui()
   * GIMP_MENU_SHELL_GET_INTERFACE (shell)->remove_ui
   * TODO
   */
  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  GIMP_MENU_SHELL_GET_INTERFACE (shell)->append (shell, priv->model);
}

static void
gimp_menu_shell_append_model_drop_top (GimpMenuShell *shell,
                                       GimpMenuModel *model)
{
  GimpMenuShellPrivate *priv    = GET_PRIVATE (shell);
  GMenuModel           *submenu = NULL;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  if (g_menu_model_get_n_items (G_MENU_MODEL (model)) == 1)
    {
      gchar *label = NULL;

      submenu = g_menu_model_get_item_link (G_MENU_MODEL (model), 0, G_MENU_LINK_SUBMENU);

      if (submenu)
        {
          gchar *path_prefix;

          g_menu_model_get_item_attribute (submenu, 0, G_MENU_ATTRIBUTE_LABEL, "s", &label);

          path_prefix = g_strdup_printf ("%s/%s", priv->path_prefix, label);
          g_free (priv->path_prefix);
          priv->path_prefix = path_prefix;
        }

      g_free (label);
    }

  gimp_menu_shell_append (shell, submenu != NULL ? GIMP_MENU_MODEL (submenu) : GIMP_MENU_MODEL (model));

  g_clear_object (&submenu);
}
