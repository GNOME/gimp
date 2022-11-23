/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2005 Peter Mattis and Spencer Kimball
 *
 * ligmaligmarc.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ligma.h"


/**
 * ligma_get_color_configuration:
 *
 * Retrieve a copy of the current color management configuration.
 *
 * Returns: (transfer full): A copy of the core's #LigmaColorConfig. You
 *          should unref this copy if you don't need it any longer.
 *
 * Since: 2.4
 */
LigmaColorConfig *
ligma_get_color_configuration (void)
{
  LigmaColorConfig *config;
  gchar           *text;
  GError          *error = NULL;

  text = _ligma_get_color_configuration ();

  g_return_val_if_fail (text != NULL, NULL);

  config = g_object_new (LIGMA_TYPE_COLOR_CONFIG, NULL);

  if (! ligma_config_deserialize_string (LIGMA_CONFIG (config), text, -1, NULL,
                                        &error))
    {
      g_warning ("failed to deserialize color configuration: %s",
                 error->message);
      g_error_free (error);
    }

  g_free (text);

  return config;
}
