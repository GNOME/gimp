/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gegl.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimptooloptions.h"
#include "gimptoolpreset.h"
#include "gimptoolpreset-load.h"
#include "gimptoolpreset-save.h"

#include "gimp-intl.h"

enum
{
  PROP_0,

  PROP_NAME,

  PROP_TOOL_OPTIONS
};


static void          gimp_tool_preset_finalize      (GObject      *object);
static void          gimp_tool_preset_set_property  (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void          gimp_tool_preset_get_property  (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);
static void
       gimp_tool_preset_dispatch_properties_changed (GObject      *object,
                                                     guint         n_pspecs,
                                                     GParamSpec  **pspecs);

static const gchar * gimp_tool_preset_get_extension (GimpData     *data);


G_DEFINE_TYPE (GimpToolPreset, gimp_tool_preset,
               GIMP_TYPE_DATA)

#define parent_class gimp_tool_preset_parent_class


static void
gimp_tool_preset_class_init (GimpToolPresetClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpDataClass *data_class   = GIMP_DATA_CLASS (klass);

  object_class->finalize                    = gimp_tool_preset_finalize;
  object_class->set_property                = gimp_tool_preset_set_property;
  object_class->get_property                = gimp_tool_preset_get_property;
  object_class->dispatch_properties_changed = gimp_tool_preset_dispatch_properties_changed;

  data_class->save                          = gimp_tool_preset_save;
  data_class->get_extension                 = gimp_tool_preset_get_extension;

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_NAME,
                                   "name", NULL,
                                   "Unnamed",
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_TOOL_OPTIONS,
                                   "tool-options", NULL,
                                   GIMP_TYPE_TOOL_PRESET,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

}

static void
gimp_tool_preset_init (GimpToolPreset *tool_preset)
{
  tool_preset->tool_options = NULL;
}

static void
gimp_tool_preset_finalize (GObject *object)
{
  GimpToolPreset *tool_preset = GIMP_TOOL_PRESET (object);

  g_object_unref (tool_preset->tool_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_preset_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpToolPreset       *tool_preset    = GIMP_TOOL_PRESET (object);
  GimpToolOptions      *src_output  = NULL;
  GimpToolOptions      *dest_output = NULL;

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (GIMP_OBJECT (tool_preset), g_value_get_string (value));
      break;

    case PROP_TOOL_OPTIONS:
      src_output  = g_value_get_object (value);
      dest_output = tool_preset->tool_options;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (src_output && dest_output)
    {
      gimp_config_copy (GIMP_CONFIG (src_output),
                        GIMP_CONFIG (dest_output),
                        GIMP_CONFIG_PARAM_SERIALIZE);
    }
}

static void
gimp_tool_preset_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpToolPreset *tool_preset = GIMP_TOOL_PRESET (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gimp_object_get_name (tool_preset));
      break;

    case PROP_TOOL_OPTIONS:
      g_value_set_object (value, tool_preset->tool_options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_preset_dispatch_properties_changed (GObject     *object,
                                              guint        n_pspecs,
                                              GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & GIMP_CONFIG_PARAM_SERIALIZE)
        {
          gimp_data_dirty (GIMP_DATA (object));
          break;
        }
    }
}

static const gchar *
gimp_tool_preset_get_extension (GimpData *data)
{
  return GIMP_TOOL_PRESET_FILE_EXTENSION;
}


/*  public functions  */

GimpData *
gimp_tool_preset_new (const gchar *name)
{
  return g_object_new (GIMP_TYPE_TOOL_PRESET,
                       "name", name,
 //                      "tool-options", options,
                       NULL);
}



GimpData *
gimp_tool_preset_get_standard (void)
{
  static GimpData *standard_tool_preset = NULL;

  if (! standard_tool_preset)
    {
      standard_tool_preset = gimp_tool_preset_new ("Standard tool preset");

      gimp_data_clean (standard_tool_preset);
      gimp_data_make_internal (standard_tool_preset, "gimp-tool-preset-standard");

      g_object_ref (standard_tool_preset);
    }

  return standard_tool_preset;
}
