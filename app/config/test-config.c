/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Test suite for GimpConfig.
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include "stdlib.h"
#include "string.h"

#include <gegl.h>
#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"
#include "core/gimpgrid.h"

#include "gimprc-unknown.h"


static void  notify_callback      (GObject     *object,
                                   GParamSpec  *pspec);
static void  output_unknown_token (const gchar *key,
                                   const gchar *value,
                                   gpointer     data);

static void  units_init           (void);


int
main (int   argc,
      char *argv[])
{
  GimpConfig  *grid;
  GimpConfig  *grid2;
  GFile       *file = g_file_new_for_path ("foorc");
  gchar       *header;
  gchar       *result;
  GList       *list;
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

  units_init ();
  gegl_init (&argc, &argv);

  g_print ("\nTesting GimpConfig ...\n");

  g_print (" Creating a new Grid object ...");
  grid = g_object_new (GIMP_TYPE_GRID, NULL);
  g_print (" done.\n");

  g_print (" Adding the unknown token (foobar \"hadjaha\") ...");
  gimp_rc_add_unknown_token (grid, "foobar", "hadjaha");
  g_print (" done.\n");

  g_print (" Serializing %s to '%s' ...",
           g_type_name (G_TYPE_FROM_INSTANCE (grid)),
           gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_file (grid,
                                       file,
                                       "foorc", "end of foorc",
                                       NULL, &error))
    {
      g_print ("%s\n", error->message);
      return EXIT_FAILURE;
    }
  g_print (" done.\n");

  g_signal_connect (grid, "notify",
                    G_CALLBACK (notify_callback),
                    NULL);

  g_print (" Deserializing from '%s' ...\n",
           gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_file (grid, file, NULL, &error))
    {
      g_print ("%s\n", error->message);
      return EXIT_FAILURE;
    }
  header = " Unknown string tokens:\n";
  gimp_rc_foreach_unknown_token (grid, output_unknown_token, &header);
  g_print (" done.\n\n");

  g_print (" Changing a property ...");
  g_object_set (grid, "style", GIMP_GRID_DOTS, NULL);

  g_print (" Testing gimp_config_duplicate() ...");
  grid2 = gimp_config_duplicate (grid);
  g_print (" done.\n");

  g_signal_connect (grid2, "notify",
                    G_CALLBACK (notify_callback),
                    NULL);

  g_print (" Changing a property in the duplicate ...");
  g_object_set (grid2, "xspacing", 20.0, NULL);

  g_print (" Creating a diff between the two ...");
  for (list = gimp_config_diff (G_OBJECT (grid), G_OBJECT (grid2), 0);
       list;
       list = list->next)
    {
      GParamSpec *pspec = list->data;

      g_print ("%c%s", (list->prev ? ',' : ' '), pspec->name);
    }
  g_print ("\n\n");

  g_object_unref (grid2);

  g_print (" Deserializing from gimpconfig.c (should fail) ...");

  if (! gimp_config_deserialize_file (grid,
                                      g_file_new_for_path ("gimpconfig.c"),
                                      NULL, &error))
    {
      g_print (" OK, failed. The error was:\n %s\n", error->message);
      g_error_free (error);
      error = NULL;
    }
  else
    {
      g_print ("This test should have failed :-(\n");
      return EXIT_FAILURE;
    }

  g_print (" Serializing to a string and back ... ");

  result = gimp_config_serialize_to_string (grid, NULL);

  grid2 = g_object_new (GIMP_TYPE_GRID, NULL);

  if (! gimp_config_deserialize_string (grid2, result, -1, NULL, &error))
    {
      g_print ("failed!\nThe error was:\n %s\n", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }
  else
    {
      GList *diff = gimp_config_diff (G_OBJECT (grid), G_OBJECT (grid2), 0);

      if (diff)
        {
          GList *list;

          g_print ("succeeded but properties differ:\n");
          for (list = diff; list; list = list->next)
            {
              GParamSpec *pspec = list->data;
              g_print ("   %s\n", pspec->name);
            }
          return EXIT_FAILURE;
        }

      g_print ("OK (%u bytes)\n", result ? (guint) strlen (result) : 0);
    }

  g_free (result);
  g_object_unref (grid2);
  g_object_unref (grid);

  g_print ("\nFinished test of GimpConfig.\n\n");

  return EXIT_SUCCESS;
}

static void
notify_callback (GObject    *object,
                 GParamSpec *pspec)
{
  GString *str;
  GValue   value = G_VALUE_INIT;

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
      g_print ("%s", *header);
      *header = NULL;
    }

  g_print ("   %s \"%s\"\n", key, escaped);

  g_free (escaped);
}


static void
units_init (void)
{
  /* Empty dummy units implementation  */
  GimpUnitVtable vtable = { 0 };

  gimp_base_init (&vtable);
}
