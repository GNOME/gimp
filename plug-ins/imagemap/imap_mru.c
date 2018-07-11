/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "imap_mru.h"

MRU_t*
mru_create(void)
{
   MRU_t *mru = g_new(MRU_t, 1);
   mru->list = NULL;
   mru->max_size = DEFAULT_MRU_SIZE;
   return mru;
}

void
mru_destruct (MRU_t *mru)
{
  g_list_free_full (mru->list, (GDestroyNotify) g_free);
  g_free (mru);
}

static void
mru_remove_link(MRU_t *mru, GList *link)
{
  if (link)
    {
      g_free(link->data);
      mru->list = g_list_remove_link(mru->list, link);
    }
}

static GList*
mru_find_link(MRU_t *mru, const gchar *filename)
{
   return g_list_find_custom(mru->list, (gpointer) filename,
                             (GCompareFunc) strcmp);
}

void
mru_add(MRU_t *mru, const gchar *filename)
{
   if (g_list_length(mru->list) == mru->max_size)
      mru_remove_link(mru, g_list_last(mru->list));
   mru->list = g_list_prepend(mru->list, g_strdup(filename));
}

void
mru_remove(MRU_t *mru, const gchar *filename)
{
   mru_remove_link(mru, mru_find_link(mru, filename));
}

void
mru_set_first(MRU_t *mru, const gchar *filename)
{
   GList *link = mru_find_link(mru, filename);
   if (link)
      mru->list = g_list_prepend(g_list_remove_link(mru->list, link),
                                 link->data);
   else
      mru_add(mru, filename);
}

void
mru_set_size(MRU_t *mru, gint size)
{
   gint diff;

   for (diff = g_list_length(mru->list) - size; diff > 0; diff--)
      mru_remove_link(mru, g_list_last(mru->list));
   mru->max_size = size;
}

void
mru_write(MRU_t *mru, FILE *out)
{
   GList *p;
   for (p = mru->list; p; p = p->next)
      fprintf(out, "(mru-entry %s)\n", (gchar*) p->data);
}
