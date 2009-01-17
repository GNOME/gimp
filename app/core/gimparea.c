/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  GSList *retval;
  GSList *l;

  retval = g_slist_prepend (NULL, area);

  for (l = list; l; l = g_slist_next (l))
    {
      GimpArea *this = l->data;
      gint      area1;
      gint      area2;
      gint      area3;

      area1 = (area->x2 - area->x1) * (area->y2 - area->y1) + OVERHEAD;
      area2 = (this->x2 - this->x1) * (this->y2 - this->y1) + OVERHEAD;
      area3 = ((MAX (this->x2, area->x2) - MIN (this->x1, area->x1)) *
               (MAX (this->y2, area->y2) - MIN (this->y1, area->y1)) + OVERHEAD);

      if (area1 + area2 < area3)
        {
          retval = g_slist_prepend (retval, this);
        }
      else
        {
          area->x1 = MIN (area->x1, this->x1);
          area->y1 = MIN (area->y1, this->y1);
          area->x2 = MAX (area->x2, this->x2);
          area->y2 = MAX (area->y2, this->y2);

          g_slice_free (GimpArea, this);
        }
    }

  if (list)
    g_slist_free (list);

  return retval;
}

void
gimp_area_list_free (GSList *areas)
{
  GSList *list;

  for (list = areas; list; list = list->next)
    gimp_area_free (list->data);

  g_slist_free (areas);
}
