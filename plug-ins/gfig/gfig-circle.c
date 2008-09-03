/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-poly.h"
#include "gfig-circle.h"

#include "libgimp/stdplugins-intl.h"

static gint        calc_radius     (GdkPoint *center,
                                    GdkPoint *edge);
static void        d_draw_circle   (GfigObject *obj);
static void        d_paint_circle  (GfigObject *obj);
static GfigObject *d_copy_circle   (GfigObject *obj);

static void        d_update_circle (GdkPoint   *pnt);

static gint
calc_radius (GdkPoint *center, GdkPoint *edge)
{
  gint dx = center->x - edge->x;
  gint dy = center->y - edge->y;

  return (gint) sqrt (dx * dx + dy * dy);
}

static void
d_draw_circle (GfigObject *obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        radius;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    {
      g_warning ("Internal error - circle no edge pnt");
    }

  radius = calc_radius (&center_pnt->pnt, &edge_pnt->pnt);
  draw_sqr (&center_pnt->pnt, obj == gfig_context->selected_obj);
  draw_sqr (&edge_pnt->pnt, obj == gfig_context->selected_obj);

  gfig_draw_arc (center_pnt->pnt.x, center_pnt->pnt.y,
                 radius, radius, 0, 360);
}

static void
d_paint_circle (GfigObject *obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        radius;
  gdouble     dpnts[4];

  g_assert (obj != NULL);

  /* Drawing circles is hard .
   * 1) select circle
   * 2) stroke it
   */
  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    {
      g_error ("Internal error - circle no edge pnt");
    }

  radius = calc_radius (&center_pnt->pnt, &edge_pnt->pnt);

  dpnts[0] = (gdouble) center_pnt->pnt.x - radius;
  dpnts[1] = (gdouble) center_pnt->pnt.y - radius;
  dpnts[3] = dpnts[2] = (gdouble) radius * 2;

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&dpnts[0], 2);
  else
    scale_to_xy (&dpnts[0], 2);

  gimp_ellipse_select (gfig_context->image_id,
                       dpnts[0], dpnts[1],
                       dpnts[2], dpnts[3],
                       selopt.type,
                       selopt.antia,
                       selopt.feather,
                       selopt.feather_radius);

  paint_layer_fill (center_pnt->pnt.x - radius,
                    center_pnt->pnt.y - radius,
                    center_pnt->pnt.x + radius,
                    center_pnt->pnt.y + radius);

  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    gimp_edit_stroke (gfig_context->drawable_id);
}

static GfigObject *
d_copy_circle (GfigObject * obj)
{
  GfigObject *nc;

  g_assert (obj->type == CIRCLE);

  nc = d_new_object (CIRCLE, obj->points->pnt.x, obj->points->pnt.y);
  nc->points->next = d_copy_dobjpoints (obj->points->next);

  return nc;
}

void
d_circle_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[CIRCLE];

  class->type      = CIRCLE;
  class->name      = "CIRCLE";
  class->drawfunc  = d_draw_circle;
  class->paintfunc = d_paint_circle;
  class->copyfunc  = d_copy_circle;
  class->update    = d_update_circle;
}

static void
d_update_circle (GdkPoint *pnt)
{
  DobjPoints *center_pnt, *edge_pnt;
  gint        radius;

  /* Undraw last one then draw new one */
  center_pnt = obj_creating->points;

  if (!center_pnt)
    return; /* No points */

  if ((edge_pnt = center_pnt->next))
    {
      /* Undraw current */
      draw_circle (&edge_pnt->pnt, TRUE);
      radius = calc_radius (&center_pnt->pnt, &edge_pnt->pnt);

      gdk_draw_arc (gfig_context->preview->window,
                    gfig_gc,
                    0,
                    center_pnt->pnt.x - (gint) RINT (radius),
                    center_pnt->pnt.y - (gint) RINT (radius),
                    (gint) RINT (radius) * 2,
                    (gint) RINT (radius) * 2,
                    0,
                    360 * 64);
      edge_pnt->pnt = *pnt;
    }
  else
    {
      edge_pnt = new_dobjpoint (pnt->x, pnt->y);
      center_pnt->next = edge_pnt;
    }

  draw_circle (&edge_pnt->pnt, TRUE);

  radius = calc_radius (&center_pnt->pnt, &edge_pnt->pnt);

  gdk_draw_arc (gfig_context->preview->window,
                gfig_gc,
                0,
                center_pnt->pnt.x - (gint) RINT (radius),
                center_pnt->pnt.y - (gint) RINT (radius),
                (gint) RINT (radius) * 2,
                (gint) RINT (radius) * 2,
                0,
                360 * 64);
}

void
d_circle_start (GdkPoint *pnt,
                gboolean  shift_down)
{
  obj_creating = d_new_object (CIRCLE, pnt->x, pnt->y);
}

void
d_circle_end (GdkPoint *pnt,
              gboolean  shift_down)
{
  /* Under contrl point */
  if (!obj_creating->points->next)
    {
      /* No circle created */
      free_one_obj (obj_creating);
    }
  else
    {
      draw_circle (pnt, TRUE);
      add_to_all_obj (gfig_context->current_obj, obj_creating);
    }

  obj_creating = NULL;
}

