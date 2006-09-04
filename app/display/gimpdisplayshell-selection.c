/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "base/boundary.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-transform.h"


#undef  VERBOSE

#define INITIAL_DELAY     15  /* in milleseconds */
#define MAX_POINTS_INC    2048
#define USE_DRAWPOINTS    TRUE


struct _Selection
{
  GimpDisplayShell *shell;            /*  shell that owns the selection     */
  GdkSegment       *segs_in;          /*  gdk segments of area boundary     */
  GdkSegment       *segs_out;         /*  gdk segments of area boundary     */
  GdkSegment       *segs_layer;       /*  gdk segments of layer boundary    */
  gint              num_segs_in;      /*  number of segments in segs1       */
  gint              num_segs_out;     /*  number of segments in segs2       */
  gint              num_segs_layer;   /*  number of segments in segs3       */
  guint             index;            /*  index of current stipple pattern  */
  gint              paused;           /*  count of pause requests           */
  gboolean          recalc;           /*  flag to recalculate the selection */
  gboolean          hidden;           /*  is the selection hidden?          */
  gboolean          layer_hidden;     /*  is the layer boundary hidden?     */
  guint             timeout_id;       /*  timer for successive draws        */
  GdkPoint         *points_in[8];     /*  points of segs_in for fast ants   */
  gint              num_points_in[8]; /*  number of points in points_in     */
};


/*  local function prototypes  */

static void      selection_stop           (Selection      *select);
static void      selection_start          (Selection      *select);

static void      selection_pause          (Selection      *select);
static void      selection_resume         (Selection      *select);

static void      selection_draw           (Selection      *select);
static void      selection_undraw         (Selection      *select);
static void      selection_layer_undraw   (Selection      *select);

static void      selection_add_point      (GdkPoint       *points[8],
                                           gint            max_npoints[8],
                                           gint            npoints[8],
                                           gint            x,
                                           gint            y);
static void      selection_render_points  (Selection      *select);

static void      selection_transform_segs (Selection      *select,
                                           const BoundSeg *src_segs,
                                           GdkSegment     *dest_segs,
                                           gint            num_segs);
static void      selection_generate_segs  (Selection      *select);
static void      selection_free_segs      (Selection      *select);

static gboolean  selection_start_timeout (Selection      *select);
static gboolean  selection_timeout        (Selection      *select);


/*  public functions  */

void
gimp_display_shell_selection_init (GimpDisplayShell *shell)
{
  Selection *select;
  gint       i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection == NULL);

  select = g_new0 (Selection, 1);

  select->shell        = shell;
  select->recalc       = TRUE;
  select->hidden       = ! gimp_display_shell_get_show_selection (shell);
  select->layer_hidden = ! gimp_display_shell_get_show_layer (shell);

  for (i = 0; i < 8; i++)
    select->points_in[i] = NULL;

  shell->selection = select;
}

void
gimp_display_shell_selection_free (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *select = shell->selection;

      selection_stop (select);
      selection_free_segs (select);

      g_free (select);

      shell->selection = NULL;
    }
}

void
gimp_display_shell_selection_control (GimpDisplayShell     *shell,
                                      GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *select = shell->selection;

      switch (control)
        {
        case GIMP_SELECTION_OFF:
          selection_stop (select);
          selection_undraw (select);
          break;

        case GIMP_SELECTION_LAYER_OFF:
          selection_layer_undraw (select);
          break;

        case GIMP_SELECTION_ON:
          selection_start (select);
          break;

        case GIMP_SELECTION_PAUSE:
          selection_pause (select);
          break;

        case GIMP_SELECTION_RESUME:
          selection_resume (select);
          break;
        }
    }
}

void
gimp_display_shell_selection_set_hidden (GimpDisplayShell *shell,
                                         gboolean          hidden)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *select = shell->selection;

      if (hidden != select->hidden)
        {
          selection_undraw (select);
          selection_layer_undraw (select);

          select->hidden = hidden;

          selection_start (select);
        }
    }
}

void
gimp_display_shell_selection_layer_set_hidden (GimpDisplayShell *shell,
                                               gboolean          hidden)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *select = shell->selection;

      if (hidden != select->layer_hidden)
        {
          selection_undraw (select);
          selection_layer_undraw (select);

          select->layer_hidden = hidden;

          selection_start (select);
        }
    }
}


/*  private functions  */

static void
selection_stop (Selection *select)
{
  if (select->timeout_id)
    {
      g_source_remove (select->timeout_id);
      select->timeout_id = 0;
    }
}

static void
selection_start (Selection *select)
{
  selection_stop (select);

  select->recalc = TRUE;

  /*  If this selection is paused, do not start it  */
  if (select->paused > 0)
    return;

  select->timeout_id = g_timeout_add (INITIAL_DELAY,
                                      (GSourceFunc) selection_start_timeout,
                                      select);
}

static void
selection_pause (Selection *select)
{
  selection_stop (select);

  select->paused++;
}

static void
selection_resume (Selection *select)
{
  if (select->paused == 1)
    select->timeout_id = g_timeout_add (INITIAL_DELAY,
                                        (GSourceFunc) selection_start_timeout,
                                        select);

  select->paused--;
}

static void
selection_draw (Selection *select)
{
  GimpCanvas *canvas = GIMP_CANVAS (select->shell->canvas);

#ifdef USE_DRAWPOINTS

#ifdef VERBOSE
  {
    gint j, sum;

    sum = 0;
    for (j = 0; j < 8; j++)
      sum += select->num_points_in[j];

    g_print ("%d segments, %d points\n", select->num_segs_in, sum);
  }
#endif
  if (select->segs_in)
    {
      gint i;

      if (select->index == 0)
        {
          for (i = 0; i < 4; i++)
            if (select->num_points_in[i])
              gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_WHITE,
                                       select->points_in[i],
                                       select->num_points_in[i]);

          for (i = 4; i < 8; i++)
            if (select->num_points_in[i])
              gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_BLACK,
                                       select->points_in[i],
                                       select->num_points_in[i]);
        }
      else
        {
          i = ((select->index + 3) & 7);
          if (select->num_points_in[i])
            gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_WHITE,
                                     select->points_in[i],
                                     select->num_points_in[i]);

          i = ((select->index + 7) & 7);
          if (select->num_points_in[i])
            gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_BLACK,
                                     select->points_in[i],
                                     select->num_points_in[i]);
        }
    }

#else  /*  ! USE_DRAWPOINTS  */
  gimp_canvas_set_stipple_index (canvas,
                                 GIMP_CANVAS_STYLE_SELECTION_IN,
                                 select->index);
  if (select->segs_in)
    gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_SELECTION_IN,
                               select->segs_in, select->num_segs_in);
#endif
}

static void
selection_undraw (Selection *select)
{
  gint x1, y1, x2, y2;

  /*  Find the bounds of the selection  */
  if (gimp_display_shell_mask_bounds (select->shell, &x1, &y1, &x2, &y2))
    {
      gimp_display_shell_expose_area (select->shell,
                                      x1, y1, (x2 - x1), (y2 - y1));
    }
  else
    {
      selection_start (select);
    }
}

static void
selection_layer_draw (Selection *select)
{
  GimpCanvas *canvas = GIMP_CANVAS (select->shell->canvas);

  if (select->segs_layer)
    gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_LAYER_BOUNDARY,
                               select->segs_layer, select->num_segs_layer);
}

static void
selection_layer_undraw (Selection *select)
{
  if (select->segs_layer != NULL && select->num_segs_layer == 4)
    {
      gint x1 = select->segs_layer[0].x1 - 1;
      gint y1 = select->segs_layer[0].y1 - 1;
      gint x2 = select->segs_layer[3].x2 + 1;
      gint y2 = select->segs_layer[3].y2 + 1;

      gint x3 = select->segs_layer[0].x1 + 1;
      gint y3 = select->segs_layer[0].y1 + 1;
      gint x4 = select->segs_layer[3].x2 - 1;
      gint y4 = select->segs_layer[3].y2 - 1;

      /*  expose the region  */
      gimp_display_shell_expose_area (select->shell,
                                      x1, y1, (x2 - x1) + 1, (y3 - y1) + 1);
      gimp_display_shell_expose_area (select->shell,
                                      x1, y3, (x3 - x1) + 1, (y4 - y3) + 1);
      gimp_display_shell_expose_area (select->shell,
                                      x1, y4, (x2 - x1) + 1, (y2 - y4) + 1);
      gimp_display_shell_expose_area (select->shell,
                                      x4, y3, (x2 - x4) + 1, (y4 - y3) + 1);
    }
}

static void
selection_add_point (GdkPoint *points[8],
                     gint      max_npoints[8],
                     gint      npoints[8],
                     gint      x,
                     gint      y)
{
  gint i, j;

  j = (x - y) & 7;

  i = npoints[j]++;
  if (i == max_npoints[j])
    {
      max_npoints[j] += 2048;
      points[j] = g_realloc (points[j], sizeof (GdkPoint) * max_npoints[j]);
    }

  points[j][i].x = x;
  points[j][i].y = y;
}


/* Render the segs_in array into points_in */
static void
selection_render_points (Selection *select)
{
  gint i, j;
  gint max_npoints[8];
  gint x, y;
  gint dx, dy;
  gint dxa, dya;
  gint r;

  if (select->segs_in == NULL)
    return;

  for (j = 0; j < 8; j++)
    {
      max_npoints[j] = MAX_POINTS_INC;
      select->points_in[j] = g_new (GdkPoint, max_npoints[j]);
      select->num_points_in[j] = 0;
    }

  for (i = 0; i < select->num_segs_in; i++)
    {
#ifdef VERBOSE
      g_print ("%2d: (%d, %d) - (%d, %d)\n", i,
               select->segs_in[i].x1,
               select->segs_in[i].y1,
               select->segs_in[i].x2,
               select->segs_in[i].y2);
#endif
      x = select->segs_in[i].x1;
      dxa = select->segs_in[i].x2 - x;

      if (dxa > 0)
        {
          dx = 1;
        }
      else
        {
          dxa = -dxa;
          dx = -1;
        }

      y = select->segs_in[i].y1;
      dya = select->segs_in[i].y2 - y;

      if (dya > 0)
        {
          dy = 1;
        }
      else
        {
          dya = -dya;
          dy = -1;
        }

      if (dxa > dya)
        {
          r = dya;
          do
            {
              selection_add_point (select->points_in,
                                   max_npoints,
                                   select->num_points_in,
                                   x, y);
              x += dx;
              r += dya;

              if (r >= (dxa << 1))
                {
                  y += dy;
                  r -= (dxa << 1);
                }
            } while (x != select->segs_in[i].x2);
        }
      else if (dxa < dya)
        {
          r = dxa;
          do
            {
              selection_add_point (select->points_in,
                                   max_npoints,
                                   select->num_points_in,
                                   x, y);
              y += dy;
              r += dxa;

              if (r >= (dya << 1))
                {
                  x += dx;
                  r -= (dya << 1);
                }
            } while (y != select->segs_in[i].y2);
        }
      else
        {
          selection_add_point (select->points_in,
                               max_npoints,
                               select->num_points_in,
                               x, y);
        }
    }
}

static void
selection_transform_segs (Selection      *select,
                          const BoundSeg *src_segs,
                          GdkSegment     *dest_segs,
                          gint            num_segs)
{
  gint xclamp = select->shell->disp_width + 1;
  gint yclamp = select->shell->disp_height + 1;
  gint i;

  gimp_display_shell_transform_segments (select->shell,
                                         src_segs, dest_segs, num_segs, FALSE);

  for (i = 0; i < num_segs; i++)
    {
      dest_segs[i].x1 = CLAMP (dest_segs[i].x1, -1, xclamp);
      dest_segs[i].y1 = CLAMP (dest_segs[i].y1, -1, yclamp);

      dest_segs[i].x2 = CLAMP (dest_segs[i].x2, -1, xclamp);
      dest_segs[i].y2 = CLAMP (dest_segs[i].y2, -1, yclamp);

      /*  If this segment is a closing segment && the segments lie inside
       *  the region, OR if this is an opening segment and the segments
       *  lie outside the region...
       *  we need to transform it by one display pixel
       */
      if (!src_segs[i].open)
        {
          /*  If it is vertical  */
          if (dest_segs[i].x1 == dest_segs[i].x2)
            {
              dest_segs[i].x1 -= 1;
              dest_segs[i].x2 -= 1;
            }
          else
            {
              dest_segs[i].y1 -= 1;
              dest_segs[i].y2 -= 1;
            }
        }
    }
}

static void
selection_generate_segs (Selection *select)
{
  const BoundSeg *segs_in;
  const BoundSeg *segs_out;
  BoundSeg       *segs_layer;

  /*  Ask the image for the boundary of its selected region...
   *  Then transform that information into a new buffer of GdkSegments
   */
  gimp_channel_boundary (gimp_image_get_mask (select->shell->display->image),
                         &segs_in, &segs_out,
                         &select->num_segs_in, &select->num_segs_out,
                         0, 0, 0, 0);

  if (select->num_segs_in)
    {
      select->segs_in = g_new (GdkSegment, select->num_segs_in);
      selection_transform_segs (select, segs_in,
                                select->segs_in, select->num_segs_in);

#ifdef USE_DRAWPOINTS
      selection_render_points (select);
#endif
    }
  else
    {
      select->segs_in = NULL;
    }

  /*  Possible secondary boundary representation  */
  if (select->num_segs_out)
    {
      select->segs_out = g_new (GdkSegment, select->num_segs_out);
      selection_transform_segs (select, segs_out,
                                select->segs_out, select->num_segs_out);
    }
  else
    {
      select->segs_out = NULL;
    }

  /*  The active layer's boundary  */
  gimp_image_layer_boundary (select->shell->display->image,
                             &segs_layer, &select->num_segs_layer);

  if (select->num_segs_layer)
    {
      select->segs_layer = g_new (GdkSegment, select->num_segs_layer);
      selection_transform_segs (select, segs_layer,
                                select->segs_layer, select->num_segs_layer);
    }
  else
    {
      select->segs_layer = NULL;
    }

  g_free (segs_layer);
}

static void
selection_free_segs (Selection *select)
{
  gint j;

  if (select->segs_in)
    g_free (select->segs_in);

  if (select->segs_out)
    g_free (select->segs_out);

  if (select->segs_layer)
    g_free (select->segs_layer);

  for (j = 0; j < 8; j++)
    {
      if (select->points_in[j])
        g_free (select->points_in[j]);

      select->points_in[j]     = NULL;
      select->num_points_in[j] = 0;
    }

  select->segs_in        = NULL;
  select->num_segs_in    = 0;
  select->segs_out       = NULL;
  select->num_segs_out   = 0;
  select->segs_layer     = NULL;
  select->num_segs_layer = 0;
}

static gboolean
selection_start_timeout (Selection *select)
{
  if (select->recalc)
    {
      selection_free_segs (select);
      selection_generate_segs (select);

      select->recalc = FALSE;
    }

  select->index = 0;

  if (! select->layer_hidden)
    selection_layer_draw (select);

  /*  Draw the ants  */
  if (! select->hidden)
    {
      GimpCanvas        *canvas = GIMP_CANVAS (select->shell->canvas);
      GimpDisplayConfig *config = GIMP_DISPLAY_CONFIG
        (select->shell->display->image->gimp->config);

      selection_draw (select);

      if (select->segs_out)
        gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_SELECTION_OUT,
                                   select->segs_out, select->num_segs_out);


      if (select->segs_in)
        select->timeout_id = g_timeout_add (config->marching_ants_speed,
                                            (GSourceFunc) selection_timeout,
                                            select);
    }

  return FALSE;
}

static gboolean
selection_timeout (Selection *select)
{
  select->index++;
  selection_draw (select);

  return TRUE;
}
