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
#include "gimpmenushell.h"
#include "gimpuimanager.h"


#define GET_PRIVATE(obj) (gimp_menu_shell_get_private ((GimpMenuShell *) (obj)))

typedef struct _GimpMenuShellPrivate GimpMenuShellPrivate;

struct _GimpMenuShellPrivate
{
  GimpUIManager *manager;
  gchar         *update_signal;

  GRegex        *path_regex;
};


static GimpMenuShellPrivate *
                gimp_menu_shell_get_private             (GimpMenuShell        *menu_shell);
static void     gimp_menu_shell_private_finalize        (GimpMenuShellPrivate *priv);

static void     gimp_menu_shell_ui_added                (GimpUIManager        *manager,
                                                         const gchar          *path,
                                                         const gchar          *action_name,
                                                         const gchar          *placeholder_key,
                                                         gboolean              top,
                                                         GimpMenuShell        *shell);


static void     gimp_menu_shell_append_model_drop_top   (GimpMenuShell        *shell,
                                                         GMenuModel           *model);

static gchar ** gimp_menu_shell_break_path              (GimpMenuShell         *shell,
                                                         const gchar           *path);


G_DEFINE_INTERFACE (GimpMenuShell, gimp_menu_shell, G_TYPE_OBJECT)


static void
gimp_menu_shell_default_init (GimpMenuShellInterface *iface)
{
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("manager",
                                                            NULL, NULL,
                                                            GIMP_TYPE_UI_MANAGER,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
}


/*  public functions  */

void
gimp_menu_shell_fill (GimpMenuShell *shell,
                      GMenuModel    *model,
                      const gchar   *update_signal,
                      gboolean       drop_top_submenu)
{
  g_return_if_fail (GTK_IS_CONTAINER (shell));

  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (drop_top_submenu)
    gimp_menu_shell_append_model_drop_top (shell, model);
  else
    GIMP_MENU_SHELL_GET_INTERFACE (shell)->append (shell, model);

  if (update_signal != NULL)
    {
      GimpMenuShellPrivate *priv = GET_PRIVATE (shell);

      if (priv->update_signal != NULL)
        {
          g_free (priv->update_signal);
          g_signal_handlers_disconnect_by_func (priv->manager,
                                                G_CALLBACK (gimp_menu_shell_ui_added),
                                                shell);
        }

      priv->update_signal = g_strdup (update_signal);
      g_signal_connect (priv->manager, update_signal,
                        G_CALLBACK (gimp_menu_shell_ui_added),
                        shell);
      gimp_ui_manager_foreach_ui (priv->manager,
                                  (GimpUIMenuCallback) gimp_menu_shell_ui_added,
                                  shell);
    }
}


/* Protected functions. */

void
gimp_menu_shell_init (GimpMenuShell *shell)
{
  GimpMenuShellPrivate *priv;

  g_return_if_fail (GIMP_IS_MENU_SHELL (shell));

  priv = GET_PRIVATE (shell);

  priv->update_signal = NULL;
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
  GimpMenuShellPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_MENU_SHELL_PROP_MANAGER:
      g_set_weak_pointer (&priv->manager, g_value_get_object (value));
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

gchar *
gimp_menu_shell_make_canonical_path (const gchar *path)
{
  gchar **split_path;
  gchar  *canon_path;

  /* The first underscore of each path item is a mnemonic. */
  split_path = g_strsplit (path, "_", 2);
  canon_path = g_strjoinv ("", split_path);
  g_strfreev (split_path);

  return canon_path;
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
  g_free (priv->update_signal);
  g_clear_pointer (&priv->path_regex, g_regex_unref);

  g_slice_free (GimpMenuShellPrivate, priv);
}

static void
gimp_menu_shell_ui_added (GimpUIManager *manager,
                          const gchar   *path,
                          const gchar   *action_name,
                          const gchar   *placeholder_key,
                          gboolean       top,
                          GimpMenuShell *shell)
{
  gchar **paths = gimp_menu_shell_break_path (shell, path);

  if (GIMP_MENU_SHELL_GET_INTERFACE (shell)->add_ui)
    GIMP_MENU_SHELL_GET_INTERFACE (shell)->add_ui (shell,
                                                   (const gchar **) paths,
                                                   action_name,
                                                   placeholder_key, top);

  g_strfreev (paths);
}

static void
gimp_menu_shell_append_model_drop_top (GimpMenuShell *shell,
                                       GMenuModel    *model)
{
  GMenuModel *submenu = NULL;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  if (g_menu_model_get_n_items (model) == 1)
    submenu = g_menu_model_get_item_link (model, 0, G_MENU_LINK_SUBMENU);

  GIMP_MENU_SHELL_GET_INTERFACE (shell)->append (shell, submenu != NULL ? submenu : model);

  g_clear_object (&submenu);
}

static gchar **
gimp_menu_shell_break_path (GimpMenuShell *shell,
                            const gchar   *path)
{
  GimpMenuShellPrivate  *priv;
  gchar                **paths;
  gint                   start = 0;

  g_return_val_if_fail (path != NULL, NULL);

  priv = GET_PRIVATE (shell);

  /* Get rid of leading slashes. */
  while (path[start] == '/' && path[start] != '\0')
    start++;

  /* Get rid of empty last item because of trailing slashes. */
  paths = g_regex_split (priv->path_regex, path + start, 0);
  if (strlen (paths[g_strv_length (paths) - 1]) == 0)
    {
      g_free (paths[g_strv_length (paths) - 1]);
      paths[g_strv_length (paths) - 1] = NULL;
    }

  for (int i = 0; paths[i]; i++)
    {
      gchar *canon_path;

      canon_path = gimp_menu_shell_make_canonical_path (paths[i]);
      g_free (paths[i]);
      paths[i] = canon_path;
    }

  return paths;
}
