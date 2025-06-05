/*
 * Copyright (C) 2025 Ondřej Míchal <harrymichal@seznam.cz>
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
#include <glib.h>

#include <libgimp/gimp.h>

#include "gegl-filter-info.h"

typedef struct _GimpGeglFilterInfo
{
  GObject parent_instance;

  gchar          *name;
  gchar          *title;
  gchar          *description;
  gchar          *categories;
  gchar          *license;
  GimpValueArray *pspecs;
} GimpGeglFilterInfo;

enum
{
  PROP_0,
  PROP_NAME,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_CATEGORIES,
  PROP_LICENSE,
  PROP_PSPECS,
  N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES] = { 0 };

G_DEFINE_FINAL_TYPE (GimpGeglFilterInfo, gimp_gegl_filter_info, G_TYPE_OBJECT)

static void
gimp_gegl_filter_info_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  GimpGeglFilterInfo *self = GIMP_GEGL_FILTER_INFO (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, self->description);
      break;
    case PROP_CATEGORIES:
      g_value_set_string (value, self->categories);
      break;
    case PROP_LICENSE:
      g_value_set_string (value, self->license);
      break;
    case PROP_PSPECS:
      g_value_set_boxed (value, self->pspecs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gimp_gegl_filter_info_set_property (GObject *object, guint prop_id,
                                    const GValue *value, GParamSpec *pspec)
{
  GimpGeglFilterInfo *self = GIMP_GEGL_FILTER_INFO (object);

  switch (prop_id)
    {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;
    case PROP_TITLE:
      self->title = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      self->description = g_value_dup_string (value);
      break;
    case PROP_CATEGORIES:
      self->categories = g_value_dup_string (value);
      break;
    case PROP_LICENSE:
      self->license = g_value_dup_string (value);
      break;
    case PROP_PSPECS:
      self->pspecs = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gimp_gegl_filter_info_class_init (GimpGeglFilterInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_gegl_filter_info_get_property;
  object_class->set_property = gimp_gegl_filter_info_set_property;

  properties[PROP_NAME]        = g_param_spec_string ("name", "Name",
                                                      "", NULL,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE      |
                                                      G_PARAM_STATIC_STRINGS);
  properties[PROP_TITLE]       = g_param_spec_string ("title", "Title",
                                                      "", NULL,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE      |
                                                      G_PARAM_STATIC_STRINGS);
  properties[PROP_DESCRIPTION] = g_param_spec_string ("description", "Description",
                                                      "", NULL,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE      |
                                                      G_PARAM_STATIC_STRINGS);
  properties[PROP_CATEGORIES]  = g_param_spec_string ("categories", "Categories",
                                                      "", NULL,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE      |
                                                      G_PARAM_STATIC_STRINGS);
  properties[PROP_LICENSE]     = g_param_spec_string ("license", "License",
                                                      "", NULL,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE      |
                                                      G_PARAM_STATIC_STRINGS);
  properties[PROP_PSPECS]      = g_param_spec_boxed ("pspecs", "Parameter Specifications",
                                                     "", GIMP_TYPE_VALUE_ARRAY,
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_READWRITE      |
                                                     G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gimp_gegl_filter_info_init (GimpGeglFilterInfo *self)
{
}
