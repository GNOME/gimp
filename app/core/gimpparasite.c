/* gimpparasite.c: Copyright 1998 Jay Cox <jaycox@earthlink.net> 
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

#include <glib.h>
#include "parasitelist.h"
#include "gimpparasite.h"

static ParasiteList *parasites = NULL;

void 
gimp_init_parasites()
{
  g_return_if_fail(parasites == NULL);
  parasites = parasite_list_new();
}

void
gimp_attach_parasite (Parasite *p)
{
  parasite_list_add(parasites, p);
}

void
gimp_detach_parasite (char *name)
{
  parasite_list_remove(parasites, name);
}

Parasite *
gimp_find_parasite (char *name)
{
  return parasite_list_find(parasites, name);
}

static void list_func(char *key, Parasite *p, char ***cur)
{
  *(*cur)++ = (char *) g_strdup (key);
}

char **
gimp_parasite_list (gint *count)
{
  char **list, **cur;

  *count = parasite_list_length (parasites);
  cur = list = (char **) g_malloc (sizeof (char *) * *count);

  parasite_list_foreach (parasites, (GHFunc)list_func, &cur);
  
  return list;
}



