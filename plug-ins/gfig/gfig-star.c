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
#include "gfig-line.h"
#include "gfig-dobject.h"
#include "gfig-star.h"
#include "gfig-dialog.h"

#include "libgimp/stdplugins-intl.h"

static gint star_num_sides = 3; /* Default to three sided object */

static void        d_draw_star   (GfigObject *obj,
                                  cairo_t    *cr);
static void        d_paint_star  (GfigObject *obj);
static GfigObject *d_copy_star   (GfigObject *obj);

static void        d_update_star (GdkPoint   *pnt);

void
tool_options_star (GtkWidget *notebook)
{
  GtkWidget *sides;

  sides = num_sides_widget (_("Star Number of Points"),
                            &star_num_sides, NULL, 3, 200);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sides, NULL);
}

static void
d_draw_star (GfigObject *obj,
             cairo_t    *cr)
{
  DobjPoints *center_pnt;
  DobjPoints *outer_radius_pnt;
  DobjPoints *inner_radius_pnt;
  gint16      shift_x;
  gint16      shift_y;
  gdouble     ang_grid;
  gdouble     ang_loop;
  gdouble     outer_radius;
  gdouble     inner_radius;
  gdouble     offset_angle;
  gint        loop;
  GdkPoint    start_pnt = { 0, 0 };
  GdkPoint    first_pnt = { 0, 0 };
  gboolean    do_line = FALSE;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  /* First point is the center */
  /* Just draw a control point around it */

  draw_sqr (&center_pnt->pnt, obj == gfig_context->selected_obj, cr);

  /* Next point defines the radius */
  outer_radius_pnt = center_pnt->next; /* this defines the vertices */

  if (!outer_radius_pnt)
    {
      return;
    }

  inner_radius_pnt = outer_radius_pnt->next; /* this defines the vertices */

  if (!inner_radius_pnt)
    {
      return;
    }

  /* Other control points */
  if (obj == obj_creating)
    {
      draw_circle (&outer_radius_pnt->pnt, TRUE, cr);
      draw_circle (&inner_radius_pnt->pnt, TRUE, cr);
    }
  else
    {
      draw_sqr (&outer_radius_pnt->pnt, obj == gfig_context->selected_obj, cr);
      draw_sqr (&inner_radius_pnt->pnt, obj == gfig_context->selected_obj, cr);
    }

  /* Have center and radius - draw star */

  shift_x = outer_radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = outer_radius_pnt->pnt.y - center_pnt->pnt.y;

  outer_radius = sqrt ((shift_x*shift_x) + (shift_y*shift_y));

  /* Lines */
  ang_grid = 2.0 * G_PI / (2.0 * (gdouble) obj->type_data);
  offset_angle = atan2 (shift_y, shift_x);

  shift_x = inner_radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = inner_radius_pnt->pnt.y - center_pnt->pnt.y;

  inner_radius = sqrt ((shift_x*shift_x) + (shift_y*shift_y));

  for (loop = 0 ; loop < 2 * obj->type_data ; loop++)
    {
      gdouble lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid + offset_angle;

      if (loop % 2)
        {
          lx = inner_radius * cos (ang_loop);
          ly = inner_radius * sin (ang_loop);
        }
      else
        {
          lx = outer_radius * cos (ang_loop);
          ly = outer_radius * sin (ang_loop);
        }

      calc_pnt.x = RINT (lx + center_pnt->pnt.x);
      calc_pnt.y = RINT (ly + center_pnt->pnt.y);

      if (do_line)
        {

          /* Miss out points that come to the same location */
          if (calc_pnt.x == start_pnt.x && calc_pnt.y == start_pnt.y)
            continue;

          gfig_draw_line (calc_pnt.x, calc_pnt.y, start_pnt.x, start_pnt.y, cr);
        }
      else
        {
          do_line = TRUE;
          first_pnt = calc_pnt;
        }
      start_pnt = calc_pnt;
    }

  gfig_draw_line (first_pnt.x, first_pnt.y, start_pnt.x, start_pnt.y, cr);
}

static void
d_paint_star (GfigObject *obj)
{
  /* first point center */
  /* Next point is radius */
  gdouble    *line_pnts;
  gint        seg_count = 0;
  gint        i = 0;
  DobjPoints *center_pnt;
  DobjPoints *outer_radius_pnt;
  DobjPoints *inner_radius_pnt;
  gint16      shift_x;
  gint16      shift_y;
  gdouble     ang_grid;
  gdouble     ang_loop;
  gdouble     outer_radius;
  gdouble     inner_radius;
  gdouble     offset_angle;
  gint        loop;
  GdkPoint    first_pnt = { 0, 0 };
  GdkPoint    last_pnt  = { 0, 0 };
  gboolean    first = TRUE;
  gdouble    *min_max;

  g_assert (obj != NULL);

  /* count - add one to close polygon */
  seg_count = 2 * obj->type_data + 1;

  center_pnt = obj->points;

  if (!center_pnt || !seg_count)
    return; /* no-line */

  line_pnts = g_new0 (gdouble, 2 * seg_count + 1);
  min_max = g_new (gdouble, 4);

  /* Go around all the points drawing a line from one to the next */
  /* Next point defines the radius */
  outer_radius_pnt = center_pnt->next; /* this defines the vetices */

  if (!outer_radius_pnt)
    {
#ifdef DEBUG
      g_warning ("Internal error in star - no outer vertice point \n");
#endif /* DEBUG */
      g_free (line_pnts);
      g_free (min_max);
      return;
    }

  inner_radius_pnt = outer_radius_pnt->next; /* this defines the vetices */

  if (!inner_radius_pnt)
    {
#ifdef DEBUG
      g_warning ("Internal error in star - no inner vertice point \n");
#endif /* DEBUG */
      g_free (line_pnts);
      g_free (min_max);
      return;
    }

  shift_x = outer_radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = outer_radius_pnt->pnt.y - center_pnt->pnt.y;

  outer_radius = sqrt ((shift_x*shift_x) + (shift_y*shift_y));

  /* Lines */
  ang_grid = 2.0 * G_PI / (2.0 * (gdouble) obj->type_data);
  offset_angle = atan2 (shift_y, shift_x);

  shift_x = inner_radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = inner_radius_pnt->pnt.y - center_pnt->pnt.y;

  inner_radius = sqrt ((shift_x*shift_x) + (shift_y*shift_y));

  for (loop = 0 ; loop < 2 * obj->type_data ; loop++)
    {
      gdouble  lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid + offset_angle;

      if (loop % 2)
        {
          lx = inner_radius * cos (ang_loop);
          ly = inner_radius * sin (ang_loop);
        }
      else
        {
          lx = outer_radius * cos (ang_loop);
          ly = outer_radius * sin (ang_loop);
        }

      calc_pnt.x = RINT (lx + center_pnt->pnt.x);
      calc_pnt.y = RINT (ly + center_pnt->pnt.y);

      /* Miss out duped pnts */
      if (!first)
        {
          if (calc_pnt.x == last_pnt.x && calc_pnt.y == last_pnt.y)
            {
              continue;
            }
        }

      line_pnts[i++] = calc_pnt.x;
      line_pnts[i++] = calc_pnt.y;
      last_pnt = calc_pnt;

      if (first)
        {
          first_pnt = calc_pnt;
          first = FALSE;
          min_max[0] = min_max[2] = calc_pnt.x;
          min_max[1] = min_max[3] = calc_pnt.y;
        }
      else
        {
          min_max[0] = MIN (min_max[0], calc_pnt.x);
          min_max[1] = MIN (min_max[1], calc_pnt.y);
          min_max[2] = MAX (min_max[2], calc_pnt.x);
          min_max[3] = MAX (min_max[3], calc_pnt.y);
        }
    }

  line_pnts[i++] = first_pnt.x;
  line_pnts[i++] = first_pnt.y;

  /* Scale before drawing */
  if (selvals.scaletoimage)
    {
      scale_to_original_xy (&line_pnts[0], i / 2);
      scale_to_original_xy (min_max, 2);
    }
  else
    {
      scale_to_xy (&line_pnts[0], i / 2);
      scale_to_xy (min_max, 2);
    }

  if (gfig_context_get_current_style ()->fill_type != FILL_NONE)
    {
      gimp_context_push ();
      gimp_context_set_antialias (selopt.antia);
      gimp_context_set_feather (selopt.feather);
      gimp_context_set_feather_radius (selopt.feather_radius, selopt.feather_radius);
      gimp_image_select_polygon (gfig_context->image_id,
                                 selopt.type,
                                 i, line_pnts);
      gimp_context_pop ();

      paint_layer_fill (min_max[0], min_max[1], min_max[2], min_max[3]);
      gimp_selection_none (gfig_context->image_id);
    }

  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    gfig_paint (selvals.brshtype, gfig_context->drawable_id, i, line_pnts);

  g_free (line_pnts);
  g_free (min_max);
}

static GfigObject *
d_copy_star (GfigObject *obj)
{
  GfigObject *np;

  g_assert (obj->type == STAR);

  np = d_new_object (STAR, obj->points->pnt.x, obj->points->pnt.y);
  np->points->next = d_copy_dobjpoints (obj->points->next);
  np->type_data = obj->type_data;

  return np;
}

void
d_star_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[STAR];

  class->type      = STAR;
  class->name      = "STAR";
  class->drawfunc  = d_draw_star;
  class->paintfunc = d_paint_star;
  class->copyfunc  = d_copy_star;
  class->update    = d_update_star;
}

static void
d_update_star (GdkPoint *pnt)
{
  DobjPoints *center_pnt, *inner_pnt, *outer_pnt;

  center_pnt = obj_creating->points;

  if (!center_pnt)
    return; /* No points */

  if ((outer_pnt = center_pnt->next))
    {
      inner_pnt = outer_pnt->next;
      outer_pnt->pnt = *pnt;
      inner_pnt->pnt.x = pnt->x + (2 * (center_pnt->pnt.x - pnt->x)) / 3;
      inner_pnt->pnt.y = pnt->y + (2 * (center_pnt->pnt.y - pnt->y)) / 3;
    }
  else
    {
      /* Radius is a few pixels away */
      /* First edge point */
      d_pnt_add_line (obj_creating, pnt->x, pnt->y,-1);
      /* Inner radius */
      d_pnt_add_line (obj_creating,
                      pnt->x + (2 * (center_pnt->pnt.x - pnt->x)) / 3,
                      pnt->y + (2 * (center_pnt->pnt.y - pnt->y)) / 3,
                      -1);
    }
}

void
d_star_start (GdkPoint *pnt,
              gboolean  shift_down)
{
  obj_creating = d_new_object (STAR, pnt->x, pnt->y);
  obj_creating->type_data = star_num_sides;
}

void
d_star_end (GdkPoint *pnt,
            gboolean  shift_down)
{
  add_to_all_obj (gfig_context->current_obj, obj_creating);
  obj_creating = NULL;
}

