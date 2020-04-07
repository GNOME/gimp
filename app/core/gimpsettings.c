/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettings.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpsettings.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TIME
};


static void    gimp_settings_get_property    (GObject       *object,
                                              guint          property_id,
                                              GValue        *value,
                                              GParamSpec    *pspec);
static void    gimp_settings_set_property    (GObject       *object,
                                              guint          property_id,
                                              const GValue  *value,
                                              GParamSpec    *pspec);

static gchar * gimp_settings_get_description (GimpViewable  *viewable,
                                              gchar        **tooltip);


G_DEFINE_TYPE (GimpSettings, gimp_settings, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_settings_parent_class


static void
gimp_settings_class_init (GimpSettingsClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property      = gimp_settings_set_property;
  object_class->get_property      = gimp_settings_get_property;

  viewable_class->get_description = gimp_settings_get_description;
  viewable_class->name_editable   = TRUE;

  GIMP_CONFIG_PROP_INT64 (object_class, PROP_TIME,
                          "time",
                          "Time",
                          "Time of settings creation",
                          0, G_MAXINT64, 0,
                          GIMP_CONFIG_PARAM_DONT_COMPARE);
}

static void
gimp_settings_init (GimpSettings *settings)
{
}

static void
gimp_settings_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpSettings *settings = GIMP_SETTINGS (object);

  switch (property_id)
    {
    case PROP_TIME:
      g_value_set_int64 (value, settings->time);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_settings_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpSettings *settings = GIMP_SETTINGS (object);

  switch (property_id)
    {
    case PROP_TIME:
      settings->time = g_value_get_int64 (value);

      if (settings->time > 0)
        {
          GDateTime *utc   = g_date_time_new_from_unix_utc (settings->time);
          GDateTime *local = g_date_time_to_local (utc);
          gchar     *name;

          name = g_date_time_format (local, "%Y-%m-%d %H:%M:%S");
          gimp_object_take_name (GIMP_OBJECT (settings), name);

          g_date_time_unref (local);
          g_date_time_unref (utc);
        }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gchar *
gimp_settings_get_description (GimpViewable  *viewable,
                               gchar        **tooltip)
{
  GimpSettings *settings = GIMP_SETTINGS (viewable);

  if (settings->time > 0)
    {
      if (tooltip)
        *tooltip = g_strdup ("You can rename automatic presets "
                             "to make them permanently saved");

      return g_strdup_printf (_("Last used: %s"),
                              gimp_object_get_name (settings));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}


/*  public functions  */

gint
gimp_settings_compare (GimpSettings *a,
                       GimpSettings *b)
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
