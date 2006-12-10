/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpresets.c
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
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

#include <errno.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimptoolinfo.h"
#include "gimptooloptions.h"
#include "gimptoolpresets.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void   gimp_tool_presets_finalize     (GObject         *object);
static void   gimp_tool_presets_set_property (GObject         *object,
                                              guint            property_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void   gimp_tool_presets_get_property (GObject         *object,
                                              guint            property_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);


G_DEFINE_TYPE (GimpToolPresets, gimp_tool_presets, GIMP_TYPE_LIST)

#define parent_class gimp_tool_presets_parent_class


static void
gimp_tool_presets_class_init (GimpToolPresetsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_tool_presets_set_property;
  object_class->get_property = gimp_tool_presets_get_property;
  object_class->finalize     = gimp_tool_presets_finalize;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_INFO,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_tool_presets_init (GimpToolPresets *list)
{
}

static void
gimp_tool_presets_finalize (GObject *object)
{
  GimpToolPresets *presets = GIMP_TOOL_PRESETS (object);

  if (presets->tool_info)
    {
      g_object_unref (presets->tool_info);
      presets->tool_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_presets_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpToolPresets *presets = GIMP_TOOL_PRESETS (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      presets->tool_info = GIMP_TOOL_INFO (g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_presets_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpToolPresets *presets = GIMP_TOOL_PRESETS (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      g_value_set_object (value, presets->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpToolPresets *
gimp_tool_presets_new (GimpToolInfo *tool_info)
{
  GimpToolPresets *presets;
  gchar           *name;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);

  presets = g_object_new (GIMP_TYPE_TOOL_PRESETS,
                          "tool-info",     tool_info,
                          "children-type", tool_info->tool_options_type,
                          "policy",        GIMP_CONTAINER_POLICY_STRONG,
                          NULL);

  name = g_strdup_printf ("%s options",
                          gimp_object_get_name (GIMP_OBJECT (tool_info)));

  gimp_object_take_name (GIMP_OBJECT (presets), name);

  return presets;
}

GimpToolOptions *
gimp_tool_presets_get_options (GimpToolPresets *presets,
                               gint             index)
{
  g_return_val_if_fail (GIMP_IS_TOOL_PRESETS (presets), NULL);

  return (GimpToolOptions *)
    gimp_container_get_child_by_index (GIMP_CONTAINER (presets), index);
}

gboolean
gimp_tool_presets_save (GimpToolPresets  *presets,
                        gboolean          be_verbose,
                        GError          **error)
{
  const gchar *name;
  gchar       *filename;
  gboolean     retval = TRUE;

  g_return_val_if_fail (GIMP_IS_TOOL_PRESETS (presets), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = gimp_object_get_name (GIMP_OBJECT (presets->tool_info));

  filename = gimp_tool_options_build_filename (name, "presets");

  if (gimp_container_num_children (GIMP_CONTAINER (presets)) > 0)
    {
      gchar *footer;

      if (be_verbose)
        g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

      footer = g_strdup_printf ("end of %s", GIMP_OBJECT (presets)->name);

      retval = gimp_config_serialize_to_file (GIMP_CONFIG (presets), filename,
                                              GIMP_OBJECT (presets)->name,
                                              footer,
                                              NULL, error);

      g_free (footer);
    }
  else if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      if (be_verbose)
        g_print ("Deleting '%s'\n", gimp_filename_to_utf8 (filename));

      if (g_unlink (filename) != 0)
        {
          retval = FALSE;

          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not delete '%s': %s"),
                       gimp_filename_to_utf8 (filename), g_strerror (errno));
        }
    }

  g_free (filename);

  return retval;
}

gboolean
gimp_tool_presets_load (GimpToolPresets  *presets,
                        gboolean          be_verbose,
                        GError          **error)
{
  GList       *list;
  const gchar *name;
  gchar       *filename;
  gboolean     retval = TRUE;

  g_return_val_if_fail (GIMP_IS_TOOL_PRESETS (presets), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = gimp_object_get_name (GIMP_OBJECT (presets->tool_info));

  filename = gimp_tool_options_build_filename (name, "presets");

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      if (be_verbose)
        g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

      retval = gimp_config_deserialize_file (GIMP_CONFIG (presets), filename,
                                             presets->tool_info->gimp,
                                             error);
      gimp_list_reverse (GIMP_LIST (presets));

      for (list = GIMP_LIST (presets)->list; list; list = g_list_next (list))
        {
          g_object_set (list->data, "tool-info", presets->tool_info, NULL);
        }
    }

  g_free (filename);

  return retval;
}
