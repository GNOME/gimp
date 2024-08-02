/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpconfig-register.c
 * Copyright (C) 2008-2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gegl-paramspecs.h>

#include "libgimpbase/gimpbase.h"

#include "gimpconfig.h"


/*  local function prototypes  */

static void     gimp_config_class_init   (GObjectClass  *klass,
                                          GParamSpec   **pspecs);
static void     gimp_config_set_property (GObject       *object,
                                          guint          property_id,
                                          const GValue  *value,
                                          GParamSpec    *pspec);
static void     gimp_config_get_property (GObject       *object,
                                          guint          property_id,
                                          GValue        *value,
                                          GParamSpec    *pspec);

static GValue * gimp_config_value_get    (GObject       *object,
                                          GParamSpec    *pspec);
static GValue * gimp_config_value_new    (GParamSpec    *pspec);
static void     gimp_config_value_free   (GValue        *value);


/*  public functions  */

/**
 * gimp_config_type_register:
 * @parent_type: type from which this type will be derived
 * @type_name:   string used as the name of the new type
 * @pspecs: (array length=n_pspecs): array of #GParamSpec to install as properties on the new type
 * @n_pspecs:    the number of param specs in @pspecs
 *
 * This function is a fancy wrapper around g_type_register_static().
 * It creates a new object type as subclass of @parent_type, installs
 * @pspecs on it and makes the new type implement the #GimpConfig
 * interface.
 *
 * Returns: the newly registered #GType
 *
 * Since: 3.0
 **/
GType
gimp_config_type_register (GType         parent_type,
                           const gchar  *type_name,
                           GParamSpec  **pspecs,
                           gint          n_pspecs)
{
  GParamSpec **terminated_pspecs;
  GTypeQuery   query;
  GType        config_type;

  g_return_val_if_fail (g_type_is_a (parent_type, G_TYPE_OBJECT), G_TYPE_NONE);
  g_return_val_if_fail (type_name != NULL, G_TYPE_NONE);
  g_return_val_if_fail (pspecs != NULL || n_pspecs == 0, G_TYPE_NONE);

  terminated_pspecs = g_new0 (GParamSpec *, n_pspecs + 1);

  memcpy (terminated_pspecs, pspecs, sizeof (GParamSpec *) * n_pspecs);

  g_type_query (parent_type, &query);

  {
    GTypeInfo info =
    {
      query.class_size,
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gimp_config_class_init,
      NULL,           /* class_finalize */
      terminated_pspecs,
      query.instance_size,
      0,              /* n_preallocs */
      (GInstanceInitFunc) NULL,
    };

    config_type = g_type_register_static (parent_type, type_name,
                                          &info, 0);

    if (! g_type_is_a (parent_type, GIMP_TYPE_CONFIG))
      {
        const GInterfaceInfo config_info =
        {
          NULL, /* interface_init     */
          NULL, /* interface_finalize */
          NULL  /* interface_data     */
        };

        g_type_add_interface_static (config_type, GIMP_TYPE_CONFIG,
                                     &config_info);
      }
  }

  return config_type;
}


/*  private functions  */

static void
gimp_config_class_init (GObjectClass  *klass,
                        GParamSpec   **pspecs)
{
  gint i;

  klass->set_property = gimp_config_set_property;
  klass->get_property = gimp_config_get_property;

  for (i = 0; pspecs[i] != NULL; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GParamSpec *copy  = gimp_config_param_spec_duplicate (pspec);

      if (copy)
        {
          g_object_class_install_property (klass, i + 1, copy);
          /* If the original param spec was floating, this would unref
           * it. Otherwise (e.g. it's a spec taken from another object),
           * nothing happens.
           */
          g_param_spec_sink (pspec);
        }
      else
        {
          GType        value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
          const gchar *type_name  = g_type_name (value_type);

          /* There are some properties that we don't care to copy because they
           * are not serializable anyway (or we don't want them to be).
           * GimpContext properties are one such property type. We can find them
           * e.g. in some custom GEGL ops, such as "gimp:offset". So we silently
           * ignore these.
           * We might add more types of properties to the list as we discover
           * more cases. We keep warnings for all the other types which we
           * explicitly don't support.
           */
          if (g_strcmp0 (type_name, "GimpContext") != 0 &&
              /* Format specs are a GParamSpecPointer. There might be other
               * pointer specs we might be able to serialize, but BablFormat are
               * not one of these (there might be easy serializable formats, but
               * many are not and anyway it's probably not a data which ops or
               * plug-ins want to remember across run).
               */
              ! GEGL_IS_PARAM_SPEC_FORMAT (pspec))
            g_warning ("%s: not supported: %s (%s | %s)\n", G_STRFUNC,
                       g_type_name (G_TYPE_FROM_INSTANCE (pspec)), pspec->name, type_name);
        }
    }
}

static void
gimp_config_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GValue *val = gimp_config_value_get (object, pspec);

  g_value_copy (value, val);
}

static void
gimp_config_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GValue *val = gimp_config_value_get (object, pspec);

  g_value_copy (val, value);
}

static GValue *
gimp_config_value_get (GObject    *object,
                       GParamSpec *pspec)
{
  GHashTable *properties;
  GValue     *value;

  properties = g_object_get_data (object, "gimp-config-properties");

  if (! properties)
    {
      properties =
        g_hash_table_new_full (g_str_hash,
                               g_str_equal,
                               (GDestroyNotify) g_free,
                               (GDestroyNotify) gimp_config_value_free);

      g_object_set_data_full (object, "gimp-config-properties", properties,
                              (GDestroyNotify) g_hash_table_unref);
    }

  value = g_hash_table_lookup (properties, pspec->name);

  if (! value)
    {
      value = gimp_config_value_new (pspec);
      g_hash_table_insert (properties, g_strdup (pspec->name), value);
    }

  return value;
}

static GValue *
gimp_config_value_new (GParamSpec *pspec)
{
  GValue *value = g_slice_new0 (GValue);

  g_value_init (value, pspec->value_type);
  g_param_value_set_default (pspec, value);

  return value;
}

static void
gimp_config_value_free (GValue *value)
{
  g_value_unset (value);
  g_slice_free (GValue, value);
}
