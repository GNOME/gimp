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
#include "gfig-poly.h"
#include "gfig-circle.h"

#include "libgimp/stdplugins-intl.h"

static gint        calc_radius     (GdkPoint *center,
                                    GdkPoint *edge);
static void        d_draw_circle   (GfigObject *obj,
                                    cairo_t    *cr);
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
d_draw_circle (GfigObject *obj,
               cairo_t    *cr)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        radius;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  draw_sqr (&center_pnt->pnt, obj == gfig_context->selected_obj, cr);

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    return;

  radius = calc_radius (&center_pnt->pnt, &edge_pnt->pnt);

  if (obj_creating == obj)
    draw_circle (&edge_pnt->pnt, TRUE, cr);
  else
    draw_sqr (&edge_pnt->pnt, obj == gfig_context->selected_obj, cr);

  gfig_draw_arc (center_pnt->pnt.x, center_pnt->pnt.y,
                 radius, radius, 0, 360, cr);
}

static void
d_paint_circle (GfigObject *obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        radius;
  gdouble     dpnts[4];

  g_assert (obj != NULL);

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

  if (gfig_context_get_current_style ()->fill_type != FILL_NONE)
    {
      gimp_context_push ();
      gimp_context_set_antialias (selopt.antia);
      gimp_context_set_feather (selopt.feather);
      gimp_context_set_feather_radius (selopt.feather_radius, selopt.feather_radius);
      gimp_image_select_ellipse (gfig_context->image,
                                 selopt.type,
                                 dpnts[0], dpnts[1],
                                 dpnts[2], dpnts[3]);
      gimp_context_pop ();

      paint_layer_fill (center_pnt->pnt.x - radius,
                        center_pnt->pnt.y - radius,
                        center_pnt->pnt.x + radius,
                        center_pnt->pnt.y + radius);
      gimp_selection_none (gfig_context->image);
    }

  /* Drawing a circle may be harder than stroking a circular selection,
   * but we have to do it or we will not be able to draw outside of the
   * layer. */
  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      const gdouble r = dpnts[2] / 2;
      const gdouble cx = dpnts[0] + r, cy = dpnts[1] + r;
      gdouble       line_pnts[362];
      gdouble       angle = 0;
      gint          i = 0;

      while (i < 361)
        {
          static const gdouble step = 2 * G_PI / 180;

          line_pnts[i++] = cx + r * cos (angle);
          line_pnts[i++] = cy + r * sin (angle);
          angle += step;
        }

      gfig_paint (selvals.brshtype, gfig_context->drawable, i, line_pnts);
    }
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
d_circle_start (GdkPoint *pnt,
                gboolean  shift_down)
{
  obj_creating = d_new_object (CIRCLE, pnt->x, pnt->y);
}

void
d_circle_end (GimpGfig *gfig,
              GdkPoint *pnt,
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
      add_to_all_obj (gfig, gfig_context->current_obj, obj_creating);
    }

  obj_creating = NULL;
}

