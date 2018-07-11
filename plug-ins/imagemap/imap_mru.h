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

#ifndef _IMAP_MRU_H
#define _IMAP_MRU_H

#include <stdio.h>

#define DEFAULT_MRU_SIZE 4

typedef struct {
   GList       *list;
   gint         max_size;
} MRU_t;

MRU_t* mru_create(void);
void mru_destruct(MRU_t *mru);
void mru_add(MRU_t *mru, const gchar *filename);
void mru_remove(MRU_t *mru, const gchar *filename);
void mru_set_first(MRU_t *mru, const gchar *filename);
void mru_set_size(MRU_t *mru, gint size);
void mru_write(MRU_t *mru, FILE *out);

#define mru_empty(mru) ((mru)->list == NULL)

#endif /* _IMAP_MRU_H */
