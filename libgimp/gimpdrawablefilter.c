/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablefilter.c
 * Copyright (C) 2024 Jehan
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

#include "gimp.h"

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimpplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _GimpDrawableFilter
{
  GObject                   parent_instance;
  gint                      id;

  gdouble                   opacity;
  GimpLayerMode             blend_mode;
  GimpLayerColorSpace       blend_space;
  GimpLayerCompositeMode    composite_mode;
  GimpLayerColorSpace       composite_space;

  GHashTable               *pad_inputs;

  GimpDrawableFilterConfig *config;
};


static void   gimp_drawable_filter_finalize      (GObject      *object);
static void   gimp_drawable_filter_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_drawable_filter_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (GimpDrawableFilter, gimp_drawable_filter, G_TYPE_OBJECT)

#define parent_class gimp_drawable_filter_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_drawable_filter_class_init (GimpDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_drawable_filter_finalize;
  object_class->set_property = gimp_drawable_filter_set_property;
  object_class->get_property = gimp_drawable_filter_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The drawable_filter id",
                      "The drawable_filter id for internal use",
                      0, G_MAXINT32, 0,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_drawable_filter_init (GimpDrawableFilter *drawable_filter)
{
  drawable_filter->config = NULL;

  drawable_filter->opacity         = 1.0;
  drawable_filter->blend_mode      = GIMP_LAYER_MODE_REPLACE;
  drawable_filter->blend_space     = GIMP_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode  = GIMP_LAYER_COMPOSITE_AUTO;
  drawable_filter->composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;

  drawable_filter->pad_inputs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
gimp_drawable_filter_finalize (GObject *object)
{
  GimpDrawableFilter *filter = GIMP_DRAWABLE_FILTER (object);

  g_clear_object (&filter->config);
  g_hash_table_unref (filter->pad_inputs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_drawable_filter_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      drawable_filter->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_filter_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, drawable_filter->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * gimp_drawable_filter_get_id:
 * @filter: The [class@Gimp.Drawable]'s filter.
 *
 * Returns: the drawable's filter ID.
 *
 * Since: 3.0
 **/
gint32
gimp_drawable_filter_get_id (GimpDrawableFilter *filter)
{
  return filter ? filter->id : -1;
}

/**
 * gimp_drawable_filter_get_by_id:
 * @filter_id: The %GimpDrawableFilter id.
 *
 * Returns: (nullable) (transfer none): a #GimpDrawableFilter for @filter_id or
 *          %NULL if @filter_id does not represent a valid drawable's filter.
 *          The object belongs to libgimp and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
GimpDrawableFilter *
gimp_drawable_filter_get_by_id (gint32 filter_id)
{
  if (filter_id > 0)
    {
      GimpPlugIn *plug_in = gimp_get_plug_in ();

      return _gimp_plug_in_get_filter (plug_in, filter_id);
    }

  return NULL;
}

/**
 * gimp_drawable_filter_is_valid:
 * @filter: The drawable's filter to check.
 *
 * Returns TRUE if the @drawable_filter is valid.
 *
 * This procedure checks if the given filter is valid and refers to an
 * existing %GimpDrawableFilter.
 *
 * Returns: Whether @drawable_filter is valid.
 *
 * Since: 3.0
 **/
gboolean
gimp_drawable_filter_is_valid (GimpDrawableFilter *filter)
{
  return gimp_drawable_filter_id_is_valid (gimp_drawable_filter_get_id (filter));
}

/**
 * gimp_drawable_filter_set_opacity:
 * @filter: The drawable's filter.
 * @opacity: the opacity.
 *
 * This procedure sets the opacity of @filter on a range from 0.0
 * (transparent) to 1.0 (opaque).
 *
 * The change is not synced immediately with the core application.
 * Use [method@Gimp.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
gimp_drawable_filter_set_opacity (GimpDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (opacity >= 0.0 && opacity <= 1.0);

  filter->opacity = opacity;
}

/**
 * gimp_drawable_filter_set_blend_mode:
 * @filter: The drawable's filter.
 * @mode: blend mode.
 *
 * This procedure sets the blend mode of @filter.
 *
 * The change is not synced immediately with the core application.
 * Use [method@Gimp.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
gimp_drawable_filter_set_blend_mode (GimpDrawableFilter *filter,
                                     GimpLayerMode       mode)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  filter->blend_mode = mode;
}

/**
 * gimp_drawable_filter_set_aux_input:
 * @filter: The drawable's filter.
 * @input_pad_name: name of the filter's input pad.
 * @input: the drawable to use as auxiliary input.
 *
 * When a filter has one or several auxiliary inputs, you can use this
 * function to set them.
 *
 * The change is not synced immediately with the core application.
 * Use [method@Gimp.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
gimp_drawable_filter_set_aux_input (GimpDrawableFilter *filter,
                                    const gchar        *input_pad_name,
                                    GimpDrawable       *input)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (GIMP_IS_DRAWABLE (input));

  g_hash_table_insert (filter->pad_inputs, (gpointer) g_strdup (input_pad_name), input);
}

/**
 * gimp_drawable_filter_get_config:
 * @filter: A drawable filter.
 *
 * Get the #GimpConfig with properties that match @filter's arguments.
 *
 * The config object will be created at the first call of this method
 * and its properties will be synced with the settings of this filter as
 * set in the core application.
 *
 * Further changes to the config's properties are not synced back
 * immediately with the core application. Use
 * [method@Gimp.Drawable.update] to trigger an actual update.
 *
 * Returns: (transfer none): The #GimpDrawableFilterConfig. Further
 *                           calls will return the same object.
 *
 * Since: 3.0
 **/
GimpDrawableFilterConfig *
gimp_drawable_filter_get_config (GimpDrawableFilter *filter)
{
  gchar          **argnames;
  GimpValueArray  *values;
  gchar           *config_type_name;
  gchar           *op_name;
  gchar           *canonical_name;
  GType            config_type;
  gint             n_args;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), NULL);

  if (filter->config)
    return filter->config;

  op_name          = gimp_drawable_filter_get_operation_name (filter);
  canonical_name   = gimp_canonicalize_identifier (op_name);
  config_type_name = g_strdup_printf ("GimpDrawableFilterConfig-%s", canonical_name);
  config_type      = g_type_from_name (config_type_name);
  n_args           = _gimp_drawable_filter_get_number_arguments (filter);

  if (! config_type)
    {
      GParamSpec **config_args;

      config_args = g_new0 (GParamSpec *, n_args);

      for (gint i = 0; i < n_args; i++)
        {
          GParamSpec *pspec;

          pspec = _gimp_drawable_filter_get_pspec (filter, i);
          config_args[i] = pspec;
        }

      config_type = gimp_config_type_register (GIMP_TYPE_DRAWABLE_FILTER_CONFIG,
                                               config_type_name, config_args, n_args);

      g_free (config_args);
    }

  g_free (op_name);
  g_free (canonical_name);

  filter->config = g_object_new (config_type, NULL);

  argnames = _gimp_drawable_filter_get_arguments (filter, &values);
  for (gint i = 0; argnames[i] != NULL; i++)
    g_object_set_property (G_OBJECT (filter->config), argnames[i],
                           gimp_value_array_index (values, i));

  g_strfreev (argnames);
  gimp_value_array_unref (values);

  return filter->config;
}

/**
 * gimp_drawable_filter_update:
 * @filter: A drawable filter.
 *
 * Syncs the #GimpConfig with properties that match @filter's arguments.
 * This procedure updates the settings of the specified filter all at
 * once, including the arguments of the [class@Gimp.DrawableFilterConfig]
 * obtained with [method@Gimp.DrawableFilter.get_config] as well as the
 * blend mode and opacity.
 *
 * In particular, if the image is displayed, rendering will be frozen
 * and will happen only once for all changed settings.
 *
 * Since: 3.0
 **/
void
gimp_drawable_filter_update (GimpDrawableFilter *filter)
{
  GStrvBuilder        *builder = g_strv_builder_new ();
  GimpValueArray      *values  = NULL;
  GStrv                propnames;
  GStrv                auxnames;
  const GimpDrawable **auxinputs;
  guint                n_aux;
  GObjectClass        *klass;
  GParamSpec         **pspecs;
  guint                n_pspecs;

  if (! filter->config)
    gimp_drawable_filter_get_config (filter);

  g_return_if_fail (filter->config != NULL);

  klass  = G_OBJECT_GET_CLASS (filter->config);
  pspecs = g_object_class_list_properties (klass, &n_pspecs);
  values = gimp_value_array_new (n_pspecs);

  for (guint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (filter->config), pspec->name, &value);

      if (g_param_value_defaults (pspec, &value))
        {
          g_value_unset (&value);
          continue;
        }

      g_strv_builder_add (builder, pspec->name);
      gimp_value_array_append (values, &value);

      g_value_unset (&value);
    }

  propnames = g_strv_builder_end (builder);
  auxnames  = (GStrv) g_hash_table_get_keys_as_array (filter->pad_inputs, &n_aux);
  auxinputs = g_new0 (const GimpDrawable *, n_aux + 1);
  for (guint i = 0; auxnames[i]; i++)
    auxinputs[i] = g_hash_table_lookup (filter->pad_inputs, auxnames[i]);

  _gimp_drawable_filter_update (filter, (const gchar **) propnames, values,
                                filter->opacity,
                                filter->blend_mode, filter->blend_space,
                                filter->composite_mode, filter->composite_space,
                                (const gchar **) auxnames, auxinputs);

  g_free (pspecs);
  g_strfreev (propnames);
  g_strv_builder_unref (builder);
  gimp_value_array_unref (values);
  g_free (auxnames);
  g_free (auxinputs);
}
