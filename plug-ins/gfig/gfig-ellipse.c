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

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-ellipse.h"

#include "libgimp/stdplugins-intl.h"


static void        d_draw_ellipse   (GfigObject *obj,
                                     cairo_t    *cr);
static void        d_paint_ellipse  (GfigObject *obj);
static GfigObject *d_copy_ellipse   (GfigObject *obj);

static void        d_update_ellipse (GdkPoint   *pnt);

static void
d_draw_ellipse (GfigObject *obj,
                cairo_t    *cr)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        bound_wx;
  gint        bound_wy;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  draw_sqr (&center_pnt->pnt, obj == gfig_context->selected_obj, cr);

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    return;

  if (obj == obj_creating)
    draw_circle (&edge_pnt->pnt, TRUE, cr);
  else
    draw_sqr (&edge_pnt->pnt, obj == gfig_context->selected_obj, cr);

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x);
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y);

  gfig_draw_arc (center_pnt->pnt.x, center_pnt->pnt.y, bound_wx, bound_wy, 0, 360, cr);
}

static void
d_paint_ellipse (GfigObject *obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        bound_wx;
  gint        bound_wy;
  gint        top_x;
  gint        top_y;
  gdouble     dpnts[4];

  g_assert (obj != NULL);

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    {
      g_error ("Internal error - ellipse no edge pnt");
    }

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x)*2;
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y)*2;

  if (edge_pnt->pnt.x > center_pnt->pnt.x)
    top_x = 2*center_pnt->pnt.x - edge_pnt->pnt.x;
  else
    top_x = edge_pnt->pnt.x;

  if (edge_pnt->pnt.y > center_pnt->pnt.y)
    top_y = 2*center_pnt->pnt.y - edge_pnt->pnt.y;
  else
    top_y = edge_pnt->pnt.y;

  dpnts[0] = (gdouble)top_x;
  dpnts[1] = (gdouble)top_y;
  dpnts[2] = (gdouble)bound_wx;
  dpnts[3] = (gdouble)bound_wy;

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&dpnts[0], 2);
  else
    scale_to_xy (&dpnts[0], 2);

  if (gfig_context_get_current_style ()->fill_type != FILL_NONE)
    {
      gimp_context_push ();
      gimp_context_set_antialias (selopt.antia);
      gimp_context_set_feather (selopt.feather);
      gimp_context_set_feather_radius (selopt.feather_radius, selopt.feather_radius);
      gimp_image_select_ellipse (gfig_context->image_id,
                                 selopt.type,
                                 dpnts[0], dpnts[1],
                                 dpnts[2], dpnts[3]);
      gimp_context_pop ();

      paint_layer_fill (top_x, top_y, top_x + bound_wx, top_y + bound_wy);
      gimp_selection_none (gfig_context->image_id);
    }

  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      const gdouble rx = dpnts[2] / 2, ry = dpnts[3] / 2;
      const gdouble cx = dpnts[0] + rx, cy = dpnts[1] + ry;
      gdouble       line_pnts[362];
      gdouble       angle = 0;
      gint          i = 0;

      while (i < 362)
        {
          static const gdouble step = 2 * G_PI / 180;

          line_pnts[i++] = cx + rx * cos (angle);
          line_pnts[i++] = cy + ry * sin (angle);
          angle += step;
        }

      gfig_paint (selvals.brshtype, gfig_context->drawable_id, i, line_pnts);
    }
}

static GfigObject *
d_copy_ellipse (GfigObject * obj)
{
  GfigObject *nc;

  g_assert (obj->type == ELLIPSE);

  nc = d_new_object (ELLIPSE, obj->points->pnt.x, obj->points->pnt.y);
  nc->points->next = d_copy_dobjpoints (obj->points->next);

  return nc;
}

void
d_ellipse_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[ELLIPSE];

  class->type      = ELLIPSE;
  class->name      = "ELLIPSE";
  class->drawfunc  = d_draw_ellipse;
  class->paintfunc = d_paint_ellipse;
  class->copyfunc  = d_copy_ellipse;
  class->update    = d_update_ellipse;
}

static void
d_update_ellipse (GdkPoint *pnt)
{
  DobjPoints *center_pnt, *edge_pnt;

  center_pnt = obj_creating->points;

  if (!center_pnt)
    return; /* No points */

  if ((edge_pnt = center_pnt->next))
    {
      edge_pnt->pnt = *pnt;
    }
  else
    {
      edge_pnt = new_dobjpoint (pnt->x, pnt->y);
      center_pnt->next = edge_pnt;
    }
}

void
d_ellipse_start (GdkPoint *pnt,
                 gboolean  shift_down)
{
  obj_creating = d_new_object (ELLIPSE, pnt->x, pnt->y);
}

void
d_ellipse_end (GdkPoint *pnt,
               gboolean  shift_down)
{
  /* Under control point */
  if (!obj_creating->points->next)
    {
      /* No circle created */
      free_one_obj (obj_creating);
    }
  else
    {
      add_to_all_obj (gfig_context->current_obj, obj_creating);
    }

  obj_creating = NULL;
}

