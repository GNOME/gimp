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
#include "gfig-dialog.h"
#include "gfig-bezier.h"

#include "libgimp/stdplugins-intl.h"

#define FP_PNT_MAX  10

typedef gdouble (*fp_pnt)[2];

static gboolean  bezier_closed     = FALSE;
static gboolean  bezier_line_frame = FALSE;
static int       fp_pnt_cnt = 0;
static int       fp_pnt_chunk = 0;
static gdouble  *fp_pnt_pnts = NULL;

GfigObject      *tmp_bezier;       /* Needed when drawing bezier curves */

static void        fp_pnt_start    (void);
static void        fp_pnt_add      (gdouble     p1,
                                    gdouble     p2,
                                    gdouble     p3,
                                    gdouble     p4);
static gdouble    *d_bz_get_array  (gint       *sz);
static void        d_bz_line       (cairo_t *cr);
static void        DrawBezier      (fp_pnt      points,
                                    gint        np,
                                    gdouble     mid,
                                    gint        depth);
static void        d_paint_bezier  (GfigObject *obj);
static GfigObject *d_copy_bezier   (GfigObject *obj);
static void        d_update_bezier (GdkPoint   *pnt);

static void
fp_pnt_start (void)
{
  fp_pnt_cnt = 0;
}

/* Add a line segment to collection array */
static void
fp_pnt_add (gdouble p1,
            gdouble p2,
            gdouble p3,
            gdouble p4)
{
  if (!fp_pnt_pnts)
    {
      fp_pnt_pnts = g_new0 (gdouble, FP_PNT_MAX);
      fp_pnt_chunk = 1;
    }

  if (((fp_pnt_cnt + 4) / FP_PNT_MAX) >= fp_pnt_chunk)
    {
      /* more space pls */
      fp_pnt_chunk++;
      fp_pnt_pnts = g_renew (gdouble, fp_pnt_pnts, fp_pnt_chunk * FP_PNT_MAX);
    }

  fp_pnt_pnts[fp_pnt_cnt++] = p1;
  fp_pnt_pnts[fp_pnt_cnt++] = p2;
  fp_pnt_pnts[fp_pnt_cnt++] = p3;
  fp_pnt_pnts[fp_pnt_cnt++] = p4;
}

static gdouble *
d_bz_get_array (gint *sz)
{
  *sz = fp_pnt_cnt;
  return fp_pnt_pnts;
}

static void
d_bz_line (cairo_t *cr)
{
  gint i, x0, y0, x1, y1;

  g_assert ((fp_pnt_cnt % 4) == 0);

  for (i = 0 ; i < fp_pnt_cnt; i += 4)
    {
      x0 = fp_pnt_pnts[i];
      y0 = fp_pnt_pnts[i + 1];
      x1 = fp_pnt_pnts[i + 2];
      y1 = fp_pnt_pnts[i + 3];

      gfig_draw_line (x0, y0, x1, y1, cr);
    }
}

/*  Return points to plot */
/* Terminate by point with DBL_MAX, DBL_MAX */
static void
DrawBezier (fp_pnt  points,
            gint    np,
            gdouble mid,
            gint    depth)
{
  gint   i, j, x0 = 0, y0 = 0, x1, y1;
  fp_pnt left;
  fp_pnt right;

    if (depth == 0) /* draw polyline */
      {
        for (i = 0; i < np; i++)
          {
            x1 = (int) points[i][0];
            y1 = (int) points[i][1];
            if (i > 0 && (x1 != x0 || y1 != y0))
              {
                /* Add pnts up */
                fp_pnt_add ((gdouble) x0, (gdouble) y0,
                            (gdouble) x1, (gdouble) y1);
              }
            x0 = x1;
            y0 = y1;
          }
      }
    else /* subdivide control points at mid */
      {
        left = (fp_pnt)g_new (gdouble, np * 2);
        right = (fp_pnt)g_new (gdouble, np * 2);
        for (i = 0; i < np; i++)
          {
            right[i][0] = points[i][0];
            right[i][1] = points[i][1];
          }
        left[0][0] = right[0][0];
        left[0][1] = right[0][1];
        for (j = np - 1; j >= 1; j--)
          {
            for (i = 0; i < j; i++)
              {
                right[i][0] = (1 - mid) * right[i][0] + mid * right[i + 1][0];
                right[i][1] = (1 - mid) * right[i][1] + mid * right[i + 1][1];
              }
            left[np - j][0] = right[0][0];
            left[np - j][1] = right[0][1];
          }
        if (depth > 0)
          {
            DrawBezier (left, np, mid, depth - 1);
            DrawBezier (right, np, mid, depth - 1);
            g_free (left);
            g_free (right);
          }
      }
}

void
d_draw_bezier (GfigObject *obj,
               cairo_t    *cr)
{
  DobjPoints *spnt;
  gint        seg_count = 0;
  gint        i = 0;
  gdouble   (*line_pnts)[2];

  spnt = obj->points;

  /* First count the number of points */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    seg_count++;

  if (!seg_count)
    return; /* no-line */

  line_pnts = (fp_pnt) g_new0 (gdouble, 2 * seg_count + 1);

  /* Go around all the points drawing a line from one to the next */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      if (! spnt->next && obj == obj_creating)
        draw_circle (&spnt->pnt, TRUE, cr);
      else
        draw_sqr (&spnt->pnt, obj == gfig_context->selected_obj, cr);
      line_pnts[i][0] = spnt->pnt.x;
      line_pnts[i][1] = spnt->pnt.y;
      i++;
    }

  /* Generate an array of doubles which are the control points */

  if (bezier_line_frame && tmp_bezier)
    {
      fp_pnt_start ();
      DrawBezier (line_pnts, seg_count, 0.5, 0);
      d_bz_line (cr);
    }

  fp_pnt_start ();
  DrawBezier (line_pnts, seg_count, 0.5, 3);
  d_bz_line (cr);

  g_free (line_pnts);
}

static void
d_paint_bezier (GfigObject *obj)
{
  gdouble    *line_pnts;
  gdouble   (*bz_line_pnts)[2];
  DobjPoints *spnt;
  gint        seg_count = 0;
  gint        i = 0;

  /* First count the number of points */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    seg_count++;

  if (!seg_count)
    return; /* no-line */

  bz_line_pnts = (fp_pnt) g_new0 (gdouble, 2 * seg_count + 1);

  /* Go around all the points drawing a line from one to the next */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      bz_line_pnts[i][0] = spnt->pnt.x;
      bz_line_pnts[i][1] = spnt->pnt.y;
      i++;
    }

  fp_pnt_start ();
  DrawBezier (bz_line_pnts, seg_count, 0.5, 5);
  line_pnts = d_bz_get_array (&i);

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&line_pnts[0], i / 2);
  else
    scale_to_xy (&line_pnts[0], i / 2);

  /* One go */
  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
                  gfig_context->drawable_id,
                  i, line_pnts);
    }

  g_free (bz_line_pnts);
  /* Don't free line_pnts - may need again */
}

static GfigObject *
d_copy_bezier (GfigObject *obj)
{
  GfigObject *np;

  g_assert (obj->type == BEZIER);

  np = d_new_object (BEZIER, obj->points->pnt.x, obj->points->pnt.y);
  np->points->next = d_copy_dobjpoints (obj->points->next);
  np->type_data = obj->type_data;

  return np;
}

void
d_bezier_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[BEZIER];

  class->type      = BEZIER;
  class->name      = "BEZIER";
  class->drawfunc  = d_draw_bezier;
  class->paintfunc = d_paint_bezier;
  class->copyfunc  = d_copy_bezier;
  class->update    = d_update_bezier;
}

static void
d_update_bezier (GdkPoint *pnt)
{
  DobjPoints *s_pnt, *l_pnt;

  g_assert (tmp_bezier != NULL);

  s_pnt = tmp_bezier->points;

  if (!s_pnt)
    return; /* No points */

  if ((l_pnt = s_pnt->next))
    {
      while (l_pnt->next)
        {
          l_pnt = l_pnt->next;
        }

      l_pnt->pnt = *pnt;
    }
  else
    {
      /* Radius is a few pixels away */
      /* First edge point */
      d_pnt_add_line (tmp_bezier, pnt->x, pnt->y,-1);
    }
}

void
d_bezier_start (GdkPoint *pnt,
                gboolean  shift_down)
{
  if (!tmp_bezier)
    {
      /* New curve */
      tmp_bezier = obj_creating = d_new_object (BEZIER, pnt->x, pnt->y);
    }
}

void
d_bezier_end (GdkPoint *pnt,
              gboolean  shift_down)
{
  DobjPoints *l_pnt;

  if (!tmp_bezier)
    {
      tmp_bezier = obj_creating;
    }

  l_pnt = tmp_bezier->points->next;

  if (!l_pnt)
    return;

  if (shift_down)
    {
      while (l_pnt->next)
        {
          l_pnt = l_pnt->next;
        }

      if (l_pnt)
        {
          if (bezier_closed)
            {
              /* if closed then add first point */
              d_pnt_add_line (tmp_bezier,
                              tmp_bezier->points->pnt.x,
                              tmp_bezier->points->pnt.y, -1);
            }

          add_to_all_obj (gfig_context->current_obj, obj_creating);
        }

      /* small mem leak if !l_pnt ? */
      tmp_bezier = NULL;
      obj_creating = NULL;
    }
  else
    {
      d_pnt_add_line (tmp_bezier, pnt->x, pnt->y,-1);
    }
}

void
tool_options_bezier (GtkWidget *notebook)
{
  GtkWidget *vbox;
  GtkWidget *toggle;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, NULL);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Closed"));
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bezier_closed);
  gimp_help_set_help_data (toggle,
                        _("Close curve on completion"), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bezier_closed);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Show Line Frame"));
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bezier_line_frame);
  gimp_help_set_help_data (toggle,
                           _("Draws lines between the control points. "
                           "Only during curve creation"), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bezier_line_frame);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
}

