/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-line.h"

#include "libgimp/stdplugins-intl.h"


static GfigObject *d_copy_line   (GfigObject *obj);
static void        d_draw_line   (GfigObject *obj,
                                  cairo_t    *cr);
static void        d_paint_line  (GfigObject *obj);

static void        d_update_line (GdkPoint   *pnt);

static GfigObject *
d_copy_line (GfigObject *obj)
{
  GfigObject *nl;

  g_assert (obj->type == LINE);

  nl = d_new_object (LINE, obj->points->pnt.x, obj->points->pnt.y);
  nl->points->next = d_copy_dobjpoints (obj->points->next);

  return nl;
}

static void
d_draw_line (GfigObject *obj,
             cairo_t    *cr)
{
  DobjPoints *spnt;
  DobjPoints *epnt;

  spnt = obj->points;

  if (!spnt)
    return; /* End-of-line */

  epnt = spnt->next;

  while (spnt && epnt)
    {
      draw_sqr (&spnt->pnt, obj == gfig_context->selected_obj, cr);
      /* Go around all the points drawing a line from one to the next */
      gfig_draw_line (spnt->pnt.x, spnt->pnt.y, epnt->pnt.x, epnt->pnt.y, cr);
      spnt = epnt;
      epnt = epnt->next;
    }
  if (obj_creating == obj)
    draw_circle (&spnt->pnt, TRUE, cr);
  else
    draw_sqr (&spnt->pnt, obj == gfig_context->selected_obj, cr);
}

static void
d_paint_line (GfigObject *obj)
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

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&line_pnts[0], i/2);
  else
    scale_to_xy (&line_pnts[0], i/2);

  /* One go */
  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
                  gfig_context->drawable_id,
                  seg_count * 2, line_pnts);
    }

  g_free (line_pnts);
}

void
d_line_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[LINE];

  class->type      = LINE;
  class->name      = "LINE";
  class->drawfunc  = d_draw_line;
  class->paintfunc = d_paint_line;
  class->copyfunc  = d_copy_line;
  class->update    = d_update_line;
}

static void
d_update_line (GdkPoint *pnt)
{
  DobjPoints *spnt, *epnt;

  /* Get start of segments */
  spnt = obj_creating->points;

  if (!spnt)
    return; /* No points */

  if ((epnt = spnt->next))
    {
      g_free (epnt);
    }

  epnt = new_dobjpoint (pnt->x, pnt->y);
  spnt->next = epnt;
}

void
d_line_start (GdkPoint *pnt,
              gboolean  shift_down)
{
  if (!obj_creating || !shift_down)
    {
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
}
