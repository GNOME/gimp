/* docindex.h - Header file for the document index.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __DOCINDEX_H__
#define __DOCINDEX_H__

#include <stdio.h>

#include <gtk/gtk.h>

void    document_index_create     (void);
void    document_index_free       (void);

void    document_index_add        (gchar *label);

FILE  * document_index_parse_init (void);
gchar * document_index_parse_line (FILE  *fp);

#endif /* __DOCINDEX_H__ */
