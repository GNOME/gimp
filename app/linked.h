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
#ifndef __LINKED_H__
#define __LINKED_H__

typedef struct _link
{
  void *data;
  struct _link *next;
} *link_ptr;

extern link_ptr alloc_list (void);
extern link_ptr free_list (link_ptr);
extern link_ptr add_to_list (link_ptr, void *);
extern link_ptr append_to_list (link_ptr, void *);
extern link_ptr insert_in_list (link_ptr, void *, int);
extern link_ptr remove_from_list (link_ptr, void *);
extern link_ptr next_item (link_ptr);
extern link_ptr nth_item (link_ptr, int);
extern int      list_length (link_ptr);

#endif
