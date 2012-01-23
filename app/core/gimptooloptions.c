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

#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h" /* For S_IRGRP etc */
#endif
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimperror.h"
#include "gimptoolinfo.h"
#include "gimptooloptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TOOL,
  PROP_TOOL_INFO
};


static void   gimp_tool_options_config_iface_init (GimpConfigInterface *iface);

static void   gimp_tool_options_dispose           (GObject         *object);
static void   gimp_tool_options_set_property      (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_tool_options_get_property      (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);

static void   gimp_tool_options_real_reset        (GimpToolOptions *tool_options);

static void   gimp_tool_options_config_reset      (GimpConfig      *config);

static void   gimp_tool_options_tool_notify       (GimpToolOptions *options,
                                                   GParamSpec      *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpToolOptions, gimp_tool_options, GIMP_TYPE_CONTEXT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_tool_options_config_iface_init))

#define parent_class gimp_tool_options_parent_class


static void
gimp_tool_options_class_init (GimpToolOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_tool_options_dispose;
  object_class->set_property = gimp_tool_options_set_property;
  object_class->get_property = gimp_tool_options_get_property;

  klass->reset               = gimp_tool_options_real_reset;

  g_object_class_override_property (object_class, PROP_TOOL, "tool");

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

  g_signal_connect (options, "notify::tool",
                    G_CALLBACK (gimp_tool_options_tool_notify),
                    NULL);
}

static void
gimp_tool_options_config_iface_init (GimpConfigInterface *iface)
{
  iface->reset = gimp_tool_options_config_reset;
}

static void
gimp_tool_options_dispose (GObject *object)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (object);

  if (options->tool_info)
    {
      g_object_unref (options->tool_info);
      options->tool_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*  This is such a horrible hack, but neccessary because we
 *  a) load an option's tool-info from disk in many cases
 *  b) screwed up in the past and saved the wrong tool-info in some cases
 */
static GimpToolInfo *
gimp_tool_options_check_tool_info (GimpToolOptions *options,
                                   GimpToolInfo    *tool_info,
                                   gboolean         warn)
{
  if (tool_info && G_OBJECT_TYPE (options) == tool_info->tool_options_type)
    {
      return tool_info;
    }
  else
    {
      GList *list;

      for (list = gimp_get_tool_info_iter (GIMP_CONTEXT (options)->gimp);
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *new_info = list->data;

          if (G_OBJECT_TYPE (options) == new_info->tool_options_type)
            {
              if (warn)
                g_printerr ("%s: correcting bogus deserialized tool "
                            "type '%s' with right type '%s'\n",
                            g_type_name (G_OBJECT_TYPE (options)),
                            tool_info ? gimp_object_get_name (tool_info) : "NULL",
                            gimp_object_get_name (new_info));

              return new_info;
            }
        }

      g_return_val_if_reached (NULL);
    }
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
    case PROP_TOOL:
      {
        GimpToolInfo *tool_info = g_value_get_object (value);
        GimpToolInfo *context_tool;

        context_tool = gimp_context_get_tool (GIMP_CONTEXT (options));

        g_return_if_fail (context_tool == NULL ||
                          context_tool == tool_info);

        tool_info = gimp_tool_options_check_tool_info (options, tool_info, TRUE);

        if (! context_tool)
          gimp_context_set_tool (GIMP_CONTEXT (options), tool_info);
      }
      break;

    case PROP_TOOL_INFO:
      {
        GimpToolInfo *tool_info = g_value_get_object (value);

        g_return_if_fail (options->tool_info == NULL ||
                          options->tool_info == tool_info);

        tool_info = gimp_tool_options_check_tool_info (options, tool_info, TRUE);

        if (! options->tool_info)
          {
            options->tool_info = g_object_ref (tool_info);

            if (tool_info->context_props)
              gimp_context_define_properties (GIMP_CONTEXT (options),
                                              tool_info->context_props, FALSE);

            gimp_context_set_serialize_properties (GIMP_CONTEXT (options),
                                                   tool_info->context_props);
          }
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
    case PROP_TOOL:
      g_value_set_object (value, gimp_context_get_tool (GIMP_CONTEXT (options)));
      break;

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

static void
gimp_tool_options_config_reset (GimpConfig *config)
{
  gchar *name = g_strdup (gimp_object_get_name (config));

  gimp_config_reset_properties (G_OBJECT (config));

  gimp_object_take_name (GIMP_OBJECT (config), name);
}

static void
gimp_tool_options_tool_notify (GimpToolOptions *options,
                               GParamSpec      *pspec)
{
  GimpToolInfo *tool_info = gimp_context_get_tool (GIMP_CONTEXT (options));
  GimpToolInfo *new_info;

  new_info = gimp_tool_options_check_tool_info (options, tool_info, FALSE);

  if (tool_info && new_info != tool_info)
    g_warning ("%s: 'tool' property on %s was set to bogus value "
               "'%s', it MUST be '%s'.",
               G_STRFUNC,
               g_type_name (G_OBJECT_TYPE (options)),
               gimp_object_get_name (tool_info),
               gimp_object_get_name (new_info));
}


/*  public functions  */

void
gimp_tool_options_reset (GimpToolOptions *tool_options)
{
  g_return_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options));

  GIMP_TOOL_OPTIONS_GET_CLASS (tool_options)->reset (tool_options);
}

gboolean
gimp_tool_options_serialize (GimpToolOptions  *tool_options,
                             GError          **error)
{
  gchar    *filename;
  gchar    *header;
  gchar    *footer;
  gboolean  retval;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_tool_info_build_options_filename (tool_options->tool_info,
                                                    NULL);

  if (tool_options->tool_info->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  header = g_strdup_printf ("GIMP %s options",
                            gimp_object_get_name (tool_options->tool_info));
  footer = g_strdup_printf ("end of %s options",
                            gimp_object_get_name (tool_options->tool_info));

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
                               GError          **error)
{
  gchar    *filename;
  gboolean  retval;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_tool_info_build_options_filename (tool_options->tool_info,
                                                    NULL);

  if (tool_options->tool_info->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  retval = gimp_config_deserialize_file (GIMP_CONFIG (tool_options),
                                         filename,
                                         NULL,
                                         error);

  g_free (filename);

  return retval;
}

gboolean
gimp_tool_options_delete (GimpToolOptions  *tool_options,
                          GError          **error)
{
  gchar    *filename;
  gboolean  retval = TRUE;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_tool_info_build_options_filename (tool_options->tool_info,
                                                    NULL);

  if (g_unlink (filename) != 0 && errno != ENOENT)
    {
      retval = FALSE;
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
		   _("Deleting \"%s\" failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
    }

  g_free (filename);

  return retval;
}

void
gimp_tool_options_create_folder (void)
{
  gchar *filename = g_build_filename (gimp_directory (), "tool-options", NULL);

  g_mkdir (filename,
           S_IRUSR | S_IWUSR | S_IXUSR |
           S_IRGRP | S_IXGRP |
           S_IROTH | S_IXOTH);

  g_free (filename);
}
