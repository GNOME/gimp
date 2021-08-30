/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayerOptions-parasite
 * Copyright (C) 2003  Hendrik Boom
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

#include <string.h>
#include <stdlib.h>

#include <glib-object.h>
#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "vectors-types.h"

#include "gimpvectorlayeroptions.h"
#include "gimpvectorlayeroptions-parasite.h"

#include "gimp-intl.h"


const gchar *
gimp_vector_layer_options_parasite_name (void)
{
  return "gimp-vector-layer-options";
}

GimpParasite *
gimp_vector_layer_options_to_parasite (const GimpVectorLayerOptions *options)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER_OPTIONS (options), NULL);

  str = gimp_config_serialize_to_string (GIMP_CONFIG (options), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite = gimp_parasite_new (gimp_vector_layer_options_parasite_name (),
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GimpVectorLayerOptions *
gimp_vector_layer_options_from_parasite (const GimpParasite  *parasite,
                                         GError             **error,
                                         Gimp                *gimp)
{
  GimpVectorLayerOptions *options;
  const gchar            *str;
  guint32                 parasite_length;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_get_name (parasite),
                                gimp_vector_layer_options_parasite_name ()) == 0,
                        NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  str = gimp_parasite_get_data (parasite, &parasite_length);
  g_return_val_if_fail (str != NULL, NULL);

  options = g_object_new (GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                          "gimp", gimp,
                          NULL);

  gimp_config_deserialize_string (GIMP_CONFIG (options),
                                  str,
                                  parasite_length,
                                  NULL,
                                  error);

  return options;
}
