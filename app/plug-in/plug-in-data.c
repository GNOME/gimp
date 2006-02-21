/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include "plug-in-types.h"

#include "core/gimp.h"

#include "plug-in-data.h"


typedef struct _PlugInData PlugInData;

struct _PlugInData
{
  gchar  *identifier;
  gint32  bytes;
  guint8 *data;
};


/*  public functions  */

void
plug_in_data_free (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->plug_in_data_list)
    {
      GList *list;

      for (list = gimp->plug_in_data_list;
           list;
           list = g_list_next (list))
        {
          PlugInData *data = list->data;

          g_free (data->identifier);
          g_free (data->data);
          g_free (data);
        }

      g_list_free (gimp->plug_in_data_list);
      gimp->plug_in_data_list = NULL;
    }
}

void
plug_in_data_set (Gimp         *gimp,
                  const gchar  *identifier,
                  gint32        bytes,
                  const guint8 *data)
{
  GList      *list;
  PlugInData *plug_in_data;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (bytes > 0);
  g_return_if_fail (data != NULL);

  for (list = gimp->plug_in_data_list; list; list = g_list_next (list))
    {
      plug_in_data = list->data;

      if (! strcmp (plug_in_data->identifier, identifier))
        break;
    }

  /* If there isn't already data with the specified identifier, create one */
  if (list == NULL)
    {
      plug_in_data = g_new0 (PlugInData, 1);
      plug_in_data->identifier = g_strdup (identifier);

      gimp->plug_in_data_list = g_list_prepend (gimp->plug_in_data_list,
                                                plug_in_data);
    }
  else
    {
      g_free (plug_in_data->data);
    }

  plug_in_data->bytes = bytes;
  plug_in_data->data  = g_memdup (data, bytes);
}

const guint8 *
plug_in_data_get (Gimp        *gimp,
                  const gchar *identifier,
                  gint32      *bytes)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (bytes != NULL, NULL);

  *bytes = 0;

  for (list = gimp->plug_in_data_list; list; list = g_list_next (list))
    {
      PlugInData *plug_in_data = list->data;

      if (! strcmp (plug_in_data->identifier, identifier))
        {
          *bytes = plug_in_data->bytes;
          return plug_in_data->data;
        }
    }

  return NULL;
}
