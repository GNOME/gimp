/* GIMP - The GNU Image Manipulation Program
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
  gboolean          visible;          /*  visility of the display shell     */
  gboolean          hidden;           /*  is the selection hidden?          */
  gboolean          layer_hidden;     /*  is the layer boundary hidden?     */
  guint             timeout;          /*  timer for successive draws        */
  GdkPoint         *points_in[8];     /*  points of segs_in for fast ants   */
  gint              num_points_in[8]; /*  number of points in points_in     */
};


/*  local function prototypes  */

static void      selection_start          (Selection      *selection);
static void      selection_stop           (Selection      *selection);

static void      selection_pause          (Selection      *selection);
static void      selection_resume         (Selection      *selection);

static void      selection_draw           (Selection      *selection);
static void      selection_undraw         (Selection      *selection);
static void      selection_layer_undraw   (Selection      *selection);

static void      selection_add_point      (GdkPoint       *points[8],
                                           gint            max_npoints[8],
                                           gint            npoints[8],
                                           gint            x,
                                           gint            y);
static void      selection_render_points  (Selection      *selection);

static void      selection_transform_segs (Selection      *selection,
                                           const BoundSeg *src_segs,
                                           GdkSegment     *dest_segs,
                                           gint            num_segs);
static void      selection_generate_segs  (Selection      *selection);
static void      selection_free_segs      (Selection      *selection);

static gboolean  selection_start_timeout  (Selection      *selection);
static gboolean  selection_timeout        (Selection      *selection);

static gboolean  selection_window_state_event      (GtkWidget           *shell,
                                                    GdkEventWindowState *event,
                                                    Selection           *selection);
static gboolean  selection_visibility_notify_event (GtkWidget           *shell,
                                                    GdkEventVisibility  *event,
                                                    Selection           *selection);


/*  public functions  */

void
gimp_display_shell_selection_init (GimpDisplayShell *shell)
{
  Selection *selection;
  gint       i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection == NULL);

  selection = g_slice_new0 (Selection);

  selection->shell        = shell;
  selection->visible      = TRUE;
  selection->hidden       = ! gimp_display_shell_get_show_selection (shell);
  selection->layer_hidden = ! gimp_display_shell_get_show_layer (shell);

  for (i = 0; i < 8; i++)
    selection->points_in[i] = NULL;

  shell->selection = selection;

  g_signal_connect (shell, "window-state-event",
                    G_CALLBACK (selection_window_state_event),
                    selection);
  g_signal_connect (shell, "visibility-notify-event",
                    G_CALLBACK (selection_visibility_notify_event),
                    selection);
}

void
gimp_display_shell_selection_free (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *selection = shell->selection;

      selection_stop (selection);

      g_signal_handlers_disconnect_by_func (shell,
                                            selection_window_state_event,
                                            selection);
      g_signal_handlers_disconnect_by_func (shell,
                                            selection_visibility_notify_event,
                                            selection);

      selection_free_segs (selection);

      g_slice_free (Selection, selection);

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
      Selection *selection = shell->selection;

      switch (control)
        {
        case GIMP_SELECTION_OFF:
          selection_undraw (selection);
          break;

        case GIMP_SELECTION_LAYER_OFF:
          selection_layer_undraw (selection);
          break;

        case GIMP_SELECTION_ON:
          selection_start (selection);
          break;

        case GIMP_SELECTION_PAUSE:
          selection_pause (selection);
          break;

        case GIMP_SELECTION_RESUME:
          selection_resume (selection);
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
      Selection *selection = shell->selection;

      if (hidden != selection->hidden)
        {
          selection_undraw (selection);
          selection_layer_undraw (selection);

          selection->hidden = hidden;

          selection_start (selection);
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
      Selection *selection = shell->selection;

      if (hidden != selection->layer_hidden)
        {
          selection_undraw (selection);
          selection_layer_undraw (selection);

          selection->layer_hidden = hidden;

          selection_start (selection);
        }
    }
}


/*  private functions  */

static void
selection_start (Selection *selection)
{
  selection_stop (selection);

  /*  If this selection is paused, do not start it  */
  if (selection->paused == 0)
    {
      selection->timeout = g_idle_add ((GSourceFunc) selection_start_timeout,
                                       selection);
    }
}

static void
selection_stop (Selection *selection)
{
  if (selection->timeout)
    {
      g_source_remove (selection->timeout);
      selection->timeout = 0;
    }
}

static void
selection_pause (Selection *selection)
{
  if (selection->paused == 0)
    selection_stop (selection);

  selection->paused++;
}

static void
selection_resume (Selection *selection)
{
  selection->paused--;

  if (selection->paused == 0)
    selection_start (selection);
}

static void
selection_draw (Selection *selection)
{
  GimpCanvas *canvas = GIMP_CANVAS (selection->shell->canvas);

#ifdef USE_DRAWPOINTS

#ifdef VERBOSE
  {
    gint j, sum;

    sum = 0;
    for (j = 0; j < 8; j++)
      sum += selection->num_points_in[j];

    g_print ("%d segments, %d points\n", selection->num_segs_in, sum);
  }
#endif
  if (selection->segs_in)
    {
      gint i;

      if (selection->index == 0)
        {
          for (i = 0; i < 4; i++)
            if (selection->num_points_in[i])
              gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_WHITE,
                                       selection->points_in[i],
                                       selection->num_points_in[i]);

          for (i = 4; i < 8; i++)
            if (selection->num_points_in[i])
              gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_BLACK,
                                       selection->points_in[i],
                                       selection->num_points_in[i]);
        }
      else
        {
          i = ((selection->index + 3) & 7);
          if (selection->num_points_in[i])
            gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_WHITE,
                                     selection->points_in[i],
                                     selection->num_points_in[i]);

          i = ((selection->index + 7) & 7);
          if (selection->num_points_in[i])
            gimp_canvas_draw_points (canvas, GIMP_CANVAS_STYLE_BLACK,
                                     selection->points_in[i],
                                     selection->num_points_in[i]);
        }
    }

#else  /*  ! USE_DRAWPOINTS  */
  gimp_canvas_set_stipple_index (canvas,
                                 GIMP_CANVAS_STYLE_SELECTION_IN,
                                 selection->index);
  if (selection->segs_in)
    gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_SELECTION_IN,
                               selection->segs_in,
                               selection->num_segs_in);
#endif
}

static void
selection_undraw (Selection *selection)
{
  gint x1, y1, x2, y2;

  selection_stop (selection);

  if (gimp_display_shell_mask_bounds (selection->shell, &x1, &y1, &x2, &y2))
    {
      /* expose will restart the selection */
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y1, (x2 - x1), (y2 - y1));
    }
  else
    {
      selection_start (selection);
    }
}

static void
selection_layer_draw (Selection *selection)
{
  GimpCanvas *canvas = GIMP_CANVAS (selection->shell->canvas);

  if (selection->segs_layer)
    gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_LAYER_BOUNDARY,
                               selection->segs_layer,
                               selection->num_segs_layer);
}

static void
selection_layer_undraw (Selection *selection)
{
  selection_stop (selection);

  if (selection->segs_layer != NULL && selection->num_segs_layer == 4)
    {
      gint x1 = selection->segs_layer[0].x1 - 1;
      gint y1 = selection->segs_layer[0].y1 - 1;
      gint x2 = selection->segs_layer[3].x2 + 1;
      gint y2 = selection->segs_layer[3].y2 + 1;

      gint x3 = selection->segs_layer[0].x1 + 1;
      gint y3 = selection->segs_layer[0].y1 + 1;
      gint x4 = selection->segs_layer[3].x2 - 1;
      gint y4 = selection->segs_layer[3].y2 - 1;

      /*  expose the region, this will restart the selection  */
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y1, (x2 - x1) + 1, (y3 - y1) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y3, (x3 - x1) + 1, (y4 - y3) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y4, (x2 - x1) + 1, (y2 - y4) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x4, y3, (x2 - x4) + 1, (y4 - y3) + 1);
    }
  else
    {
      selection_start (selection);
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
selection_render_points (Selection *selection)
{
  gint i, j;
  gint max_npoints[8];
  gint x, y;
  gint dx, dy;
  gint dxa, dya;
  gint r;

  if (selection->segs_in == NULL)
    return;

  for (j = 0; j < 8; j++)
    {
      max_npoints[j] = MAX_POINTS_INC;
      selection->points_in[j] = g_new (GdkPoint, max_npoints[j]);
      selection->num_points_in[j] = 0;
    }

  for (i = 0; i < selection->num_segs_in; i++)
    {
#ifdef VERBOSE
      g_print ("%2d: (%d, %d) - (%d, %d)\n", i,
               selection->segs_in[i].x1,
               selection->segs_in[i].y1,
               selection->segs_in[i].x2,
               selection->segs_in[i].y2);
#endif
      x = selection->segs_in[i].x1;
      dxa = selection->segs_in[i].x2 - x;

      if (dxa > 0)
        {
          dx = 1;
        }
      else
        {
          dxa = -dxa;
          dx = -1;
        }

      y = selection->segs_in[i].y1;
      dya = selection->segs_in[i].y2 - y;

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
              selection_add_point (selection->points_in,
                                   max_npoints,
                                   selection->num_points_in,
                                   x, y);
              x += dx;
              r += dya;

              if (r >= (dxa << 1))
                {
                  y += dy;
                  r -= (dxa << 1);
                }
            } while (x != selection->segs_in[i].x2);
        }
      else if (dxa < dya)
        {
          r = dxa;
          do
            {
              selection_add_point (selection->points_in,
                                   max_npoints,
                                   selection->num_points_in,
                                   x, y);
              y += dy;
              r += dxa;

              if (r >= (dya << 1))
                {
                  x += dx;
                  r -= (dya << 1);
                }
            } while (y != selection->segs_in[i].y2);
        }
      else
        {
          selection_add_point (selection->points_in,
                               max_npoints,
                               selection->num_points_in,
                               x, y);
        }
    }
}

static void
selection_transform_segs (Selection      *selection,
                          const BoundSeg *src_segs,
                          GdkSegment     *dest_segs,
                          gint            num_segs)
{
  gint xclamp = selection->shell->disp_width + 1;
  gint yclamp = selection->shell->disp_height + 1;
  gint i;

  gimp_display_shell_transform_segments (selection->shell,
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
selection_generate_segs (Selection *selection)
{
  const BoundSeg *segs_in;
  const BoundSeg *segs_out;
  BoundSeg       *segs_layer;

  /*  Ask the image for the boundary of its selected region...
   *  Then transform that information into a new buffer of GdkSegments
   */
  gimp_channel_boundary (gimp_image_get_mask (selection->shell->display->image),
                         &segs_in, &segs_out,
                         &selection->num_segs_in, &selection->num_segs_out,
                         0, 0, 0, 0);

  if (selection->num_segs_in)
    {
      selection->segs_in = g_new (GdkSegment, selection->num_segs_in);
      selection_transform_segs (selection, segs_in,
                                selection->segs_in, selection->num_segs_in);

#ifdef USE_DRAWPOINTS
      selection_render_points (selection);
#endif
    }
  else
    {
      selection->segs_in = NULL;
    }

  /*  Possible secondary boundary representation  */
  if (selection->num_segs_out)
    {
      selection->segs_out = g_new (GdkSegment, selection->num_segs_out);
      selection_transform_segs (selection, segs_out,
                                selection->segs_out, selection->num_segs_out);
    }
  else
    {
      selection->segs_out = NULL;
    }

  /*  The active layer's boundary  */
  gimp_image_layer_boundary (selection->shell->display->image,
                             &segs_layer, &selection->num_segs_layer);

  if (selection->num_segs_layer)
    {
      selection->segs_layer = g_new (GdkSegment, selection->num_segs_layer);
      selection_transform_segs (selection, segs_layer,
                                selection->segs_layer,
                                selection->num_segs_layer);
    }
  else
    {
      selection->segs_layer = NULL;
    }

  g_free (segs_layer);
}

static void
selection_free_segs (Selection *selection)
{
  gint j;

  if (selection->segs_in)
    g_free (selection->segs_in);

  if (selection->segs_out)
    g_free (selection->segs_out);

  if (selection->segs_layer)
    g_free (selection->segs_layer);

  selection->segs_in        = NULL;
  selection->num_segs_in    = 0;
  selection->segs_out       = NULL;
  selection->num_segs_out   = 0;
  selection->segs_layer     = NULL;
  selection->num_segs_layer = 0;

  for (j = 0; j < 8; j++)
    {
      if (selection->points_in[j])
        g_free (selection->points_in[j]);

      selection->points_in[j]     = NULL;
      selection->num_points_in[j] = 0;
    }

}

static gboolean
selection_start_timeout (Selection *selection)
{
  selection_free_segs (selection);
  selection_generate_segs (selection);

  selection->index = 0;

  if (! selection->layer_hidden)
    selection_layer_draw (selection);

  /*  Draw the ants  */
  if (! selection->hidden)
    {
      GimpCanvas        *canvas = GIMP_CANVAS (selection->shell->canvas);
      GimpDisplayConfig *config = GIMP_DISPLAY_CONFIG
        (selection->shell->display->image->gimp->config);

      selection_draw (selection);

      if (selection->segs_out)
        gimp_canvas_draw_segments (canvas, GIMP_CANVAS_STYLE_SELECTION_OUT,
                                   selection->segs_out,
                                   selection->num_segs_out);


      if (selection->segs_in && selection->visible)
        selection->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
                                                 config->marching_ants_speed,
                                                 (GSourceFunc) selection_timeout,
                                                 selection, NULL);
    }

  return FALSE;
}

static gboolean
selection_timeout (Selection *selection)
{
  selection->index++;
  selection_draw (selection);

  return TRUE;
}

static void
selection_set_visible (Selection *selection,
                       gboolean   visible)
{
  if (selection->visible != visible)
    {
      selection->visible = visible;

      if (visible)
        selection_start (selection);
      else
        selection_stop (selection);
    }
}

static gboolean
selection_window_state_event (GtkWidget           *shell,
                              GdkEventWindowState *event,
                              Selection           *selection)
{
  selection_set_visible (selection,
                         (event->new_window_state & (GDK_WINDOW_STATE_WITHDRAWN |
                                                     GDK_WINDOW_STATE_ICONIFIED)) == 0);

  return FALSE;
}
static gboolean
selection_visibility_notify_event (GtkWidget          *shell,
                                   GdkEventVisibility *event,
                                   Selection          *selection)
{
  selection_set_visible (selection,
                         event->state != GDK_VISIBILITY_FULLY_OBSCURED);

  return FALSE;
}
