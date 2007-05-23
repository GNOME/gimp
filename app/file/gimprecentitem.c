/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Recent File Storage,
 * see http://freedesktop.org/Standards/recent-file-spec/
 *
 * This code is taken from libegg and has been adapted to the GIMP needs.
 * The original author is James Willcox <jwillcox@cs.indiana.edu>,
 * responsible for bugs in this version is Sven Neumann <sven@gimp.org>.
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
#include <time.h>

#include <glib-object.h>

#include "gimprecentitem.h"


struct _GimpRecentItem
{
  gchar    *uri;
  gchar    *mime_type;
  time_t    timestamp;

  gboolean  private_data;
  GList    *groups;

  gint      refcount;
};


GimpRecentItem *
gimp_recent_item_new (void)
{
  GimpRecentItem *item;

  item = g_slice_new (GimpRecentItem);

  item->groups       = NULL;
  item->private_data = FALSE;
  item->uri          = NULL;
  item->mime_type    = NULL;

  item->refcount = 1;

  return item;
}

static void
gimp_recent_item_free (GimpRecentItem *item)
{
  if (item->uri)
    g_free (item->uri);

  if (item->mime_type)
    g_free (item->mime_type);

  if (item->groups)
    {
      g_list_foreach (item->groups, (GFunc) g_free, NULL);
      g_list_free (item->groups);
      item->groups = NULL;
    }

  g_slice_free (GimpRecentItem, item);
}

GimpRecentItem *
gimp_recent_item_ref (GimpRecentItem *item)
{
  item->refcount++;

  return item;
}

GimpRecentItem *
gimp_recent_item_unref (GimpRecentItem *item)
{
  item->refcount--;

  if (item->refcount == 0)
    gimp_recent_item_free (item);

  return item;
}

GimpRecentItem *
gimp_recent_item_new_from_uri (const gchar *uri)
{
  GimpRecentItem *item;

  g_return_val_if_fail (uri != NULL, NULL);

  item = gimp_recent_item_new ();

  if (!gimp_recent_item_set_uri (item ,uri))
    {
      gimp_recent_item_free (item);
      return NULL;
    }

  return item;
}

gboolean
gimp_recent_item_set_uri (GimpRecentItem *item,
                          const gchar    *uri)
{
  gchar *utf8_uri = g_filename_to_utf8 (uri, -1, NULL, NULL, NULL);

  if (! utf8_uri)
    {
      g_warning ("%s: URI can't be converted to UTF-8", G_STRFUNC);
      return FALSE;
    }

  g_free (utf8_uri);

  if (item->uri)
    g_free (item->uri);

  item->uri = g_strdup (uri);

  return TRUE;
}

const gchar *
gimp_recent_item_get_uri (const GimpRecentItem *item)
{
  return item->uri;
}

gchar *
gimp_recent_item_get_uri_utf8 (const GimpRecentItem *item)
{
  /* this could fail, but it's not likely, since we've already done it
   * once in set_uri()
   */
  return g_filename_to_utf8 (item->uri, -1, NULL, NULL, NULL);
}

void
gimp_recent_item_set_mime_type (GimpRecentItem *item,
                                const gchar    *mime)
{
  if (item->mime_type)
    g_free (item->mime_type);

  item->mime_type = g_strdup (mime);
}

const gchar *
gimp_recent_item_get_mime_type (const GimpRecentItem *item)
{
  return item->mime_type;
}

void
gimp_recent_item_set_timestamp (GimpRecentItem *item,
                                time_t          timestamp)
{
  if (timestamp == (time_t) -1)
    time (&timestamp);

  item->timestamp = timestamp;
}

time_t
gimp_recent_item_get_timestamp (const GimpRecentItem *item)
{
  return item->timestamp;
}

const GList *
gimp_recent_item_get_groups (const GimpRecentItem *item)
{
  return item->groups;
}

gboolean
gimp_recent_item_in_group (const GimpRecentItem *item,
                           const gchar          *group_name)
{
  GList *tmp = item->groups;

  for (tmp = item->groups; tmp; tmp = tmp->next)
    {
      const gchar *value = tmp->data;

      if (strcmp (group_name, value) == 0)
        return TRUE;
    }

  return FALSE;
}

void
gimp_recent_item_add_group (GimpRecentItem *item,
                            const gchar    *group_name)
{
  g_return_if_fail (group_name != NULL);

  if (!gimp_recent_item_in_group (item, group_name))
    item->groups = g_list_append (item->groups, g_strdup (group_name));
}

void
gimp_recent_item_remove_group (GimpRecentItem *item,
                               const gchar    *group_name)
{
  GList *tmp;

  g_return_if_fail (group_name != NULL);

  for (tmp = item->groups; tmp; tmp = tmp->next)
    {
      gchar *value = tmp->data;

      if (strcmp (group_name, value) == 0)
        {
          item->groups = g_list_remove (item->groups, value);
          g_free (value);
          break;
        }
    }
}

void
gimp_recent_item_set_private (GimpRecentItem *item,
                              gboolean        priv)
{
  item->private_data = priv;
}

gboolean
gimp_recent_item_get_private (const GimpRecentItem *item)
{
  return item->private_data;
}
