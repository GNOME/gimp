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
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "linked.h"


static GMemChunk *list_mem_chunk = NULL;
static link_ptr free_list_list = NULL;


link_ptr
alloc_list ()
{
  link_ptr new_list;

  if (!list_mem_chunk)
    list_mem_chunk = g_mem_chunk_new ("gimp list mem chunk",
				      sizeof (struct _link),
				      1024, G_ALLOC_ONLY);

  if (free_list_list)
    {
      new_list = free_list_list;
      free_list_list = free_list_list->next;
    }
  else
    {
      new_list = g_mem_chunk_alloc (list_mem_chunk);
    }

  new_list->data = NULL;
  new_list->next = NULL;

  return new_list;
}

link_ptr
free_list (l)
     link_ptr l;
{
  link_ptr temp_list;

  if (!l)
    return NULL;

  temp_list = l;
  while (temp_list->next)
    temp_list = temp_list->next;
  temp_list->next = free_list_list;
  free_list_list = l;

  return NULL;
}

link_ptr
append_to_list (list, data)
     link_ptr list;
     void *data;
{
  link_ptr new_link;
  link_ptr head = list;

  new_link = alloc_list ();

  new_link->data = data;
  new_link->next = NULL;
  if (!list)
    return new_link;
  while (list->next)
    list = next_item (list);
  list->next = new_link;
  return head;
}

link_ptr
insert_in_list (list, data, index)
     link_ptr list;
     void *data;
     int index;
{
  link_ptr new_link;
  link_ptr head = list;
  link_ptr prev = NULL;
  int count = 0;

  new_link = alloc_list ();

  new_link->data = data;
  new_link->next = NULL;

  if (!list)
    return new_link;

  while (list && (count < index))
    {
      count++;
      prev = list;
      list = next_item (list);
    }

  new_link->next = list;
  if (prev)
    prev->next = new_link;
  else
    head = new_link;

  return head;
}

link_ptr
add_to_list (list, data)
     link_ptr list;
     void *data;
{
  link_ptr new_link;

  new_link = alloc_list ();

  new_link->data = data;
  new_link->next = list;
  return new_link;
}

link_ptr
remove_from_list (list, data)
     link_ptr list;
     void *data;
{
  link_ptr tmp;

  if (!list)
    return NULL;

  if (list->data == data)
    {
      tmp = list->next;
      list->next = NULL;
      free_list (list);
      return tmp;
    }
  else
    list->next = remove_from_list (list->next, data);

  return list;
}

link_ptr
next_item (list)
     link_ptr list;
{
  if (!list)
    return NULL;
  else
    return list->next;
}

link_ptr
nth_item (list, n)
     link_ptr list;
     int n;
{
  if (list && (n > 0))
    return nth_item (next_item (list), n - 1);

  return list;
}

int
list_length (list)
     link_ptr list;
{
  int n;

  n = 0;
  while (list)
    {
      n += 1;
      list = list->next;
    }

  return n;
}
