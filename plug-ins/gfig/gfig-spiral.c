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
#include "gfig-line.h"
#include "gfig-spiral.h"
#include "gfig-dialog.h"

#include "libgimp/stdplugins-intl.h"

static void        d_draw_spiral   (GfigObject *obj,
                                    cairo_t    *cr);
static void        d_paint_spiral  (GfigObject *obj);
static GfigObject *d_copy_spiral   (GfigObject *obj);

static void        d_update_spiral (GdkPoint  *pnt);

static gint spiral_num_turns = 4; /* Default to 4 turns */
static gint spiral_toggle    = 0; /* 0 = clockwise -1 = anti-clockwise */

void
tool_options_spiral (GtkWidget *notebook)
{
  GtkWidget *sides;

  sides = num_sides_widget (_("Spiral Number of Turns"),
                            &spiral_num_turns, &spiral_toggle, 1, 20);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sides, NULL);
}

static void
d_draw_spiral (GfigObject *obj,
               cairo_t    *cr)
{
  DobjPoints *center_pnt;
  DobjPoints *radius_pnt;
  gint16      shift_x;
  gint16      shift_y;
  gdouble     ang_grid;
  gdouble     ang_loop;
  gdouble     radius;
  gdouble     offset_angle;
  gdouble     sp_cons;
  gint        loop;
  GdkPoint    start_pnt = { 0, 0 };
  gboolean    do_line = FALSE;
  gint        clock_wise = 1;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  /* First point is the center */
  /* Just draw a control point around it */

  draw_sqr (&center_pnt->pnt, obj == gfig_context->selected_obj, cr);

  /* Next point defines the radius */
  radius_pnt = center_pnt->next; /* this defines the vetices */

  if (!radius_pnt)
    {
#ifdef DEBUG
      g_warning ("Internal error in spiral - no vertice point \n");
#endif /* DEBUG */
      return;
    }

  /* Other control point */
  if (obj_creating == obj)
    draw_circle (&radius_pnt->pnt, TRUE, cr);
  else
    draw_sqr (&radius_pnt->pnt, obj == gfig_context->selected_obj, cr);

  /* Have center and radius - draw spiral */

  shift_x = radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = radius_pnt->pnt.y - center_pnt->pnt.y;

  radius = sqrt ((shift_x * shift_x) + (shift_y * shift_y));

  offset_angle = atan2 (shift_y, shift_x);

  clock_wise = obj->type_data / abs (obj->type_data);

  if (offset_angle < 0)
    offset_angle += 2.0 * G_PI;

  sp_cons = radius/(obj->type_data * 2 * G_PI + offset_angle);
  /* Lines */
  ang_grid = 2.0 * G_PI / 180.0;


  for (loop = 0 ; loop <= abs (obj->type_data * 180) +
         clock_wise * (gint)RINT (offset_angle/ang_grid) ; loop++)
    {
      gdouble  lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid;

      lx = sp_cons * ang_loop * cos (ang_loop)*clock_wise;
      ly = sp_cons * ang_loop * sin (ang_loop);

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
        }
      start_pnt = calc_pnt;
    }
}

static void
d_paint_spiral (GfigObject *obj)
{
  /* first point center */
  /* Next point is radius */
  gdouble    *line_pnts;
  gint        seg_count = 0;
  gint        i = 0;
  DobjPoints *center_pnt;
  DobjPoints *radius_pnt;
  gint16      shift_x;
  gint16      shift_y;
  gdouble     ang_grid;
  gdouble     ang_loop;
  gdouble     radius;
  gdouble     offset_angle;
  gdouble     sp_cons;
  gint        loop;
  GdkPoint    last_pnt = { 0, 0 };
  gint        clock_wise = 1;

  g_assert (obj != NULL);

  center_pnt = obj->points;

  if (!center_pnt || !center_pnt->next)
    return; /* no-line */

  /* Go around all the points drawing a line from one to the next */

  radius_pnt = center_pnt->next; /* this defines the vetices */

  /* Have center and radius - get lines */
  shift_x = radius_pnt->pnt.x - center_pnt->pnt.x;
  shift_y = radius_pnt->pnt.y - center_pnt->pnt.y;

  radius = sqrt ((shift_x * shift_x) + (shift_y * shift_y));

  clock_wise = obj->type_data / abs (obj->type_data);

  offset_angle = atan2 (shift_y, shift_x);

  if (offset_angle < 0)
    offset_angle += 2.0 * G_PI;

  sp_cons = radius/(obj->type_data * 2.0 * G_PI + offset_angle);
  /* Lines */
  ang_grid = 2.0 * G_PI / 180.0;

  /* count - */
  seg_count = abs (obj->type_data * 180) + clock_wise * (gint)RINT (offset_angle/ang_grid);

  line_pnts = g_new0 (gdouble, 2 * seg_count + 3);

  for (loop = 0 ; loop <= seg_count; loop++)
    {
      gdouble  lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid;

      lx = sp_cons * ang_loop * cos (ang_loop)*clock_wise;
      ly = sp_cons * ang_loop * sin (ang_loop);

      calc_pnt.x = RINT (lx + center_pnt->pnt.x);
      calc_pnt.y = RINT (ly + center_pnt->pnt.y);

      /* Miss out duped pnts */
      if (!loop)
        {
          if (calc_pnt.x == last_pnt.x && calc_pnt.y == last_pnt.y)
            {
              continue;
            }
        }

      line_pnts[i++] = calc_pnt.x;
      line_pnts[i++] = calc_pnt.y;
      last_pnt = calc_pnt;
    }

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&line_pnts[0], i / 2);
  else
    scale_to_xy (&line_pnts[0], i / 2);

  /* One go */
  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
                  gfig_context->drawable,
                  i, line_pnts);
    }

  g_free (line_pnts);
}

static GfigObject *
d_copy_spiral (GfigObject *obj)
{
  GfigObject *np;

  g_assert (obj->type == SPIRAL);

  np = d_new_object (SPIRAL, obj->points->pnt.x, obj->points->pnt.y);
  np->points->next = d_copy_dobjpoints (obj->points->next);
  np->type_data = obj->type_data;

  return np;
}

void
d_spiral_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[SPIRAL];

  class->type      = SPIRAL;
  class->name      = "SPIRAL";
  class->drawfunc  = d_draw_spiral;
  class->paintfunc = d_paint_spiral;
  class->copyfunc  = d_copy_spiral;
  class->update    = d_update_spiral;
}

static void
d_update_spiral (GdkPoint *pnt)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;

  center_pnt = obj_creating->points;

  if (!center_pnt)
    return; /* No points */

  if ((edge_pnt = center_pnt->next))
    {
      edge_pnt->pnt = *pnt;
    }
  else
    {
      /* Radius is a few pixels away */
      /* First edge point */
      d_pnt_add_line (obj_creating, pnt->x, pnt->y, -1);
    }
}

void
d_spiral_start (GdkPoint *pnt,
                gboolean  shift_down)
{
  obj_creating = d_new_object (SPIRAL, pnt->x, pnt->y);
  obj_creating->type_data = spiral_num_turns * ((spiral_toggle == 0) ? 1 : -1);
}

void
d_spiral_end (GimpGfig *gfig,
              GdkPoint *pnt,
              gboolean  shift_down)
{
  add_to_all_obj (gfig, gfig_context->current_obj, obj_creating);
  obj_creating = NULL;
}
