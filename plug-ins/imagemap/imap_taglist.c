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

#include <stdio.h>

#include <glib.h>

#include "imap_string.h"
#include "imap_taglist.h"

static Tag_t*
tag_create(const gchar *name, const gchar *value)
{
   Tag_t *tag = g_new(Tag_t, 1);
   tag->name = g_strdup(name);
   tag->value = g_strdup(value);
   return tag;
}

static Tag_t*
tag_clone(Tag_t *src)
{
   return tag_create(src->name, src->value);
}

static void
tag_destruct(Tag_t *tag)
{
   g_free(tag->name);
   g_free(tag->value);
   g_free(tag);
}

static void
tag_write(Tag_t *tag)
{
   printf("\"%s\"=\"%s\"\n", tag->name, tag->value);
}

TagList_t*
taglist_create(void)
{
   TagList_t *tlist = g_new(TagList_t, 1);
   tlist->list = NULL;
   return tlist;
}

TagList_t*
taglist_clone(TagList_t *src)
{
   return taglist_copy(src, taglist_create());
}

static void
taglist_remove_all(TagList_t *tlist)
{
   GList *p;
   for (p = tlist->list; p; p = p->next)
      tag_destruct((Tag_t*) p->data);
   g_list_free(tlist->list);
   tlist->list = NULL;
}

TagList_t*
taglist_copy(TagList_t *src, TagList_t *des)
{
   GList *p;

   taglist_remove_all(des);
   for (p = src->list; p; p = p->next)
      des->list = g_list_append(des->list, tag_clone((Tag_t*) p->data));
   return des;
}

void
taglist_destruct(TagList_t *tlist)
{
   taglist_remove_all(tlist);
   g_free(tlist);
}

void
taglist_set(TagList_t *tlist, const gchar *name, const gchar *value)
{
   GList *p;
   Tag_t *tag;

   for (p = tlist->list; p; p = p->next) {
      tag = (Tag_t*) p->data;
      if (!g_ascii_strcasecmp(tag->name, name)) {
         g_strreplace(&tag->value, value);
         return;
      }
   }
   /* Tag not found, add a new tag */
   tlist->list = g_list_append(tlist->list, tag_create(name, value));
}

void
taglist_write(TagList_t *tlist)
{
   GList *p;
   for (p = tlist->list; p; p = p->next)
      tag_write((Tag_t*) p->data);
}
