/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dobject.h"

#include "libgimp/stdplugins-intl.h"

Dobject *
d_copy_line (Dobject *obj)
{
  Dobject *nl;

  g_assert (obj->type == LINE);

  nl = d_new_object (LINE, obj->points->pnt.x, obj->points->pnt.y);
  nl->points->next = d_copy_dobjpoints (obj->points->next);

  return nl;
}

void
d_draw_line (Dobject *obj)
{
  DobjPoints *spnt;
  DobjPoints *epnt;

  spnt = obj->points;

  if (!spnt)
    return; /* End-of-line */

  epnt = spnt->next;

  while (spnt && epnt)
    {
      draw_sqr (&spnt->pnt);
      /* Go around all the points drawing a line from one to the next */
      gfig_draw_line (spnt->pnt.x, spnt->pnt.y, epnt->pnt.x, epnt->pnt.y);
      spnt = epnt;
      epnt = epnt->next;
    }
  draw_sqr (&spnt->pnt);
}

void
d_paint_line (Dobject *obj)
{
  DobjPoints *spnt;
  gdouble    *line_pnts;
  gint        seg_count = 0;
  gint        i = 0;

  for (spnt = obj->points; spnt; spnt = spnt->next)
    seg_count++;

  if (!seg_count)
    return; /* no-line */

  line_pnts = g_new0 (gdouble, 2 * seg_count + 1);

  /* Go around all the points drawing a line from one to the next */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      line_pnts[i++] = spnt->pnt.x;
      line_pnts[i++] = spnt->pnt.y;
    }

  /* Reverse line if approp */
  if (selvals.reverselines)
    reverse_pairs_list (&line_pnts[0], i/2);

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&line_pnts[0], i/2);
  else
    scale_to_xy (&line_pnts[0], i/2);

  /* One go */
  if (selvals.painttype == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
                  gfig_context->drawable_id,
                  seg_count * 2, line_pnts);
    }
  else
    {
      gimp_free_select (gfig_context->image_id,
                        seg_count * 2, line_pnts,
                        selopt.type,
                        selopt.antia,
                        selopt.feather,
                        selopt.feather_radius);
    }

  g_free (line_pnts);
}

/* Create a new line object. starting at the x, y point might add styles
 * later.
 */

void
d_line_object_class_init (void)
{
  DobjClass *class = &dobj_class[LINE];

  class->type      = LINE;
  class->name      = "Line";
  class->drawfunc  = d_draw_line;
  class->paintfunc = d_paint_line;
  class->copyfunc  = d_copy_line;
}

/* You guessed it delete the object !*/
/*
static void
d_delete_line (Dobject *obj)
{
  g_assert (obj != NULL);
  * First free the list of points - then the object itself *
  d_delete_dobjpoints (obj->points);
  g_free (obj);
}
*/

/* Add a point to a line (given x, y)
 * pos = 0 = head
 * pos = -1 = tail
 * 0 < pos = nth position
 */

void
d_pnt_add_line (Dobject *obj,
                gint     x,
                gint     y,
                gint     pos)
{
  DobjPoints *npnts = new_dobjpoint (x, y);

  g_assert (obj != NULL);

  if (!pos)
    {
      /* Add to head */
      npnts->next = obj->points;
      obj->points = npnts;
    }
  else
    {
      DobjPoints *pnt = obj->points;

      /* Go down chain until the end if pos */
      while (pos < 0 || pos-- > 0)
        {
          if (!(pnt->next) || !pos)
            {
              npnts->next = pnt->next;
              pnt->next = npnts;
              break;
            }
          else
            {
              pnt = pnt->next;
            }
        }
    }
}

/* Update end point of line */
void
d_update_line (GdkPoint *pnt)
{
  DobjPoints *spnt, *epnt;
  /* Get last but one segment and undraw it -
   * Then draw new segment in.
   * always dealing with the static object.
   */

  /* Get start of segments */
  spnt = obj_creating->points;

  if (!spnt)
    return; /* No points */

  if ((epnt = spnt->next))
    {
      /* undraw  current */
      /* Draw square on point */
      draw_circle (&epnt->pnt);

      gdk_draw_line (gfig_context->preview->window,
                     /*gfig_context->preview->style->bg_gc[GTK_STATE_NORMAL],*/
                     gfig_gc,
                     spnt->pnt.x,
                     spnt->pnt.y,
                     epnt->pnt.x,
                     epnt->pnt.y);
      g_free (epnt);
    }

  /* draw new */
  /* Draw circle on point */
  draw_circle (pnt);

  epnt = new_dobjpoint (pnt->x, pnt->y);

  gdk_draw_line (gfig_context->preview->window,
                 /*gfig_context->preview->style->bg_gc[GTK_STATE_NORMAL],*/
                 gfig_gc,
                 spnt->pnt.x,
                 spnt->pnt.y,
                 epnt->pnt.x,
                 epnt->pnt.y);
  spnt->next = epnt;
}

void
d_line_start (GdkPoint *pnt,
              gboolean  shift_down)
{
  if (!obj_creating || !shift_down)
    {
      /* Draw square on point */
      /* Must delete obj_creating if we have one */
      obj_creating = d_new_object (LINE, pnt->x, pnt->y);
    }
  else
    {
      /* Contniuation */
      d_update_line (pnt);
    }
}

void
d_line_end (GdkPoint *pnt,
            gboolean  shift_down)
{
  /* Undraw the last circle */
  draw_circle (pnt);

  if (shift_down)
    {
      if (tmp_line)
        {
          GdkPoint tmp_pnt = *pnt;

          if (need_to_scale)
            {
              tmp_pnt.x = pnt->x * scale_x_factor;
              tmp_pnt.y = pnt->y * scale_y_factor;
            }

          d_pnt_add_line (tmp_line, tmp_pnt.x, tmp_pnt.y, -1);
          free_one_obj (obj_creating);
          /* Must free obj_creating */
        }
      else
        {
          tmp_line = obj_creating;
          add_to_all_obj (gfig_context->current_obj, obj_creating);
        }

      obj_creating = d_new_object (LINE, pnt->x, pnt->y);
    }
  else
    {
      if (tmp_line)
        {
          GdkPoint tmp_pnt = *pnt;

          if (need_to_scale)
            {
              tmp_pnt.x = pnt->x * scale_x_factor;
              tmp_pnt.y = pnt->y * scale_y_factor;
            }

          d_pnt_add_line (tmp_line, tmp_pnt.x, tmp_pnt.y, -1);
          free_one_obj (obj_creating);
          /* Must free obj_creating */
        }
      else
        {
          add_to_all_obj (gfig_context->current_obj, obj_creating);
        }
      obj_creating = NULL;
      tmp_line = NULL;
    }
  /*gtk_widget_queue_draw (gfig_context->preview);*/
}
