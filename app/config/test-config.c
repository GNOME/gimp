/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Test suite for GimpConfig.
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

#include <glib-object.h>

#include "libgimpbase/gimplimits.h"
#include "libgimpbase/gimpbasetypes.h"

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimprc.h"


static void  notify_callback      (GObject     *object,
                                   GParamSpec  *pspec);
static void  output_unknown_token (const gchar *key,
                                   const gchar *value,
                                   gpointer     data);


int
main (int   argc,
      char *argv[])
{
  GimpRc      *gimprc;
  GimpRc      *gimprc2;
  const gchar *filename = "foorc";
  gchar       *header;
  gchar       *result;
  gint         i;
  GError      *error = NULL;

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          GLogLevelFlags fatal_mask;
        
          fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
          fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
          g_log_set_always_fatal (fatal_mask);
        }
    }

  g_type_init ();

  g_print ("\nTesting GimpConfig ...\n\n");

  g_print (" Creating a new GimpRc object ...");
  gimprc = g_object_new (GIMP_TYPE_RC, NULL);
  g_print (" done.\n\n");
  
  g_print (" Adding the unknown token (foobar \"hadjaha\") ..."); 
  gimp_config_add_unknown_token (G_OBJECT (gimprc), "foobar", "hadjaha");
  g_print (" done.\n\n");

  g_print (" Serializing %s to '%s' ...", 
           g_type_name (G_TYPE_FROM_INSTANCE (gimprc)), filename);

  if (! gimp_config_serialize (G_OBJECT (gimprc),
                               filename,
                               "# foorc\n",
                               "# end of foorc\n",
                               NULL, &error))
    {
      g_print ("%s\n", error->message);
      return EXIT_FAILURE;
    }
  g_print (" done.\n\n");

  g_signal_connect (gimprc, "notify",
                    G_CALLBACK (notify_callback),
                    NULL);

  g_print (" Deserializing from '%s' ...\n\n", filename);
  if (! gimp_config_deserialize (G_OBJECT (gimprc), filename, NULL, &error))
    {
      g_print ("%s\n", error->message);
      return EXIT_FAILURE;
    }
  header = "\n  Unknown string tokens:\n";
  gimp_config_foreach_unknown_token (G_OBJECT (gimprc), 
                                     output_unknown_token, &header);
  g_print ("\n done.\n");

  g_print ("\n Changing a property ...");
  g_object_set (gimprc, "use-help", FALSE, NULL);

  g_print ("\n Testing gimp_config_duplicate() ...");
  gimprc2 = GIMP_RC (gimp_config_duplicate (G_OBJECT (gimprc)));
  g_print (" done.\n");

  g_signal_connect (gimprc2, "notify",
                    G_CALLBACK (notify_callback),
                    NULL);

  g_print ("\n Changing a property in the duplicate ...");
  g_object_set (gimprc2, "show-tips", FALSE, NULL);

  g_print ("\n Querying for \"default-comment\" ... ");
  
  result = gimp_rc_query (gimprc2, "default-comment");
  if (result)
    {
      g_print ("OK, found \"%s\".\n", result);
    }
  else
    {
      g_print ("failed!\n");
      return EXIT_FAILURE;
    }
  g_free (result);

  g_print (" Querying for \"foobar\" ... ");
  
  result = gimp_rc_query (gimprc2, "foobar");
  if (result && strcmp (result, "hadjaha") == 0)
    {
      g_print ("OK, found \"%s\".\n", result);
    }
  else
    {
      g_print ("failed!\n");
      return EXIT_FAILURE;
    }

  g_free (result);

  g_object_unref (gimprc2);

  g_print ("\n Deserializing from gimpconfig.c (should fail) ...");
  if (! gimp_config_deserialize (G_OBJECT (gimprc),
                                 "gimpconfig.c", NULL, &error))
    {
      g_print (" OK, failed. The error was:\n %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("This test should have failed :-(\n");
      return EXIT_FAILURE;
    }

  g_object_unref (gimprc);
  
  g_print ("\nFinished test of GimpConfig.\n\n");

  return EXIT_SUCCESS;
}

static void
notify_callback (GObject    *object,
                 GParamSpec *pspec)
{
  GString *str;
  GValue   value = { 0, };

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  g_value_init (&value, pspec->value_type);
  g_object_get_property (object, pspec->name, &value);

  str = g_string_new (NULL);

  if (gimp_config_serialize_value (&value, str, TRUE))
    {
      g_print ("  %s -> %s\n", pspec->name, str->str);
    }
  else
    {
      g_print ("  %s changed but we failed to serialize its value!\n", 
               pspec->name);
    }

  g_string_free (str, TRUE);
  g_value_unset (&value);
}

static void
output_unknown_token (const gchar *key,
                      const gchar *value,
                      gpointer     data)
{
  gchar **header  = (gchar **) data;
  gchar  *escaped = g_strescape (value, NULL);

  if (*header)
    {
      g_print (*header);
      *header = NULL;
    }

  g_print ("   %s \"%s\"\n", key, value);
  g_free (escaped);
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
