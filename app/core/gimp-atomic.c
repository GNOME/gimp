/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-atomic.c
 * Copyright (C) 2018 Ell
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
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp-atomic.h"


/*  GSList  */


static GSList gimp_atomic_slist_sentinel;


void
gimp_atomic_slist_push_head (GSList   **list,
                             gpointer   data)
{
  GSList *old_head;
  GSList *new_head;

  g_return_if_fail (list != NULL);

  new_head = g_slist_alloc ();

  new_head->data = data;

  do
    {
      do
        {
          old_head = g_atomic_pointer_get (list);
        }
      while (old_head == &gimp_atomic_slist_sentinel);

      new_head->next = old_head;
    }
  while (! g_atomic_pointer_compare_and_exchange (list, old_head, new_head));
}

gpointer
gimp_atomic_slist_pop_head (GSList **list)
{
  GSList   *old_head;
  GSList   *new_head;
  gpointer  data;

  g_return_val_if_fail (list != NULL, NULL);

  do
    {
      do
        {
          old_head = g_atomic_pointer_get (list);
        }
      while (old_head == &gimp_atomic_slist_sentinel);

      if (! old_head)
        return NULL;
    }
  while (! g_atomic_pointer_compare_and_exchange (list,
                                                  old_head,
                                                  &gimp_atomic_slist_sentinel));

  new_head = old_head->next;
  data     = old_head->data;

  g_atomic_pointer_set (list, new_head);

  g_slist_free1 (old_head);

  return data;
}
