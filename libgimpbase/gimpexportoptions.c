/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#include "gimpbasetypes.h"

#include "gimpexportoptions.h"

/**
 * SECTION: gimpexportoptions
 * @title: gimpexportoptions
 * @short_description: Generic Export Options
 *
 * A class holding generic export options.

 * Note: right now, GIMP does not provide any generic export option to
 * manipulate, and there is practically no reason for you to create this
 * object yourself. In Export PDB procedure, or again in functions such
 * as [func@Gimp.file_save], you may just pass %NULL.
 *
 * In the future, this object will enable to pass various generic
 * options, such as ability to crop or resize images at export time.
 **/

enum
{
  PROP_0,
  PROP_CAPABILITIES,
  N_PROPS
};

struct _GimpExportOptions
{
  GObject                 parent_instance;

  GimpExportCapabilities  capabilities;
};


static void   gimp_export_options_finalize      (GObject              *object);
static void   gimp_export_options_set_property  (GObject              *object,
                                                 guint                 property_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void   gimp_export_options_get_property  (GObject              *object,
                                                 guint                 property_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);


G_DEFINE_TYPE (GimpExportOptions, gimp_export_options, G_TYPE_OBJECT)

#define parent_class gimp_export_options_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static void
gimp_export_options_class_init (GimpExportOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_export_options_finalize;
  object_class->get_property = gimp_export_options_get_property;
  object_class->set_property = gimp_export_options_set_property;

  /**
   * GimpExportOptions:capabilities:
   *
   * What [flags@ExportCapabilities] are supported.
   *
   * Since: 3.0.0
   */
  props[PROP_CAPABILITIES] = g_param_spec_flags ("capabilities",
                                                 "Supported image capabilities",
                                                 NULL,
                                                 GIMP_TYPE_EXPORT_CAPABILITIES,
                                                 0,
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_export_options_init (GimpExportOptions *options)
{
  options->capabilities = 0;
}

static void
gimp_export_options_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_export_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpExportOptions *options = GIMP_EXPORT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAPABILITIES:
      options->capabilities = g_value_get_flags (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_export_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpExportOptions *options = GIMP_EXPORT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAPABILITIES:
      g_value_set_flags (value, options->capabilities);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*
 * GIMP_TYPE_PARAM_EXPORT_OPTIONS
 */

static void       gimp_param_export_options_class_init        (GParamSpecClass *klass);
static void       gimp_param_export_options_init              (GParamSpec      *pspec);

GType
gimp_param_export_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_export_options_class_init,
        NULL, NULL,
        sizeof (GParamSpecObject),
        0,
        (GInstanceInitFunc) gimp_param_export_options_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamExportOptions", &info, 0);
    }

  return type;
}

static void
gimp_param_export_options_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_EXPORT_OPTIONS;
}

static void
gimp_param_export_options_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_export_options:
 * @name:         Canonical name of the property specified.
 * @nick:         Nick name of the property specified.
 * @blurb:        Description of the property specified.
 * @flags:        Flags for the property specified.
 *
 * Creates a new #GimpParamSpecExportOptions specifying a
 * #G_TYPE_INT property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecExportOptions.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_export_options (const gchar *name,
                                const gchar *nick,
                                const gchar *blurb,
                                GParamFlags  flags)
{
  GParamSpec *options_spec;

  options_spec = g_param_spec_internal (GIMP_TYPE_PARAM_EXPORT_OPTIONS,
                                        name, nick, blurb, flags);

  g_return_val_if_fail (options_spec, NULL);

  return options_spec;
}
