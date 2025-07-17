/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "core/gimp.h"

#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimpoperationsettings.h"
#include "gimp-operation-config.h"


/*  local function prototypes  */

static void    gimp_operation_config_config_sync   (GObject          *config,
                                                    const GParamSpec *gimp_pspec,
                                                    GeglNode         *node);
static void    gimp_operation_config_config_notify (GObject          *config,
                                                    const GParamSpec *gimp_pspec,
                                                    GeglNode         *node);
static void    gimp_operation_config_node_notify   (GeglNode         *node,
                                                    const GParamSpec *gegl_pspec,
                                                    GObject          *config);

static GFile * gimp_operation_config_get_file      (GType             config_type);
static void    gimp_operation_config_add_sep       (GimpContainer    *container);
static void    gimp_operation_config_remove_sep    (GimpContainer    *container);


static GHashTable *config_types      = NULL;
static GHashTable *config_containers = NULL;
static GList      *custom_config_ops = NULL;
static gboolean    custom_init_done  = FALSE;


/*  public functions  */

void
gimp_operation_config_init_start (Gimp *gimp)
{
  config_types      = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             (GDestroyNotify) g_free,
                                             NULL);
  config_containers = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             (GDestroyNotify) g_object_unref);
}

void
gimp_operation_config_init_end (Gimp *gimp)
{
  custom_init_done = TRUE;
}

void
gimp_operation_config_exit (Gimp *gimp)
{
  g_hash_table_unref (config_types);
  g_hash_table_unref (config_containers);
  g_list_free (custom_config_ops);
}


void
gimp_operation_config_register (Gimp        *gimp,
                                const gchar *operation,
                                GType        config_type)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (operation != NULL);
  g_return_if_fail (g_type_is_a (config_type, GIMP_TYPE_OBJECT));

  if (! custom_init_done)
    /* Custom ops are registered with static string, not generated names. */
    custom_config_ops = g_list_prepend (custom_config_ops, (gpointer) operation);

  g_hash_table_insert (config_types,
                       g_strdup (operation),
                       (gpointer) config_type);
}

gboolean
gimp_operation_config_is_custom (Gimp        *gimp,
                                 const gchar *operation)
{
  return (g_list_find_custom (custom_config_ops, operation, (GCompareFunc) g_strcmp0) != NULL);
}

GType
gimp_operation_config_get_type (Gimp        *gimp,
                                const gchar *operation,
                                const gchar *icon_name,
                                GType        parent_type)
{
  GType config_type;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), G_TYPE_NONE);
  g_return_val_if_fail (operation != NULL, G_TYPE_NONE);
  g_return_val_if_fail (g_type_is_a (parent_type, GIMP_TYPE_OBJECT),
                        G_TYPE_NONE);

  config_type = (GType) g_hash_table_lookup (config_types, operation);

  if (! config_type)
    {
      GParamSpec **pspecs;
      guint        n_pspecs;
      gchar       *type_name;
      gint         i, j;

      pspecs = gegl_operation_list_properties (operation, &n_pspecs);

      for (i = 0, j = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec = pspecs[i];

          if ((pspec->flags & G_PARAM_READABLE) &&
              (pspec->flags & G_PARAM_WRITABLE) &&
              strcmp (pspec->name, "input")     &&
              strcmp (pspec->name, "output"))
            {
              GParamFlags flags;

              flags = pspec->flags & ~(GEGL_PARAM_PAD_INPUT | GEGL_PARAM_PAD_OUTPUT);

              if (GEGL_IS_PARAM_SPEC_COLOR (pspec))
                {
                  /* As special exception, let's transform GeglParamColor
                   * into GimpParamColor in all core code. This way, we
                   * have one less param type to handle.
                   */
                  gchar **prop_keys;
                  guint   n_keys = 0;

                  pspecs[j] = gimp_param_spec_color (pspec->name,
                                                     g_param_spec_get_nick (pspec),
                                                     g_param_spec_get_blurb (pspec),
                                                     TRUE,
                                                     gegl_param_spec_color_get_default (pspec),
                                                     flags);
                  prop_keys = gegl_operation_list_property_keys (operation, pspec->name, &n_keys);
                  for (gint k = 0; k < n_keys; k++)
                    {
                      const gchar *key;

                      key = gegl_param_spec_get_property_key (pspec, prop_keys[k]);
                      gegl_param_spec_set_property_key (pspecs[j], prop_keys[k], key);
                    }

                  g_free (prop_keys);
                }
              else
                {
                  pspecs[j] = gimp_config_param_spec_duplicate (pspec);
                  if (pspecs[j])
                    pspecs[j]->flags = flags;
                }

              if (pspecs[j])
                j++;
            }
        }

      n_pspecs = j;

      type_name = g_strdup_printf ("GimpGegl-%s-config", operation);

      g_strcanon (type_name,
                  G_CSET_DIGITS "-" G_CSET_a_2_z G_CSET_A_2_Z, '-');

      config_type = gimp_config_type_register (parent_type,
                                               type_name,
                                               pspecs, n_pspecs);

      g_free (pspecs);
      g_free (type_name);

      if (icon_name && g_type_is_a (config_type, GIMP_TYPE_VIEWABLE))
        {
          GimpViewableClass *viewable_class = g_type_class_ref (config_type);

          viewable_class->default_icon_name = g_strdup (icon_name);

          g_type_class_unref (viewable_class);
        }

      gimp_operation_config_register (gimp, operation, config_type);
    }

  return config_type;
}

GimpContainer *
gimp_operation_config_get_container (Gimp         *gimp,
                                     GType         config_type,
                                     GCompareFunc  sort_func)
{
  GimpContainer *container;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (config_type, GIMP_TYPE_OBJECT), NULL);

  container = g_hash_table_lookup (config_containers, (gpointer) config_type);

  if (! container)
    {
      container = gimp_list_new (config_type, TRUE);
      gimp_list_set_sort_func (GIMP_LIST (container), sort_func);

      g_hash_table_insert (config_containers,
                           (gpointer) config_type, container);

      gimp_operation_config_deserialize (gimp, container, NULL);

      if (gimp_container_get_n_children (container) == 0)
        {
          GFile *file = gimp_operation_config_get_file (config_type);

          if (! g_file_query_exists (file, NULL))
            {
              GQuark  quark = g_quark_from_static_string ("compat-file");
              GFile  *compat_file;

              compat_file = g_type_get_qdata (config_type, quark);

              if (compat_file)
                {
                  if (! g_file_move (compat_file, file, 0,
                                     NULL, NULL, NULL, NULL))
                    {
                      gimp_operation_config_deserialize (gimp, container,
                                                         compat_file);
                    }
                  else
                    {
                      gimp_operation_config_deserialize (gimp, container, NULL);
                    }
                }
            }

          g_object_unref (file);
        }

      gimp_operation_config_add_sep (container);
    }

  return container;
}

void
gimp_operation_config_serialize (Gimp          *gimp,
                                 GimpContainer *container,
                                 GFile         *file)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (file)
    {
      g_object_ref (file);
    }
  else
    {
      GType config_type = gimp_container_get_child_type (container);

      file = gimp_operation_config_get_file (config_type);
    }

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_operation_config_remove_sep (container);

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (container),
                                       file,
                                       "settings",
                                       "end of settings",
                                       NULL, &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }

  gimp_operation_config_add_sep (container);

  g_object_unref (file);
}

void
gimp_operation_config_deserialize (Gimp          *gimp,
                                   GimpContainer *container,
                                   GFile         *file)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (file)
    {
      g_object_ref (file);
    }
  else
    {
      GType config_type = gimp_container_get_child_type (container);

      file = gimp_operation_config_get_file (config_type);
    }

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (container),
                                      file,
                                      NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                              error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);
}

void
gimp_operation_config_sync_node (GObject  *config,
                                 GeglNode *node)
{
  GParamSpec **pspecs;
  gchar       *operation;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (GEGL_IS_NODE (node));

  gegl_node_get (node,
                 "operation", &operation,
                 NULL);

  g_return_if_fail (operation != NULL);

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);
  g_free (operation);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *gegl_pspec = pspecs[i];
      GParamSpec *gimp_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                             gegl_pspec->name);

      /*  if the operation has an object property of the config's
       *  type, use the config object directly
       */
      if (G_IS_PARAM_SPEC_OBJECT (gegl_pspec) &&
          gegl_pspec->value_type == G_TYPE_FROM_INSTANCE (config))
        {
          gegl_node_set (node,
                         gegl_pspec->name, config,
                         NULL);
        }
      else if (gimp_pspec)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, gimp_pspec->value_type);

          g_object_get_property (G_OBJECT (config), gimp_pspec->name,
                                 &value);

          gegl_node_set_property (node, gegl_pspec->name,
                                  &value);

          g_value_unset (&value);
        }
    }

  g_free (pspecs);
}

void
gimp_operation_config_connect_node (GObject  *config,
                                    GeglNode *node)
{
  GParamSpec **pspecs;
  gchar       *operation;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (GEGL_IS_NODE (node));

  gegl_node_get (node,
                 "operation", &operation,
                 NULL);

  g_return_if_fail (operation != NULL);

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);
  g_free (operation);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      /*  if the operation has an object property of the config's
       *  type, connect it to a special callback and done
       */
      if (G_IS_PARAM_SPEC_OBJECT (pspec) &&
          pspec->value_type == G_TYPE_FROM_INSTANCE (config))
        {
          g_signal_connect_object (config, "notify",
                                   G_CALLBACK (gimp_operation_config_config_sync),
                                   node, 0);
          g_free (pspecs);
          return;
        }
    }

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *gegl_pspec = pspecs[i];
      GParamSpec *gimp_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                             gegl_pspec->name);

      if (gimp_pspec)
        {
          gchar *notify_name = g_strconcat ("notify::", gimp_pspec->name, NULL);

          g_signal_connect_object (config, notify_name,
                                   G_CALLBACK (gimp_operation_config_config_notify),
                                   node, 0);

          g_signal_connect_object (node, notify_name,
                                   G_CALLBACK (gimp_operation_config_node_notify),
                                   config, 0);

          g_free (notify_name);
        }
    }

  g_free (pspecs);
}

GParamSpec **
gimp_operation_config_list_properties (GObject     *config,
                                       GType        owner_type,
                                       GParamFlags  flags,
                                       guint       *n_pspecs)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  gint         i, j;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                                &n_param_specs);

  for (i = 0, j = 0; i < n_param_specs; i++)
    {
      GParamSpec *pspec = param_specs[i];

      /*  ignore properties of parent classes of owner_type  */
      if (! g_type_is_a (pspec->owner_type, owner_type))
        continue;

      if (flags && ((pspec->flags & flags) != flags))
        continue;

      if (gimp_gegl_param_spec_has_key (pspec, "role", "output-extent"))
        continue;

      param_specs[j] = param_specs[i];
      j++;
    }

  if (n_pspecs)
    *n_pspecs = j;

  if (j == 0)
    {
      g_free (param_specs);
      param_specs = NULL;
    }

  return param_specs;
}


/*  private functions  */

static void
gimp_operation_config_config_sync (GObject          *config,
                                   const GParamSpec *gimp_pspec,
                                   GeglNode         *node)
{
  gimp_operation_config_sync_node (config, node);
}

static void
gimp_operation_config_config_notify (GObject          *config,
                                     const GParamSpec *gimp_pspec,
                                     GeglNode         *node)
{
  GParamSpec *gegl_pspec = gegl_node_find_property (node, gimp_pspec->name);

  if (gegl_pspec)
    {
      GValue value = G_VALUE_INIT;
      gulong handler;

      g_value_init (&value, gimp_pspec->value_type);
      g_object_get_property (config, gimp_pspec->name, &value);

      handler = g_signal_handler_find (node,
                                       G_SIGNAL_MATCH_DETAIL |
                                       G_SIGNAL_MATCH_FUNC   |
                                       G_SIGNAL_MATCH_DATA,
                                       0,
                                       g_quark_from_string (gegl_pspec->name),
                                       NULL,
                                       gimp_operation_config_node_notify,
                                       config);

      if (handler)
        g_signal_handler_block (node, handler);

      gegl_node_set_property (node, gegl_pspec->name, &value);
      g_value_unset (&value);

      if (handler)
        g_signal_handler_unblock (node, handler);

    }
}

static void
gimp_operation_config_node_notify (GeglNode         *node,
                                   const GParamSpec *gegl_pspec,
                                   GObject          *config)
{
  GParamSpec *gimp_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                         gegl_pspec->name);

  if (gimp_pspec)
    {
      GValue value = G_VALUE_INIT;
      gulong handler;

      g_value_init (&value, gegl_pspec->value_type);
      gegl_node_get_property (node, gegl_pspec->name, &value);

      handler = g_signal_handler_find (config,
                                       G_SIGNAL_MATCH_DETAIL |
                                       G_SIGNAL_MATCH_FUNC   |
                                       G_SIGNAL_MATCH_DATA,
                                       0,
                                       g_quark_from_string (gimp_pspec->name),
                                       NULL,
                                       gimp_operation_config_config_notify,
                                       node);

      if (handler)
        g_signal_handler_block (config, handler);

      g_object_set_property (config, gimp_pspec->name, &value);
      g_value_unset (&value);

      if (handler)
        g_signal_handler_unblock (config, handler);
    }
}

static GFile *
gimp_operation_config_get_file (GType config_type)
{
  GFile *file;
  gchar *basename;

  basename = g_strconcat (g_type_name (config_type), ".settings", NULL);
  file = gimp_directory_file ("filters", basename, NULL);
  g_free (basename);

  return file;
}

static void
gimp_operation_config_add_sep (GimpContainer *container)
{
  GimpObject *sep = g_object_get_data (G_OBJECT (container), "separator");

  if (! sep)
    {
      sep = g_object_new (gimp_container_get_child_type (container),
                          NULL);

      gimp_container_add (container, sep);
      g_object_unref (sep);

      g_object_set_data (G_OBJECT (container), "separator", sep);
    }
}

static void
gimp_operation_config_remove_sep (GimpContainer *container)
{
  GimpObject *sep = g_object_get_data (G_OBJECT (container), "separator");

  if (sep)
    {
      gimp_container_remove (container, sep);

      g_object_set_data (G_OBJECT (container), "separator", NULL);
    }
}
