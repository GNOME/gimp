/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfig object property dumper. 
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include "stdlib.h"
#include "string.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpbase/gimplimits.h"
#include "libgimpbase/gimpbasetypes.h"

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-types.h"
#include "gimprc.h"


static gint    dump_system_gimprc (void);
static gchar * dump_get_comment   (GParamSpec *param_spec);


int
main (int   argc,
      char *argv[])
{
  GObject *rc;
  
  g_type_init ();

  if (argc > 1)
    {
      if (strcmp (argv[1], "--system-gimprc") == 0)
	{
	  return dump_system_gimprc ();
	}
      else
	{
	  g_printerr ("%s -- GimpConfig dump utility\n\n", argv[0]);
	  g_printerr ("Options:\n"
		      "  --system-gimprc   create a commented system gimprc\n");

	  return EXIT_FAILURE;
	}
    }

  rc = g_object_new (GIMP_TYPE_RC, NULL);

  g_print ("# Dump of the GIMP default configuration\n\n");
  gimp_config_serialize_properties (rc, 1, 0);
  g_print ("\n");

  g_object_unref (rc);

  return EXIT_SUCCESS;
}


static gint
dump_system_gimprc (void)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  GObject       *rc;
  GString       *str;
  guint          n_property_specs;
  guint          i;

  str = g_string_new
    ("# This is the system-wide gimprc file.  Any change made in this file\n"
     "# will affect all users of this system, provided that they are not\n"
     "# overriding the default values in their personal gimprc file.\n"
     "#\n"
     "# Lines that start with a '#' are comments.\n"
     "# Blank lines are ignored.\n"
     "\n"
     "# The variable gimp_dir is set to either the interned value\n"
     "# @gimpdir@ or the environment variable GIMP_DIRECTORY.  If\n"
     "# the path in GIMP_DIRECTORY is relative, it is considered\n"
     "# relative to your home directory.\n"
     "\n"
     "(prefix \"@prefix@\"\n"
     "(exec_prefix \"@exec_prefix@\")\n"
     "(gimp_data_dir \"@gimpdatadir@\")\n"
     "(gimp_plugin_dir \"@gimpplugindir@\")\n"
     "\n");
  
  write (1, str->str, str->len);

  rc = g_object_new (GIMP_TYPE_RC, NULL);
  klass = G_OBJECT_GET_CLASS (rc);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      gchar      *comment;

      if (! (prop_spec->flags & GIMP_PARAM_SERIALIZE))
        continue;

      g_string_assign (str, "");

      comment = dump_get_comment (prop_spec);
      if (comment)
	{
	  gimp_config_serialize_comment (str, comment);
	  g_free (comment);
	  g_string_append (str, "#\n");
	}

      g_string_append (str, "# ");

      if (! gimp_config_serialize_property (rc, prop_spec, str, TRUE))
	{
	  g_string_append (str, "\n");

          write (1, str->str, str->len);
        }
    }

  g_free (property_specs);
  g_object_unref (rc);
  g_string_free (str, TRUE);

  return EXIT_SUCCESS;
}

static gchar *
dump_get_comment (GParamSpec *param_spec)
{
  GType        type;
  const gchar *blurb;
  const gchar *values = NULL;

  blurb = g_param_spec_get_blurb (param_spec);

  if (!blurb)
    {
      g_warning ("FIXME: Property '%s' has no blurb.", param_spec->name);
      blurb = param_spec->name;
    }

  type = param_spec->value_type;

  if (g_type_is_a (type, GIMP_TYPE_COLOR))
    {
      values = "The color is specified as a list of doubles (r g b a).";
    }
  else if (g_type_is_a (type, GIMP_TYPE_MEMSIZE))
    {
      values =
	"The integer size can contain a suffix of 'B', 'K', 'M' or 'G' which "
	"makes GIMP interpret the size as being specified in bytes, kilobytes, "
	"megabytes or gigabytes. If no suffix is specified the size defaults "
	"to being specified in kilobytes.";
    }
  else if (g_type_is_a (type, GIMP_TYPE_PATH))
    {
      values = "This is a colon-separated list of directories to search.";
    }
  else if (g_type_is_a (type, GIMP_TYPE_UNIT))
    {
      values =
	"The unit can be one inches, millimeters, points or picas plus "
	"those in your user units database.";
    }
  else
    {
      switch (G_TYPE_FUNDAMENTAL (type))
	{
	case G_TYPE_BOOLEAN:
	  values = "Possible values are yes and no.";
	  break;
	case G_TYPE_INT:
	case G_TYPE_UINT:
	case G_TYPE_LONG:
	case G_TYPE_ULONG:
	  values = "This is an integer value.";
	  break;
	case G_TYPE_FLOAT:
	case G_TYPE_DOUBLE:
	  values = "This is a float value.";
	  break;
	case G_TYPE_STRING:
	  values = "This is a string value.";
	  break;
	case G_TYPE_ENUM:
	  {
	    GEnumClass *enum_class;
	    GEnumValue *enum_value;
	    GString    *str;
	    gint        i;
	    
	    enum_class = g_type_class_peek (type);
	    
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
    g_warning ("FIXME: Can't tell anything about a %s.", g_type_name (type));

  return g_strdup_printf ("%s  %s", blurb, values);
}



/* some dummy funcs so we can properly link this beast */

const gchar *
gimp_unit_get_identifier (GimpUnit unit)
{
  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      return "pixels";
    case GIMP_UNIT_INCH:
      return "inches";
    case GIMP_UNIT_MM:
      return "millimeters";
    case GIMP_UNIT_POINT:
      return "points";
    case GIMP_UNIT_PICA:
      return "picas";
    default:
      return NULL;
    }
}

gint
gimp_unit_get_number_of_units (void)
{
  return GIMP_UNIT_END;
}
