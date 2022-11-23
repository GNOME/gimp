/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-data.c
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

#include <string.h>

#include <gio/gio.h>

#include "plug-in-types.h"

#include "ligmapluginmanager.h"
#include "ligmapluginmanager-data.h"


typedef struct _LigmaPlugInData LigmaPlugInData;

struct _LigmaPlugInData
{
  gchar  *identifier;
  gint32  bytes;
  guint8 *data;
};


/*  public functions  */

void
ligma_plug_in_manager_data_free (LigmaPlugInManager *manager)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));

  if (manager->data_list)
    {
      GList *list;

      for (list = manager->data_list;
           list;
           list = g_list_next (list))
        {
          LigmaPlugInData *data = list->data;

          g_free (data->identifier);
          g_free (data->data);
          g_slice_free (LigmaPlugInData, data);
        }

      g_list_free (manager->data_list);
      manager->data_list = NULL;
    }
}

void
ligma_plug_in_manager_set_data (LigmaPlugInManager *manager,
                               const gchar       *identifier,
                               gint32             bytes,
                               const guint8      *data)
{
  LigmaPlugInData *plug_in_data;
  GList          *list;

  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (bytes > 0);
  g_return_if_fail (data != NULL);

  for (list = manager->data_list; list; list = g_list_next (list))
    {
      plug_in_data = list->data;

      if (! strcmp (plug_in_data->identifier, identifier))
        break;
    }

  /* If there isn't already data with the specified identifier, create one */
  if (list == NULL)
    {
      plug_in_data = g_slice_new0 (LigmaPlugInData);
      plug_in_data->identifier = g_strdup (identifier);

      manager->data_list = g_list_prepend (manager->data_list, plug_in_data);
    }
  else
    {
      g_free (plug_in_data->data);
    }

  plug_in_data->bytes = bytes;
  plug_in_data->data  = g_memdup2 (data, bytes);
}

const guint8 *
ligma_plug_in_manager_get_data (LigmaPlugInManager *manager,
                               const gchar       *identifier,
                               gint32            *bytes)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (bytes != NULL, NULL);

  *bytes = 0;

  for (list = manager->data_list; list; list = g_list_next (list))
    {
      LigmaPlugInData *plug_in_data = list->data;

      if (! strcmp (plug_in_data->identifier, identifier))
        {
          *bytes = plug_in_data->bytes;
          return plug_in_data->data;
        }
    }

  return NULL;
}
