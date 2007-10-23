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
#include "gfig-rectangle.h"

#include "libgimp/stdplugins-intl.h"

static void        d_draw_rectangle   (GfigObject *obj);
static void        d_paint_rectangle  (GfigObject *obj);
static GfigObject *d_copy_rectangle   (GfigObject *obj);

static void        d_update_rectangle (GdkPoint   *pnt);

static gint
gfig_scale_x (gint x)
{
  if (!selvals.scaletoimage)
    return (gint) (x * (1 / scale_x_factor));
  else
    return x;
}

static gint
gfig_scale_y (gint y)
{
  if (!selvals.scaletoimage)
    return (gint) (y * (1 / scale_y_factor));
  else
    return y;
}

static void
d_draw_rectangle (GfigObject *obj)
{
  DobjPoints *first_pnt;
  DobjPoints *second_pnt;
  gint        xmin, ymin;
  gint        xmax, ymax;

  first_pnt = obj->points;

  if (!first_pnt)
    return; /* End-of-line */

  second_pnt = first_pnt->next;

  if (!second_pnt)
    {
      g_warning ("Internal error - rectangle no edge pnt");
    }

  draw_sqr (&first_pnt->pnt, obj == gfig_context->selected_obj);
  draw_sqr (&second_pnt->pnt, obj == gfig_context->selected_obj);

  xmin = MIN (gfig_scale_x (first_pnt->pnt.x),
              gfig_scale_x (second_pnt->pnt.x));
  ymin = MIN (gfig_scale_y (first_pnt->pnt.y),
              gfig_scale_y (second_pnt->pnt.y));
  xmax = MAX (gfig_scale_x (first_pnt->pnt.x),
              gfig_scale_x (second_pnt->pnt.x));
  ymax = MAX (gfig_scale_y (first_pnt->pnt.y),
              gfig_scale_y (second_pnt->pnt.y));
  gdk_draw_rectangle (gfig_context->preview->window,
                      gfig_gc,
                      FALSE,
                      xmin, ymin,
                      xmax - xmin, ymax - ymin);
}

static void
d_paint_rectangle (GfigObject *obj)
{
  DobjPoints *first_pnt;
  DobjPoints *second_pnt;
  gdouble     dpnts[4];

  g_assert (obj != NULL);

  /* Drawing rectangles is hard .
   * 1) select rectangle
   * 2) stroke it
   */
  first_pnt = obj->points;

  if (!first_pnt)
    return; /* End-of-line */

  second_pnt = first_pnt->next;

  if (!second_pnt)
    {
      g_error ("Internal error - rectangle no second pnt");
    }

  dpnts[0] = (gdouble) MIN (first_pnt->pnt.x, second_pnt->pnt.x);
  dpnts[1] = (gdouble) MIN (first_pnt->pnt.y, second_pnt->pnt.y);
  dpnts[2] = (gdouble) MAX (first_pnt->pnt.x, second_pnt->pnt.x);
  dpnts[3] = (gdouble) MAX (first_pnt->pnt.y, second_pnt->pnt.y);

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&dpnts[0], 2);
  else
    scale_to_xy (&dpnts[0], 2);

  gimp_rect_select (gfig_context->image_id,
                    dpnts[0], dpnts[1],
                    dpnts[2] - dpnts[0],
                    dpnts[3] - dpnts[1],
                    selopt.type,
                    selopt.feather,
                    selopt.feather_radius);

  paint_layer_fill (dpnts[0], dpnts[1], dpnts[2], dpnts[3]);

  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    gimp_edit_stroke (gfig_context->drawable_id);
}

static GfigObject *
d_copy_rectangle (GfigObject * obj)
{
  GfigObject *new_rectangle;

  g_assert (obj->type == RECTANGLE);

  new_rectangle = d_new_object (RECTANGLE,
                                obj->points->pnt.x, obj->points->pnt.y);
  new_rectangle->points->next = d_copy_dobjpoints (obj->points->next);

  return new_rectangle;
}

void
d_rectangle_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[RECTANGLE];

  class->type      = RECTANGLE;
  class->name      = "RECTANGLE";
  class->drawfunc  = d_draw_rectangle;
  class->paintfunc = d_paint_rectangle;
  class->copyfunc  = d_copy_rectangle;
  class->update    = d_update_rectangle;
}

static void
d_update_rectangle (GdkPoint *pnt)
{
  DobjPoints *first_pnt;
  DobjPoints *second_pnt;
  gint        xmin, ymin;
  gint        xmax, ymax;

  /* Undraw last one then draw new one */
  first_pnt = obj_creating->points;

  if (!first_pnt)
    return; /* No points */

  if ((second_pnt = first_pnt->next))
    {
      /* Undraw current */
      draw_circle (&second_pnt->pnt, TRUE);

      xmin = MIN (gfig_scale_x (first_pnt->pnt.x),
                  gfig_scale_x (second_pnt->pnt.x));
      ymin = MIN (gfig_scale_y (first_pnt->pnt.y),
                  gfig_scale_y (second_pnt->pnt.y));
      xmax = MAX (gfig_scale_x (first_pnt->pnt.x),
                  gfig_scale_x (second_pnt->pnt.x));
      ymax = MAX (gfig_scale_y (first_pnt->pnt.y),
                  gfig_scale_y (second_pnt->pnt.y));
      gdk_draw_rectangle (gfig_context->preview->window,
                          gfig_gc,
                          FALSE,
                          xmin, ymin, xmax - xmin, ymax - ymin);
      second_pnt->pnt.x = pnt->x;
      second_pnt->pnt.y = pnt->y;
    }
  else
    {
      second_pnt = new_dobjpoint (pnt->x, pnt->y);
      first_pnt->next = second_pnt;
    }

  draw_circle (&second_pnt->pnt, TRUE);

  xmin = MIN (gfig_scale_x (first_pnt->pnt.x),
              gfig_scale_x (second_pnt->pnt.x));
  ymin = MIN (gfig_scale_y (first_pnt->pnt.y),
              gfig_scale_y (second_pnt->pnt.y));
  xmax = MAX (gfig_scale_x (first_pnt->pnt.x),
              gfig_scale_x (second_pnt->pnt.x));
  ymax = MAX (gfig_scale_y (first_pnt->pnt.y),
              gfig_scale_y (second_pnt->pnt.y));
  gdk_draw_rectangle (gfig_context->preview->window,
                      gfig_gc, FALSE,
                      xmin, ymin, xmax - xmin, ymax - ymin);
}

void
d_rectangle_start (GdkPoint *pnt,
                   gboolean  shift_down)
{
  obj_creating = d_new_object (RECTANGLE, pnt->x, pnt->y);
}

void
d_rectangle_end (GdkPoint *pnt,
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

