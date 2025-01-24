/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfig object property dumper.
 * Copyright (C) 2001-2006  Sven Neumann <sven@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#include <io.h> /* get_osfhandle */
#include <gio/gwin32outputstream.h>
#else
#include <gio/gunixoutputstream.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimpconfig-dump.h"
#include "gimprc.h"



static void    dump_gimprc_system   (GimpConfig       *rc,
                                     GimpConfigWriter *writer,
                                     GOutputStream    *output);
static void    dump_gimprc_manpage  (GimpConfig       *rc,
                                     GimpConfigWriter *writer,
                                     GOutputStream    *output);
static gchar * dump_describe_param  (GParamSpec       *param_spec);
static void    dump_with_linebreaks (GOutputStream    *output,
                                     const gchar      *text);


gboolean
gimp_config_dump (GObject              *gimp,
                  GimpConfigDumpFormat  format)
{
  GOutputStream    *output;
  GimpConfigWriter *writer;
  GimpConfig       *rc;

  g_return_val_if_fail (G_IS_OBJECT (gimp), FALSE);

  rc = g_object_new (GIMP_TYPE_RC,
                     "gimp", gimp,
                     NULL);

#ifdef G_OS_WIN32
  output = g_win32_output_stream_new ((gpointer) _get_osfhandle (fileno (stdout)), FALSE);
#else
  output = g_unix_output_stream_new (1, FALSE);
#endif

  writer = gimp_config_writer_new_from_stream (output, NULL, NULL);

  switch (format)
    {
    case GIMP_CONFIG_DUMP_NONE:
      break;

    case GIMP_CONFIG_DUMP_GIMPRC:
      gimp_config_writer_comment (writer,
                                  "Dump of the GIMP default configuration");
      gimp_config_writer_linefeed (writer);
      gimp_config_serialize_properties (rc, writer);
      gimp_config_writer_linefeed (writer);
      break;

    case GIMP_CONFIG_DUMP_GIMPRC_SYSTEM:
      dump_gimprc_system (rc, writer, output);
      break;

    case GIMP_CONFIG_DUMP_GIMPRC_MANPAGE:
      dump_gimprc_manpage (rc, writer, output);
      break;
    }

  gimp_config_writer_finish (writer, NULL, NULL);
  g_object_unref (output);
  g_object_unref (rc);

  return TRUE;
}


static const gchar system_gimprc_header[] =
"This is the system-wide gimprc file.  Any change made in this file "
"will affect all users of this system, provided that they are not "
"overriding the default values in their personal gimprc file.\n"
"\n"
"Lines that start with a '#' are comments. Blank lines are ignored.\n"
"\n"
"By default everything in this file is commented out.  The file then "
"documents the default values and shows what changes are possible.\n"
"\n"
"The variable ${gimp_dir} is set to the value of the environment "
"variable GIMP3_DIRECTORY or, if that is not set, the compiled-in "
"default value is used.  If GIMP3_DIRECTORY is not an absolute path, "
"it is interpreted relative to your home directory.";

static void
dump_gimprc_system (GimpConfig       *rc,
                    GimpConfigWriter *writer,
                    GOutputStream    *output)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  gimp_config_writer_comment (writer, system_gimprc_header);
  gimp_config_writer_linefeed (writer);

  klass = G_OBJECT_GET_CLASS (rc);
  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      gchar      *comment;

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
        continue;

      if (prop_spec->flags & GIMP_CONFIG_PARAM_IGNORE)
        continue;

      comment = dump_describe_param (prop_spec);
      if (comment)
        {
          gimp_config_writer_comment (writer, comment);
          g_free (comment);
        }

      gimp_config_writer_comment_mode (writer, TRUE);
      gimp_config_writer_linefeed (writer);

      if (! strcmp (prop_spec->name, "num-processors"))
        {
          gimp_config_writer_open (writer, "num-processors");
          gimp_config_writer_printf (writer, "1");
          gimp_config_writer_close (writer);
        }
      else if (! strcmp (prop_spec->name, "tile-cache-size"))
        {
          gimp_config_writer_open (writer, "tile-cache-size");
          gimp_config_writer_printf (writer, "2g");
          gimp_config_writer_close (writer);
        }
      else if (! strcmp (prop_spec->name, "undo-size"))
        {
          gimp_config_writer_open (writer, "undo-size");
          gimp_config_writer_printf (writer, "1g");
          gimp_config_writer_close (writer);
        }
      else if (! strcmp (prop_spec->name, "mypaint-brush-path"))
        {
          gchar *path = g_strdup_printf ("@mypaint_brushes_dir@%s"
                                         "~/.mypaint/brushes",
                                         G_SEARCHPATH_SEPARATOR_S);

          gimp_config_writer_open (writer, "mypaint-brush-path");
          gimp_config_writer_string (writer, path);
          gimp_config_writer_close (writer);

          g_free (path);
        }
      else
        {
          gimp_config_serialize_property (rc, prop_spec, writer);
        }

      gimp_config_writer_comment_mode (writer, FALSE);
      gimp_config_writer_linefeed (writer);
    }

  g_free (property_specs);
}


static const gchar man_page_header[] =
".\\\" This man-page is auto-generated by gimp --dump-gimprc-manpage.\n"
"\n"
".TH GIMPRC 5 \"Version " GIMP_VERSION "\" \"GIMP Manual Pages\"\n"
".SH NAME\n"
"gimprc \\- gimp configuration file\n"
".SH DESCRIPTION\n"
"The\n"
".B gimprc\n"
"file is a configuration file read by GIMP when it starts up.  There\n"
"are two of these: one system-wide one stored in\n"
"@gimpsysconfdir@/gimprc and a per-user @manpage_gimpdir@/gimprc\n"
"which may override system settings.\n"
"\n"
"Comments are introduced by a hash sign (#), and continue until the end\n"
"of the line.  Blank lines are ignored.\n"
"\n"
"The\n"
".B gimprc\n"
"file associates values with properties.  These properties may be set\n"
"by lisp-like assignments of the form:\n"
".IP\n"
"\\f3(\\f2property\\-name\\ value\\f3)\\f1\n"
".TP\n"
"where:\n"
".TP 10\n"
".I property\\-name\n"
"is one of the property names described below.\n"
".TP\n"
".I value\n"
"is the value the property is to be set to.\n"
".PP\n"
"\n"
"Either spaces or tabs may be used to separate the name from the value.\n"
".PP\n"
".SH PROPERTIES\n"
"Valid properties and their default values are:\n"
"\n";

static const gchar *man_page_path =
".PP\n"
".SH PATH EXPANSION\n"
"Strings of type PATH are expanded in a manner similar to\n"
".BR bash (1).\n"
"Specifically: tilde (~) is expanded to the user's home directory. Note that\n"
"the bash feature of being able to refer to other user's home directories\n"
"by writing ~userid/ is not valid in this file.\n"
"\n"
"${variable} is expanded to the current value of an environment variable.\n"
"There are a few variables that are pre-defined:\n"
".TP\n"
".I gimp_dir\n"
"The personal gimp directory which is set to the value of the environment\n"
"variable GIMP3_DIRECTORY or to @manpage_gimpdir@.\n"
".TP\n"
".I gimp_data_dir\n"
"Base for paths to shareable data, which is set to the value of the\n"
"environment variable GIMP3_DATADIR or to the compiled-in default value\n"
"@gimpdatadir@.\n"
".TP\n"
".I gimp_plug_in_dir\n"
"Base to paths for architecture-specific plug-ins and modules, which is set\n"
"to the value of the environment variable GIMP3_PLUGINDIR or to the\n"
"compiled-in default value @gimpplugindir@.\n"
".TP\n"
".I gimp_sysconf_dir\n"
"Path to configuration files, which is set to the value of the environment\n"
"variable GIMP3_SYSCONFDIR or to the compiled-in default value \n"
"@gimpsysconfdir@.\n"
".TP\n"
".I gimp_cache_dir\n"
"Path to cached files, which is set to the value of the environment\n"
"variable GIMP3_CACHEDIR or to the system default for per-user cached files.\n"
".TP\n"
".I gimp_temp_dir\n"
"Path to temporary files, which is set to the value of the environment\n"
"variable GIMP3_TEMPDIR or to the system default for temporary files.\n"
"\n";

static const gchar man_page_footer[] =
".SH FILES\n"
".TP\n"
".I @gimpsysconfdir@/gimprc\n"
"System-wide configuration file\n"
".TP\n"
".I @manpage_gimpdir@/gimprc\n"
"Per-user configuration file\n"
"\n"
".SH \"SEE ALSO\"\n"
".BR gimp (1)\n";


static void
dump_gimprc_manpage (GimpConfig       *rc,
                     GimpConfigWriter *writer,
                     GOutputStream    *output)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  g_output_stream_printf (output, NULL, NULL, NULL,
                          "%s", man_page_header);

  klass = G_OBJECT_GET_CLASS (rc);
  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      gchar      *desc;
      gboolean    success;

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
        continue;

      if (prop_spec->flags & GIMP_CONFIG_PARAM_IGNORE)
        continue;

      g_output_stream_printf (output, NULL, NULL, NULL,
                              ".TP\n");

      if (! strcmp (prop_spec->name, "num-processors"))
        {
          gimp_config_writer_open (writer, "num-processors");
          gimp_config_writer_printf (writer, "1");
          gimp_config_writer_close (writer);

          success = TRUE;
        }
      else if (! strcmp (prop_spec->name, "tile-cache-size"))
        {
          gimp_config_writer_open (writer, "tile-cache-size");
          gimp_config_writer_printf (writer, "2g");
          gimp_config_writer_close (writer);

          success = TRUE;
        }
      else if (! strcmp (prop_spec->name, "undo-size"))
        {
          gimp_config_writer_open (writer, "undo-size");
          gimp_config_writer_printf (writer, "1g");
          gimp_config_writer_close (writer);

          success = TRUE;
        }
      else if (! strcmp (prop_spec->name, "mypaint-brush-path"))
        {
          gchar *path = g_strdup_printf ("@mypaint_brushes_dir@%s"
                                         "~/.mypaint/brushes",
                                         G_SEARCHPATH_SEPARATOR_S);

          gimp_config_writer_open (writer, "mypaint-brush-path");
          gimp_config_writer_string (writer, path);
          gimp_config_writer_close (writer);

          g_free (path);

          success = TRUE;
        }
      else
        {
          success = gimp_config_serialize_property (rc, prop_spec, writer);
        }

      if (success)
        {
          g_output_stream_printf (output, NULL, NULL, NULL,
                                  "\n");

          desc = dump_describe_param (prop_spec);

          dump_with_linebreaks (output, desc);

          g_output_stream_printf (output, NULL, NULL, NULL,
                                  "\n");

          g_free (desc);
        }
    }

  g_free (property_specs);

  g_output_stream_printf (output, NULL, NULL, NULL,
                          "%s", man_page_path);
  g_output_stream_printf (output, NULL, NULL, NULL,
                          "%s", man_page_footer);
}


static const gchar display_format_description[] =
"This is a format string; certain % character sequences are recognised and "
"expanded as follows:\n"
"\n"
"%%  literal percent sign\n"
"%f  bare filename, or \"Untitled\"\n"
"%F  full path to file, or \"Untitled\"\n"
"%p  PDB image id\n"
"%i  view instance number\n"
"%t  image type (RGB, grayscale, indexed)\n"
"%z  zoom factor as a percentage\n"
"%s  source scale factor\n"
"%d  destination scale factor\n"
"%Dx expands to x if the image is dirty, the empty string otherwise\n"
"%Cx expands to x if the image is clean, the empty string otherwise\n"
"%B  expands to (modified) if the image is dirty, the empty string otherwise\n"
"%A  expands to (clean) if the image is clean, the empty string otherwise\n"
"%Nx expands to x if the image is export-dirty, the empty string otherwise\n"
"%Ex expands to x if the image is export-clean, the empty string otherwise\n"
"%l  the number of layers\n"
"%L  the number of layers (long form)\n"
"%m  memory used by the image\n"
"%n  the name of the active layer/channel\n"
"%P  the PDB id of the active layer/channel\n"
"%w  image width in pixels\n"
"%W  image width in real-world units\n"
"%h  image height in pixels\n"
"%H  image height in real-world units\n"
"%M  the image size expressed in megapixels\n"
"%u  unit symbol\n"
"%U  unit abbreviation\n"
"%x  the width of the active layer/channel in pixels\n"
"%X  the width of the active layer/channel in real-world units\n"
"%y  the height of the active layer/channel in pixels\n"
"%Y  the height of the active layer/channel in real-world units\n"
"%o  the name of the image's color profile\n\n";


static gchar *
dump_describe_param (GParamSpec *param_spec)
{
  const gchar *blurb  = g_param_spec_get_blurb (param_spec);
  const gchar *values = NULL;

  if (!blurb)
    {
      g_warning ("FIXME: Property '%s' has no blurb.", param_spec->name);

      blurb = g_strdup_printf ("The %s property has no description.",
                               param_spec->name);
    }

  if (GIMP_IS_PARAM_SPEC_COLOR (param_spec))
    {
      if (gimp_param_spec_color_has_alpha (param_spec))
        values = "The color is specified as opaque GeglColor (any Alpha channel is ignored).";
      else
        values = "The color is specified as GeglColor.";
    }
  else if (GIMP_IS_PARAM_SPEC_MEMSIZE (param_spec))
    {
      values =
        "The integer size can contain a suffix of 'B', 'K', 'M' or 'G' which "
        "makes GIMP interpret the size as being specified in bytes, kilobytes, "
        "megabytes or gigabytes. If no suffix is specified the size defaults "
        "to being specified in kilobytes.";
    }
  else if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      switch (gimp_param_spec_config_path_type (param_spec))
        {
        case GIMP_CONFIG_PATH_FILE:
          values = "This is a single filename.";
          break;

        case GIMP_CONFIG_PATH_FILE_LIST:
          switch (G_SEARCHPATH_SEPARATOR)
            {
            case ':':
              values = "This is a colon-separated list of files.";
              break;
            case ';':
              values = "This is a semicolon-separated list of files.";
              break;
            default:
              g_warning ("unhandled G_SEARCHPATH_SEPARATOR value");
              break;
            }
          break;

        case GIMP_CONFIG_PATH_DIR:
          values = "This is a single folder.";
          break;

        case GIMP_CONFIG_PATH_DIR_LIST:
          switch (G_SEARCHPATH_SEPARATOR)
            {
            case ':':
              values = "This is a colon-separated list of folders to search.";
              break;
            case ';':
              values = "This is a semicolon-separated list of folders to search.";
              break;
            default:
              g_warning ("unhandled G_SEARCHPATH_SEPARATOR value");
              break;
            }
          break;
        }
    }
  else if (GIMP_IS_PARAM_SPEC_UNIT (param_spec))
    {
      if (gimp_param_spec_unit_pixel_allowed (param_spec) && gimp_param_spec_unit_percent_allowed (param_spec))
        values = "The unit can be one inches, millimeters, points or picas plus "
                 "those in your user units database. Pixel And Percent units are allowed too.";
      else if (gimp_param_spec_unit_pixel_allowed (param_spec))
        values = "The unit can be one inches, millimeters, points or picas plus "
                 "those in your user units database. Pixel unit is allowed too.";
      else if (gimp_param_spec_unit_percent_allowed (param_spec))
        values = "The unit can be one inches, millimeters, points or picas plus "
                 "those in your user units database. Percent unit is allowed too.";
      else
        values = "The unit can be one inches, millimeters, points or picas plus "
                 "those in your user units database.";
    }
  else if (g_type_is_a (param_spec->value_type, GIMP_TYPE_CONFIG))
    {
      values = "This is a parameter list.";
    }
  else
    {
      switch (G_TYPE_FUNDAMENTAL (param_spec->value_type))
        {
        case G_TYPE_BOOLEAN:
          values = "Possible values are yes and no.";
          break;
        case G_TYPE_INT:
        case G_TYPE_UINT:
        case G_TYPE_LONG:
        case G_TYPE_ULONG:
        case G_TYPE_INT64:
          values = "This is an integer value.";
          break;
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE:
          values = "This is a float value.";
          break;
        case G_TYPE_STRING:
          /* eek */
          if (strcmp (g_param_spec_get_name (param_spec), "image-title-format")
              &&
              strcmp (g_param_spec_get_name (param_spec), "image-status-format"))
            {
              values = "This is a string value.";
            }
          else
            {
              values = display_format_description;
            }
          break;
        case G_TYPE_ENUM:
          {
            GEnumClass *enum_class;
            GEnumValue *enum_value;
            GString    *str;
            gint        i;

            enum_class = g_type_class_peek (param_spec->value_type);

            str = g_string_new (blurb);

            g_string_append (str, "  Possible values are ");

            for (i = 0, enum_value = enum_class->values;
                 i < enum_class->n_values;
                 i++, enum_value++)
              {
                g_string_append (str, enum_value->value_nick);

                switch (enum_class->n_values - i)
                  {
                  case 1:
                    g_string_append_c (str, '.');
                    break;
                  case 2:
                    g_string_append (str, " and ");
                    break;
                  default:
                    g_string_append (str, ", ");
                    break;
                  }
              }

            return g_string_free (str, FALSE);
          }
          break;
        default:
          break;
        }
    }

  if (!values)
    g_warning ("FIXME: Can't tell anything about a %s.",
               g_type_name (param_spec->value_type));

  if (strcmp (blurb, "") == 0)
    return g_strdup_printf ("%s", values);
  else
    return g_strdup_printf ("%s  %s", blurb, values);
}


#define LINE_LENGTH 78

static void
dump_with_linebreaks (GOutputStream *output,
                      const gchar   *text)
{
  gint len = strlen (text);

  while (len > 0)
    {
      const gchar *t;
      gint         i, space;

      /*  groff doesn't like lines to start with a single quote  */
      if (*text == '\'')
        g_output_stream_printf (output, NULL, NULL, NULL,
                                "\\&");  /*  a zero width space  */

      for (t = text, i = 0, space = 0;
           *t != '\n' && (i <= LINE_LENGTH || space == 0) && i < len;
           t++, i++)
        {
          if (g_ascii_isspace (*t))
            space = i;
        }

      if (i > LINE_LENGTH && space && *t != '\n')
        i = space;

      g_output_stream_write_all (output, text, i, NULL, NULL, NULL);
      g_output_stream_printf (output, NULL, NULL, NULL,
                              "\n");

      if (*t == '\n')
        g_output_stream_printf (output, NULL, NULL, NULL,
                                ".br\n");

      i++;

      text += i;
      len  -= i;
    }
}
