/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimptoolinfo.h"
#include "gimptooloptions.h"


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void   gimp_tool_options_set_property (GObject         *object,
                                              guint            property_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void   gimp_tool_options_get_property (GObject         *object,
                                              guint            property_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);

static void   gimp_tool_options_real_reset   (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpToolOptions, gimp_tool_options, GIMP_TYPE_CONTEXT)


static void
gimp_tool_options_class_init (GimpToolOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_tool_options_set_property;
  object_class->get_property = gimp_tool_options_get_property;

  klass->reset               = gimp_tool_options_real_reset;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_INFO,
                                                        GIMP_PARAM_READWRITE));

}

static void
gimp_tool_options_init (GimpToolOptions *options)
{
  options->tool_info = NULL;
}

static void
gimp_tool_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      {
        GimpToolInfo *tool_info = g_value_get_object (value);

        g_return_if_fail (options->tool_info == NULL ||
                          options->tool_info == tool_info);

        if (! options->tool_info)
          options->tool_info = GIMP_TOOL_INFO (g_value_dup_object (value));
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      g_value_set_object (value, options->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_options_real_reset (GimpToolOptions *tool_options)
{
  gimp_config_reset (GIMP_CONFIG (tool_options));
}

void
gimp_tool_options_reset (GimpToolOptions *tool_options)
{
  g_return_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options));

  GIMP_TOOL_OPTIONS_GET_CLASS (tool_options)->reset (tool_options);
}

gchar *
gimp_tool_options_build_filename (GimpToolOptions *tool_options,
                                  const gchar     *extension)
{
  gchar *filename;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  if (extension)
    {
      gchar *basename;

      basename = g_strconcat (GIMP_OBJECT (tool_options->tool_info)->name,
                              ".", extension, NULL);

      filename = g_build_filename (gimp_directory (),
                                   "tool-options",
                                   basename,
                                   NULL);
      g_free (basename);
    }
  else
    {
      filename = g_build_filename (gimp_directory (),
                                   "tool-options",
                                   GIMP_OBJECT (tool_options->tool_info)->name,
                                   NULL);
    }

  return filename;
}

gboolean
gimp_tool_options_serialize (GimpToolOptions  *tool_options,
                             const gchar      *extension,
                             GError          **error)
{
  gchar    *filename;
  gchar    *header;
  gchar    *footer;
  gboolean  retval;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_tool_options_build_filename (tool_options, extension);

  if (tool_options->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  header = g_strdup_printf ("GIMP %s options",
                            GIMP_OBJECT (tool_options->tool_info)->name);
  footer = g_strdup_printf ("end of %s options",
                            GIMP_OBJECT (tool_options->tool_info)->name);

  retval = gimp_config_serialize_to_file (GIMP_CONFIG (tool_options),
                                          filename,
                                          header, footer,
                                          NULL,
                                          error);

  g_free (filename);
  g_free (header);
  g_free (footer);

  return retval;
}

gboolean
gimp_tool_options_deserialize (GimpToolOptions  *tool_options,
                               const gchar      *extension,
                               GError          **error)
{
  gchar    *filename;
  gboolean  retval;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_tool_options_build_filename (tool_options, extension);

  if (tool_options->tool_info->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  retval = gimp_config_deserialize_file (GIMP_CONFIG (tool_options),
                                         filename,
                                         NULL,
                                         error);

  g_free (filename);

  return retval;
}
