/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaRc serialization routines
 * Copyright (C) 2001-2005  Sven Neumann <sven@ligma.org>
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

#include <gio/gio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "config-types.h"

#include "ligmarc.h"
#include "ligmarc-serialize.h"
#include "ligmarc-unknown.h"


static gboolean ligma_rc_serialize_properties_diff (LigmaConfig       *config,
                                                   LigmaConfig       *compare,
                                                   LigmaConfigWriter *writer);
static gboolean ligma_rc_serialize_unknown_tokens  (LigmaConfig       *config,
                                                   LigmaConfigWriter *writer);


gboolean
ligma_rc_serialize (LigmaConfig       *config,
                   LigmaConfigWriter *writer,
                   gpointer          data)
{
  if (data && LIGMA_IS_RC (data))
    {
      if (! ligma_rc_serialize_properties_diff (config, data, writer))
        return FALSE;
    }
  else
    {
      if (! ligma_config_serialize_properties (config, writer))
        return FALSE;
    }

  return ligma_rc_serialize_unknown_tokens (config, writer);
}

static gboolean
ligma_rc_serialize_properties_diff (LigmaConfig       *config,
                                   LigmaConfig       *compare,
                                   LigmaConfigWriter *writer)
{
  GList    *diff;
  GList    *list;
  gboolean  retval = TRUE;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (compare), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (config) ==
                        G_TYPE_FROM_INSTANCE (compare), FALSE);

  diff = ligma_config_diff (G_OBJECT (config),
                           G_OBJECT (compare), LIGMA_CONFIG_PARAM_SERIALIZE);

  for (list = diff; list; list = g_list_next (list))
    {
      GParamSpec *prop_spec = list->data;

      if (! (prop_spec->flags & LIGMA_CONFIG_PARAM_SERIALIZE))
        continue;

      if (! ligma_config_serialize_property (config, prop_spec, writer))
        {
          retval = FALSE;
          break;
        }
    }

  g_list_free (diff);

  return retval;
}

static void
serialize_unknown_token (const gchar *key,
                         const gchar *value,
                         gpointer     data)
{
  LigmaConfigWriter *writer = data;

  ligma_config_writer_open (writer, key);
  ligma_config_writer_string (writer, value);
  ligma_config_writer_close (writer);
}

static gboolean
ligma_rc_serialize_unknown_tokens (LigmaConfig       *config,
                                  LigmaConfigWriter *writer)
{
  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  ligma_config_writer_linefeed (writer);
  ligma_rc_foreach_unknown_token (config, serialize_unknown_token, writer);

  return TRUE;
}
