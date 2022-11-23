/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "core/ligma.h"

#include "core/ligmalist.h"
#include "core/ligmaviewable.h"

#include "gegl/ligma-gegl-utils.h"

#include "ligma-operation-config.h"


/*  local function prototypes  */

static void    ligma_operation_config_config_sync   (GObject          *config,
                                                    const GParamSpec *ligma_pspec,
                                                    GeglNode         *node);
static void    ligma_operation_config_config_notify (GObject          *config,
                                                    const GParamSpec *ligma_pspec,
                                                    GeglNode         *node);
static void    ligma_operation_config_node_notify   (GeglNode         *node,
                                                    const GParamSpec *gegl_pspec,
                                                    GObject          *config);

static GFile * ligma_operation_config_get_file      (GType             config_type);
static void    ligma_operation_config_add_sep       (LigmaContainer    *container);
static void    ligma_operation_config_remove_sep    (LigmaContainer    *container);


/*  public functions  */

static GHashTable *
ligma_operation_config_get_type_table (Ligma *ligma)
{
  static GHashTable *config_types = NULL;

  if (! config_types)
    config_types = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          (GDestroyNotify) g_free,
                                          NULL);

  return config_types;
}

static GHashTable *
ligma_operation_config_get_container_table (Ligma *ligma)
{
  static GHashTable *config_containers = NULL;

  if (! config_containers)
    config_containers = g_hash_table_new_full (g_direct_hash,
                                               g_direct_equal,
                                               NULL,
                                               (GDestroyNotify) g_object_unref);

  return config_containers;
}


/*  public functions  */

void
ligma_operation_config_register (Ligma        *ligma,
                                const gchar *operation,
                                GType        config_type)
{
  GHashTable *config_types;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (operation != NULL);
  g_return_if_fail (g_type_is_a (config_type, LIGMA_TYPE_OBJECT));

  config_types = ligma_operation_config_get_type_table (ligma);

  g_hash_table_insert (config_types,
                       g_strdup (operation),
                       (gpointer) config_type);
}

GType
ligma_operation_config_get_type (Ligma        *ligma,
                                const gchar *operation,
                                const gchar *icon_name,
                                GType        parent_type)
{
  GHashTable *config_types;
  GType       config_type;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), G_TYPE_NONE);
  g_return_val_if_fail (operation != NULL, G_TYPE_NONE);
  g_return_val_if_fail (g_type_is_a (parent_type, LIGMA_TYPE_OBJECT),
                        G_TYPE_NONE);

  config_types = ligma_operation_config_get_type_table (ligma);

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
              pspecs[j] = pspec;
              j++;
            }
        }

      n_pspecs = j;

      type_name = g_strdup_printf ("LigmaGegl-%s-config", operation);

      g_strcanon (type_name,
                  G_CSET_DIGITS "-" G_CSET_a_2_z G_CSET_A_2_Z, '-');

      config_type = ligma_config_type_register (parent_type,
                                               type_name,
                                               pspecs, n_pspecs);

      g_free (pspecs);
      g_free (type_name);

      if (icon_name && g_type_is_a (config_type, LIGMA_TYPE_VIEWABLE))
        {
          LigmaViewableClass *viewable_class = g_type_class_ref (config_type);

          viewable_class->default_icon_name = g_strdup (icon_name);

          g_type_class_unref (viewable_class);
        }

      ligma_operation_config_register (ligma, operation, config_type);
    }

  return config_type;
}

LigmaContainer *
ligma_operation_config_get_container (Ligma         *ligma,
                                     GType         config_type,
                                     GCompareFunc  sort_func)
{
  GHashTable    *config_containers;
  LigmaContainer *container;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (g_type_is_a (config_type, LIGMA_TYPE_OBJECT), NULL);

  config_containers = ligma_operation_config_get_container_table (ligma);

  container = g_hash_table_lookup (config_containers, (gpointer) config_type);

  if (! container)
    {
      container = ligma_list_new (config_type, TRUE);
      ligma_list_set_sort_func (LIGMA_LIST (container), sort_func);

      g_hash_table_insert (config_containers,
                           (gpointer) config_type, container);

      ligma_operation_config_deserialize (ligma, container, NULL);

      if (ligma_container_get_n_children (container) == 0)
        {
          GFile *file = ligma_operation_config_get_file (config_type);

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
                      ligma_operation_config_deserialize (ligma, container,
                                                         compat_file);
                    }
                  else
                    {
                      ligma_operation_config_deserialize (ligma, container, NULL);
                    }
                }
            }

          g_object_unref (file);
        }

      ligma_operation_config_add_sep (container);
    }

  return container;
}

void
ligma_operation_config_serialize (Ligma          *ligma,
                                 LigmaContainer *container,
                                 GFile         *file)
{
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_CONTAINER (container));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (file)
    {
      g_object_ref (file);
    }
  else
    {
      GType config_type = ligma_container_get_children_type (container);

      file = ligma_operation_config_get_file (config_type);
    }

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  ligma_operation_config_remove_sep (container);

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (container),
                                       file,
                                       "settings",
                                       "end of settings",
                                       NULL, &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }

  ligma_operation_config_add_sep (container);

  g_object_unref (file);
}

void
ligma_operation_config_deserialize (Ligma          *ligma,
                                   LigmaContainer *container,
                                   GFile         *file)
{
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_CONTAINER (container));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (file)
    {
      g_object_ref (file);
    }
  else
    {
      GType config_type = ligma_container_get_children_type (container);

      file = ligma_operation_config_get_file (config_type);
    }

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (container),
                                      file,
                                      NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR,
                              error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);
}

void
ligma_operation_config_sync_node (GObject  *config,
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
      GParamSpec *ligma_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
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
      else if (ligma_pspec)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, ligma_pspec->value_type);

          g_object_get_property (G_OBJECT (config), ligma_pspec->name,
                                 &value);

          if (GEGL_IS_PARAM_SPEC_COLOR (gegl_pspec))
            {
              LigmaRGB    ligma_color;
              GeglColor *gegl_color;

              ligma_value_get_rgb (&value, &ligma_color);
              g_value_unset (&value);

              gegl_color = ligma_gegl_color_new (&ligma_color, NULL);

              g_value_init (&value, gegl_pspec->value_type);
              g_value_take_object (&value, gegl_color);
            }

          gegl_node_set_property (node, gegl_pspec->name,
                                  &value);

          g_value_unset (&value);
        }
    }

  g_free (pspecs);
}

void
ligma_operation_config_connect_node (GObject  *config,
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
                                   G_CALLBACK (ligma_operation_config_config_sync),
                                   node, 0);
          g_free (pspecs);
          return;
        }
    }

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *gegl_pspec = pspecs[i];
      GParamSpec *ligma_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                             gegl_pspec->name);

      if (ligma_pspec)
        {
          gchar *notify_name = g_strconcat ("notify::", ligma_pspec->name, NULL);

          g_signal_connect_object (config, notify_name,
                                   G_CALLBACK (ligma_operation_config_config_notify),
                                   node, 0);

          g_signal_connect_object (node, notify_name,
                                   G_CALLBACK (ligma_operation_config_node_notify),
                                   config, 0);

          g_free (notify_name);
        }
    }

  g_free (pspecs);
}

GParamSpec **
ligma_operation_config_list_properties (GObject     *config,
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

      if (ligma_gegl_param_spec_has_key (pspec, "role", "output-extent"))
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
ligma_operation_config_config_sync (GObject          *config,
                                   const GParamSpec *ligma_pspec,
                                   GeglNode         *node)
{
  ligma_operation_config_sync_node (config, node);
}

static void
ligma_operation_config_config_notify (GObject          *config,
                                     const GParamSpec *ligma_pspec,
                                     GeglNode         *node)
{
  GParamSpec *gegl_pspec = gegl_node_find_property (node, ligma_pspec->name);

  if (gegl_pspec)
    {
      GValue value = G_VALUE_INIT;
      gulong handler;

      g_value_init (&value, ligma_pspec->value_type);
      g_object_get_property (config, ligma_pspec->name, &value);

      if (GEGL_IS_PARAM_SPEC_COLOR (gegl_pspec))
        {
          LigmaRGB    ligma_color;
          GeglColor *gegl_color;

          ligma_value_get_rgb (&value, &ligma_color);
          g_value_unset (&value);

          gegl_color = ligma_gegl_color_new (&ligma_color, NULL);

          g_value_init (&value, gegl_pspec->value_type);
          g_value_take_object (&value, gegl_color);
        }

      handler = g_signal_handler_find (node,
                                       G_SIGNAL_MATCH_DETAIL |
                                       G_SIGNAL_MATCH_FUNC   |
                                       G_SIGNAL_MATCH_DATA,
                                       0,
                                       g_quark_from_string (gegl_pspec->name),
                                       NULL,
                                       ligma_operation_config_node_notify,
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
ligma_operation_config_node_notify (GeglNode         *node,
                                   const GParamSpec *gegl_pspec,
                                   GObject          *config)
{
  GParamSpec *ligma_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                         gegl_pspec->name);

  if (ligma_pspec)
    {
      GValue value = G_VALUE_INIT;
      gulong handler;

      g_value_init (&value, gegl_pspec->value_type);
      gegl_node_get_property (node, gegl_pspec->name, &value);

      if (GEGL_IS_PARAM_SPEC_COLOR (gegl_pspec))
        {
          GeglColor *gegl_color;
          LigmaRGB    ligma_color;

          gegl_color = g_value_dup_object (&value);
          g_value_unset (&value);

          if (gegl_color)
            {
              gegl_color_get_rgba (gegl_color,
                                   &ligma_color.r,
                                   &ligma_color.g,
                                   &ligma_color.b,
                                   &ligma_color.a);
              g_object_unref (gegl_color);
            }
          else
            {
              ligma_rgba_set (&ligma_color, 0.0, 0.0, 0.0, 1.0);
            }

          g_value_init (&value, ligma_pspec->value_type);
          ligma_value_set_rgb (&value, &ligma_color);
        }

      handler = g_signal_handler_find (config,
                                       G_SIGNAL_MATCH_DETAIL |
                                       G_SIGNAL_MATCH_FUNC   |
                                       G_SIGNAL_MATCH_DATA,
                                       0,
                                       g_quark_from_string (ligma_pspec->name),
                                       NULL,
                                       ligma_operation_config_config_notify,
                                       node);

      if (handler)
        g_signal_handler_block (config, handler);

      g_object_set_property (config, ligma_pspec->name, &value);
      g_value_unset (&value);

      if (handler)
        g_signal_handler_unblock (config, handler);
    }
}

static GFile *
ligma_operation_config_get_file (GType config_type)
{
  GFile *file;
  gchar *basename;

  basename = g_strconcat (g_type_name (config_type), ".settings", NULL);
  file = ligma_directory_file ("filters", basename, NULL);
  g_free (basename);

  return file;
}

static void
ligma_operation_config_add_sep (LigmaContainer *container)
{
  LigmaObject *sep = g_object_get_data (G_OBJECT (container), "separator");

  if (! sep)
    {
      sep = g_object_new (ligma_container_get_children_type (container),
                          NULL);

      ligma_container_add (container, sep);
      g_object_unref (sep);

      g_object_set_data (G_OBJECT (container), "separator", sep);
    }
}

static void
ligma_operation_config_remove_sep (LigmaContainer *container)
{
  LigmaObject *sep = g_object_get_data (G_OBJECT (container), "separator");

  if (sep)
    {
      ligma_container_remove (container, sep);

      g_object_set_data (G_OBJECT (container), "separator", NULL);
    }
}
