/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Test suite for GimpConfig.
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "libgimpbase/gimplimits.h"
#include "libgimpbase/gimpbasetypes.h"

#include "base/base-enums.h"
#include "core/core-enums.h"

#include "gimpconfig.h"
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
  gint         i;

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

  g_print ("Testing GimpConfig ...\n\n");

  gimprc = gimp_rc_new ();

  g_print (" Serializing %s to '%s' ... ", 
           g_type_name (G_TYPE_FROM_INSTANCE (gimprc)), filename);
  gimp_config_serialize (G_OBJECT (gimprc), filename);
  g_print ("done.\n\n");

  g_signal_connect (G_OBJECT (gimprc), "notify",
                    G_CALLBACK (notify_callback),
                    NULL);

  g_print (" Deserializing from '%s' ...\n", filename);
  gimp_config_deserialize (G_OBJECT (gimprc), filename);

  header = "\n  Unknown string tokens:\n";
  gimp_config_foreach_unknown_token (G_OBJECT (gimprc), 
                                     output_unknown_token, &header);

  g_print ("\n\nChanging a property ... ");
  g_object_set (G_OBJECT (gimprc), "use-help", FALSE, NULL);

  g_print ("\nTesting gimp_rc_duplicate() ... ");
  gimprc2 = gimp_rc_duplicate (gimprc);
  g_print ("done.\n");

  g_print ("\nTesting gimp_rc_write_changes() ... \n\n");

  g_object_set (G_OBJECT (gimprc2), "show-tips", FALSE, NULL);
  gimp_rc_write_changes (gimprc2, gimprc, NULL);

  g_print ("\n");

  g_object_unref (G_OBJECT (gimprc));
  g_object_unref (G_OBJECT (gimprc2));
  
  g_print ("Done.\n\n");

  return 0;
}


void
notify_callback (GObject    *object,
                 GParamSpec *pspec)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (g_value_type_transformable (pspec->value_type, G_TYPE_STRING))
    {
      GValue  src  = { 0, };
      GValue  dest = { 0, };

      g_value_init (&src,  pspec->value_type);
      g_object_get_property (object, pspec->name, &src);

      g_value_init (&dest, G_TYPE_STRING);      
      g_value_transform (&src, &dest);

      g_print ("  %s -> %s\n", pspec->name, g_value_get_string (&dest));

      g_value_unset (&dest);
      g_value_unset (&src);
    }
  else
    {
      g_print ("  %s changed\n", pspec->name);
    }
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
