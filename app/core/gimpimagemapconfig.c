/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagemapconfig.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpimagemapconfig.h"


enum
{
  PROP_0,
  PROP_TIME
};


static void   gimp_image_map_config_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void   gimp_image_map_config_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (GimpImageMapConfig, gimp_image_map_config,
               GIMP_TYPE_VIEWABLE)

#define parent_class gimp_image_map_config_parent_class


static void
gimp_image_map_config_class_init (GimpImageMapConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_image_map_config_set_property;
  object_class->get_property = gimp_image_map_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_UINT (object_class, PROP_TIME,
                                 "time",
                                 "Time of settings creation",
                                 0, G_MAXUINT, 0, 0);
}

static void
gimp_image_map_config_init (GimpImageMapConfig *config)
{
}

static void
gimp_image_map_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpImageMapConfig *config = GIMP_IMAGE_MAP_CONFIG (object);

  switch (property_id)
    {
    case PROP_TIME:
      g_value_set_uint (value, config->time);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_map_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpImageMapConfig *config = GIMP_IMAGE_MAP_CONFIG (object);

  switch (property_id)
    {
    case PROP_TIME:
      config->time = g_value_get_uint (value);

      if (config->time > 0)
        {
          time_t     t;
          struct tm  tm;
          gchar      buf[64];
          gchar     *name;

          t = config->time;
          tm = *localtime (&t);
          strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", &tm);

          name = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
          gimp_object_set_name (GIMP_OBJECT (config), name);
          g_free (name);
        }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

gint
gimp_image_map_config_compare (GimpImageMapConfig *a,
                               GimpImageMapConfig *b)
{
  const gchar *name_a = gimp_object_get_name (a);
  const gchar *name_b = gimp_object_get_name (b);

  if (a->time > 0 && b->time > 0)
    {
      return - strcmp (name_a, name_b);
    }
  else if (a->time > 0)
    {
      return -1;
    }
  else if (b->time > 0)
    {
      return 1;
    }
  else if (name_a && name_b)
    {
      return strcmp (name_a, name_b);
    }
  else if (name_a)
    {
      return 1;
    }
  else if (name_b)
    {
      return -1;
    }

  return 0;
}
