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

#include "gimpconfig.h"
#include "gimpcoreconfig.h"


static void  notify_callback      (GObject     *object,
                                   GParamSpec  *pspec);
static void  output_unknown_token (const gchar *key,
                                   const gchar *value,
                                   gpointer     data);


int
main (int   argc,
      char *argv[])
{
  GimpBaseConfig *config;
  const gchar    *filename = "foorc";
  gboolean        header   = TRUE;

  g_type_init ();

  g_print ("Testing GimpConfig ...\n");

  config = g_object_new (GIMP_TYPE_CORE_CONFIG, NULL);


  g_print (" Serializing default properties of %s to '%s' ...\n", 
           g_type_name (G_TYPE_FROM_INSTANCE (config)), filename);

  gimp_config_serialize (G_OBJECT (config), filename);


  g_print (" Deserializing from '%s' ...\n", filename);

  g_signal_connect (G_OBJECT (config), "notify",
                    G_CALLBACK (notify_callback),
                    NULL);
  gimp_config_deserialize (G_OBJECT (config), filename, TRUE);
  
  gimp_config_foreach_unknown_token (G_OBJECT (config), 
                                     output_unknown_token, &header);


  g_object_unref (config);
  
  g_print ("Done.\n");

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

      g_value_unset (&src);
      g_value_unset (&dest);
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
  gboolean *header  = (gboolean *) data;
  gchar    *escaped = g_strescape (value, NULL);

  if (*header)
    {
      g_print ("  Unknown string tokens:\n");
      *header = FALSE;
    }

  g_print ("   %s \"%s\"\n", key, value);
  g_free (escaped);
}
