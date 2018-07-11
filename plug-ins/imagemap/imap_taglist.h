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

#ifndef _IMAP_TAGLIST_H
#define _IMAP_TAGLIST_H

#include <glib.h>

typedef struct {
   gchar *name;
   gchar *value;
} Tag_t;

typedef struct {
   GList *list;
} TagList_t;

TagList_t *taglist_create(void);
TagList_t *taglist_clone(TagList_t *src);
TagList_t *taglist_copy(TagList_t *src, TagList_t *des);
void taglist_destruct(TagList_t *tlist);
void taglist_set(TagList_t *tlist, const gchar *id, const gchar *value);
void taglist_write(TagList_t *tlist);

#endif /* _IMAP_TAGLIST_H */
