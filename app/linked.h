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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This is compatibility stuff only... */
#ifndef __LINKED_H__
#define __LINKED_H__

#include <glib.h>

typedef GSList * link_ptr;

#define alloc_list() g_slist_alloc()
#define free_list(x) g_slist_free((x))
#define add_to_list(x, y) g_slist_prepend((x), (y))
#define append_to_list(x, y) g_slist_append((x), (y))
#define insert_in_list(x, y, z) g_slist_insert((x), (y), (z))
#define remove_from_list(x, y) g_slist_remove((x), (y))
#define next_item(x) (x)?g_slist_next((x)):NULL
#define nth_item(x, y) g_slist_nth((x), (y))
#define list_length(x) g_slist_length((x))

#endif
