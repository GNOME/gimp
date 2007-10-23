/* GIMP - The GNU Image Manipulation Program
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "gimparea.h"


#define OVERHEAD 25  /*  in units of pixel area  */


GimpArea *
gimp_area_new (gint x1,
               gint y1,
               gint x2,
               gint y2)
{
  GimpArea *area = g_slice_new (GimpArea);

  area->x1 = x1;
  area->y1 = y1;
  area->x2 = x2;
  area->y2 = y2;

  return area;
}

void
gimp_area_free (GimpArea *area)
{
  g_slice_free (GimpArea, area);
}


/*
 *  As far as I can tell, this function takes a GimpArea and unifies it with
 *  an existing list of GimpAreas, trying to avoid overdraw.  [adam]
 */
GSList *
gimp_area_list_process (GSList   *list,
                        GimpArea *area)
{
  GSList *new_list;
  GSList *l;
  gint    area1, area2, area3;

  /*  start new list off  */
  new_list = g_slist_prepend (NULL, area);

  for (l = list; l; l = g_slist_next (l))
    {
      GimpArea *ga2 = l->data;

      area1 = (area->x2 - area->x1) * (area->y2 - area->y1) + OVERHEAD;
      area2 = (ga2->x2 - ga2->x1) * (ga2->y2 - ga2->y1) + OVERHEAD;
      area3 = ((MAX (ga2->x2, area->x2) - MIN (ga2->x1, area->x1)) *
               (MAX (ga2->y2, area->y2) - MIN (ga2->y1, area->y1)) + OVERHEAD);

      if ((area1 + area2) < area3)
        {
          new_list = g_slist_prepend (new_list, ga2);
        }
      else
        {
          area->x1 = MIN (area->x1, ga2->x1);
          area->y1 = MIN (area->y1, ga2->y1);
          area->x2 = MAX (area->x2, ga2->x2);
          area->y2 = MAX (area->y2, ga2->y2);

          g_slice_free (GimpArea, ga2);
        }
    }

  if (list)
    g_slist_free (list);

  return new_list;
}

void
gimp_area_list_free (GSList *areas)
{
  GSList *list;

  for (list = areas; list; list = list->next)
    gimp_area_free (list->data);

  g_slist_free (list);
}
